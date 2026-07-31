import { describe, expect, it } from 'vitest'
import {
  LineBuffer,
  encodeMessage,
  parseMessage,
  PROTOCOL_VERSION,
  DEFAULT_TCP_PORT,
} from '../src/freewili/protocol.js'

describe('freewili protocol', () => {
  it('exports version + default TCP port', () => {
    expect(PROTOCOL_VERSION).toBe(1)
    expect(DEFAULT_TCP_PORT).toBe(48763)
  })

  it('encodeMessage appends a newline', () => {
    const line = encodeMessage({ v: 1, type: 'ping' })
    expect(line.endsWith('\n')).toBe(true)
    expect(JSON.parse(line.trim())).toEqual({ v: 1, type: 'ping' })
  })

  it('parseMessage round-trips hello / action / feedback', () => {
    const hello = parseMessage(
      encodeMessage({
        v: 1,
        type: 'hello',
        role: 'device',
        fw: 't',
        uiVersion: 1,
      }),
    )
    expect(hello).toMatchObject({ type: 'hello', role: 'device' })

    const action = parseMessage(
      encodeMessage({ v: 1, type: 'action', action: { type: 'accept' } }),
    )
    expect(action).toMatchObject({ type: 'action', action: { type: 'accept' } })

    const feedback = parseMessage(
      encodeMessage({
        v: 1,
        type: 'feedback',
        sessions: [{ state: 'idle' }],
        focusIndex: 0,
        lightbar: { r: 1, g: 2, b: 3 },
        playerLeds: 1,
        layer: 0,
      }),
    )
    expect(feedback).toMatchObject({ type: 'feedback', layer: 0 })
  })

  it('parseMessage returns null for blank lines', () => {
    expect(parseMessage('')).toBeNull()
    expect(parseMessage('   \n')).toBeNull()
  })

  it('parseMessage throws on unsupported shape', () => {
    expect(() => parseMessage('{"v":2,"type":"ping"}\n')).toThrow(/unsupported/)
    expect(() => parseMessage('not-json\n')).toThrow()
  })

  it('LineBuffer splits incremental chunks', () => {
    const buf = new LineBuffer()
    expect(buf.push('{"v":1,')).toEqual([])
    expect(buf.push('"type":"ping"}\n{"v":1,"type":"pong"}\npartial')).toEqual([
      '{"v":1,"type":"ping"}',
      '{"v":1,"type":"pong"}',
    ])
    expect(buf.push('\n')).toEqual(['partial'])
    buf.clear()
    expect(buf.push('x\n')).toEqual(['x'])
  })
})
