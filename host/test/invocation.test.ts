import { describe, expect, it } from 'vitest'
import { parseInvocation } from '../src/invocation.js'

const emptyFw = {
  freewili: false,
  freewiliTcp: null,
  freewiliSerial: null,
  noHid: false,
}

describe('parseInvocation', () => {
  it('defaults to claude with no args', () => {
    expect(parseInvocation([])).toEqual({
      kind: 'claude',
      agentArgs: [],
      help: false,
      version: false,
      doctor: false,
      doctorCapture: false,
      ...emptyFw,
    })
  })

  it('treats a leading flag as claude args, not a harness kind', () => {
    expect(parseInvocation(['--resume', 'x'])).toEqual({
      kind: 'claude',
      agentArgs: ['--resume', 'x'],
      help: false,
      version: false,
      doctor: false,
      doctorCapture: false,
      ...emptyFw,
    })
  })

  it('takes a leading bare word as the harness kind', () => {
    expect(parseInvocation(['codex', '--foo'])).toEqual({
      kind: 'codex',
      agentArgs: ['--foo'],
      help: false,
      version: false,
      doctor: false,
      doctorCapture: false,
      ...emptyFw,
    })
  })

  it('passes an unknown bare word through as the kind (cli validates it)', () => {
    expect(parseInvocation(['gemini'])).toEqual({
      kind: 'gemini',
      agentArgs: [],
      help: false,
      version: false,
      doctor: false,
      doctorCapture: false,
      ...emptyFw,
    })
  })

  it('flags --help', () => {
    expect(parseInvocation(['--help']).help).toBe(true)
    expect(parseInvocation(['-h']).help).toBe(true)
  })

  it('flags the doctor subcommand', () => {
    expect(parseInvocation(['doctor'])).toEqual({
      kind: 'claude',
      agentArgs: [],
      help: false,
      version: false,
      doctor: true,
      doctorCapture: false,
      ...emptyFw,
    })
  })

  it('flags doctor --capture', () => {
    const parsed = parseInvocation(['doctor', '--capture'])
    expect(parsed.doctor).toBe(true)
    expect(parsed.doctorCapture).toBe(true)
  })

  it('strips FreeWili flags from anywhere in argv', () => {
    const parsed = parseInvocation([
      'claude',
      '--freewili',
      '--no-hid',
      '--resume',
      'x',
      '--freewili-tcp',
      '49000',
      '--freewili-serial',
      'COM5',
    ])
    expect(parsed.kind).toBe('claude')
    expect(parsed.agentArgs).toEqual(['--resume', 'x'])
    expect(parsed.freewili).toBe(true)
    expect(parsed.freewiliTcp).toBe(49000)
    expect(parsed.freewiliSerial).toBe('COM5')
    expect(parsed.noHid).toBe(true)
  })

  it('treats --freewili-tcp alone as freewili with default port null', () => {
    const parsed = parseInvocation(['--freewili-tcp', '--resume'])
    expect(parsed.freewili).toBe(true)
    expect(parsed.freewiliTcp).toBeNull()
    expect(parsed.agentArgs).toEqual(['--resume'])
  })
})

describe('--version', () => {
  it.each([['--version'], ['-V'], ['-v']])('%s reports openmicro, not the agent', (flag) => {
    const parsed = parseInvocation([flag])
    expect(parsed.version).toBe(true)
    expect(parsed.agentArgs).toEqual([])
  })

  it('passes --version through when a harness is named', () => {
    const parsed = parseInvocation(['claude', '--version'])
    expect(parsed.version).toBe(false)
    expect(parsed.agentArgs).toEqual(['--version'])
  })
})
