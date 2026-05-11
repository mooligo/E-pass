# E-pass Firmware Architecture

This repository contains firmware for E-pass, a low-power picture display device based on STM32F411.

## 1. Product Goal

E-pass is designed to display images for long periods with very low power draw.

Primary workflow:
1. Phone triggers handshake through NFC.
2. Content is transferred over Wi-Fi through ESP32-C3 or loaded from SD card.
3. STM32F411 decodes/renders to ILI9341 display.
4. Device quickly returns to low-power mode when idle.

## 2. Hardware Targets

- MCU: STM32F411CEU6
- Display: ILI9341, 240x320, SPI
- Storage: MicroSD, 4-bit SDIO
- Wireless: ESP32-C3 over SPI
- NFC: ST25DV over I2C
- Input: two buttons (next/prev), long-press menu
- Power: USB-C + gated rails (backlight and SD power control)

## 2.1 Pin Map (Current Firmware)

This table reflects the current CubeMX pin map in firmware and should be used as the reference for schematic drawing.

| Function | STM32 Pin | Net Label (Firmware) | Direction | Notes |
|---|---|---|---|---|
| Status LED | PA1 | LED | Output | Debug/bring-up indicator |
| LCD Reset | PA2 | SPI1_RST | Output | ILI9341 reset pin |
| LCD D/C | PA3 | SPI1_DC | Output | ILI9341 command/data select |
| LCD CS | PA4 | SPI1_CS | Output | ILI9341 chip select |
| SPI1 SCK | PA5 | SPI1_SCK | AF Output | Shared SPI clock |
| SDIO CMD | PA6 | SDIO_CMD | AF Bidir | SD card command line |
| SPI1 MOSI | PA7 | SPI1_MOSI | AF Output | LCD data out from MCU |
| SDIO D1 | PA8 | SDIO_D1 | AF Bidir | SD 4-bit data |
| SDIO D2 | PA9 | SDIO_D2 | AF Bidir | SD 4-bit data |
| USB FS DM | PA11 | USB_OTG_FS_DM | AF Bidir | USB data minus |
| USB FS DP | PA12 | USB_OTG_FS_DP | AF Bidir | USB data plus |
| Button 1 (Prev) | PB0 | Button_1 | Input/EXTI | Pull-up + falling interrupt |
| Button 2 (Next) | PB1 | Button_2 | Input/EXTI | Pull-up + falling interrupt |
| ST25 I2C SDA | PB3 | ST25_I2C_SDA | AF Open-Drain | I2C2 SDA for ST25DV |
| SPI1 MISO | PB4 | SPI1_MISO | AF Input | Optional for LCD, useful for shared SPI bus |
| SDIO D3 | PB5 | SDIO_D3 | AF Bidir | SD 4-bit data |
| SD Card Detect | PB6 | Detect_SDIO | Input | Active-low card detect input |
| SDIO D0 | PB7 | SDIO_D0 | AF Bidir | SD 4-bit data |
| ST25 I2C SCL | PB10 | ST25_I2C_SCL | AF Open-Drain | I2C2 SCL for ST25DV |
| SDIO CLK | PB15 | SDIO_CK | AF Output | SD clock |

Important schematic notes:
1. LCD backlight control pin is not assigned in firmware yet. Reserve one GPIO for BL_EN or PWM dimming.
2. ST25DV I2C pins are finalized in firmware; ESP32-C3 pins are still not finalized in this revision. Keep schematic placeholders for the ESP32 bus and interrupts.
3. Buttons are active-low with pull-up and EXTI on falling edge.
4. SPI1 currently has dedicated LCD control pins (CS/DC/RST) and can still be shared if bus arbitration is added later.

## 3. Architecture Summary

E-pass uses a layered architecture inspired by X-TRACK principles but simplified for a display-first, low-power product.

### 3.1 Layering

1. HAL layer
2. Core services
3. Application state machine
4. Optional UI adapter layer

### 3.2 Runtime Model

- Event-driven main loop (no busy polling loops)
- Interrupts and callbacks post events into a lightweight queue
- State machine controls render, transfer, and sleep transitions
- STOP mode entered aggressively when idle

## 4. Firmware Layers and Ownership

## 4.1 HAL Layer

Purpose: Isolate hardware details behind clear interfaces.

Planned modules:
- hal_display: ILI9341 init and SPI transfer helpers
- hal_storage: SDIO and file access hooks
- hal_wifi_spi: ESP32-C3 transport channel
- hal_nfc: ST25DV access and interrupt events
- hal_buttons: GPIO/EXTI button input
- hal_power: backlight/rail gating and STOP entry helpers
- hal_time: tick and timer abstraction

## 4.2 Core Services

Purpose: Keep product logic independent of hardware drivers.

Planned modules:
- content_manager: source selection and active image lifecycle
- image_decoder: JPEG and RAW decode/load entry points
- slideshow_engine: index, interval, next/prev behavior
- event_queue: single producer/consumer event transport
- power_manager: sleep policy and wake recovery flow

## 4.3 Application State Machine

Planned states:
- BOOT
- IDLE_SHOW
- LOAD_NEXT
- RENDER
- TRANSFER_WIFI
- TRANSFER_USB
- SLEEP_PREP
- STOP_SLEEP
- WAKE_RECOVER

State transitions are event-driven (button, NFC, transfer complete, timeout, USB attach).

## 4.4 UI Strategy

Hybrid strategy:
- MVP path uses bare display rendering for lower RAM and lower overhead.
- LVGL integration hooks are reserved for future menu expansion.

## 5. Image Pipeline

Dual-path strategy:

1. RAW path (SD card)
- Fast display switching.
- Preferred when content is preprocessed offline.

2. JPEG path (Wi-Fi and SD optional)
- Better storage efficiency.
- Decode and render in tiles/stripes due to STM32F411 RAM limits.

Design rule:
- Avoid full 240x320 RGB565 framebuffer in SRAM.
- Use tile-based rendering to stay within memory budget.

## 6. Data Sources and Transfer Contracts

1. SD card
- Simple folder scan under /images.
- Ordered by filename for slideshow sequence.

2. Wi-Fi via ESP32-C3
- MVP contract: whole-file transfer first, then decode/render.
- Streamed render mode is deferred.

3. USB OTG
- Included in MVP scope.
- Intended for image import workflow and storage access path.

4. NFC
- MVP role: wake and pairing token handoff only.
- Image payload is not transferred over NFC.

## 7. Low-Power Policy

Low power is the first priority.

Policy:
- Enter STOP mode quickly when no pending display/transfer action exists.
- Gate backlight and unnecessary rails before STOP entry.
- Wake on button EXTI, NFC interrupt, and required timers.
- Recover quickly to last valid display state.

## 8. Concurrency and Performance

- DMA-first policy for SPI1 TX and SDIO RX/TX.
- ISR and DMA callbacks only push events; heavy work stays in main loop.
- Timeouts and retries required for SDIO, SPI transport, and I2C NFC operations.

## 9. MVP Feature Scope

Included:
- Picture display from SD and Wi-Fi content path
- Two-button navigation (next/prev)
- Long-press menu with:
	- sleep now
	- source select (SD/Wi-Fi/USB)
	- brightness level
	- slideshow interval
- Aggressive STOP sleep behavior
- USB OTG MVP path

Deferred:
- Multi-page animated UI
- Streaming render protocol over Wi-Fi
- Advanced metadata sync over NFC

## 10. Implementation Phases

Phase A: Platform foundation
- Pin assignment finalization
- DMA setup for SPI1 and SDIO
- IRQ wiring and event queue skeleton

Phase B: Display path
- ILI9341 driver
- RAW render path
- Tile render utility

Phase C: Content ingress
- SD scan and slideshow index
- Wi-Fi whole-file receive contract
- NFC wake/token flow

Phase D: Power hardening
- STOP entry/exit policy
- Wake recovery and stability tests
- Current draw optimization

Phase E: USB integration
- USB image import/storage workflow
- Interaction with sleep policy

## 11. Current Repository Status

The firmware is currently a CubeMX-generated skeleton with initialized peripherals and an empty application loop.

Architecture in this README is the target baseline for upcoming implementation.

Current bring-up behavior:
1. Display is initialized over SPI using ILI9341-compatible command sequence.
2. Button 1 cycles previous color fill, Button 2 cycles next color fill.
3. Long-press toggles status LED for quick input validation.

## 12. USB CDC SD Transport (Current MVP)

The firmware now uses a FatFS-based CDC upload protocol that writes image files into `0:/images`.

Supported commands over USB CDC:
- `PING`
	- Response: `PONG`
- `HELP`
	- Response: command summary
- `MOUNT`
	- Mounts SD filesystem if needed
	- Response: `OK MOUNT` or `ERR MOUNT`
- `START <name> <bytes>`
	- Opens `0:/images/<name>` and prepares to receive `<bytes>` payload bytes
	- Response: `READY <bytes>`
	- Host then streams exactly `<bytes>` binary data bytes
	- Final response on success: `OK WRITE`
- `ABORT`
	- Aborts active transfer and closes file handle
	- Response: `OK ABORT`

Filename constraints:
- Allowed characters: letters, digits, `_`, `-`, `.`
- Parent path traversal (`..`) is rejected

Error responses:
- `ERR CMD` unknown command
- `ERR ARGS` invalid command arguments
- `ERR MOUNT` mount failed
- `ERR MKDIR` failed to create image directory
- `ERR OPEN` failed to open output file
- `ERR BUSY` transfer already active
- `ERR LINE` command line too long
- `ERR WRITE` FatFS write failed
- `ERR EXTRA` received more bytes than declared
