# 🛡️ Smart Helmet IoT System for Industrial Safety

This project is a solar-powered Smart Helmet designed to improve industrial worker safety by monitoring vital signs, environmental conditions, geolocation, and impact detection in real-time. The system uses **two ESP8266 NodeMCUs** and one **ESP32-CAM**, each connected to its own **ThingSpeak channel**, and communicates via Wi-Fi. A **solar panel** powers all hardware, making it sustainable for field and remote environments.

---

## 📦 System Overview

### ⚙️ Microcontroller Roles

- **ESP8266 #1 – Health & Environmental Monitoring**  
  Connected to:
  - `MAX30102`: Heart rate and SpO2 sensor  
  - `DHT22`: Temperature and humidity sensor  
  - `MQ-7`: Carbon Monoxide (CO) sensor  
  Sends data to **ThingSpeak Channel #2**

- **ESP8266 #2 – GPS & Impact Detection**  
  Connected to:
  - `NEO-6M`: GPS module  
  - `801S`: Vibration sensor  
  Sends data to **ThingSpeak Channel #3**  
  - Uses **MATLAB custom visualizations** to convert IP address and GPS readings into readable text format

- **ESP32-CAM – Visual Monitoring**  
  Captures snapshots periodically or during specific events  
  Sends status messages to **ThingSpeak Channel #1**  
  - Uses **MATLAB visualizations** to convert ESP32-CAM IP and activity into readable text

---

## ☁️ ThingSpeak Integration

Each microcontroller writes data to a **dedicated ThingSpeak channel** to avoid conflicts, since ThingSpeak does not support simultaneous updates from multiple devices to a single channel.

| Channel | Device         | Data Sent                          | Visualization |
|---------|----------------|-------------------------------------|---------------|
| #1      | ESP32-CAM      | IP address or camera status         | MATLAB text viewer |
| #2      | ESP8266 (Sensors) | CO, temperature, humidity, heart rate, SpO2 | ThingSpeak default charts |
| #3      | ESP8266 (GPS + Vibration) | GPS coordinates, IP address, vibration alert | MATLAB text viewer |

---

## 📁 Project Structure

| File Name                        | Description |
|----------------------------------|-------------|
| `esp8266_sensors.ino`            | Code for ESP8266 handling MAX30102, DHT22, and MQ-7 |
| `esp8266_gps_vibration.ino`     | Code for ESP8266 handling NEO-6M GPS and 801S vibration sensor |
| `esp32cam.ino`                   | Code for ESP32-CAM handling camera capture and IP display |
| `docs/`                          | Contains project report, wiring diagrams, and helmet images |
| `README.md`                      | Project overview and documentation (this file) |

---

## 📊 Data Fields Example
**Channel #1 – ESP32-CAM**
- Field 1: IP address or camera activity (text output via MATLAB)

**Channel #2 – Sensor Data**
- Field 1: CO (ppm)
- Field 2: Temperature (°C)
- Field 3: Humidity (%)
- Field 4: Heart Rate
- Field 5: SpO2

**Channel #3 – GPS + Vibration**
- Field 1: Latitude
- Field 2: Longitude
- Field 3: IP Address (via MATLAB)
- Field 4: Vibration Alert (ON/OFF)



## 🔌 Hardware Setup

- All modules are powered by a **solar panel** system with appropriate voltage regulation.
- Sensors are connected to the microcontrollers using standard GPIOs (see circuit diagram).
- Each ESP connects to a Wi-Fi network and pushes data to its assigned ThingSpeak channel using HTTP requests.

---

## 🖼️ Images & Documentation

### 🪖 Final Helmet Photo
in the folder


### 📄 Final Report
A detailed project report covering objectives, sensor specifications, wiring, ThingSpeak configuration, and outcomes is included here in the folder

---

## 🧪 How to Use the Project

1. **Set Up ThingSpeak Channels**
   - Create 3 channels (one for each ESP)
   - Note your Write API Keys and Field numbers

2. **Update Code Files**
   - Enter your Wi-Fi SSID, password, and API keys
   - Upload the respective `.ino` file to each microcontroller using the Arduino IDE

3. **Power Up & Monitor**
   - Ensure all modules receive solar power or USB power during testing
   - View real-time data and visualizations on ThingSpeak
   - MATLAB code (within ThingSpeak) handles the display of IP addresses and status as readable text

---



## 👤 Author

**[Noor Eldeen Mohamed]**  
Electronics & Communications Engineer  
