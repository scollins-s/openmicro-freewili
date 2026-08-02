#!/usr/bin/env tsx
// TCP device simulator for the FreeWili OpenMicro protocol.
// Connects to 127.0.0.1:48763, sends device hello, prints host feedback,
// and can fire actions via --action <type> [args].

import net from 'node:net'
import {
  DEFAULT_TCP_PORT,
  LineBuffer,
  encodeMessage,
  parseMessage,
} from '../src/freewili/protocol.js'
import type { Action } from '../src/harness/types.js'

function usage(): never {
  console.log(`device-sim — FreeWili TCP client for OpenMicro

Usage (from demo_inspo/OpenMicro):
  npm run device:sim -- --action accept
  npm run device:sim -- --action reject
  npm run device:sim -- --action new_chat
  npm run device:sim -- --action push_to_talk
  npm run device:sim -- --action keys --text model
  npm run device:sim -- --action open_model
  npm run device:sim -- --action prompt --text "Say exactly: openmicro freewili ok"
  npm run device:sim -- --action workflow --preset review-pr
  npm run device:sim -- --action thinking_depth --delta 1
  npm run device:sim -- --action layer --index 1
  npm run device:sim -- --action focus_session --index 0
  npm run device:sim -- --action keys --text up

  # Or raw JSON:
  npm run device:sim -- --action "{\"type\":\"prompt\",\"text\":\"ping\"}"

Keep openmicro running with: node dist/cli.js --freewili --no-hid claude
`)
  process.exit(0)
}

function normalizeType(raw: string): string {
  return raw.trim().replace(/-/g, '_')
}

function parseAction(
  actionType: string,
  opts: {
    preset?: string
    text?: string
    index?: number
    delta?: 1 | -1
  },
): Action {
  // Allow full JSON: --action '{"type":"prompt","text":"hi"}'
  if (actionType.startsWith('{')) {
    const parsed = JSON.parse(actionType) as Action
    if (!parsed || typeof parsed !== 'object' || !('type' in parsed)) {
      throw new Error(`invalid action JSON: ${actionType}`)
    }
    return parsed
  }

  const t = normalizeType(actionType)
  switch (t) {
    case 'accept':
    case 'reject':
    case 'new_chat':
    case 'open_model':
    case 'resume_session':
    case 'push_to_talk':
    case 'herdr_space':
      return { type: t }
    case 'model':
      return { type: 'open_model' }
    case 'workflow':
      return { type: 'workflow', presetId: opts.preset ?? 'review-pr' }
    case 'focus_session':
      return { type: 'focus_session', index: opts.index ?? 0 }
    case 'layer':
      return { type: 'layer', index: opts.index ?? 0 }
    case 'thinking_depth':
      return { type: 'thinking_depth', delta: opts.delta === -1 ? -1 : 1 }
    case 'prompt':
      return {
        type: 'prompt',
        text: opts.text ?? opts.preset ?? 'Say exactly: openmicro freewili ok',
      }
    case 'keys': {
      const name = opts.text ?? opts.preset ?? 'up'
      const arrows: Record<string, string> = {
        up: '\x1b[A',
        down: '\x1b[B',
        right: '\x1b[C',
        left: '\x1b[D',
      }
      if (name === 'model' || name === '/model') return { type: 'open_model' }
      if (name === 'resume' || name === '/resume') return { type: 'resume_session' }
      return { type: 'keys', bytes: arrows[name] ?? name }
    }
    default:
      throw new Error(`unknown action: ${actionType}`)
  }
}

function parseArgs(argv: string[]): {
  port: number
  action: Action | null
  help: boolean
} {
  let port = DEFAULT_TCP_PORT
  let help = false
  let actionType: string | null = null
  let preset: string | undefined
  let text: string | undefined
  let index: number | undefined
  let delta: 1 | -1 | undefined

  for (let i = 0; i < argv.length; i++) {
    const a = argv[i]!
    if (a === '--help' || a === '-h') help = true
    else if (a === '--port') port = Number(argv[++i])
    else if (a === '--action') actionType = argv[++i] ?? null
    else if (a === '--preset') preset = argv[++i]
    else if (a === '--text') text = argv[++i]
    else if (a === '--index') index = Number(argv[++i])
    else if (a === '--delta') delta = Number(argv[++i]) as 1 | -1
  }

  let action: Action | null = null
  if (actionType) {
    try {
      action = parseAction(actionType, { preset, text, index, delta })
    } catch (err) {
      console.error((err as Error).message)
      process.exit(1)
    }
  }
  return { port, action, help }
}

async function main(): Promise<void> {
  const { port, action, help } = parseArgs(process.argv.slice(2))
  if (help) usage()

  const sock = net.connect({ host: '127.0.0.1', port })
  const buf = new LineBuffer()

  await new Promise<void>((resolve, reject) => {
    sock.once('connect', () => resolve())
    sock.once('error', reject)
  })
  sock.setEncoding('utf8')
  console.log(`connected to 127.0.0.1:${port}`)

  sock.write(
    encodeMessage({
      v: 1,
      type: 'hello',
      role: 'device',
      fw: 'device-sim',
      uiVersion: 1,
    }),
  )

  if (action) {
    sock.write(encodeMessage({ v: 1, type: 'action', action }))
    console.log('sent action', action)
  }

  sock.on('data', (chunk: string) => {
    for (const line of buf.push(chunk)) {
      try {
        const msg = parseMessage(line)
        if (!msg) continue
        console.log('←', JSON.stringify(msg))
      } catch (err) {
        console.log('← (bad)', line, err)
      }
    }
  })

  sock.on('close', () => {
    console.log('disconnected')
    process.exit(0)
  })

  // Keep alive until Ctrl+C unless we only wanted a one-shot action.
  if (action) {
    setTimeout(() => {
      sock.end()
    }, 1500)
  } else {
    console.log('listening for feedback (Ctrl+C to quit)…')
  }
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
