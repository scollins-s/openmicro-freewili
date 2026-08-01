# openmicro_voice — PDM onboard mic → WAV on USB MSC (v2 phase 1)

Offline proof that FreeWili 2 **onboard PDM mics** capture usable audio.
Does **not** stream to Claude yet (see
[`docs/openmicro-freewili/V2-ONBOARD-MIC-SKETCH.md`](../../../../docs/openmicro-freewili/V2-ONBOARD-MIC-SKETCH.md)).

## What it does

1. Shows a simple 480×320 UI: **REC**, status line, mic RMS bar.
2. Tap **REC** → capture **3 seconds** of mono 16 kHz PCM from mic A (after DC block).
3. Writes `0:/openmicro/voice_NNN.wav` on a FAT32 USB stick (host MSC).
4. Amber LED on pixel 0 while recording; green flash on success.

## Pass criteria

- Stick mounted: `usb_store` DIAG shows mount OK.
- Tap REC → RTT prints `recording…` then `wrote 0:/openmicro/voice_NNN.wav (N bytes)`.
- Play the WAV on a PC: speech / taps near the board are audible (not silence / pure DC).

## Flash

```powershell
cd "C:\repos\free wili 2\device stuff\wilibsp"
fw flash openmicro_voice
fw rtt
```

Or from the kit: `npm run flash:voice`.

## Non-goals

- Live stream / virtual mic / Claude `/voice` (later phases).
- PCM over FwGUI / OneWili / RTT.
- USB Audio host microphone.
