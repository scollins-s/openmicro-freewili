// PC-side OneWili serial client: probe FreeWili USB CDC and send wire commands.
// Used by spike scripts; the OpenMicro bridge can also open a COM path directly
// for the line-delimited JSON protocol (once main firmware relays it).

import { logger } from '../logger.js'

export interface ProbeResult {
  ok: boolean
  portPath: string
  detail: string
  sample?: string
}

export interface ListedPort {
  path: string
  manufacturer?: string
  vendorId?: string
  productId?: string
  friendlyName?: string
}

const RESET = 0x02
const DEFAULT_BAUD = 115200
const PROBE_TIMEOUT_MS = 2500

/** FreeWili / RP2350 USB VID hints (hex, case-insensitive). */
const VID_HINTS = new Set(['2e8a', '093c', '0525', '1d50'])

function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

/**
 * List serial ports; prefer FreeWili/RP VID/manufacturer hints when available.
 */
export async function listCandidatePorts(): Promise<ListedPort[]> {
  try {
    const { SerialPort } = await import('serialport')
    const ports = await SerialPort.list()
    const mapped: ListedPort[] = ports.map((p) => ({
      path: p.path,
      manufacturer: p.manufacturer,
      vendorId: p.vendorId,
      productId: p.productId,
      friendlyName: (p as { friendlyName?: string }).friendlyName,
    }))
    const score = (p: ListedPort): number => {
      const blob = `${p.manufacturer ?? ''} ${p.friendlyName ?? ''} ${p.path}`.toLowerCase()
      let s = 0
      if (p.vendorId && VID_HINTS.has(p.vendorId.toLowerCase())) s += 10
      if (/free.?wili|intrepid|raspberry|pico|rp2|cmsis|dap/.test(blob)) s += 5
      if (/bluetooth|bt/.test(blob)) s -= 20
      return s
    }
    return mapped.sort((a, b) => score(b) - score(a) || a.path.localeCompare(b.path))
  } catch (err) {
    logger.warn('serialport list failed', err)
    return []
  }
}

/**
 * Open a COM/tty, send OneWili reset + `i\\g\\u\\n`, and look for a `[` response frame.
 */
export async function probe(portPath: string, baudRate = DEFAULT_BAUD): Promise<ProbeResult> {
  let SerialPort: typeof import('serialport').SerialPort
  try {
    ;({ SerialPort } = await import('serialport'))
  } catch (err) {
    return {
      ok: false,
      portPath,
      detail: `serialport unavailable: ${(err as Error).message}`,
    }
  }

  const port = new SerialPort({ path: portPath, baudRate, autoOpen: false })
  const chunks: Buffer[] = []
  try {
    await new Promise<void>((resolve, reject) => {
      port.open((err) => (err ? reject(err) : resolve()))
    })
    port.on('error', () => {
      /* swallowed — probe result covers failures */
    })
    port.on('data', (d: Buffer) => chunks.push(Buffer.from(d)))

    // Flush any stale menu noise.
    try {
      port.flush()
    } catch {
      /* ignore */
    }
    await sleep(80)

    try {
      port.write(Buffer.from([RESET]))
    } catch (err) {
      return { ok: false, portPath, detail: `write reset failed: ${(err as Error).message}` }
    }
    await sleep(120)
    // GPIO read-all — stock OneWili path that answers with a framed `[...]` line.
    try {
      port.write(Buffer.from('i\\g\\u\n', 'utf8'))
    } catch (err) {
      return { ok: false, portPath, detail: `write probe failed: ${(err as Error).message}` }
    }

    const deadline = Date.now() + PROBE_TIMEOUT_MS
    while (Date.now() < deadline) {
      const sample = Buffer.concat(chunks).toString('utf8')
      if (sample.includes('[')) {
        return {
          ok: true,
          portPath,
          detail: 'saw OneWili-style `[` response frame',
          sample: sample.slice(0, 200),
        }
      }
      await sleep(50)
    }
    const sample = Buffer.concat(chunks).toString('utf8')
    return {
      ok: false,
      portPath,
      detail: sample
        ? `no framed response (got ${sample.length} bytes)`
        : 'no serial data (device silent or wrong port)',
      sample: sample.slice(0, 200) || undefined,
    }
  } catch (err) {
    return { ok: false, portPath, detail: (err as Error).message }
  } finally {
    try {
      if (port.isOpen) port.close()
    } catch {
      /* ignore */
    }
  }
}

/**
 * Auto-detect: walk preferred ports and return the first successful probe.
 */
export async function findFreeWiliPort(): Promise<ProbeResult | null> {
  const ports = await listCandidatePorts()
  for (const p of ports) {
    const result = await probe(p.path)
    if (result.ok) return result
  }
  return null
}
