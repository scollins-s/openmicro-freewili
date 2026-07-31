#!/usr/bin/env node
// Windows host spike: print Node version, build OpenMicro, smoke-import the
// FreeWili protocol module. No agent CLI required.

import { spawnSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'
import path from 'node:path'

const pkgRoot = fileURLToPath(new URL('..', import.meta.url))

function run(cmd, args, opts = {}) {
  const r = spawnSync(cmd, args, {
    cwd: pkgRoot,
    stdio: 'inherit',
    shell: process.platform === 'win32',
    ...opts,
  })
  if (r.status !== 0) {
    process.exit(r.status ?? 1)
  }
}

console.log('=== openmicro spike:windows ===')
console.log(`node: ${process.version}`)
console.log(`platform: ${process.platform} ${process.arch}`)
console.log(`cwd: ${pkgRoot}`)

run('npm', ['run', 'build'])

const distProtocol = path.join(pkgRoot, 'dist', 'freewili', 'protocol.js')
const mod = await import(pathToFileUrl(distProtocol))
if (typeof mod.encodeMessage !== 'function' || typeof mod.parseMessage !== 'function') {
  console.error('FAIL: protocol exports missing after build')
  process.exit(1)
}
const line = mod.encodeMessage({ v: 1, type: 'ping' })
const parsed = mod.parseMessage(line)
if (!parsed || parsed.type !== 'ping') {
  console.error('FAIL: encode/parse round-trip')
  process.exit(1)
}
console.log('OK: build + freewili/protocol smoke import')
process.exit(0)

function pathToFileUrl(p) {
  const resolved = path.resolve(p)
  let pathname = resolved.replace(/\\/g, '/')
  if (!pathname.startsWith('/')) pathname = `/${pathname}`
  return `file://${pathname}`
}
