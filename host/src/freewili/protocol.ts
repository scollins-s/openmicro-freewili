// FreeWili <-> OpenMicro line-delimited JSON protocol (v1).
// Device sends actions; host sends feedback/config. Shared by TCP, serial, and
// the FwGUI/OneWili relay (see docs/ONEWILI-OPENMICRO-COMMANDS.md).

import type { Action, AgentState } from '../harness/types.js'
import type { RGB } from '../feedback.js'

export const PROTOCOL_VERSION = 1 as const
export const DEFAULT_TCP_PORT = 48763

export interface SessionFeedback {
  state: AgentState
  id?: string
}

export type DeviceToHost =
  | { v: 1; type: 'hello'; role: 'device'; fw: string; uiVersion: number }
  | { v: 1; type: 'action'; action: Action }
  | { v: 1; type: 'ping' }
  | { v: 1; type: 'pong' }

export type HostToDevice =
  | { v: 1; type: 'hello'; role: 'host'; hostVersion: string }
  | {
      v: 1
      type: 'feedback'
      sessions: SessionFeedback[]
      focusIndex: number
      lightbar: RGB
      playerLeds: number
      layer: number
      layerName?: string
      aggregateState?: AgentState
    }
  | {
      v: 1
      type: 'config'
      workflows: Record<string, string>
      layerNames: string[]
    }
  | { v: 1; type: 'ping' }
  | { v: 1; type: 'pong' }

export type OmMessage = DeviceToHost | HostToDevice

export function encodeMessage(msg: OmMessage): string {
  return JSON.stringify(msg) + '\n'
}

/**
 * Parse one JSON line. Returns null for blank lines; throws on malformed JSON
 * or unsupported shape (caller decides whether to drop or log).
 */
export function parseMessage(line: string): OmMessage | null {
  const trimmed = line.trim()
  if (!trimmed) return null
  const raw = JSON.parse(trimmed) as Record<string, unknown>
  if (raw.v !== 1 || typeof raw.type !== 'string') {
    throw new Error(`unsupported freewili message: ${trimmed.slice(0, 80)}`)
  }
  return raw as unknown as OmMessage
}

/** Incremental line splitter for TCP/serial byte streams. */
export class LineBuffer {
  private buf = ''

  push(chunk: string): string[] {
    this.buf += chunk
    const lines: string[] = []
    for (;;) {
      const i = this.buf.indexOf('\n')
      if (i < 0) break
      lines.push(this.buf.slice(0, i))
      this.buf = this.buf.slice(i + 1)
    }
    return lines
  }

  clear(): void {
    this.buf = ''
  }
}
