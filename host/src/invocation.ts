// Command-line parsing: `openmicro [claude|codex] [...userArgs]`.
//
// The first token is a harness kind only when it is a bare word (not a flag);
// otherwise everything is passed straight to the default harness (claude). The
// kind is NOT validated here — the cli resolves it via harnessFor, which throws
// a clear "unknown harness" error listing the registered kinds.
//
// FreeWili host flags (`--freewili`, `--freewili-tcp`, `--freewili-serial`,
// `--no-hid`) may appear anywhere and are stripped from agentArgs.

export interface ParsedInvocation {
  /** Harness kind to run. Defaults to 'claude'. Validated later by harnessFor. */
  kind: string
  /** Arguments forwarded verbatim to the agent CLI. */
  agentArgs: string[]
  /** True when `--help`/`-h` was requested (cli prints usage and exits). */
  help: boolean
  /** True when `--version`/`-V` was requested (cli prints openmicro's version and exits). */
  version: boolean
  /** True when the `doctor` subcommand was requested (cli runs the diagnostic and exits). */
  doctor: boolean
  /** True when `doctor --capture` was requested (force raw capture-only mode). */
  doctorCapture: boolean
  /** Enable FreeWili TCP/serial bridge on the host. */
  freewili: boolean
  /** TCP listen port for FreeWili peers; null = protocol default when freewili. */
  freewiliTcp: number | null
  /** Optional COM/tty path for FreeWili serial; null = TCP only. */
  freewiliSerial: string | null
  /** Skip HidManager (gamepad) — FreeWili-only or headless. */
  noHid: boolean
}

const DEFAULT_KIND = 'claude'

function stripFreewiliFlags(args: string[]): {
  rest: string[]
  freewili: boolean
  freewiliTcp: number | null
  freewiliSerial: string | null
  noHid: boolean
} {
  const rest: string[] = []
  let freewili = false
  let freewiliTcp: number | null = null
  let freewiliSerial: string | null = null
  let noHid = false

  for (let i = 0; i < args.length; i++) {
    const a = args[i]!
    if (a === '--freewili') {
      freewili = true
      continue
    }
    if (a === '--no-hid') {
      noHid = true
      continue
    }
    if (a === '--freewili-tcp') {
      freewili = true
      const next = args[i + 1]
      if (next && !next.startsWith('-') && /^\d+$/.test(next)) {
        freewiliTcp = Number(next)
        i++
      } else {
        freewiliTcp = null // protocol default
      }
      continue
    }
    if (a.startsWith('--freewili-tcp=')) {
      freewili = true
      const n = Number(a.slice('--freewili-tcp='.length))
      freewiliTcp = Number.isFinite(n) ? n : null
      continue
    }
    if (a === '--freewili-serial') {
      freewili = true
      const next = args[i + 1]
      if (next && !next.startsWith('-')) {
        freewiliSerial = next
        i++
      }
      continue
    }
    if (a.startsWith('--freewili-serial=')) {
      freewili = true
      freewiliSerial = a.slice('--freewili-serial='.length) || null
      continue
    }
    rest.push(a)
  }
  return { rest, freewili, freewiliTcp, freewiliSerial, noHid }
}

/**
 * Parse process argv (already sliced past node + script).
 *
 * Args:
 *     args (string[]): Raw user arguments.
 *
 * Returns:
 *     ParsedInvocation: kind + forwarded args + help / FreeWili flags.
 */
export function parseInvocation(args: string[]): ParsedInvocation {
  const { rest, freewili, freewiliTcp, freewiliSerial, noHid } = stripFreewiliFlags(args)
  const base: ParsedInvocation = {
    kind: DEFAULT_KIND,
    agentArgs: [],
    help: false,
    version: false,
    doctor: false,
    doctorCapture: false,
    freewili,
    freewiliTcp,
    freewiliSerial,
    noHid,
  }
  if (rest[0] === '--help' || rest[0] === '-h') {
    return { ...base, help: true }
  }
  // Leading --version/-V reports openmicro's own version. To query the agent's
  // instead, name it: `openmicro claude --version`.
  if (rest[0] === '--version' || rest[0] === '-V' || rest[0] === '-v') {
    return { ...base, version: true }
  }
  if (rest[0] === 'doctor') {
    return {
      ...base,
      doctor: true,
      doctorCapture: rest.includes('--capture'),
    }
  }
  // A leading bare word names the harness; a leading flag (or nothing) means
  // "default harness, these are its args".
  if (rest.length > 0 && rest[0] !== undefined && !rest[0].startsWith('-')) {
    return { ...base, kind: rest[0], agentArgs: rest.slice(1) }
  }
  return { ...base, agentArgs: rest }
}

export const USAGE = `openmicro — drive an AI agent CLI with a game controller.

Usage:
  openmicro [claude|codex|codex-app] [...agent args]
                                             Wrap the agent CLI (default: claude);
                                             codex-app drives the Codex desktop app
  openmicro doctor [--capture]               Diagnose your controller, write a report
                                             (--capture: record raw reports only,
                                             for pads the parsers misread)
  openmicro --version                        Show openmicro's version
  openmicro --help                           Show this message

FreeWili (optional host bridge; flags may appear anywhere):
  --freewili                                 Enable TCP bridge (port 48763)
  --freewili-tcp [port]                      TCP listen port (implies --freewili)
  --freewili-serial COMx                     Also open USB serial (implies --freewili)
  --no-hid                                   Skip gamepad HID (FreeWili-only / headless)

The first instance to start becomes the host: it owns the controller and
aggregates agent state. Later instances register as clients and receive
forwarded keystrokes. Remap controls in ~/.openmicro/config.json.`
