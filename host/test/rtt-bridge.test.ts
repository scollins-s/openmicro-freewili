import { describe, expect, it } from 'vitest'
import { parseOmtxLine, wireToAction } from '../scripts/rtt-bridge.ts'

describe('wireToAction', () => {
  it('parses face actions', () => {
    expect(wireToAction('a\\om\\a accept')).toEqual({ type: 'accept' })
    expect(wireToAction('a\\om\\a reject')).toEqual({ type: 'reject' })
    expect(wireToAction('a\\om\\a new_chat')).toEqual({ type: 'new_chat' })
    expect(wireToAction('a\\om\\a push_to_talk')).toEqual({ type: 'push_to_talk' })
    expect(wireToAction('a\\om\\a model')).toEqual({ type: 'open_model' })
    expect(wireToAction('a\\om\\a open_model')).toEqual({ type: 'open_model' })
    expect(wireToAction('a\\om\\a resume')).toEqual({ type: 'resume_session' })
  })

  it('parses workflow focus thinking layer keys', () => {
    expect(wireToAction('a\\om\\w review-pr')).toEqual({
      type: 'workflow',
      presetId: 'review-pr',
    })
    expect(wireToAction('a\\om\\f 2')).toEqual({ type: 'focus_session', index: 2 })
    expect(wireToAction('a\\om\\t -1')).toEqual({ type: 'thinking_depth', delta: -1 })
    expect(wireToAction('a\\om\\l 3')).toEqual({ type: 'layer', index: 3 })
    expect(wireToAction('a\\om\\k up')).toEqual({ type: 'keys', bytes: 'up' })
  })

  it('ignores ping and unknown', () => {
    expect(wireToAction('a\\om\\a ping')).toBeNull()
    expect(wireToAction('a\\s\\r')).toBeNull()
  })
})

describe('parseOmtxLine', () => {
  it('extracts wire after OMTX prefix', () => {
    expect(parseOmtxLine('OMTX a\\om\\a accept')).toBe('a\\om\\a accept')
    expect(parseOmtxLine('12:34 om_link: OMTX a\\om\\w debug')).toBe('a\\om\\w debug')
    expect(parseOmtxLine('hello')).toBeNull()
  })
})
