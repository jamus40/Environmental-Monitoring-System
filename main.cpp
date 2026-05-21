#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

              /* ----- Version 0.6 ----- */
// - Added MQ-135 air quality sensor on GPIO4
//     Reason: GPIO4 is ADC1 which remains stable when WiFi is active

//   ADC2 pins (including GPIO18) conflict with WiFi and return garbage values

// - Moved TOUCH_PIN from GPIO4 to GPIO19
//     Reason: freed GPIO4 for MQ-135 analog input

// - MQ-135 powered from 3.3V to keep AO output within ESP32 safe range
//     Reason: sensor VCC on 5V would output up to 5V on AO, exceeding ESP32 3.3V max

// - Raw ADC value published to MQTT (0-4095 range)
//     Reason: MQ-135 is uncalibrated out of box, raw values used for relative trending

//   Lower value = cleaner air, higher value = more contaminants detected

              /* ----- Version 0.5 ----- */
// - Moved TOUCH_PIN from GPIO5 to GPIO25 (then to GPIO4, now GPIO19)
//     Reason: GPIO5 is a strapping pin with built-in pull-up overriding INPUT_PULLDOWN

// - Added debounce filter (200ms hold required)

// - Added MQTT "0" publish after migraine marker so HA binary sensor clears

              /* ----- Version 0.4 ----- */
// - Confirmed Migraine marker topic correct
// - Fixed MQTT heartbeat printout

              /* ----- Version 0.3 ----- */
// - Migrated to ESP32-DevKitM-1
// - Added MQTT heartbeat every 30s
// - Added MQTT connection status printout every 30s

/* --------- CONFIG --------- */
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";

const char* mqtt_server = "192.168.x.x";  // Your MQTT broker IP
const int   mqtt_port   = 1883;


const char* mqtt_user = "Your_MQTT_Username";
const char* mqtt_pass = "Your_MQTT_Password";

char mqttClientId[32];



/* --------- PINS ---------- */
#define I2C_SDA   21
#define I2C_SCL   22
#define TOUCH_PIN 19
#define MQ135_PIN 32
   // using GPIO5 as discussed

WiFiClient espClient;
PubSubClient mqtt(espClient);
Adafruit_BMP280 bmp;

unsigned long lastPublish = 0;
bool lastTouch = false;

void setupWiFi() {
  Serial.println("[WiFi] Connecting...");
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\n[WiFi] FAILED (timeout)");
      return;
    }
  }

  Serial.println("\n[WiFi] Connected");
  Serial.print("[WiFi] IP: ");
  Serial.println(WiFi.localIP());
}


void connectMQTT() {
  if (mqtt.connected()) return;

  Serial.println("[MQTT] Connecting...");

  if (mqtt.connect(mqttClientId, mqtt_user, mqtt_pass)) {
    Serial.println("[MQTT] Connected");
    mqtt.publish("home/baro/status", "online", true);
  } else {
    Serial.print("[MQTT] FAILED, rc=");
    Serial.println(mqtt.state());
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 BOOT ===");

  // INPUT_PULLDOWN holds pin firmly LOW until button pulls it HIGH
  // prevents floating pin false triggers
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  /* analogReadResolution sets ADC to 12-bit mode.
   This means readings range from 0 (no voltage) to 4095 (3.3V).
   Default on ESP32 is already 12-bit but declaring it explicitly
   makes the code self-documenting and portable to other boards */
  analogReadResolution(12);

  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("[I2C] Started");

  if (!bmp.begin(0x76)) {
    Serial.println("[BMP280] NOT FOUND");
  } else {
    Serial.println("[BMP280] OK");
  }

  setupWiFi();

  
  randomSeed(esp_random());
  snprintf(mqttClientId, sizeof(mqttClientId),
         "baro-esp32-%04X", random(0xFFFF));


  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setKeepAlive(60);
  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    Serial.println("[MQTT] Disconnected, retrying...");
    connectMQTT();
    delay(2000);
  }
  mqtt.loop();

  // ---- MQTT heartbeat *every 30s* ----
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) {
    lastHeartbeat = millis();
    mqtt.publish("home/baro/heartbeat", "Still Connected");
    
    Serial.println(mqtt.connected()
      ? "[MQTT] connected"
      : "[MQTT] disconnected");

  }

  /* Touch button */
bool touch = digitalRead(TOUCH_PIN);

if (touch && !lastTouch) {
    // Pin just went HIGH — wait 200ms and confirm it's still HIGH
    delay(200);
    if (digitalRead(TOUCH_PIN)) {
      if (mqtt.connected()) {
        Serial.println("[EVENT] Migraine marker sent");
        mqtt.publish("home/baro/event/--MIGRAINE--", "1");

        /* Hold the ON state for 2 seconds so HA can register it
         then publish "0" to clear the binary sensor back to off.
         Without this HA never receives an off state and stays ON permanently. */
        delay(2000);
        mqtt.publish("home/baro/event/--MIGRAINE--", "0");
        Serial.println("[EVENT] Migraine marker cleared");
      }
    }
}
lastTouch = touch;



  /* Periodic sensor publish every 10s */
  if (millis() - lastPublish > 10000) {   // 10s for testing
    lastPublish = millis();

    // BMP Readings
    float temp = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0;

    /*             MQ-135 Reading
     analogRead returns 0-4095 on ESP32 12-bit ADC
     we take 10 samples and average them to smooth out
     moment-to-moment electrical noise in the analog signal  */
    int mq13Sum = 0;
    for (int i = 0; i < 10; i++) {
      mq13Sum += analogRead(MQ135_PIN);
      delay(5); // small delay between readings for stability
    }
    int mq135Raw = mq13Sum / 10;

    char buf[16];

    // Publish Pressure
    dtostrf(pressure, 6, 2, buf);
    mqtt.publish("home/baro/pressure", buf);
    // Publish Temperature
    dtostrf(temp, 5, 2, buf);
    mqtt.publish("home/baro/temperature", buf);
    // Publish Air Quality (raw ADC value)
    itoa(mq135Raw, buf, 10);
    mqtt.publish("home/baro/air_quality", buf);
    // print all three to serial monitor for local debugging
    Serial.print("[PUBLISH] P=");
    Serial.print(pressure);
    Serial.print(" T=");
    Serial.println(temp);
    Serial.print(" AQ=");
    Serial.println(mq135Raw);
  }


  delay(50); // keeps loop sane
}
