// Spawns the selected agent under a pty and passes its TUI through untouched: user
// keyboard → pty, pty output → stdout, window resizes forwarded. Controller
// keystrokes are just extra writes into the same pty.

import fs from 'node:fs'
import { execFileSync } from 'node:child_process'
import { createRequire } from 'node:module'
import path from 'node:path'
import * as pty from 'node-pty'
import { logger } from './logger.js'

// node-pty's npm tarball ships spawn-helper without the exec bit, and the
// package.json postinstall chmod can't reach it when npm hoists node-pty out
// of our own node_modules (npx, install-as-dependency). Fix it here, where
// require() tells us where node-pty actually resolved to. Best effort: the
// postinstall still covers root-owned global installs this can't write to.
export function fixSpawnHelperPermissions(prebuildsDir?: string): void {
  try {
    const dir =
      prebuildsDir ??
      path.join(
        path.dirname(createRequire(import.meta.url).resolve('node-pty/package.json')),
        'prebuilds',
      )
    for (const entry of fs.readdirSync(dir)) {
      const helper = path.join(dir, entry, 'spawn-helper')
      if (fs.existsSync(helper)) fs.chmodSync(helper, 0o755)
    }
  } catch {
    // no prebuilds (built from source) or no write permission — if the exec
    // bit is genuinely missing, pty.spawn will surface the failure.
  }
}

/**
 * Resolve an agent CLI name to an absolute path node-pty can spawn.
 * Windows ConPTY often fails with a bare command name when PATH differs from
 * the parent shell (e.g. Codex installed under Local\\Programs\\OpenAI).
 */
export function resolveAgentCommand(command: string): string {
  if (!command) {
    throw new Error('openmicro: agent command is empty')
  }
  if (path.isAbsolute(command) && fs.existsSync(command)) return command
  if ((command.includes('/') || command.includes('\\')) && fs.existsSync(command)) {
    return path.resolve(command)
  }

  try {
    if (process.platform === 'win32') {
      const out = execFileSync('where.exe', [command], {
        encoding: 'utf8',
        windowsHide: true,
      }).trim()
      const lines = out
        .split(/\r?\n/)
        .map((l) => l.trim())
        .filter(Boolean)
      const exe = lines.find((l) => l.toLowerCase().endsWith('.exe'))
      if (exe && fs.existsSync(exe)) return exe
      if (lines[0] && fs.existsSync(lines[0])) return lines[0]
    } else {
      const out = execFileSync('which', [command], { encoding: 'utf8' }).trim()
      if (out && fs.existsSync(out)) return out
    }
  } catch {
    // not on PATH in this process
  }

  if (process.platform === 'win32') {
    const local = process.env.LOCALAPPDATA ?? ''
    const home = process.env.USERPROFILE ?? ''
    const base = command.replace(/\.exe$/i, '').toLowerCase()
    const candidates: string[] = []
    if (base === 'codex') {
      candidates.push(
        path.join(local, 'Programs', 'OpenAI', 'Codex', 'bin', 'codex.exe'),
        path.join(home, '.codex', 'packages', 'standalone', 'current', 'bin', 'codex.exe'),
      )
    }
    for (const c of candidates) {
      if (c && fs.existsSync(c)) return c
    }
  }

  const tip =
    command === 'codex' || command === 'codex.exe'
      ? ' Tip: add %LOCALAPPDATA%\\Programs\\OpenAI\\Codex\\bin to your User PATH, or open a new terminal.'
      : command === 'claude' || command === 'claude.exe'
        ? ' Tip: install Claude Code CLI and ensure `claude` is on PATH.'
        : ''
  throw new Error(`openmicro: agent command '${command}' not found on PATH.${tip}`)
}

type PtySpawner = typeof pty.spawn

export function spawnAgentProcess(
  spawnPty: PtySpawner,
  command: string,
  args: string[],
  wrapperId: string | undefined,
): pty.IPty {
  const env = { ...process.env } as Record<string, string>
  // herdr's own agent integration hooks (e.g. ~/.claude/hooks/herdr-agent-state.sh)
  // gate on HERDR_ENV=1. If the wrapped agent runs them, it claims the herdr
  // pane's session as herdr:<agent>, and herdr then silently drops every
  // report from any other source — including openmicro's state reports
  // (session-owner conflict; herdr can't verify "openmicro" as a takeover
  // agent). Hide HERDR_ENV from the child so only openmicro reports for the
  // pane. HERDR_PANE_ID stays: openmicro's hook curls echo it back to us.
  delete env.HERDR_ENV
  if (wrapperId) env.OPENMICRO_INSTANCE_ID = wrapperId
  return spawnPty(command, args, {
    name: process.env.TERM ?? 'xterm-256color',
    cols: process.stdout.columns,
    rows: process.stdout.rows,
    cwd: process.cwd(),
    env,
  })
}

export class AgentPty {
  private proc: pty.IPty
  private focusReporting = false

  constructor(
    command: string,
    args: string[],
    wrapperId: string | undefined,
    onExit: (code: number) => void,
    onFocusChange?: (focused: boolean) => void,
  ) {
    fixSpawnHelperPermissions()
    const resolved = resolveAgentCommand(command)
    if (resolved !== command) logger.info(`resolved agent command: ${resolved}`)
    this.proc = spawnAgentProcess(pty.spawn, resolved, args, wrapperId)

    this.proc.onData((data) => process.stdout.write(data))
    this.proc.onExit(({ exitCode }) => onExit(exitCode))

    if (process.stdin.isTTY) process.stdin.setRawMode(true)
    // Terminal focus reporting (mode 1004): the terminal sends ESC[I / ESC[O
    // on window/pane focus changes. We observe them here and still pass them
    // through to the wrapped agent, which understands the same events.
    if (onFocusChange && process.stdout.isTTY) {
      process.stdout.write('\x1b[?1004h')
      this.focusReporting = true
    }
    process.stdin.on('data', (data: Buffer) => {
      const bytes = data.toString('utf8')
      if (onFocusChange) {
        // ponytail: per-chunk match — an event split across reads is missed;
        // buffer across chunks if that ever shows up in practice.
        if (bytes.includes('\x1b[I')) onFocusChange(true)
        else if (bytes.includes('\x1b[O')) onFocusChange(false)
      }
      this.proc.write(bytes)
    })

    process.stdout.on('resize', () => {
      try {
        this.proc.resize(process.stdout.columns, process.stdout.rows)
      } catch (err) {
        logger.warn('pty resize failed', err)
      }
    })
  }

  write(data: string): void {
    this.proc.write(data)
  }

  dispose(): void {
    if (this.focusReporting) process.stdout.write('\x1b[?1004l')
    if (process.stdin.isTTY) process.stdin.setRawMode(false)
    process.stdin.pause()
    try {
      this.proc.kill()
    } catch {
      // already dead
    }
  }
}
