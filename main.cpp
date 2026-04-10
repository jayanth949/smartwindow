#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define DHTPIN 4
#define DHTTYPE DHT22

// Existing Sensors
#define MQ_SENSOR 34
#define RAIN_SENSOR 13

#define LDR_SENSOR 35

// 🔥 ADDED SENSORS
#define WIND_SENSOR 32
#define VIB_SENSOR 14   // vibration (use free pin if needed)

// 🔥 ADDED ACTUATOR
#define LED_PIN 2

Adafruit_BMP280 bmp;
DHT dht(DHTPIN, DHTTYPE);

// Actuators
#define SERVO_PIN 18
#define BUZZER 15
#define RELAY 19

// PWM
const int pwmChannel = 0;
const int pwmFreq = 50;
const int pwmResolution = 16;

uint32_t servoOpen = 6000;
uint32_t servoClose = 2000;

// Thresholds
#define TEMP_HIGH 30
#define TEMP_LOW 18
#define LDR_NIGHT 30
#define HUMIDITY_HIGH 80

void setup() {

  Serial.begin(115200);
  dht.begin();

  pinMode(RAIN_SENSOR, INPUT_PULLUP);

  // 🔥 NEW PIN MODES
  pinMode(VIB_SENSOR, INPUT);
  pinMode(LED_PIN, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);

  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(SERVO_PIN, pwmChannel);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Wire.begin(21, 32);

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found!");
  } else {
    Serial.println("BMP280 Initialized");
  }
}

void loop() {

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int rain = digitalRead(RAIN_SENSOR);

  int rawLight = analogRead(LDR_SENSOR);
  float light = (rawLight / 4095.0) * 100;

  int aq = analogRead(MQ_SENSOR);
  float pressure = bmp.readPressure() / 100.0F;

  // 🔥 NEW SENSOR READ
  int wind = analogRead(WIND_SENSOR);
  int vibration = digitalRead(VIB_SENSOR);

  if (isnan(temperature)) temperature = 25;
  if (isnan(humidity)) humidity = 60;

  Serial.println("------ Sensor Data ------");

  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("Rain: "); Serial.println(rain == LOW ? "YES" : "NO");
  Serial.print("Light (%): "); Serial.println(light);
  Serial.print("Air Quality (MQ): "); Serial.println(aq);
  Serial.print("Pressure (hPa): "); Serial.println(pressure);

  // 🔥 NEW PRINTS
  Serial.print("Wind: "); Serial.println(wind);
  Serial.print("Vibration: "); Serial.println(vibration);

  // 🔥 LED logic (gas alert)
  if (aq > 2000) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  

  // 🔥 NEW: vibration alert
  if (vibration == HIGH) {
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);
  }

  // ===== SAME LOGIC (UNCHANGED) =====

  if (rain == LOW) {
    ledcWrite(pwmChannel, servoClose);
    digitalWrite(RELAY, LOW);
  }
  else if (temperature > TEMP_HIGH) {
    ledcWrite(pwmChannel, servoOpen);
    digitalWrite(RELAY, HIGH);
  }
  else if (temperature < TEMP_LOW) {
    ledcWrite(pwmChannel, servoClose);
    digitalWrite(RELAY, LOW);
  }
  else if (light < LDR_NIGHT) {
    ledcWrite(pwmChannel, servoClose);
    digitalWrite(RELAY, LOW);
  }
  else if (humidity > HUMIDITY_HIGH) {
    ledcWrite(pwmChannel, servoOpen);
    digitalWrite(RELAY, HIGH);
  }
  else if (aq > 2000) {
    ledcWrite(pwmChannel, servoOpen);
    digitalWrite(RELAY, HIGH);
  }
  else if (pressure < 1000) {
    ledcWrite(pwmChannel, servoClose);
    digitalWrite(RELAY, LOW);
  }
  else {
    ledcWrite(pwmChannel, (servoOpen + servoClose) / 2);
    digitalWrite(RELAY, HIGH);
  }

  // 🌐 SEND DATA (ONLY ADDED FIELDS)
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin("http://172.16.214.9:5000/api/data");
    http.addHeader("Content-Type", "application/json");

    if (isnan(pressure)) pressure = 0;

String json = "{";
json += "\"temp\":" + String(temperature) + ",";
json += "\"humidity\":" + String(humidity) + ",";
json += "\"rain\":" + String(rain == LOW ? "true" : "false") + ",";
json += "\"light\":" + String(light) + ",";
json += "\"aq\":" + String(aq) + ",";
json += "\"pressure\":" + String(pressure) + ",";
json += "\"wind\":" + String(wind) + ",";
json += "\"vibration\":" + String(vibration) + ",";
json += "\"window_open\":" + String((temperature > TEMP_HIGH) ? "true" : "false") + ",";
json += "\"servo_angle\":" + String((temperature > TEMP_HIGH) ? 90 : 0);
json += "}";

    int httpResponseCode = http.POST(json);
    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  }

  delay(5000);
}
