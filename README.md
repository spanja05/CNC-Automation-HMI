# CNC Machine HMI – Automated Compressed Air Control

## Quick Start

1. **Hardware Setup**
   - Connect the components according to the [Pin Configurations](#pin-configurations) table below.
   - Ensure the rotary encoder and TFT LCD are wired to the ESP32 as listed.

2. **Install Arduino Libraries**
   - Open Arduino IDE (or PlatformIO) and install:
     - `TFT_eSPI`
     - `Preferences`
     - `AiEsp32RotaryEncoder`
   - Configure `TFT_eSPI` for your display in `User_Setup.h` (match your screen driver and SPI pins).

3. **Upload the Code**
   - Select **ESP32** as the board in your IDE.
   - Compile and upload the code to the ESP32.

4. **Use the Interface**
   - **Dials Page**:
     - Shows vacuum level, table pressure, compressed air, and pump status.
     - Table dial flashes red if below threshold (default: 20).
   - **Menu Page**:
     - Scroll to **Upper Limit**, **Lower Limit**, or **Back to Dials**.
     - Adjust limits with the rotary encoder.
     - Pump turns on when air < lower limit, off when air ≥ upper limit.

---

## Project Media

### Photos
![Setup](https://github.com/user-attachments/assets/ce4e1a94-a347-444a-a7c9-3b233591cb33)
![Dials Page](https://github.com/user-attachments/assets/b74f5d38-fb36-496d-a933-4ed3a341f9bc)
![Menu Page](https://github.com/user-attachments/assets/2ca6a293-d1a1-4ca1-892a-dc364a9fa102)

---

## About the Project

Manual intervention in machinery management can be **time-consuming**, **waste resources**, and pose **safety hazards**.  
This project provides a **Human-Machine Interface (HMI)** to control a CNC machine by automating the use of compressed air for creating fixtures in producing workpieces.

### Features
- **Two main display pages**:  
  - **Dials Page**:  
    - Two dials show:
      - Vacuum level in the tank
      - Table pressure  
    - Displays:
      - Current compressed air level in the machine
      - Pump status (on/off)  
    - Table dial **flashes red** if pressure < threshold (default: 20).  
  - **Menu Page**:  
    - Scrollable options:
      1. **Upper Limit**
      2. **Lower Limit**
      3. **Back to Dials**
    - Selecting **Upper/Lower Limit** allows value changes:
      - **Upper limit**: min 23.5 inHg, max 27.0 inHg
      - **Lower limit**: min 15.5 inHg, max 23.4 inHg  
    - **Back to Dials** returns to the main dial page.

- **Pump automation**:
  - Turns **on** when compressed air < lower limit
  - Turns **off** when compressed air ≥ upper limit

---

## Hardware Used
- 4" 480×320 SPI-driven TFT LCD
- KY-040 Rotary Encoder
- Potentiometers
- ESP32 Microcontroller

---

## Libraries Used
- **TFT_eSPI.h** – For rendering on the TFT LCD  
- **Preferences.h** – Stores user-set upper/lower limits in ESP32 flash memory (persistent between resets)  
- **AiEsp32RotaryEncoder.h** – Handles rotary encoder input for navigating menus

---

## Pin Configurations

**Rotary Encoder**
| Function | Pin |
|----------|-----|
| CLK      | 33  |
| DT       | 17  |
| SW       | 32  |
| VCC      | -1  |

**ESP32 Hardware SPI**
| Function  | Pin |
|-----------|-----|
| TFT_MOSI  | 23  |
| TFT_SCLK  | 18  |
| TFT_CS    | 15  |
| TFT_DC    | 2   |
| TFT_RST   | 4   |
