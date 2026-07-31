#!/usr/bin/env node
/**
 * Best-effort stop helper if a previous start.mjs left orphans.
 * Prefer Ctrl+C on the start.mjs terminal; this is a fallback.
 *
 * Kills listeners on FreeWili TCP (48763) and OpenOCD RTT (9090) when possible.
 */

import { execSync } from 'node:child_process'

const PORTS = [48763, 9090]

function pidsOnPort(port) {
  try {
    if (process.platform === 'win32') {
      const out = execSync(`netstat -ano | findstr :${port}`, { encoding: 'utf8' })
      const pids = new Set()
      for (const line of out.split(/\r?\n/)) {
        if (!line.includes('LISTENING')) continue
        const parts = line.trim().split(/\s+/)
        const pid = parts[parts.length - 1]
        if (pid && /^\d+$/.test(pid) && pid !== '0') pids.add(pid)
      }
      return [...pids]
    }
    const out = execSync(`lsof -tiTCP:${port} -sTCP:LISTEN`, { encoding: 'utf8' })
    return out
      .split(/\s+/)
      .map((s) => s.trim())
      .filter(Boolean)
  } catch {
    return []
  }
}

for (const port of PORTS) {
  const pids = pidsOnPort(port)
  if (pids.length === 0) {
    console.log(`port ${port}: idle`)
    continue
  }
  for (const pid of pids) {
    console.log(`port ${port}: killing pid ${pid}`)
    try {
      if (process.platform === 'win32') {
        execSync(`taskkill /pid ${pid} /T /F`, { stdio: 'inherit' })
      } else {
        execSync(`kill ${pid}`, { stdio: 'inherit' })
      }
    } catch (err) {
      console.error(`failed to kill ${pid}:`, err.message)
    }
  }
}
