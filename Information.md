# 🤖 AI Agent Context: Project Petite-Frame

## 1. Project Identity & Goal
* **Name:** Petite-Frame (Desk-Frame)
* **Target:** Low-power, high-fidelity smart art display.
* **Core Workflow:** Phone (NFC/Wi-Fi) -> STM32 -> SD Card -> ILI9341-IPS Screen.

## 2. Hardware Specification (The "Hard" Constraints)
* **MCU:** STM32F411CEU6 (BlackPill) .
* **Display:** ILI9341-IPS (240x320) via SPI.
* **Storage:** MicroSD via 4-bit SDIO (SDIO1).
* **Wireless:** ESP32-C3 via SPI for Wi-Fi/BLE.
* **NFC:** ST25DV for NDEF Handover.
* **PCB:** 4-Layer Stackup (Sig/GND/PWR/Sig).
* **Power:** USB-C (5V) -> 3.3V Buck. Hardware-gated Backlight and SD power via P-MOSFETs.

## 3. Peripheral Pin Mapping (Reference Table)
| Peripheral | Signal | STM32 Pin | Notes |

 
## 4. Firmware Architecture Requirements
* **HAL:** STM32Cube HAL.
* **SYS:** Bare Metal.
* **USB Class:** MSC (Mass Storage) for SD access via PC.
* **Memory Management:** Use DMA for SPI (Display) and SDIO (Storage) to ensure non-blocking performance.

## 5. AI Instructions for Code Generation
1.  
2.  **User Code Blocks:** Always wrap generated code in `/* USER CODE BEGIN ... */` tags compatible with STM32CubeMX.
3.  **Low Power First:** When suggesting logic, prioritize `HAL_PWR_EnterSTOPMode` and peripheral suspension.


## 6. Current Progress & Focus
* [X] Hardware selection (ILI9341 + F411).
