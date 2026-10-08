# Automotive Instrument Cluster — CAN Bus Communication (LPC2129)

A two-node embedded system simulating an automotive instrument cluster using CAN 2.0B protocol on ARM7TDMI (LPC2129) microcontrollers.
Designed and developed as part of embedded systems training at Vector India, Bengaluru.

## Overview

This project implements a real-time automotive instrument cluster using CAN bus communication between two LPC2129 microcontroller nodes. Node A acts as a sensor/input ECU that reads vehicle data and transmits CAN frames. Node B acts as a display ECU that receives frames and drives an LCD + LED indicators.

The design follows automotive communication patterns:

- Event-triggered frames for switch state changes (headlight, indicators)
- Periodic frames for continuous sensor data (speed, temperature)

## System Architecture
```
  ┌──────────────────────────────────┐         ┌──────────────────────────────────┐
  │           NODE A (TX)            │         │           NODE B (RX)            │
  │         LPC2129 ARM7             │         │         LPC2129 ARM7             │
  │                                  │         │                                  │
  │  LM35 (Temp)   → ADC Ch1         │  CAN    │  CAN RX ISR  → Frame Parser      │
  │  Potentiometer → ADC Ch2         │ ──────► │  LCD Driver  → Speed / Temp      │
  │  EINT0  → Headlight Switch       │  125K   │  LED1 (P0.17) → Left Indicator   │
  │  EINT1  → Left Indicator         │  bps    │  LED2 (P0.18) → Headlight        │
  │  EINT2  → Right Indicator        │         │  LED3 (P0.19) → Right Indicator  │
  │  Timer1 ISR → 10ms tick          │         │  Timer1 ISR  → 80ms blink tick   │
  └──────────────────────────────────┘         └──────────────────────────────────┘
```
## Hardware

| Component | Node A | Node B |
| --- | --- | --- |
| MCU | LPC2129 (ARM7TDMI-S) | LPC2129 (ARM7TDMI-S) |
| CAN Transceiver | MCP2551 (CAN 2.0B) | MCP2551 (CAN 2.0B) |
| Sensors | LM35 (Temp), Potentiometer (Speed) | — |
| Input | 3x Tactile Switches (EINT0/1/2) | — |
| Display | — | 16×2 LCD (4-bit mode, PORT1) |
| Indicators| — |3x LEDs (PORT0: P0.17, P0.18, P0.19) |
| CAN Bus Speed | 125 Kbps | 125 Kbps |
| Crystal | 12 MHz (PLLCLK = 60 MHz) | 12 MHz (PLLCLK = 60 MHz) |

## Features

- CAN 2.0B communication at 125 Kbps
- Dual frame strategy: event-triggered (ID 0x10) + periodic (ID 0x50)
- ADC-based speed averaging over 10 samples (100ms window) for noise rejection
- Interrupt-driven switch detection via EINT0/1/2 with mutual exclusion logic (left/right indicators cannot be active simultaneously)
- Timer1 ISR-driven periodic tasks on Node A (10ms tick for speed ADC sampling)
- Timer1 ISR-driven LED blinking on Node B (80ms tick for indicator blink)
- XOR-based switch state toggle with change-detection before transmitting
- Custom LCD CGRAM characters: left arrow, right arrow, headlight icon, temperature symbol
- ```body_status``` byte as single source of truth for all switch states

## CAN Frame Design

### Event Frame — ID 0x10 (Switch State Change)

| Byte | Content |
|---|---|
| Byte 0 | ```body_status``` — switch state byte |

### Bit map of body_status:
| Bit | Function |
|---|---|
| Bit 0 | Headlight (0 = OFF, 1 = ON) |
| Bit 1 | Left Indicator (0 = OFF, 1 = ON) |
| Bit 2 | Right Indicator (0 = OFF, 1 = ON) |

Transmitted only when switch state changes. Follows AUTOSAR TRIGGERED transmission mode pattern.

### Periodic Frame — ID 0x50 (Sensor Data)

|Bits | Content |
|---|---|
| [7:0] | Speed (0–120 Km/h, mapped from 10-sample ADC average) |
| [15:8] | Temperature (°C, from LM35 via ADC) |
| [23:16] | ```body_status``` (switch state redundancy) |

Transmitted every 150ms. Follows AUTOSAR PERIODIC transmission mode pattern.

CAN ID Priority: ``` 0x10 < 0x50 ``` — Event frame wins bus arbitration, ensuring switch response is never delayed by periodic data.

## Project Structure
```
automotive-instrument-cluster/
│
├── Node_A/
│   ├── header.h                  # Struct definitions, function prototypes
│   ├── main_nodeA.c              # Main loop: ADC sampling, CAN TX logic
│   ├── adc_driver.c              # ADC init and channel read
│   ├── can2_driver.c             # CAN2 init and TX (LPC2129 CAN2 peripheral)
│   └── interrupt_handler.c       # EINT0/1/2 ISRs + Timer1 ISR (10ms tick)
│
├── Node_B/
│   ├── headerr.h                 # Struct definitions, function prototypes
│   ├── main_nodeB.c              # Main loop: CAN RX parse, LCD/LED update
│   ├── lcd_driver.c              # 16x2 LCD init, data/cmd, CGRAM custom chars
│   ├── can1_Rx_driver.c          # CAN2 RX ISR + Timer1 ISR (80ms blink tick)
│   └── delay.c                   # Blocking delay using Timer0
│
├── Docs/
│   └── system_architecture.png   # Block diagram (optional: add photo/schematic)
│
└── README.md
```
## Node A — Transmitter

- Switch Input Handling (EINT ISRs): Each external interrupt ISR sets a flag and clears the EXTINT register. The main loop processes flags, toggles the corresponding bit in ```switch_stat``` using XOR, and sets ```event_ff = 1``` to signal a pending CAN transmission.

- Left/Right indicators are mutually exclusive — activating one unconditionally clears the other's bit before toggling. This is enforced at the source (Node A) so Node B never receives an invalid state.
```
if(Left_flag)
{
    Left_flag = 0;
    switch_stat ^= (1<<1);   // toggle left
    switch_stat &= ~(1<<2);  // force-clear right
    event_ff = 1;
}
```
- Change Detection Before TX: Event frame is only transmitted when ```switch_stat != prev_stat```. This prevents redundant CAN frames from bouncing ISRs.
- Speed Averaging: ADC is sampled every 10ms (Timer1 ISR tick). Average is computed over 10 samples (100ms window) to filter potentiometer noise. Mapped to 0–120 Km/h range.
- Temperature Reading (LM35): LM35 output: 10mV/°C, offset 500mV at 0°C.

```
Vout (V) = (adc_val × 3.3) / 1023
Temp (°C) = (Vout − 0.5) / 0.01
```

### Node B — Receiver / Display

- CAN RX ISR — Minimal Work Pattern: ISR copies raw CAN registers into a shared struct, sets ```RECEIVER_FLAG```, releases the RX buffer ```(C2CMR = 0x4)```, and exits. All parsing and display updates happen in the main loop.
```
void CAN2_RX_HANDLER(void) __irq
{
    R1.id    = C2RID;
    R1.dlc   = ((C2RFS >> 16) & 0xF);
    R1.rtr   = ((C2RFS >> 30) & 1);
    R1.byteA = C2RDA;
    RECEIVER_FLAG = 1;
    C2CMR = 0x4;           // release RX buffer
    VICVectAddr = 0;
}
```
Frame Routing by CAN ID:

    0x10 → extract switch byte only, set event = 1
    0x50 → extract speed, temp, switch byte; update LCD immediately; set event = 1

Switch state changes are processed in both paths through a unified ```light_change()``` call, ensuring consistent behavior regardless of which frame carries the update.

- Indicator Blinking (Timer1, 80ms): Non-blocking blink using a Timer1 ISR flag. Each ISR fires at 80ms, main loop toggles the LED if the respective indicator bit is active. When an indicator is turned off, the LED is forced ON (not left in a random state) before clearing the flag.

### Custom CGRAM Characters (LCD): Five characters stored in CGRAM at init:

    0x00 — horizontal bar (speed indicator bar)
    0x01 — left arrow (left indicator symbol)
    0x02 — right arrow (right indicator symbol)
    0x03 — headlight icon
    0x04 — degree symbol (temperature)

## Build & Flash

Environment: Keil MDK (µVision) — ARM7 device pack for LPC2129
- Clone the repository
- Open Keil µVision → New Project → Select LPC2129
- Add source files from Node_A/ or Node_B/ as needed
- Set PLLCLK = 60 MHz (12 MHz crystal, M=5, P=2)
- Build → Flash via JTAG or UART ISP (Flash Magic)

CAN Baud Rate Config (C2BTR = 0x001C001D):
- PCLK = 60 MHz, Prescaler = 30 → tq = 500ns
- Segments: TSEG1 = 12, TSEG2 = 2 → 15 tq/bit → 125 Kbps

### Known Limitations & Planned Improvements
| # | Current Limitation | Planned Improvement |
|---|---|---|
| 1 | ```delay_ms()``` in Node B LCD driver is a blocking Timer0 poll — ties up CPU during each LCD write | Replace with Timer interrupt-based non-blocking delay or DMA-buffered LCD writes |
| 2 | ```VPBDIV``` lookup array uses index ```VPBDIV % 4``` (Node A) vs direct index (Node B) — inconsistent across nodes | Unify into a single shared utility function with bounds-checked PCLK calculation |
| 3 | No CAN error handling (bus-off, passive error, TX timeout) | Implement CAN error ISR using ```C2GSR / C2ICR``` error flags; add recovery logic |
| 4 | Temperature conversion uses floating-point (```3.3, 0.5, 0.01```) — expensive on ARM7 with no FPU | Replace with integer arithmetic: ```temp = ((adc_val * 330) / 1023 - 50) / 10``` |
| 5 | Switch debounce relies on VIC ISR only — mechanical bounce can cause double-triggers | Add software debounce in main loop (ignore re-trigger within 50ms window) |

### Tools & Environment
| Tool | Version / Detail |
|---|---|
| IDE | Keil MDK µVision |
| Compiler | ARM RealView C Compiler (RVCT) |
| MCU | NXP LPC2129 (ARM7TDMI-S, 60 MHz) |
| CAN Transceiver | MCP2551 |
| Programmer | Flash Magic (UART ISP) / JTAG |
| Protocol | CAN 2.0B, 125 Kbps |

----------------------------------------------------------------------------------
## 🤝 Let's Connect

I'm always open to collaborating on **Embedded Systems** or **Firmware**. Feel free to reach out!

⭐ If you found this interesting, consider giving it a star!
<div align="center">

[![LinkedIn](https://img.shields.io/badge/Connect_on_LinkedIn-0077B5?style=for-the-badge&logo=linkedin&logoColor=white)](https://www.linkedin.com/in/ijaidevpandya)
[![Email](https://img.shields.io/badge/Send_an_Email-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:pandya99jaidev@gmail.com)
</div>

<p align="center">

![Release](https://img.shields.io/github/v/release/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol?include_prereleases)
![License](https://img.shields.io/github/license/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)
![GitHub Stars](https://img.shields.io/github/stars/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)
[![Forks](https://img.shields.io/github/forks/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)](https://github.com/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol/network/members)
![Issues](https://img.shields.io/github/issues/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)
![Pull Requests](https://img.shields.io/github/issues-pr/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)
![Last Commit](https://img.shields.io/github/last-commit/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)
![Repo Size](https://img.shields.io/github/repo-size/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)
![Language](https://img.shields.io/github/languages/top/jaidev-11/Realtime-Digital-Automotive-Instrument-Cluster-with-CAN-protocol)

</p>

---------------------------------------------------------------------------------
## 👨🏽‍🚀Author

jaidev-11
