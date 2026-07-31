// FreeWili <-> OpenMicro host bridge: TCP (and optional serial) line protocol.
// Device clients send actions; the host broadcasts feedback/config to all peers.

import { createServer, type Server, type Socket } from 'node:net'
import { readFileSync } from 'node:fs'
import type { Action } from '../harness/types.js'
import { logger } from '../logger.js'
import {
  DEFAULT_TCP_PORT,
  LineBuffer,
  encodeMessage,
  parseMessage,
  type HostToDevice,
  type OmMessage,
  type SessionFeedback,
} from './protocol.js'
import type { RGB } from '../feedback.js'
import type { AgentState } from '../harness/types.js'

export interface FreeWiliBridgeOptions {
  tcpPort?: number
  /** Optional COM/tty path; opens via dynamic `serialport` import. */
  serialPath?: string | null
  onAction?: (action: Action) => void
  hostVersion?: string
}

function packageVersion(): string {
  try {
    const pkg = JSON.parse(
      readFileSync(new URL('../../package.json', import.meta.url), 'utf8'),
    ) as { version: string }
    return pkg.version
  } catch {
    return '0.0.0'
  }
}

interface Peer {
  write(data: string): void
  destroy(): void
}

/**
 * Listens for FreeWili protocol peers (TCP + optional USB serial) and routes
 * device actions into the OpenMicro host while broadcasting feedback.
 */
export class FreeWiliBridge {
  readonly tcpPort: number
  readonly serialPath: string | null
  onAction: ((action: Action) => void) | null
  private readonly hostVersion: string
  private server: Server | null = null
  private serial: { close(): void; write(data: string | Buffer): void } | null = null
  private readonly peers = new Set<Peer>()
  private started = false

  constructor(opts: FreeWiliBridgeOptions = {}) {
    this.tcpPort = opts.tcpPort ?? DEFAULT_TCP_PORT
    this.serialPath = opts.serialPath ?? null
    this.onAction = opts.onAction ?? null
    this.hostVersion = opts.hostVersion ?? packageVersion()
  }

  async start(): Promise<void> {
    if (this.started) return
    this.started = true
    await new Promise<void>((resolve, reject) => {
      const server = createServer((socket) => this.attachSocket(socket))
      server.once('error', reject)
      server.listen(this.tcpPort, '127.0.0.1', () => {
        server.off('error', reject)
        this.server = server
        logger.info(`freewili TCP bridge listening on 127.0.0.1:${this.tcpPort}`)
        resolve()
      })
    })
    if (this.serialPath) {
      await this.openSerial(this.serialPath)
    }
  }

  async stop(): Promise<void> {
    this.started = false
    for (const peer of this.peers) peer.destroy()
    this.peers.clear()
    if (this.serial) {
      try {
        this.serial.close()
      } catch {
        /* ignore */
      }
      this.serial = null
    }
    const server = this.server
    this.server = null
    if (server) {
      await new Promise<void>((resolve) => server.close(() => resolve()))
    }
  }

  pushFeedback(fb: {
    sessions: SessionFeedback[]
    focusIndex: number
    lightbar: RGB
    playerLeds: number
    layer: number
    layerName?: string
    aggregateState?: AgentState
  }): void {
    this.broadcast({
      v: 1,
      type: 'feedback',
      sessions: fb.sessions,
      focusIndex: fb.focusIndex,
      lightbar: fb.lightbar,
      playerLeds: fb.playerLeds,
      layer: fb.layer,
      layerName: fb.layerName,
      aggregateState: fb.aggregateState,
    })
  }

  pushConfig(config: { workflows: Record<string, string>; layerNames: string[] }): void {
    this.broadcast({
      v: 1,
      type: 'config',
      workflows: config.workflows,
      layerNames: config.layerNames,
    })
  }

  private broadcast(msg: HostToDevice): void {
    const line = encodeMessage(msg)
    for (const peer of this.peers) {
      try {
        peer.write(line)
      } catch (err) {
        logger.warn('freewili broadcast failed', err)
      }
    }
  }

  private hostHello(): HostToDevice {
    return { v: 1, type: 'hello', role: 'host', hostVersion: this.hostVersion }
  }

  private attachSocket(socket: Socket): void {
    const buf = new LineBuffer()
    const peer: Peer = {
      write: (data) => {
        if (!socket.destroyed) socket.write(data)
      },
      destroy: () => {
        socket.destroy()
      },
    }
    this.peers.add(peer)
    socket.setEncoding('utf8')
    socket.write(encodeMessage(this.hostHello()))
    socket.on('data', (chunk: string) => this.handleChunk(buf, chunk))
    socket.on('close', () => this.peers.delete(peer))
    socket.on('error', () => this.peers.delete(peer))
  }

  private async openSerial(path: string): Promise<void> {
    try {
      const { SerialPort } = await import('serialport')
      const port = new SerialPort({ path, baudRate: 115200, autoOpen: false })
      await new Promise<void>((resolve, reject) => {
        port.open((err) => (err ? reject(err) : resolve()))
      })
      const buf = new LineBuffer()
      const peer: Peer = {
        write: (data) => {
          if (port.isOpen) port.write(data)
        },
        destroy: () => {
          try {
            port.close()
          } catch {
            /* ignore */
          }
        },
      }
      this.peers.add(peer)
      this.serial = port
      port.setEncoding('utf8')
      port.write(encodeMessage(this.hostHello()))
      port.on('data', (chunk: string | Buffer) =>
        this.handleChunk(buf, typeof chunk === 'string' ? chunk : chunk.toString('utf8')),
      )
      port.on('close', () => {
        this.peers.delete(peer)
        this.serial = null
      })
      port.on('error', (err) => logger.warn('freewili serial error', err))
      logger.info(`freewili serial bridge open on ${path}`)
    } catch (err) {
      logger.warn(`freewili serial open failed (${path})`, err)
    }
  }

  private handleChunk(buf: LineBuffer, chunk: string): void {
    for (const line of buf.push(chunk)) {
      let msg: OmMessage | null
      try {
        msg = parseMessage(line)
      } catch (err) {
        logger.warn('freewili bad message', err)
        continue
      }
      if (!msg) continue
      if (msg.type === 'action' && 'action' in msg) {
        try {
          this.onAction?.(msg.action)
        } catch (err) {
          logger.error('freewili onAction failed', err)
        }
        continue
      }
      if (msg.type === 'ping') {
        this.broadcast({ v: 1, type: 'pong' })
        continue
      }
      if (msg.type === 'hello' && 'role' in msg && msg.role === 'device') {
        logger.info(`freewili device hello fw=${msg.fw} ui=${msg.uiVersion}`)
      }
    }
  }
}
