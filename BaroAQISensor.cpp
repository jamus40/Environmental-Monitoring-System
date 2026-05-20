
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

/* ----- Version 0.4----- */
// - Confirmed Migraine marker topic is correct (home/baro/event/--MIGRAINE--)
// - Fixed MQTT heartbeat printout to reflect actual connection status (was always printing "connected" before)


/* ----- Version 0.3----- */
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
#define TOUCH_PIN 5   // using GPIO5 as discussed

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

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

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
if (touch && !lastTouch && mqtt.connected()) {
  Serial.println("[EVENT] Migraine marker sent");
  mqtt.publish("home/baro/event/--MIGRAINE--", "1");
}
lastTouch = touch;



  /* Periodic publish */
  if (millis() - lastPublish > 10000) {   // 10s for testing
    lastPublish = millis();

    float temp = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0;

    char buf[16];

    dtostrf(pressure, 6, 2, buf);
    mqtt.publish("home/baro/pressure", buf);

    dtostrf(temp, 5, 2, buf);
    mqtt.publish("home/baro/temperature", buf);

    Serial.print("[PUBLISH] P=");
    Serial.print(pressure);
    Serial.print(" T=");
    Serial.println(temp);
  }


  delay(50); // keep loop sane
}  

