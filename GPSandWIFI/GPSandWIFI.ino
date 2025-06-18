
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "MK";
const char* password = "hehe@1234";

// ThingSpeak API
const char* thingSpeakServer = "http://api.thingspeak.com/update";
const char* apiKey = "......................";

// Google Geolocation API key
const char* google_api_key = "............................";

// GPS setup
#define RXPin D1
#define TXPin D2
TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXPin, TXPin);

// Vibration sensor
#define VIBRATION_SENSOR_PIN A0
const int lightImpactThreshold = 200;
const int strongImpactThreshold = 600;
const int noiseThreshold = 50;

WiFiClient client;
WiFiServer server(80); // for serving local IP map
String latitude = "0", longitude = "0";

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 15000;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("Device IP: " + WiFi.localIP().toString());
  server.begin();
}

void loop() {
  updateGPS();

  String ipLink = "http://" + WiFi.localIP().toString();  // IP hosting the map
  String locationLink;

  if (gps.location.isValid() && gps.location.isUpdated()) {
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);
    locationLink = ipLink;
    Serial.println("✅ GPS Location Used");
  } else {
    Serial.println("⚠️ Using WiFi Positioning");
    locationLink = ipLink;
    getWiFiLocation(); // Updates latitude and longitude
  }

  // Read vibration level
  int vibrationValue = 0;
  for (int i = 0; i < 10; i++) {
    vibrationValue += analogRead(VIBRATION_SENSOR_PIN);
    delay(10);
  }
  vibrationValue /= 10;
  int vibrationLevel = map(vibrationValue, 0, 1023, 0, 1000);
  if (vibrationLevel < noiseThreshold) vibrationLevel = 0;

  // Classify impact
  String impactStatus = "No Impact";
  if (vibrationLevel > strongImpactThreshold) impactStatus = "Strong Impact";
  else if (vibrationLevel > lightImpactThreshold) impactStatus = "Light Impact";

  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();
    String url = String(thingSpeakServer) +
                 "?api_key=" + apiKey +
                 "&field1=" + locationLink +
                 "&field2=" + String(vibrationLevel) +
                 "&field3=" + String((impactStatus == "Strong Impact") ? 2 : (impactStatus == "Light Impact") ? 1 : 0);
    
    Serial.println("Sending to ThingSpeak: " + url);
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(client, url);
      int httpCode = http.GET();
      if (httpCode > 0) {
        Serial.printf("✅ Data sent, response: %d\n", httpCode);
      } else {
        Serial.printf("❌ Send failed: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }

  serveMapPage();
}

void updateGPS() {
  unsigned long start = millis();
  while (millis() - start < 1000) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
  }
}

void getWiFiLocation() {
  int n = WiFi.scanNetworks();
  if (n == 0) return;

  DynamicJsonDocument doc(1024);
  JsonArray wifiArray = doc.createNestedArray("wifiAccessPoints");
  for (int i = 0; i < n && i < 5; i++) {
    JsonObject ap = wifiArray.createNestedObject();
    ap["macAddress"] = WiFi.BSSIDstr(i);
    ap["signalStrength"] = WiFi.RSSI(i);
  }

  String requestBody;
  serializeJson(doc, requestBody);

  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  HTTPClient https;

  String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + String(google_api_key);
  https.begin(secureClient, url);
  https.addHeader("Content-Type", "application/json");

  int httpResponseCode = https.POST(requestBody);
  if (httpResponseCode > 0) {
    String response = https.getString();
    DynamicJsonDocument jsonResponse(1024);
    deserializeJson(jsonResponse, response);
    latitude = String(jsonResponse["location"]["lat"].as<float>(), 6);
    longitude = String(jsonResponse["location"]["lng"].as<float>(), 6);
    Serial.println("📍 Wi-Fi Location: " + latitude + "," + longitude);
  } else {
    Serial.println("❌ Wi-Fi location failed.");
  }
  https.end();
}

void serveMapPage() {
  WiFiClient client = server.available();
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n' && request.endsWith("\r\n\r\n")) break;
      }
    }

    String html = "<!DOCTYPE html><html><head><title>Map</title><style>#map{height:90vh;width:100%;}</style></head><body><h2>Location Map</h2><div id='map'></div><script>function initMap(){var loc={lat:" + latitude + ",lng:" + longitude + "};var map=new google.maps.Map(document.getElementById('map'),{zoom:16,center:loc});new google.maps.Marker({position:loc,map:map});}</script><script async defer src='https://maps.googleapis.com/maps/api/js?key=" + String(google_api_key) + "&callback=initMap'></script></body></html>";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.print(html);
    client.stop();
    Serial.println("📡 Map served to browser.");
  }
}
