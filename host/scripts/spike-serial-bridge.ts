#!/usr/bin/env tsx
// Serial + TCP bridge spike. Lists COM ports, probes OneWili on each candidate,
// starts FreeWiliBridge briefly, connects a device-sim style client, sends
// accept, and verifies feedback round-trip. Exit 0 on TCP success even without hardware.

import net from 'node:net'
import {
  FreeWiliBridge,
  DEFAULT_TCP_PORT,
  encodeMessage,
  parseMessage,
  LineBuffer,
  listCandidatePorts,
  probe,
} from '../src/freewili/index.js'
import type { Action } from '../src/harness/types.js'

const TCP_PORT = DEFAULT_TCP_PORT
const ROUNDTRIP_MS = 5000

function sleep(ms: number): Promise<void> {
  return new Promise((r) => setTimeout(r, ms))
}

async function tcpRoundTrip(): Promise<boolean> {
  const seen: Action[] = []
  const bridge = new FreeWiliBridge({
    tcpPort: TCP_PORT,
    onAction: (a) => seen.push(a),
  })
  await bridge.start()
  bridge.pushConfig({ workflows: { review: 'review' }, layerNames: ['L0'] })

  const gotFeedback = await new Promise<boolean>((resolve) => {
    const sock = net.connect({ host: '127.0.0.1', port: TCP_PORT })
    const buf = new LineBuffer()
    let feedbackOk = false
    let hostHello = false
    const timer = setTimeout(() => {
      sock.destroy()
      resolve(feedbackOk)
    }, ROUNDTRIP_MS)

    const finishIfReady = (): void => {
      if (feedbackOk && seen.some((a) => a.type === 'accept')) {
        clearTimeout(timer)
        sock.end()
        resolve(true)
      }
    }

    sock.setEncoding('utf8')
    sock.on('connect', () => {
      sock.write(
        encodeMessage({
          v: 1,
          type: 'hello',
          role: 'device',
          fw: 'spike',
          uiVersion: 1,
        }),
      )
    })
    sock.on('data', (chunk: string) => {
      for (const line of buf.push(chunk)) {
        try {
          const msg = parseMessage(line)
          if (!msg) continue
          if (msg.type === 'hello' && 'role' in msg && msg.role === 'host' && !hostHello) {
            hostHello = true
            sock.write(encodeMessage({ v: 1, type: 'action', action: { type: 'accept' } }))
            // Give the server a tick to parse the action before feedback broadcast.
            setTimeout(() => {
              bridge.pushFeedback({
                sessions: [{ state: 'waiting' }],
                focusIndex: 0,
                lightbar: { r: 255, g: 176, b: 0 },
                playerLeds: 1,
                layer: 0,
                layerName: 'L0',
                aggregateState: 'waiting',
              })
            }, 50)
          }
          if (msg.type === 'feedback') {
            feedbackOk = true
            finishIfReady()
          }
        } catch {
          /* ignore */
        }
      }
    })
    sock.on('error', () => {
      clearTimeout(timer)
      resolve(false)
    })

    // Also poll for action arrival (may beat feedback).
    const poll = setInterval(() => {
      finishIfReady()
      if (feedbackOk && seen.some((a) => a.type === 'accept')) clearInterval(poll)
    }, 20)
    setTimeout(() => clearInterval(poll), ROUNDTRIP_MS)
  })

  // Drain any late action delivery.
  await sleep(100)
  await bridge.stop()
  const actionOk = seen.some((a) => a.type === 'accept')
  console.log(`TCP action received: ${actionOk}`)
  console.log(`TCP feedback received: ${gotFeedback}`)
  return actionOk && gotFeedback
}

async function main(): Promise<void> {
  console.log('=== openmicro spike:serial ===')
  console.log(`node: ${process.version}`)

  const ports = await listCandidatePorts()
  console.log(`serial ports (${ports.length}):`)
  for (const p of ports) {
    console.log(
      `  ${p.path}  vid=${p.vendorId ?? '-'} pid=${p.productId ?? '-'}  ${p.manufacturer ?? ''}`,
    )
  }

  let anyProbe = false
  for (const p of ports) {
    if (process.platform === 'win32' && !/^COM\d+$/i.test(p.path)) continue
    console.log(`probing ${p.path}…`)
    try {
      const r = await probe(p.path)
      console.log(`  ${r.ok ? 'OK' : 'FAIL'}: ${r.detail}`)
      if (r.ok) anyProbe = true
    } catch (err) {
      console.log(`  FAIL: ${(err as Error).message}`)
    }
  }
  if (!anyProbe) console.log('(no OneWili hardware — continuing with TCP harness)')

  const ok = await tcpRoundTrip()
  if (!ok) {
    console.error('FAIL: TCP bridge round-trip')
    process.exit(1)
  }
  console.log('OK: TCP FreeWili bridge round-trip')
  process.exit(0)
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
