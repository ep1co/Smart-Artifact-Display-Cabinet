# Smart Artifact Display Cabinet

## 1. Introduction

**Smart IoT Artifact Display Cabinet** is an intelligent artifact display system designed to support automatic audio presentation, monitor the surrounding area, and trigger alerts when a visitor or object gets too close to the displayed artifact. The system follows an IoT-based approach by combining sensors, actuators, audio playback, and remote monitoring/control through the Blynk platform.

The main goal of this project is to simulate a smart museum display cabinet that can:

* Play audio introductions for visitors.
* Scan the area in front of the cabinet.
* Detect people or objects approaching the artifact.
* Trigger warnings when a dangerous approach is detected.
* Automatically activate a protection mechanism to hide or lower the artifact.
* Allow staff to monitor and control the system remotely through an IoT application.

## 2. Main Features

* **Scanning mechanism using servo motor**
  A servo motor rotates the sensor module to expand the monitoring area in front of the display cabinet.

* **Distance measurement using HC-SR04**
  The ultrasonic sensor is used to measure the distance between a person/object and the artifact.

* **Object detection using IR sensor**
  The infrared sensor helps confirm the presence of a person or object within the scanning area.

* **Dangerous approach warning**
  When a visitor’s hand or an object gets too close to the artifact, the system plays a warning sound and activates the protection mechanism.

* **Audio playback using DFPlayer Mini**
  The system plays audio files for artifact introduction, soft warning, and danger alert.

* **Artifact holder control using servo motor**
  A servo motor is used to control the mechanism that raises, lowers, or hides the artifact when protection is required.

* **IoT interface via Blynk**
  Users can monitor system status, distance values, receive alerts, and control selected functions remotely.

* **Audio priority mechanism**
  Warning audio has a higher priority than introduction audio, ensuring that the system responds correctly in dangerous situations.

## 3. Hardware Components

| Component        | Function                                                |
| ---------------- | ------------------------------------------------------- |
| ESP32            | Main controller                                         |
| HC-SR04          | Distance measurement                                    |
| IR Sensor        | Object/person detection                                 |
| Scanning Servo   | Rotates the sensor module to expand the monitoring area |
| Protection Servo | Controls the artifact holder or hiding mechanism        |
| DFPlayer Mini    | Plays audio files from a microSD card                   |
| Speaker          | Outputs introduction and warning sounds                 |
| 5V Power Supply  | Provides power for servos, DFPlayer, and peripherals    |
| Blynk App        | IoT monitoring and remote control interface             |

## 4. Technologies Used

* **ESP32 Arduino Core**
* **Blynk IoT**
* **DFRobotDFPlayerMini Library**
* **ESP32Servo Library**
* **HC-SR04 Ultrasonic Sensor**
* **IR Sensor**
* **Servo Motor Control**
* **IoT Monitoring and Remote Control**

## 5. System Architecture

```text
                ┌─────────────────────┐
                │      Blynk App       │
                │ Monitoring / Control │
                └──────────┬──────────┘
                           │
                           │ Internet / Wi-Fi
                           │
┌──────────────────────────▼──────────────────────────┐
│                       ESP32                         │
│                                                      │
│  ┌─────────────┐   ┌─────────────┐   ┌────────────┐ │
│  │  HC-SR04    │   │  IR Sensor  │   │   Blynk    │ │
│  │ Distance    │   │ Detection   │   │ Interface  │ │
│  └──────┬──────┘   └──────┬──────┘   └──────┬─────┘ │
│         │                 │                 │       │
│         └──────────┬──────┴─────────────────┘       │
│                    │                                  │
│          Logic Processing / State Control             │
│                    │                                  │
│     ┌──────────────┼──────────────┐                  │
│     │              │              │                  │
│ ┌───▼────┐   ┌─────▼─────┐   ┌────▼─────┐            │
│ │ Servo  │   │ DFPlayer  │   │ Servo    │            │
│ │ Scan   │   │ + Speaker │   │ Protect  │            │
│ └────────┘   └───────────┘   └──────────┘            │
└──────────────────────────────────────────────────────┘
```

## 6. Operating Principle

The system operates based on several main states.

### 6.1. Normal State

In the normal state, the scanning servo continuously rotates the HC-SR04 and IR sensor module to observe multiple angles in front of the cabinet. The system updates the measured distance and sensor status to the Blynk application.

### 6.2. Artifact Introduction Mode

When visitors are within a suitable range, the system can play artifact introduction audio through the DFPlayer Mini. The introduction tracks can be played periodically or triggered by a physical button/app control.

### 6.3. Warning Mode

When the HC-SR04 detects a distance below the warning threshold and the IR sensor confirms the presence of a person or object within the scanning area, the system switches to warning mode.

Depending on the danger level, the system can:

* Play a soft warning sound.
* Play a danger alert sound.
* Activate the protection servo to hide or lower the artifact.
* Send a notification to the Blynk App.

### 6.4. Remote Control

Through the Blynk App, users can:

* Monitor the measured distance.
* View system status.
* Trigger staff-controlled audio playback.
* Control the protection mechanism.
* Receive notifications when a dangerous approach is detected.

## 7. Audio Priority Mechanism

The system has multiple audio sources, so a priority mechanism is required to prevent incorrect track playback or overlapping audio.

Example priority levels:

| Priority Level | Audio Source            | Description                    |
| -------------- | ----------------------- | ------------------------------ |
| 3              | Sensor warning          | Highest-priority danger alert  |
| 2              | Staff control via app   | Staff-triggered audio playback |
| 1              | Visitor physical button | Artifact introduction audio    |
| 0              | Periodic playback       | Automatic introduction audio   |

When a high-priority audio track is playing, lower-priority requests are ignored. This ensures that safety warnings are always handled before introduction audio.

## 8. Audio Track Structure

Audio files are stored on the DFPlayer Mini microSD card using the following naming format:

```text
0001.mp3  - Soft warning
0002.mp3  - Danger alert
0003.mp3  - Artifact introduction part A
0004.mp3  - Artifact introduction part B
```

Notes:

* The microSD card should be formatted as FAT32.
* Audio files should be placed in the root directory of the microSD card.
* File names should follow the format `0001.mp3`, `0002.mp3`, and so on.

## 9. Blynk Configuration

The Virtual Pins can be configured as follows:

| Virtual Pin | Function                                                 |
| ----------- | -------------------------------------------------------- |
| V0          | Display measured distance                                |
| V1          | Display system status                                    |
| V2          | Control reset/open function for the protection mechanism |
| V3          | Staff audio playback button                              |
| V4/V5       | Optional extended functions depending on system design   |

Sensitive information should not be published directly, including:

* Blynk Auth Token
* Wi-Fi SSID
* Wi-Fi Password

It is recommended to store these values in a separate configuration file, for example:

```cpp
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
```

## 10. Installation Guide

### 10.1. Install Arduino Libraries

Install the following libraries in Arduino IDE:

* Blynk
* ESP32Servo
* DFRobotDFPlayerMini

### 10.2. Select Board

In Arduino IDE, select the appropriate ESP32 board, for example:

```text
Tools → Board → ESP32 Arduino → ESP32 Dev Module
```

### 10.3. Upload Program

1. Connect the ESP32 to the computer.
2. Select the correct COM port.
3. Configure Wi-Fi information and Blynk Auth Token.
4. Upload the program.
5. Open Serial Monitor to observe the system status.

## 11. Technical Notes

* The HC-SR04 may produce false readings due to reflections from glass, cabinet edges, or artifact surfaces.
* Combining HC-SR04 with an IR sensor helps reduce false alarms.
* Servos and the DFPlayer Mini require a stable power supply.
* All components should share a common ground.
* A filtering mechanism or continuous confirmation should be used before triggering an alarm.
* Excessive use of `delay()` should be avoided in IoT systems because it can slow down sensor reading and Blynk communication.

## 12. Future Improvements

Possible future improvements include:

* Adding a camera to capture images when an alert occurs.
* Storing alert history in a cloud database.
* Integrating a dedicated MQTT broker for more flexible control.
* Adding a web dashboard to monitor multiple display cabinets.
* Improving the sensor filtering algorithm.
* Adding user roles for visitors and staff.
* Adding backup power or power-loss detection.

## 13. Learning Objectives

This project provides hands-on practice in:

* ESP32 programming.
* Reading ultrasonic and infrared sensors.
* Controlling servo motors.
* Playing audio using DFPlayer Mini.
* Designing priority logic in embedded systems.
* Connecting an embedded system to Blynk IoT.
* Handling alerts and remote control.
* Designing an embedded IoT system for a practical application.
