



-- Indoor Environmental Monitoring & Event Tracking System --

📌 Overview

  This project is a distributed embedded system designed to monitor indoor environmental conditions and track user health events (migraine triggers) in real time.
  The system integrates multiple sensors with an ESP32, transmits data via MQTT, and visualizes it through a centralized dashboard for analysis and trend correlation.

  

🎯 Purpose

  - Track environmental factors that may correlate with migraines
  
  - Monitor air quality and barometric pressure trends
  
  - Provide real-time and historical data visualization
  
  - Create a foundation for future automated environmental control

    


🧬 System Architecture
[ESP32 32]
├─ BMP280 (Pressure / Temperature)

├─ MQ-135 (Air Quality)

├─ TTP223 Touch Sensor (Event Input)
		
↓
	MQTT Broker - Mosquitto (Ubuntu)
		↓
		Home Assistant Dashboard
		
├─ Real-time sensors 				
		
├─ Historical graphs
		
├─ Event tracking




🔧 Hardware

- 	ESP32 DevKitM-1

-	BMP280 barometric pressure sensor (I2C)

-	MQ-135 air quality sensor (analog, GPIO32)

-	Capacitive touch input (GPIO19)




💻 Software

-	C++ (Arduino framework)
	
-	MQTT (PubSubClient)
	
-	Mosquitto broker (Ubuntu)
	
-	Home Assistant (dashboard + visualization)




📡 Data Flow

- Sensor data collected every 10 seconds

- Values averaged/smoothed where required (AQI)

Data published via MQTT topics:

- home/baro/pressure

- home/baro/temperature

- home/baro/air_quality




Home Assistant subscribes and updates dashboard
✅ Features
	
✅ Real-time environmental monitoring

✅ Historical trend visualization (pressure, AQI, temperature)

✅ Event-based input (migraine marker via touch sensor)

✅ MQTT heartbeat and connection monitoring

✅ Noise reduction via averaged ADC sampling




🧠 System Behaviors
-	Air quality readings averaged over 10 samples to reduce noise

- Touch input includes debounce and state reset for reliability
	 
- MQTT heartbeat sent every 30 seconds to monitor connectivity
	 
- Unique MQTT client ID generated per device boot




📊 Example Output

Pressure: 1006.03 hPa

Temperature: 77.6°F

Air Quality: 112 (raw ADC-derived scale)




📸 Dashboard
<img width="1272" height="1164" alt="image" src="https://github.com/user-attachments/assets/87616af4-bd0b-4baf-ab44-d498290b5458" />
This dashboard provides:

Real-time gauges (pressure, AQI)

Historical trend graphs

Event tracking system for migraine correlation





⚠️ Challenges / Lessons Learned

- ESP32 ADC2 pins conflict with WiFi → switched MQ-135 to ADC1 (GPIO4/32)

- MQ-135 requires voltage consideration → powered from 3.3V to prevent overvoltage

- Raw AQI values used due to lack of calibration → useful for relative trend analysis

- Touch input required debounce + delayed reset to integrate properly with Home Assistant

- Originally had an SD Card reader - Card reader wasn't reliable due to inherent instability under load. 




📈 Future Improvements

Add calibrated AQI conversion

Correlate migraine events with pressure/AQI trends

Add alerting system based on thresholds

Implement AI to assess trends to refine alerting

Add more sensors to gain housewide average / room specific data
