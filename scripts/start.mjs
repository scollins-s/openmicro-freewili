#!/usr/bin/env node
/**
 * One-command FreeWili OpenMicro launcher.
 *
 * Spawns (in one terminal):
 *   1) OpenOCD RTT server (:9090) via wilibsp fw.py --print command
 *   2) OpenMicro host with --freewili --no-hid
 *   3) RTT → TCP bridge (or device:sim in --sim mode)
 *
 * Usage:
 *   node scripts/start.mjs
 *   node scripts/start.mjs --sim
 *   node scripts/start.mjs --agent codex
 *   node scripts/start.mjs --no-rtt          # host only (manual OpenOCD / CDC)
 *   node scripts/start.mjs --skip-bridge     # OpenOCD + host only
 *   node scripts/start.mjs --iface 1
 */

import { spawn } from 'node:child_process'
import net from 'node:net'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { existsSync } from 'node:fs'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const KIT_ROOT = path.resolve(__dirname, '..')
const HOST_ROOT = path.join(KIT_ROOT, 'host')
const WILIBSP = path.join(KIT_ROOT, 'wilibsp')
const FW_PY = path.join(WILIBSP, 'tools', 'fw.py')

const RTT_HOST = '127.0.0.1'
const RTT_PORT = 9090
const OM_PORT = 48763

const children = []
let shuttingDown = false

function parseArgs(argv) {
  const opts = {
    sim: false,
    noRtt: false,
    skipBridge: false,
    agent: 'claude',
    iface: null,
    agentArgs: [],
    help: false,
  }
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i]
    if (a === '--help' || a === '-h') opts.help = true
    else if (a === '--sim') opts.sim = true
    else if (a === '--no-rtt') opts.noRtt = true
    else if (a === '--skip-bridge') opts.skipBridge = true
    else if (a === '--agent') opts.agent = argv[++i] || 'claude'
    else if (a.startsWith('--agent=')) opts.agent = a.slice('--agent='.length) || 'claude'
    else if (a === '--iface') opts.iface = argv[++i]
    else if (a.startsWith('--iface=')) opts.iface = a.slice('--iface='.length)
    else if (a === '--') {
      opts.agentArgs = argv.slice(i + 1)
      break
    } else {
      opts.agentArgs.push(a)
    }
  }
  return opts
}

function usage() {
  console.log(`openmicro-freewili — one-command FreeWili + OpenMicro launcher

Usage:
  npm start
  npm start -- --sim
  npm start -- --agent codex
  npm start -- --iface 1
  npm start -- --no-rtt          # host only (you run OpenOCD / use CDC yourself)
  npm run freewili:sim

Prereqs:
  npm run setup                  # install + build host/
  npm run flash:display          # once, with probe connected
  Claude or Codex CLI on PATH
`)
}

function log(prefix, chunk, stream) {
  const text = chunk.toString()
  for (const line of text.split(/\r?\n/)) {
    if (line.length === 0) continue
    stream.write(`[${prefix}] ${line}\n`)
  }
}

function spawnLogged(prefix, command, args, options = {}) {
  const inheritAll = options.stdio === 'inherit'
  const child = spawn(command, args, {
    cwd: options.cwd || KIT_ROOT,
    env: { ...process.env, ...(options.env || {}) },
    stdio: options.stdio || ['ignore', 'pipe', 'pipe'],
    shell: options.shell || false,
    windowsHide: true,
  })
  children.push({ prefix, child })
  if (!inheritAll) {
    if (child.stdout) {
      child.stdout.on('data', (c) => log(prefix, c, process.stdout))
    }
    if (child.stderr) {
      child.stderr.on('data', (c) => log(prefix, c, process.stderr))
    }
  }
  child.on('exit', (code, signal) => {
    if (shuttingDown) return
    console.error(`[${prefix}] exited code=${code} signal=${signal}`)
    if (options.fatal !== false) {
      void shutdown(code ?? 1)
    }
  })
  return child
}

function waitForPort(host, port, timeoutMs = 30000) {
  const start = Date.now()
  return new Promise((resolve, reject) => {
    const tryOnce = () => {
      const sock = net.connect({ host, port }, () => {
        sock.end()
        resolve()
      })
      sock.on('error', () => {
        sock.destroy()
        if (Date.now() - start > timeoutMs) {
          reject(new Error(`Timed out waiting for ${host}:${port}`))
        } else {
          setTimeout(tryOnce, 250)
        }
      })
    }
    tryOnce()
  })
}

function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms))
}

/** Resolve OpenOCD argv via wilibsp fw.rtt_command (no Python RTT consumer). */
async function resolveOpenOcdArgv(iface) {
  if (!existsSync(FW_PY)) {
    throw new Error(`wilibsp fw.py not found at ${FW_PY}`)
  }
  const helper = `
import json, sys
sys.path.insert(0, r${JSON.stringify(path.join(WILIBSP, 'tools'))})
import fw
iface = ${iface == null ? 'None' : Number(iface)}
print(json.dumps(fw.rtt_command(iface)))
`
  return await new Promise((resolve, reject) => {
    const chunks = []
    const errChunks = []
    const python = process.platform === 'win32' ? 'py' : 'python3'
    const pythonArgs = process.platform === 'win32' ? ['-3'] : []
    const p = spawn(python, [...pythonArgs, '-c', helper], {
      cwd: WILIBSP,
      windowsHide: true,
    })
    p.stdout.on('data', (c) => chunks.push(c))
    p.stderr.on('data', (c) => errChunks.push(c))
    p.on('error', reject)
    p.on('exit', (code) => {
      const out = Buffer.concat(chunks).toString('utf8').trim()
      const err = Buffer.concat(errChunks).toString('utf8').trim()
      if (code !== 0) {
        reject(new Error(`resolve openocd argv failed: ${err || out}`))
        return
      }
      try {
        resolve(JSON.parse(out))
      } catch {
        reject(new Error(`bad openocd argv JSON: ${out}`))
      }
    })
  })
}

function killTree(child) {
  if (!child || child.killed) return
  try {
    if (process.platform === 'win32') {
      spawn('taskkill', ['/pid', String(child.pid), '/T', '/F'], {
        stdio: 'ignore',
        windowsHide: true,
      })
    } else {
      child.kill('SIGTERM')
    }
  } catch {
    /* ignore */
  }
}

async function shutdown(code = 0) {
  if (shuttingDown) return
  shuttingDown = true
  console.error('\n[kit] shutting down…')
  for (const { child } of [...children].reverse()) {
    killTree(child)
  }
  // Give taskkill a moment on Windows
  await sleep(400)
  process.exit(code)
}

function ensureHostBuilt() {
  const cli = path.join(HOST_ROOT, 'dist', 'cli.js')
  if (!existsSync(cli)) {
    throw new Error(
      `Host not built (${cli} missing). Run from kit root:\n  npm run setup`,
    )
  }
  return cli
}

async function main() {
  const opts = parseArgs(process.argv.slice(2))
  if (opts.help) {
    usage()
    process.exit(0)
  }

  ensureHostBuilt()

  process.on('SIGINT', () => void shutdown(0))
  process.on('SIGTERM', () => void shutdown(0))

  if (opts.sim) {
    console.log('[kit] sim mode: OpenMicro host + device:sim (no OpenOCD)')
    const cli = ensureHostBuilt()
    // Host needs a real TTY for the wrapped agent UI — do not pipe its stdio.
    spawnLogged(
      'host',
      process.execPath,
      [cli, '--freewili', '--no-hid', opts.agent, ...opts.agentArgs],
      { cwd: HOST_ROOT, stdio: 'inherit', fatal: true },
    )
    await waitForPort('127.0.0.1', OM_PORT, 20000)
    // Prefer tsx from host node_modules for device-sim
    const tsxCli = path.join(HOST_ROOT, 'node_modules', 'tsx', 'dist', 'cli.mjs')
    const simScript = path.join(HOST_ROOT, 'scripts', 'device-sim.ts')
    if (existsSync(tsxCli)) {
      spawnLogged('sim', process.execPath, [tsxCli, simScript, '--action', 'prompt', '--text', 'Say hi'], {
        cwd: HOST_ROOT,
      })
    } else {
      spawnLogged('sim', 'npm', ['run', 'device:sim', '--', '--action', 'prompt', '--text', 'Say hi'], {
        cwd: HOST_ROOT,
        shell: true,
      })
    }
    console.log('[kit] running — Ctrl+C to stop')
    return
  }

  if (!opts.noRtt) {
    console.log('[kit] starting OpenOCD RTT…')
    const argv = await resolveOpenOcdArgv(opts.iface)
    const [exe, ...args] = argv
    spawnLogged('openocd', exe, args, { cwd: WILIBSP, fatal: true })
    try {
      await waitForPort(RTT_HOST, RTT_PORT, 45000)
      console.log(`[kit] RTT ready on ${RTT_HOST}:${RTT_PORT}`)
    } catch (err) {
      console.error('[kit]', err.message)
      console.error('[kit] Is the CMSIS-DAP probe connected? Try --iface 1')
      await shutdown(1)
      return
    }
  } else {
    console.log('[kit] --no-rtt: skipping OpenOCD (start fw rtt yourself, or use CDC)')
  }

  const cli = ensureHostBuilt()
  console.log(`[kit] starting OpenMicro host (${opts.agent})…`)
  // Host needs a real TTY for the wrapped agent UI — do not pipe its stdio.
  spawnLogged(
    'host',
    process.execPath,
    [cli, '--freewili', '--no-hid', opts.agent, ...opts.agentArgs],
    { cwd: HOST_ROOT, stdio: 'inherit', fatal: true },
  )

  try {
    await waitForPort('127.0.0.1', OM_PORT, 20000)
    console.log(`[kit] FreeWili TCP ready on 127.0.0.1:${OM_PORT}`)
  } catch (err) {
    console.error('[kit]', err.message)
    await shutdown(1)
    return
  }

  if (!opts.skipBridge && !opts.noRtt) {
    console.log('[kit] starting RTT bridge…')
    const tsxCli = path.join(HOST_ROOT, 'node_modules', 'tsx', 'dist', 'cli.mjs')
    const bridgeScript = path.join(HOST_ROOT, 'scripts', 'rtt-bridge.ts')
    if (existsSync(tsxCli)) {
      spawnLogged('bridge', process.execPath, [tsxCli, bridgeScript], { cwd: HOST_ROOT })
    } else {
      spawnLogged('bridge', 'npm', ['run', 'bridge:rtt'], { cwd: HOST_ROOT, shell: true })
    }
  }

  console.log('[kit] all processes up — tap Accept on the FreeWili; Ctrl+C to stop')
}

main().catch(async (err) => {
  console.error('[kit] fatal:', err)
  await shutdown(1)
})
