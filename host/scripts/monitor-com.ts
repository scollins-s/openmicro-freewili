#!/usr/bin/env tsx
// Dump bytes from FreeWili main USB CDC (OneWili text port) while tapping the UI.
// Use this to see whether display→main wires ever appear on the PC.
//
//   npm run monitor:com -- COM15
//   npm run monitor:com -- --list

import { SerialPort } from 'serialport'

function usage(): never {
  console.log(`monitor:com — raw dump of a FreeWili USB serial port

Usage:
  npm run monitor:com -- --list
  npm run monitor:com -- COMx
  npm run monitor:com -- COMx --baud 115200

Tap Accept/Reject on the device while this runs.
- If you see nothing: display wires are not reaching this COM (or wrong port).
- If you see OneWili/menu text but no a\\om\\: main got traffic but not our cmds.
- OpenMicro only accepts line-JSON on --freewili-serial, not a\\om\\ wires.
`)
  process.exit(0)
}

async function listPorts(): Promise<void> {
  const ports = await SerialPort.list()
  for (const p of ports) {
    console.log(
      `${p.path}\tvid=${p.vendorId ?? '-'} pid=${p.productId ?? '-'}\t${p.friendlyName ?? p.manufacturer ?? ''}`,
    )
  }
}

async function main(): Promise<void> {
  const argv = process.argv.slice(2)
  if (argv.length === 0 || argv.includes('--help') || argv.includes('-h')) usage()
  if (argv.includes('--list')) {
    await listPorts()
    return
  }

  const path = argv.find((a) => !a.startsWith('-'))
  if (!path) usage()
  let baud = 115200
  const bi = argv.indexOf('--baud')
  if (bi >= 0 && argv[bi + 1]) baud = Number(argv[bi + 1])

  const port = new SerialPort({ path: path!, baudRate: baud, autoOpen: false })
  await new Promise<void>((resolve, reject) => {
    port.open((err) => (err ? reject(err) : resolve()))
  })
  console.log(`listening on ${path} @ ${baud} (Ctrl+C to quit)`)
  console.log('tap FreeWili buttons and watch for data…\n')

  port.on('data', (buf: Buffer) => {
    const hex = [...buf].map((b) => b.toString(16).padStart(2, '0')).join(' ')
    const text = buf.toString('utf8').replace(/[^\x20-\x7e\r\n\t]/g, '.')
    const ts = new Date().toISOString().slice(11, 23)
    console.log(`[${ts}] ${buf.length}B  hex=${hex}`)
    if (text.trim()) console.log(`         text=${JSON.stringify(text)}`)
  })
  port.on('error', (err) => console.error('serial error', err))
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
