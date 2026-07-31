#!/usr/bin/env tsx
/**
 * RTT → OpenMicro bridge: board taps drive Claude without custom main firmware.
 *
 * Display logs `OMTX a\om\…` over SEGGER RTT. This script:
 *   1) connects to OpenOCD RTT (127.0.0.1:9090)
 *   2) connects to OpenMicro FreeWili TCP (127.0.0.1:48763) as a device peer
 *   3) converts each OMTX wire into a JSON action
 *
 * Terminals:
 *   A) node dist/cli.js --freewili --no-hid claude
 *   B) cd wilibsp && fw rtt
 *   C) npm run bridge:rtt
 */

import net from 'node:net'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import type { Action } from '../src/harness/types.js'
import {
  DEFAULT_TCP_PORT,
  LineBuffer,
  encodeMessage,
  parseMessage,
} from '../src/freewili/protocol.js'

const RTT_HOST = '127.0.0.1'
const RTT_PORT = 9090
const OM_HOST = '127.0.0.1'

function usage(): never {
  console.log(`bridge:rtt — FreeWili RTT taps → OpenMicro TCP

Usage:
  npm run bridge:rtt
  npm run bridge:rtt -- --rtt-port 9090 --om-port 48763

Prereqs:
  1. Flash apps/openmicro to the display CPU (fw flash openmicro)
  2. Start OpenMicro: node dist/cli.js --freewili --no-hid claude
  3. Start RTT:      cd "device stuff/wilibsp" && fw rtt
  4. Run this bridge

Each board tap should print OMTX → action and Claude should react.
`)
  process.exit(0)
}

/** Parse OneWili quiet-path wire into an OpenMicro Action. */
export function wireToAction(wire: string): Action | null {
  let line = wire.trim()
  if (!line) return null
  if (line.charCodeAt(0) === 0x02) line = line.slice(1)

  const prefix = 'a\\om\\'
  if (!line.startsWith(prefix)) return null

  const rest = line.slice(prefix.length) // e.g. "a accept", "w review-pr"
  if (!rest) return null
  const op = rest[0]!
  const args = rest.slice(1).trim()

  switch (op) {
    case 'a': {
      const name = args.replace(/-/g, '_')
      if (
        name === 'accept' ||
        name === 'reject' ||
        name === 'new_chat' ||
        name === 'push_to_talk' ||
        name === 'herdr_space' ||
        name === 'open_model'
      ) {
        return { type: name }
      }
      // FreeWili firmware sends a\om\a model
      if (name === 'model') return { type: 'open_model' }
      return null // ping / unknown
    }
    case 'w':
      return { type: 'workflow', presetId: args || 'review-pr' }
    case 'f': {
      const index = Number(args)
      return { type: 'focus_session', index: Number.isFinite(index) ? index : 0 }
    }
    case 't': {
      const d = Number(args)
      return { type: 'thinking_depth', delta: d < 0 ? -1 : 1 }
    }
    case 'l': {
      const index = Number(args)
      return { type: 'layer', index: Number.isFinite(index) ? index : 0 }
    }
    case 'k':
      return { type: 'keys', bytes: args || 'up' }
    default:
      return null
  }
}

/** Extract OMTX payload from an RTT log line. */
export function parseOmtxLine(line: string): string | null {
  const idx = line.indexOf('OMTX ')
  if (idx < 0) return null
  return line.slice(idx + 5).trim()
}

function parseArgs(argv: string[]): { rttPort: number; omPort: number; help: boolean } {
  let rttPort = RTT_PORT
  let omPort = DEFAULT_TCP_PORT
  let help = false
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i]!
    if (a === '--help' || a === '-h') help = true
    else if (a === '--rtt-port') rttPort = Number(argv[++i])
    else if (a === '--om-port') omPort = Number(argv[++i])
  }
  return { rttPort, omPort, help }
}

function connectTcp(host: string, port: number): Promise<net.Socket> {
  return new Promise((resolve, reject) => {
    const sock = net.connect({ host, port }, () => resolve(sock))
    sock.once('error', reject)
  })
}

async function main(): Promise<void> {
  const { rttPort, omPort, help } = parseArgs(process.argv.slice(2))
  if (help) usage()

  console.log(`OpenMicro TCP ${OM_HOST}:${omPort}`)
  let om: net.Socket
  try {
    om = await connectTcp(OM_HOST, omPort)
  } catch (err) {
    console.error(
      `Cannot connect to OpenMicro on ${omPort}. Start:\n  node dist/cli.js --freewili --no-hid claude\n`,
      err,
    )
    process.exit(1)
  }
  om.setEncoding('utf8')
  om.write(
    encodeMessage({
      v: 1,
      type: 'hello',
      role: 'device',
      fw: 'rtt-bridge',
      uiVersion: 1,
    }),
  )
  console.log('connected to OpenMicro as device peer')

  const omBuf = new LineBuffer()
  om.on('data', (chunk: string) => {
    for (const line of omBuf.push(chunk)) {
      try {
        const msg = parseMessage(line)
        if (msg?.type === 'feedback' || msg?.type === 'hello' || msg?.type === 'config') {
          console.log('← host', msg.type)
        }
      } catch {
        /* ignore */
      }
    }
  })
  om.on('close', () => {
    console.error('OpenMicro disconnected')
    process.exit(1)
  })

  console.log(`RTT ${RTT_HOST}:${rttPort} (start: fw rtt)`)
  let rtt: net.Socket
  try {
    rtt = await connectTcp(RTT_HOST, rttPort)
  } catch (err) {
    console.error(
      `Cannot connect to RTT on ${rttPort}. In another terminal:\n  cd "device stuff/wilibsp" && fw rtt\n`,
      err,
    )
    process.exit(1)
  }
  rtt.setEncoding('utf8')
  console.log('listening for OMTX… tap the FreeWili UI')

  const rttBuf = new LineBuffer()
  rtt.on('data', (chunk: string) => {
    const normalized = chunk.replace(/\r\n/g, '\n').replace(/\r/g, '\n')
    for (const line of rttBuf.push(normalized)) {
      const wire = parseOmtxLine(line)
      if (!wire) continue
      const action = wireToAction(wire)
      if (!action) {
        console.log(`OMTX ignore: ${wire}`)
        continue
      }
      om.write(encodeMessage({ v: 1, type: 'action', action }))
      console.log('OMTX →', action)
    }
  })
  rtt.on('close', () => {
    console.error('RTT disconnected')
    process.exit(1)
  })
  rtt.on('error', (err) => console.error('RTT error', err))
}

const isMain =
  !!process.argv[1] && fileURLToPath(import.meta.url) === path.resolve(process.argv[1])

if (isMain) {
  main().catch((err) => {
    console.error(err)
    process.exit(1)
  })
}
