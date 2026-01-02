# Sleep Tracker (Arduino Uno + Grove RGB LCD)

A simple, interactive **Sleep Tracker** built using an **Arduino Uno**, a **Grove RGB LCD (16×2)**, and Grove input modules (**touch sensor** + **button**). The device guides the user through **setting the time**, then runs a live **HH:MM:SS** clock display. During use, the touch sensor can toggle between **Day Mode** and **Sleep Mode**, and (optionally) record timestamps over Serial for future logging/analysis.

---

## Overview

This project is designed as a lightweight foundation for sleep tracking workflows:
- A clear on-device UI (LCD + backlight colors)
- Simple user interaction (touch increments, button confirms/advances)
- Optional timestamp capture (e.g., “bed time” and “wake time”)

> Note: Without an RTC module, time resets when power is removed. This project uses software timekeeping via `TimeLib`. You can upgrade later by adding a DS3231 RTC.

---

## Features

### Core
- Displays time in **HH:MM:SS** on a Grove RGB LCD
- **Time-setting wizard** using touch + button input
- Real-time updates using `TimeLib`

### UI / Modes
- Startup **welcome screen**
- **Backlight color indicators** during setup and mode changes
- Two user modes:
  - **Day Mode** (bright/white backlight)
  - **Sleep Mode** (dark purple backlight)

### Optional Logging (via Serial Monitor)
- Capture timestamps for:
  - **Bed time**
  - **Wake time**
- Output can be extended to JSON, SD logging, BLE, etc.

---

## Hardware Requirements

### Required Components
- Arduino Uno (or compatible)
- Grove Base Shield (recommended)
- Grove RGB LCD Display (16×2, I2C)
- Grove Touch Sensor / Touchpad (digital)
- Grove Button (digital)
- Grove cables / jumpers

### Optional (Recommended Upgrades)
- DS3231 RTC module (time persists after power-off)
- SD card module (store sleep sessions)
- BLE/Wi-Fi module (sync to phone)
- Motion sensor (IMU/accelerometer) for activity-based tracking

---

## Wiring / Connections

| Module | Connection |
|-------|------------|
| Grove RGB LCD (16×2) | Grove **I2C** port |
| Grove Touch Sensor | **D2** |
| Grove Button | **D3** |
| Arduino Uno | Powered by USB or 5V supply |

> If you use `INPUT_PULLUP`, wiring/logic may invert (pressed = LOW). Keep pin mode and wiring consistent.

---

## Software Requirements

- Arduino IDE (or PlatformIO)

### Libraries Used
- `Wire.h` (I2C communication — included by default)
- `rgb_lcd.h` (Grove RGB LCD control)
- `TimeLib.h` (timekeeping utilities)

### Installing Libraries (Arduino IDE)
1. Open **Arduino IDE**
2. Go to **Tools → Manage Libraries**
3. Search and install:
   - A Grove RGB LCD library that provides `rgb_lcd.h` (commonly “Grove LCD RGB Backlight”)
   - **Time** (by Michael Margolis) for `TimeLib.h`

---

## Installation & Setup

1. Connect the hardware using the wiring table above.
2. Open the project sketch in Arduino IDE (example: `sleep_tracker.ino`).
3. Select:
   - **Tools → Board → Arduino Uno**
   - **Tools → Port →** your Arduino port
4. Click **Upload**.
5. (Optional) Open **Tools → Serial Monitor** and set baud to **9600** to view timestamps/log output.

---

## How It Works (User Guide)

### 1) Startup
- The LCD shows a **Welcome** message for a few seconds.

### 2) Set Time (Setup Mode)
The device enters a time-setting sequence.

- **Touch sensor press**: increments the active digit/field
- **Button press**: confirms and moves to the next field

Typical order:
- Seconds → Minutes → Hours  
(Exact sequence depends on the sketch version.)

During setup, the LCD backlight can change color to show which field you’re setting.

### 3) Run Mode (Clock Display)
After time is set:
- LCD displays a live updating **Time: HH:MM:SS**
- The system updates continuously

### 4) Sleep Mode Toggle + Timestamp Capture (Optional)
In run mode:
- **Touch sensor press** toggles:
  - **Day Mode** (bright/white backlight + friendly message)
  - **Sleep Mode** (dark purple backlight + “Good Night!”)
- Each toggle can optionally capture a timestamp:
  - First capture: “Bed” time
  - Second capture: “Wake” time  
These times are printed to Serial and can be used to compute sleep duration later.

---

## Example Project Layout

```text
Sleep-Tracker/
├─ Refreshed_Riser.ino
└─ README.md
