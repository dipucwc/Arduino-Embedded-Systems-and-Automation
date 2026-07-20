# Sensor Acquisition and Signal Processing

## Overview

This folder contains a planned collection of intermediate-to-advanced embedded-system projects focused on sensor interfacing, reliable data acquisition, digital filtering, calibration, feature extraction, event detection, and real-time monitoring.

The projects are designed to move beyond simple sensor-reading examples. Each project will implement a complete measurement chain, beginning with a physical sensor input and ending with processed information, alarms, visualization, storage, or communication.

The main objective is to demonstrate practical embedded C++ development and signal-processing skills using Arduino-compatible microcontrollers.

> **Status:** Planned and under development

---

## Project Scope

The projects in this folder will cover:

- Analog and digital sensor interfacing
- ADC-based data acquisition
- I2C, SPI, UART, and pulse-based communication
- Configurable sampling intervals
- Non-blocking firmware execution
- Sensor calibration and offset correction
- Digital filtering and noise reduction
- Statistical signal analysis
- Peak, RMS, and rate-of-change calculations
- Threshold and event detection
- Fault detection and measurement validation
- Data logging and visualization
- Modular embedded C++ development
- Python based result verification

---

## Planned Projects

### 1. Analog Sensor Acquisition and Digital Filtering

**Folder**

```text
01_analog_sensor_acquisition_and_filtering/
```

This project will implement a complete analog-signal acquisition and processing pipeline.

The controller will read an analog sensor through the ADC, convert the raw measurement into voltage or another engineering unit, validate the input, apply digital filtering, calculate useful statistics, and report both raw and processed values.

#### Planned input sources

- Potentiometer
- Light-dependent resistor
- Analog temperature sensor
- Force-sensitive resistor
- Microphone module
- External low-voltage analog signal

#### Planned features

- Configurable sampling interval
- Raw ADC acquisition
- ADC-to-voltage conversion
- Moving-average filtering
- Exponential moving-average filtering
- Median filtering
- First-order low-pass filtering
- Runtime filter selection
- Adjustable filter parameters
- Minimum and maximum value tracking
- Mean-value calculation
- Standard-deviation calculation
- Peak detection
- ADC saturation detection
- Sensor-disconnection detection
- Serial Monitor diagnostics
- Serial Plotter output
- CSV-formatted data output

#### Skills demonstrated

- Analog-to-digital conversion
- Sampling-frequency control
- ADC resolution and quantization
- Signal conditioning
- Digital filtering
- Noise reduction
- Statistical signal analysis
- Non-blocking embedded programming

---

### 2. Multi-Sensor Environmental Monitoring System

**Folder**

```text
02_multi_sensor_environmental_monitor/
```

This project will integrate several environmental sensors into one structured monitoring platform.

The controller will collect measurements at independent sampling intervals, validate the data, apply filtering, calculate environmental statistics, and generate warnings when configured limits are exceeded.

#### Planned measurements

- Temperature
- Relative humidity
- Atmospheric pressure
- Ambient light
- Optional air-quality or gas level

#### Planned features

- Multiple sensor initialization
- Sensor-connection verification
- Independent sampling schedules
- I2C and analog sensor support
- Measurement validation
- Invalid-reading rejection
- Outlier detection
- Moving-average filtering
- Minimum, maximum, and average tracking
- Configurable warning thresholds
- High- and low-temperature warnings
- Humidity-range monitoring
- Light-level monitoring
- Sensor-failure detection
- LED and buzzer alarms
- Optional OLED or LCD display
- Serial Monitor status reporting
- CSV-formatted output

#### Skills demonstrated

- Multi-sensor integration
- I2C communication
- Sensor scheduling
- Threshold monitoring
- Fault handling
- Modular firmware architecture
- Real-time environmental monitoring

---

### 3. Ultrasonic Distance Measurement and Object Tracking

**Folder**

```text
03_ultrasonic_distance_and_object_tracking/
```

This project will use an ultrasonic sensor to measure distance, detect nearby objects, and estimate object movement.

The controller will measure echo time, calculate object distance, reject invalid readings, apply filtering, and determine whether an object is approaching, stationary, or moving away.

#### Planned hardware

- HC-SR04 ultrasonic sensor
- Arduino Uno or compatible board
- Servo motor
- LED
- Buzzer
- Optional OLED or LCD display

#### Planned features

- Trigger-pulse generation
- Echo-pulse measurement
- Time-of-flight distance calculation
- Echo timeout protection
- Invalid-reading rejection
- Median filtering
- Moving-average filtering
- Configurable distance zones
- Near-object warning
- Object-presence detection
- Distance-rate calculation
- Approaching-object detection
- Departing-object detection
- Servo-based angular scanning
- Distance-versus-angle measurement
- Serial Monitor reporting
- Serial Plotter visualization

#### Skills demonstrated

- Pulse timing
- Time-of-flight measurement
- Sensor filtering
- Object detection
- Motion estimation
- Servo control
- State-based alarm handling

---

### 4. Motion and Vibration Monitoring System

**Folder**

```text
04_motion_and_vibration_monitor/
```

This project will use an accelerometer or inertial measurement unit to measure movement, tilt, vibration, and abnormal mechanical activity.

The system will acquire three-axis acceleration data, remove sensor offsets, filter noise, calculate motion features, and generate warnings when abnormal vibration is detected.

#### Planned hardware

- MPU6050, MPU9250, or similar IMU
- Arduino Uno, Nano, ESP32, or compatible board
- LED
- Buzzer
- Optional OLED display

#### Planned features

- Three-axis acceleration acquisition
- Sensor initialization and communication checks
- Startup offset calibration
- X, Y, and Z signal filtering
- Acceleration-magnitude calculation
- Motion detection
- Tilt detection
- Peak acceleration calculation
- RMS vibration calculation
- Configurable vibration threshold
- Threshold hysteresis
- Event counting
- Event-duration measurement
- Abnormal-vibration warning
- Serial Plotter output
- Optional data logging

#### Planned processing methods

- Offset removal
- Moving-average filtering
- Low-pass filtering
- High-pass filtering
- Magnitude calculation
- RMS calculation
- Peak detection
- Threshold comparison
- Event-duration analysis

#### Skills demonstrated

- IMU interfacing
- Three-axis signal processing
- Sensor calibration
- RMS calculation
- Peak detection
- Vibration monitoring
- Event detection
- Real-time alarm management

---

### 5. Real-Time Sensor Data Logger and Event Detector

**Folder**

```text
05_real_time_sensor_data_logger/
```

This project will collect, process, and store sensor measurements at a controlled sampling rate.

The system will record both raw and filtered values while monitoring for important events and measurement faults.

#### Planned hardware

- Arduino Uno, Nano, Mega, or ESP32
- Analog or digital sensor
- SD-card module
- Real-time clock module
- Optional OLED display
- Optional wireless communication module

#### Planned features

- Fixed-rate data acquisition
- Timestamped measurements
- Raw-data logging
- Filtered-data logging
- CSV file generation
- File-header generation
- Circular buffering
- Event-triggered recording
- Pre-event and post-event data capture
- Threshold-based event detection
- Rate-of-change detection
- Missed-sample detection
- Buffer-overflow detection
- Runtime logging control
- SD-card initialization checks
- File-write error handling
- Logging-status reporting

#### Advanced extension

The logger may use two sampling rates:

```text
Normal condition  -> low-rate background logging
Detected event    -> high-rate detailed logging
```

This reduces storage requirements while preserving detailed information around important events.

#### Skills demonstrated

- Real-time data acquisition
- Sampling-rate management
- Data buffering
- SD-card file handling
- Event-triggered logging
- Timestamp management
- Embedded memory management
- Error detection

---

## Possible Future Extensions

The following projects may be added later:

```text
06_sensor_calibration_and_linearization/
07_frequency_and_rpm_measurement/
08_audio_level_and_frequency_analysis/
09_voltage_current_and_power_monitoring/
10_wireless_sensor_node/
```

Possible extensions include:

- Two-point and multi-point calibration
- EEPROM calibration storage
- Linear and piecewise-linear correction
- Pulse-frequency measurement
- RPM calculation
- Missing-pulse detection
- Audio-level measurement
- RMS and peak analysis
- Zero-crossing frequency estimation
- FFT-based frequency analysis
- Voltage and current measurement
- Power and energy calculation
- Bluetooth Low Energy communication
- Wi-Fi communication
- LoRa communication
- Low-power sensor-node operation

---

## Common System Architecture

Each project will follow a structured signal-processing chain.

```text
Physical quantity
        |
        v
Sensor
        |
        v
Analog or digital interface
        |
        v
Data acquisition
        |
        v
Measurement validation
        |
        v
Calibration and correction
        |
        v
Digital filtering
        |
        v
Feature extraction
        |
        v
Decision or event detection
        |
        v
Display, alarm, storage, or communication
```

---

## Common Firmware Structure

Each project should use modular functions with clearly separated responsibilities.

```cpp
configureHardware();
initializeSensors();
readSensors();
validateMeasurements();
calibrateMeasurements();
filterMeasurements();
calculateFeatures();
detectEvents();
updateOutputs();
logData();
processSerialCommands();
printStatus();
```

A typical non-blocking main loop may follow this structure:

```cpp
void loop()
{
  processSerialCommands();
  updateSensorAcquisition();
  processMeasurements();
  detectEvents();
  updateOutputs();
  updateDataLogger();
}
```

Long blocking delays should be avoided during normal system operation.

---

## Engineering Requirements

### Non-Blocking Timing

Normal operation should use:

- `millis()`
- `micros()`
- Hardware timers
- Interrupts where appropriate

This allows the controller to acquire data, process measurements, update displays, check buttons, control alarms, and handle serial commands without stopping the complete program.

### Sampling Control

Each project should define and document:

- Sampling interval
- Sampling frequency
- Processing interval
- Display-update interval
- Logging interval

Example:

```cpp
constexpr unsigned long SAMPLE_INTERVAL_MS = 10;
constexpr unsigned long DISPLAY_INTERVAL_MS = 500;
constexpr unsigned long LOG_INTERVAL_MS = 100;
```

Sampling, display updates, and logging should be scheduled independently.

### Measurement Validation

The firmware should detect and handle:

- Sensor initialization failure
- Communication timeout
- Invalid readings
- Out-of-range values
- ADC saturation
- Disconnected sensors
- Repeated invalid samples
- Sudden unrealistic measurement changes
- Missed samples
- Invalid calibration coefficients

Invalid measurements should not be used directly for control decisions.

### Digital Filtering

Depending on the project, the following methods may be implemented:

- Moving-average filter
- Exponential moving-average filter
- Median filter
- First-order low-pass filter
- High-pass filter
- Offset removal
- Outlier rejection

### Calibration

Calibration may include:

- Zero-offset correction
- Gain correction
- Two-point calibration
- Multi-point calibration
- Reference-value comparison
- EEPROM coefficient storage

A basic linear calibration model is:

```text
calibrated_value = gain × raw_value + offset
```

### Runtime Configuration

Important parameters may be changed through the Serial Monitor:

- Sampling interval
- Alarm threshold
- Filter type
- Filter length
- Filter coefficient
- Calibration offset
- Calibration gain
- Logging mode
- Display mode

### Diagnostics

Each project should report useful system information.

Example:

```text
System status
Sensor state: READY
Sample count: 1250
Rejected samples: 4
Raw value: 518
Filtered value: 512.6
Sampling interval: 10 ms
Filter: MOVING AVERAGE
Alarm state: NORMAL
Logging state: ACTIVE
```

Possible error states include:

```text
SENSOR_NOT_FOUND
COMMUNICATION_TIMEOUT
ADC_SATURATION
INVALID_MEASUREMENT
CALIBRATION_ERROR
SD_CARD_ERROR
BUFFER_OVERFLOW
MISSED_SAMPLE
```

---

## Verification Approach

Each completed project should include a documented verification procedure.

Typical verification steps are:

1. Verify successful sensor initialization.
2. Confirm all hardware connections.
3. Measure the actual sampling interval.
4. Compare raw measurements with expected values.
5. Test minimum and maximum input conditions.
6. Introduce noise and observe filter performance.
7. Test alarm thresholds.
8. Disconnect the sensor and verify fault detection.
9. Reconnect the sensor and verify recovery.
10. Compare embedded results with Python or MATLAB.
11. Save plots, screenshots, and measured results.
12. Document known limitations.

---

## Python and MATLAB Verification

Python or MATLAB may be used for independent verification.

### Python applications

- Reading CSV log files
- Plotting raw and filtered data
- Calculating statistics
- Comparing filter outputs
- Detecting missed samples
- Measuring sampling jitter
- Verifying calibration equations
- Performing frequency analysis

### MATLAB applications

- Filter-response analysis
- Frequency-domain analysis
- Signal-quality comparison
- Reference algorithm implementation
- Calibration curve fitting
- Embedded-result verification

---

## Recommended Folder Structure

```text
02_sensor_acquisition_and_signal_processing/
│
├── README.md
│
├── 01_analog_sensor_acquisition_and_filtering/
│   ├── Analog_Sensor_Acquisition_and_Filtering.ino
│   ├── README.md
│   ├── circuit/
│   └── results/
│
├── 02_multi_sensor_environmental_monitor/
│   ├── Multi_Sensor_Environmental_Monitor.ino
│   ├── README.md
│   ├── circuit/
│   └── results/
│
├── 03_ultrasonic_distance_and_object_tracking/
│   ├── Ultrasonic_Distance_and_Object_Tracking.ino
│   ├── README.md
│   ├── circuit/
│   └── results/
│
├── 04_motion_and_vibration_monitor/
│   ├── Motion_and_Vibration_Monitor.ino
│   ├── README.md
│   ├── circuit/
│   └── results/
│
└── 05_real_time_sensor_data_logger/
    ├── Real_Time_Sensor_Data_Logger.ino
    ├── README.md
    ├── circuit/
    └── results/
```

---

## Development Roadmap

The recommended development order is:

```text
1. Analog sensor acquisition and filtering
2. Multi-sensor environmental monitoring
3. Ultrasonic distance and object tracking
4. Motion and vibration monitoring
5. Real-time sensor data logging
```

The first project will establish the common acquisition, filtering, validation, and Serial Monitor framework. Later projects will add multiple sensors, event detection, motion analysis, buffering, logging, and more advanced signal-processing functions.

---

## Project Status

| Project | Status |
|---|---|
| Analog Sensor Acquisition and Digital Filtering | Planned |
| Multi-Sensor Environmental Monitoring System | Planned |
| Ultrasonic Distance Measurement and Object Tracking | Planned |
| Motion and Vibration Monitoring System | Planned |
| Real-Time Sensor Data Logger and Event Detector | Planned |

---

## Expected Outcome

After completing this folder, the repository will contain a connected set of embedded-system projects that demonstrate the complete sensor-processing workflow:

```text
Physical input
      ↓
Sensor acquisition
      ↓
Validation and calibration
      ↓
Digital filtering
      ↓
Feature extraction
      ↓
Event detection
      ↓
Display, alarm, storage, or communication
```

These projects will provide a strong practical foundation for further work in industrial sensing, condition monitoring, IoT systems, automation, embedded signal processing, and real-time instrumentation.

```
## Author

**Md Moklesur Rahman**  
Embedded Systems, Wireless Communication, RF, and Signal Processing Engineer  
Oulu, Finland
```
