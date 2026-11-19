/**
 * @file esp32_telemetry_rig.ino
 * @brief Embedded Hardware Validation & Telemetry Firmware
 * @author Tirth Shah (B.Tech EXTC)
 * 
 * Target Platform: ESP32 / ESP32-CAM Development Board
 * Description:
 * Implements hardware telemetry acquisition, power rail monitoring,
 * optical sensor polling, and automated fault reporting over UART.
 * Communicates with host Python automated test bench via structured serial commands.
 */

#include <Arduino.h>

// ======================== HARDWARE PIN DEFINITIONS ========================
#define PIN_STATUS_LED        2    // Onboard diagnostic heartbeat LED
#define PIN_OPTICAL_SENSOR   34    // Digital I/O line from optical sensor (ADC1_CH6)
#define PIN_RAIL_3V3_SENSE   35    // Analog voltage sense for 3.3V power rail (ADC1_CH7)
#define PIN_RAIL_5V_SENSE    32    // Analog voltage sense for 5.0V power rail via divider (ADC1_CH4)
#define PIN_ACTUATOR_TRIG    25    // Output pulse to actuator/servo driver stage

// ======================== CALIBRATION & THRESHOLDS ========================
const float VREF = 3.30f;                  // ESP32 ADC reference voltage
const int   ADC_MAX_COUNTS = 4095;         // 12-bit ADC resolution
const float VOLTAGE_DIVIDER_RATIO_5V = 2.0f; // 10k:10k precision divider for 5V sensing

// Power Rail Tolerance Windows
const float RAIL_3V3_MIN = 3.135f; // -5% tolerance
const float RAIL_3V3_MAX = 3.465f; // +5% tolerance
const float RAIL_5V_MIN  = 4.750f; // -5% tolerance
const float RAIL_5V_MAX  = 5.250f; // +5% tolerance

// ======================== STATE VARIABLES =================================
bool isTestingActive = false;
unsigned long cycleCount = 0;
unsigned long lastHeartbeatTime = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 500;

// ======================== FUNCTION PROTOTYPES =============================
float readVoltage(uint8_t pin, float scaleFactor = 1.0f);
void processHostCommand(String cmd);
void transmitTelemetryPacket();

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Allow UART to stabilize

    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(PIN_ACTUATOR_TRIG, OUTPUT);
    pinMode(PIN_OPTICAL_SENSOR, INPUT);
    pinMode(PIN_RAIL_3V3_SENSE, INPUT);
    pinMode(PIN_RAIL_5V_SENSE, INPUT);

    digitalWrite(PIN_STATUS_LED, LOW);
    digitalWrite(PIN_ACTUATOR_TRIG, LOW);

    // Configure 12-bit ADC with 11dB attenuation (0 - 3.3V range)
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    Serial.println(F("{\"status\":\"INITIALIZED\",\"device\":\"ESP32_VALIDATION_RIG\",\"firmware\":\"v1.2\"}"));
}

void loop() {
    // 1. Listen for host commands from Python automated test bench
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command.length() > 0) {
            processHostCommand(command);
        }
    }

    // 2. Active testing loop (triggered by host)
    if (isTestingActive) {
        transmitTelemetryPacket();
        delay(100); // 10 Hz sampling cadence for test validation
    }

    // 3. Heartbeat LED pulse
    if (millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeatTime = millis();
        digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
    }
}

/**
 * @brief Reads analog channel and converts to real-world voltage
 */
float readVoltage(uint8_t pin, float scaleFactor) {
    // 8-sample moving average to filter out high-frequency switching noise
    uint32_t rawSum = 0;
    for (int i = 0; i < 8; i++) {
        rawSum += analogRead(pin);
        delayMicroseconds(50);
    }
    float rawAvg = (float)rawSum / 8.0f;
    return (rawAvg / (float)ADC_MAX_COUNTS) * VREF * scaleFactor;
}

/**
 * @brief Parses incoming serial commands from Python automated test bench
 */
void processHostCommand(String cmd) {
    if (cmd.equalsIgnoreCase("TEST:START")) {
        isTestingActive = true;
        cycleCount = 0;
        Serial.println(F("{\"cmd_ack\":\"TEST:START\",\"status\":\"RUNNING\"}"));
    } 
    else if (cmd.equalsIgnoreCase("TEST:STOP")) {
        isTestingActive = false;
        Serial.println(F("{\"cmd_ack\":\"TEST:STOP\",\"status\":\"IDLE\"}"));
    } 
    else if (cmd.equalsIgnoreCase("ACTUATOR:TRIGGER")) {
        // Trigger high-current actuator stage to test rail transient response
        digitalWrite(PIN_ACTUATOR_TRIG, HIGH);
        delay(20);
        digitalWrite(PIN_ACTUATOR_TRIG, LOW);
        Serial.println(F("{\"cmd_ack\":\"ACTUATOR:TRIGGER\",\"transient_event\":true}"));
    } 
    else if (cmd.equalsIgnoreCase("PING")) {
        Serial.println(F("{\"cmd_ack\":\"PING\",\"response\":\"PONG\"}"));
    } 
    else {
        Serial.print(F("{\"error\":\"UNKNOWN_COMMAND\",\"received\":\""));
        Serial.print(cmd);
        Serial.println(F("\"}"));
    }
}

/**
 * @brief Formats and transmits real-time telemetry packet to host
 */
void transmitTelemetryPacket() {
    cycleCount++;
    unsigned long timestamp = millis();

    float v33 = readVoltage(PIN_RAIL_3V3_SENSE, 1.0f);
    float v50 = readVoltage(PIN_RAIL_5V_SENSE, VOLTAGE_DIVIDER_RATIO_5V);
    int opticalState = digitalRead(PIN_OPTICAL_SENSOR);

    // Evaluate hardware safety status
    bool railFault = (v33 < RAIL_3V3_MIN || v33 > RAIL_3V3_MAX || v50 < RAIL_5V_MIN || v50 > RAIL_5V_MAX);
    String faultCode = railFault ? "ERR_VOLTAGE_OUT_OF_SPEC" : "NONE";

    // Transmit structured JSON telemetry payload
    Serial.print(F("{\"cycle\":"));
    Serial.print(cycleCount);
    Serial.print(F(",\"timestamp_ms\":"));
    Serial.print(timestamp);
    Serial.print(F(",\"rail_3v3\":"));
    Serial.print(v33, 3);
    Serial.print(F(",\"rail_5v\":"));
    Serial.print(v50, 3);
    Serial.print(F(",\"optical_sensor\":"));
    Serial.print(opticalState);
    Serial.print(F(",\"fault\":\""));
    Serial.print(faultCode);
    Serial.println(F("\"}"));
}
