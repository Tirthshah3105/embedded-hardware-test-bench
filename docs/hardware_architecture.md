# Hardware Architecture & Electrical Design Specification

**Author:** Tirth Shah (B.Tech EXTC, SVKM's Dwarkadas J. Sanghvi College of Engineering)  
**System:** Embedded Hardware Validation & Telemetry Rig  
**Target Platform:** ESP32 / ESP32-CAM  

---

## 1. System Block Diagram

```
+-----------------------------------------------------------------------------------+
|                               HARDWARE ARCHITECTURE                               |
+-----------------------------------------------------------------------------------+

 [ 12V DC External Battery / Power Supply ]
           |
           +-----------------------------+
           |                             |
           v                             v
   [ LM2596 Buck Converter ]     [ Actuator / Servo Driver Stage ]
     (12V -> 5.0V Main Bus)        (High-Current Inductive Load)
           |                                     ^
           +---------------+                     |
           |               |                     |
           v               v                     |  [ PC817 Optocoupler ]
    [ AMS1117-3.3 LDO ]  [ Optical Sensor ]      |  (Galvanic Noise Isolation)
    (5.0V -> 3.3V VDD)   (Active Low Trigger)    |         ^
           |                       |             |         |
           +----------+            |             +---------+
                      |            |                       |
                      v            v                       |
           +-----------------------------------------------+
           |         ESP32 / ESP32-CAM Microcontroller     |
           |                                               |
           |   - ADC1_CH7 (GPIO 35) <--- 3.3V Rail Sense   |
           |   - ADC1_CH4 (GPIO 32) <--- 5.0V Rail Divider |
           |   - GPIO 34            <--- Optical Sensor In |
           |   - GPIO 25            ---> Actuator Pulse    |
           |   - UART0 (TX0 / RX0)  <---> Host PC (Python) |
           +-----------------------------------------------+
```

---

## 2. Power Rail Design & Decoupling Scheme

To prevent microcontroller brownout resets during high-current actuator triggering, the power distribution network implements a two-stage decoupling topology:

### 5.0V Main Bus Rail:
* **Regulator:** LM2596 Step-Down Switching Regulator (Adjustable, configured for $V_{out} = 5.00\text{V} \pm 1.5\%$).
* **Input Bulk Capacitor:** $100\mu\text{F} / 25\text{V}$ Low-ESR Electrolytic.
* **Output Bulk Capacitor:** $220\mu\text{F} / 16\text{V}$ Electrolytic + $0.1\mu\text{F}$ Multilayer Ceramic Capacitor (MLCC) placed adjacent to the switching inductor to suppress high-frequency ripple.

### 3.3V Logic Rail (ESP32 VDD):
* **Regulator:** AMS1117-3.3 Low-Dropout (LDO) Linear Regulator.
* **Output Filtering:** $10\mu\text{F}$ Tantalum capacitor in parallel with $100\text{nF}$ ceramic decoupling capacitor positioned within 5mm of the ESP32 VDD pins.
* **Transient Immunity:** The LDO rejects high-frequency switching noise from the primary LM2596 buck stage, ensuring an ultra-clean analog reference ($V_{ref} = 3.30\text{V}$).

---

## 3. Optical Isolation & Noise Immunity (PC817)

Inductive kickback from switching DC motors or servo actuators produces severe back-EMF spikes ($V = -L \frac{di}{dt}$) that can latch up microcontroller GPIOs or inject ground bounce:

* **Isolation IC:** PC817 Phototransistor Optocoupler.
* **Current-Limiting Resistor:** $330\Omega$ on the internal IR diode input driven by GPIO 25 ($I_F \approx 6.5\text{mA}$).
* **Output Stage:** Open-collector phototransistor with a $4.7\text{k}\Omega$ pull-up resistor to the isolated 12V actuator supply rail.
* **Ground Separation:** Digital ground ($GND_{logic}$) and power ground ($GND_{power}$) are galvanically isolated across the optocoupler barrier, preventing ground loop circulation.

---

## 4. Test Point (TP) Probing Matrix for Lab Instrumentation

The printed circuit board and prototype assembly designate specific test points for verification with Digital Multimeters (DMM) and Digital Storage Oscilloscopes (DSO):

| Test Point | Signal / Rail | Nominal Voltage | Spec Tolerance | Instrument | Measurement Objective |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **TP1** | 12V DC Input | 12.00 V | 10.8V – 13.2V | DMM | Battery state-of-charge check |
| **TP2** | 5.0V Main Bus | 5.00 V | 4.75V – 5.25V | DSO (AC Coupling) | Peak-to-peak ripple ($V_{p-p} < 50\text{mV}$) |
| **TP3** | 3.3V Logic VDD | 3.30 V | 3.135V – 3.465V | DSO (AC Coupling) | Brownout threshold margin under load |
| **TP4** | Sensor I/O | 0.0V / 3.3V | Logic High > 2.64V | DSO (DC Coupling) | Rise time ($t_r < 50\text{ns}$), bounce analysis |
| **TP5** | Actuator Trigger | 0.0V / 3.3V | PWM 50Hz | DSO / Logic Analyzer | Pulse duration verification (1.0ms – 2.0ms) |
| **TP_GND** | Digital Ground | 0.00 V | Reference | DSO Ground Spring | Low-inductance scope grounding |
