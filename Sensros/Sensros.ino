#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define DHT_PIN D3
#define DHT_TYPE DHT22
#define MQ7_PIN A0

DHT dht(DHT_PIN, DHT_TYPE);

const char* ssid = "MK";
const char* password = "hehe@1234";

const char* server = "api.thingspeak.com";
String apiKey = ".................";
const int channelID = 2858389;

#define MAX_BRIGHTNESS 255

uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t bufferLength = 100;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

long lastBeat = 0;
int beatsPerMinute = 0;
byte rateArray[4];
byte rateArrayIndex = 0;
long total = 0;
int average = 0;

int spo2Array[4] = {0, 0, 0, 0};
int spo2ArrayIndex = 0;
int spo2Total = 0;
int spo2Average = 0;

unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 20000;
int validReadingsCount = 0;

float temperature = 0.0;
float humidity = 0.0;
float co_ppm = 0.0;
int mq7_raw = 0;

void sendToThingSpeak(float co, float temp, float hum, int heartRate, int spo2);
void readEnvironmentalSensors();
void simpleHeartRateDetection();
bool checkForBeat(long sample);

void setup() {
  Serial.begin(115200);
  Serial.println("MAX30102 Heart Rate and SpO2 Monitor with ThingSpeak");
  Serial.println("===================================================");

  Wire.begin();

  dht.begin();
  delay(2000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());

  if (!particleSensor.begin()) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    while (1);
  }

  Serial.println("MAX30102 sensor found!");
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("Place your finger on the sensor...");
  delay(2000);
}

void loop() {
  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false) {
      particleSensor.check();
    }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  for (byte i = 25; i < 100; i++) {
    redBuffer[i - 25] = redBuffer[i];
    irBuffer[i - 25] = irBuffer[i];
  }

  for (byte i = 75; i < 100; i++) {
    while (particleSensor.available() == false) {
      particleSensor.check();
    }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  readEnvironmentalSensors();

  Serial.println("===================");
  Serial.print("Timestamp: ");
  Serial.println(millis());

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  Serial.print("CO Level: ");
  Serial.print(co_ppm);
  Serial.print(" ppm (Raw: ");
  Serial.print(mq7_raw);
  Serial.println(")");

  if (validHeartRate == 1) {
    Serial.print("Heart Rate: ");
    Serial.print(heartRate);
    Serial.println(" BPM");

    if (heartRate > 50 && heartRate < 150) {
      total = total - rateArray[rateArrayIndex];
      rateArray[rateArrayIndex] = (byte)heartRate;
      total = total + rateArray[rateArrayIndex];
      rateArrayIndex++;
      rateArrayIndex %= 4;

      average = total / 4;
      Serial.print("Average HR: ");
      Serial.print(average);
      Serial.println(" BPM");
    }
  } else {
    Serial.println("Heart Rate: Invalid reading");
  }

  if (validSPO2 == 1 && spo2 > 70 && spo2 < 100) {
    Serial.print("SpO2: ");
    Serial.print(spo2);
    Serial.println("%");

    spo2Total = spo2Total - spo2Array[spo2ArrayIndex];
    spo2Array[spo2ArrayIndex] = spo2;
    spo2Total = spo2Total + spo2Array[spo2ArrayIndex];
    spo2ArrayIndex++;
    spo2ArrayIndex %= 4;

    spo2Average = spo2Total / 4;
    Serial.print("Average SpO2: ");
    Serial.print(spo2Average);
    Serial.println("%");

    validReadingsCount++;
  } else {
    Serial.println("SpO2: Invalid reading");
  }

  Serial.print("IR Signal: ");
  Serial.println(irBuffer[99]);
  Serial.print("Red Signal: ");
  Serial.println(redBuffer[99]);

  if (irBuffer[99] < 50000) {
    Serial.println("*** Please place your finger properly on the sensor ***");
  }

  if (millis() - lastThingSpeakUpdate >= thingSpeakInterval) {
    if (average > 0 && spo2Average > 0) {
      sendToThingSpeak(co_ppm, temperature, humidity, average, spo2Average);
    } else {
      sendToThingSpeak(co_ppm, temperature, humidity, 0, 0);
    }
    lastThingSpeakUpdate = millis();
    Serial.println("Data sent to ThingSpeak!");
  }

  Serial.println("===================");
  delay(1000);
}

void simpleHeartRateDetection() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      total = total - rateArray[rateArrayIndex];
      rateArray[rateArrayIndex] = (byte)beatsPerMinute;
      total = total + rateArray[rateArrayIndex];
      rateArrayIndex++;
      rateArrayIndex %= 4;

      average = total / 4;
      Serial.print("Simple HR: ");
      Serial.print(average);
      Serial.println(" BPM");
    }
  }
}

bool checkForBeat(long sample) {
  static long lastSample = 0;
  static long threshold = 100000;
  static long lastBeatTime = 0;
  static bool beatDetected = false;

  if (sample > threshold && (millis() - lastBeatTime) > 300) {
    if (!beatDetected) {
      beatDetected = true;
      lastBeatTime = millis();
      return true;
    }
  } else if (sample < threshold * 0.8) {
    beatDetected = false;
  }

  return false;
}

void readEnvironmentalSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT22 sensor!");
    temperature = 0.0;
    humidity = 0.0;
  }

  mq7_raw = analogRead(MQ7_PIN);
  float voltage = (mq7_raw / 1024.0) * 3.3;

  // Approximate mapping of voltage to CO ppm for range 0–10 ppm
  if (voltage > 0.1 && voltage < 1.0) {
    co_ppm = 100 * voltage;
    if (co_ppm > 10) co_ppm = 10;
  } else {
    co_ppm = 0;
  }
}

void sendToThingSpeak(float co, float temp, float hum, int heartRate, int spo2) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    String url = "http://api.thingspeak.com/update?api_key=" + apiKey;
    url += "&field1=" + String(co, 2);
    url += "&field2=" + String(temp, 2);
    url += "&field3=" + String(hum, 2);
    url += "&field4=" + String(heartRate);
    url += "&field5=" + String(spo2);

    Serial.println("Sending to ThingSpeak...");
    Serial.println("URL: " + url);

    http.begin(client, url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("ThingSpeak Response: " + response);
      if (response.toInt() > 0) {
        Serial.println("✓ Data successfully sent to ThingSpeak");
      } else {
        Serial.println("✗ ThingSpeak update failed");
      }
    } else {
      Serial.println("✗ HTTP Error: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("✗ WiFi not connected");
    WiFi.begin(ssid, password);
  }
}
