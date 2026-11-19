# Automated Hardware Validation & Telemetry Test Bench

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32%20%2F%20ESP32--CAM-red.svg)](https://www.espressif.com/)
[![Python: 3.8+](https://img.shields.io/badge/Python-3.8%2B-blue.svg)](https://www.python.org/)
[![Validation: Automated HIL](https://img.shields.io/badge/Test%20Validation-HIL%20%2F%20Regression-brightgreen.svg)]()
[![Tools: KiCad & DSO](https://img.shields.io/badge/Tools-KiCad%20%7C%20DSO%20%7C%20DMM-orange.svg)]()

> Production-ready embedded firmware and host Python test automation suite for Hardware-in-the-Loop (HIL) testing, power rail ripple validation, and multi-cycle sensor telemetry logging.

---

## 📌 Executive Overview

In electronic product development and silicon validation, manual multimeter probing across multi-board batches is slow, unrepeatable, and error-prone. 

This repository provides an **end-to-end automated test bench** combining:
1. **Target Embedded Firmware (`/firmware`):** Written in C/C++ for the ESP32. Executes real-time voltage sensing, sensor polling, actuator control, and structured UART telemetry streaming.
2. **Host Test Automation Suite (`/scripts`):** Written in Python. Automatically communicates over serial (UART via `PySerial`), commands multi-cycle stress runs, validates power rail voltages against $\pm 5\%$ tolerance bands, and exports pass/fail CSV logs.
3. **Hardware Architecture & Schematic Guidelines (`/docs`):** Details decoupling capacitor calculations, buck converter regulation (LM2596), and galvanic optocoupler isolation (PC817) to prevent microcontroller brownout resets under inductive transient loads.

---

## 🏗️ System Architecture

```
+-----------------------------------------------------------------------------------------+
|                                  SYSTEM ARCHITECTURE                                    |
+-----------------------------------------------------------------------------------------+

 [ Host Workstation ]
         |
         |  UART (Serial 115200 Baud / PySerial)
         |  Commands: TEST:START, TEST:STOP, ACTUATOR:TRIGGER, PING
         v
 [ ESP32 Microcontroller ]
         |
         +---> GPIO 35 (ADC1) <--- [ 3.3V Logic Rail ] (AMS1117 LDO Sense)
         +---> GPIO 32 (ADC1) <--- [ 5.0V Main Bus ]   (Precision Resistor Divider)
         +---> GPIO 34 (DI)   <--- [ Optical Sensor ]   (Active-Low Defect Detect)
         +---> GPIO 25 (DO)   ---> [ PC817 Opto ]      ---> [ High-Current Actuator / Servo ]
         +---> GPIO 02 (DO)   ---> [ Heartbeat LED ]   (Diagnostic State Indication)
```

---

## ⚡ Key Hardware & Circuit Design Features

* **Dual-Stage Voltage Regulation:** Implements an external **LM2596 DC-DC buck regulator** stepping down 12V inputs to 5V, feeding an **AMS1117-3.3V LDO** for logic VDD.
* **Brownout Elimination:** Evaluated using Digital Storage Oscilloscope (DSO) AC coupling. Transient servo inrush current ($di/dt$) is mitigated using a $220\mu\text{F}$ low-ESR electrolytic capacitor in parallel with $0.1\mu\text{F}$ MLCC decoupling filters.
* **Galvanic Noise Isolation:** Actuators and switching motors are isolated from logic ground using a **PC817 optocoupler**, eliminating inductive ground bounce.
* **Designated Lab Test Points:** Circuit schematics in KiCad establish test points (`TP1`–`TP5`) for multimeter probing and oscilloscope ground spring attachment.

---

## 🚀 Quickstart & Execution

### 1. Repository Setup
```bash
# Clone the repository
git clone https://github.com/your-username/embedded-hardware-test-bench.git
cd embedded-hardware-test-bench

# Install host dependencies
pip install -r requirements.txt
```

### 2. Flashing Embedded Firmware
1. Open `firmware/esp32_telemetry_rig/esp32_telemetry_rig.ino` in Arduino IDE or PlatformIO.
2. Select Board: **ESP32 Dev Module**.
3. Compile and upload to your ESP32 board over USB.

### 3. Running the Python Automated Test Bench

#### Option A: Running with Physical Hardware Connected
```bash
python scripts/automated_test_bench.py --port COM3 --baud 115200 --cycles 100
```

#### Option B: Offline Demonstration / Mock Simulation Mode
*(No physical hardware required — runs realistic sensor and transient voltage simulation):*
```bash
python scripts/automated_test_bench.py --mock --cycles 100
```

---

## 📊 Sample Output & Verification

### Live Console Stream:
```text
==================== COMMENCING HARDWARE VALIDATION SWEEP (100 CYCLES) ====================
[Cycle 001/100]  3.3V Rail: 3.301V  |  5.0V Rail: 5.032V  |  Status: PASS
[Cycle 002/100]  3.3V Rail: 3.328V  |  5.0V Rail: 5.001V  |  Status: PASS
...
[Cycle 025/100]  3.3V Rail: 3.168V  |  5.0V Rail: 4.850V  |  Status: PASS  <-- Actuator Transient
...
==================== TEST RUN SUMMARY ====================
Total Cycles Completed: 100
Passed: 100  |  Failed: 0
Hardware Validation Yield: 100.00%
[SUCCESS] Telemetry dataset exported to: data/sample_hardware_test_log.csv
```

### Telemetry Dataset Schema (`data/sample_hardware_test_log.csv`):
| cycle | timestamp_ms | rail_3v3 (V) | rail_5v (V) | optical_sensor | fault | result |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| 1 | 142100 | 3.301 | 5.032 | 1 | NONE | PASS |
| 2 | 142142 | 3.328 | 5.001 | 1 | NONE | PASS |
| 25 | 143115 | 3.168 | 4.850 | 1 | NONE | PASS |

---

## 📂 Repository Structure

```
embedded-hardware-test-bench/
├── README.md                          # Project documentation and test report
├── requirements.txt                   # Host Python dependencies
├── firmware/
│   └── esp32_telemetry_rig/
│       └── esp32_telemetry_rig.ino    # Embedded C/C++ firmware for ESP32
├── scripts/
│   └── automated_test_bench.py        # Python automated test bench & validation suite
├── data/
│   └── sample_hardware_test_log.csv   # Verified 100-cycle test telemetry dataset
└── docs/
    └── hardware_architecture.md       # Electrical schematic & test point specs
```

---

## 👨‍💻 Author

**Tirth Mehul Shah**  
*Bachelor of Technology in Electronics & Telecommunication (EXTC)*  
**SVKM’s Dwarkadas J. Sanghvi College of Engineering (DJSCE)**, Mumbai  
* **Email:** [tirthshah0531@gmail.com](mailto:tirthshah0531@gmail.com)  
* **LinkedIn:** [linkedin.com/in/tirth-shah](https://linkedin.com)  
* **Portfolio / GitHub:** [github.com/tirth-shah](https://github.com)  

---
*License: This project is licensed under the MIT Open Source License.*
