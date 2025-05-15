#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

HardwareSerial LoRaSerial(2);  

const char* ssid = "Wi-Fi Name";
const char* password = "Wi-Fi Password";
const char* thingspeakApiKey = "ThingSpeak Write API Key";

void setup() {
  Serial.begin(115200);
  LoRaSerial.begin(115200, SERIAL_8N1, 16, 17);

  delay(2000);

  // Configure LoRa
  LoRaSerial.println("AT+BAND=915900000");
  delay(100);
  LoRaSerial.println("AT+ADDRESS=10");
  delay(100);
  LoRaSerial.println("AT+NETWORKID=26");
  delay(100);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void loop() {
  if (LoRaSerial.available()) {
    String receivedData = LoRaSerial.readString();
    Serial.println("Received: " + receivedData);

    float temp, humidity, pressure;

    // Parse sensor data
    int tIndex = receivedData.indexOf("TEMP:");
    int hIndex = receivedData.indexOf("|HUM:");
    int pIndex = receivedData.indexOf("|PRES:");

    if (tIndex >= 0 && hIndex >= 0 && pIndex >= 0) {
      temp = receivedData.substring(tIndex + 5, hIndex).toFloat();
      humidity = receivedData.substring(hIndex + 5, pIndex).toFloat();
      pressure = receivedData.substring(pIndex + 6).toFloat();

      Serial.printf("Parsed BME280 -> Temp: %.2f, Hum: %.2f, Pres: %.2f\n", temp, humidity, pressure);

      // Fetch Open-Meteo API data
      float apiTemp, apiHumidity, apiPressure;
      if (fetchWeather(apiTemp, apiHumidity, apiPressure)) {
        // Send everything to ThingSpeak
        updateThingSpeak(temp, humidity, pressure, apiTemp, apiHumidity, apiPressure);
      }
    }
  }
}

bool fetchWeather(float& temp, float& humidity, float& pressure) {
  HTTPClient http;
  http.begin("https://api.open-meteo.com/v1/forecast?latitude=38.22&longitude=-85.66&current=temperature_2m,relative_humidity_2m,pressure_msl");

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Weather API response: " + payload);

    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      Serial.println("Failed to parse weather JSON");
      return false;
    }

    temp = doc["current"]["temperature_2m"];
    humidity = doc["current"]["relative_humidity_2m"];
    pressure = doc["current"]["pressure_msl"];

    Serial.printf("Parsed Weather -> Temp: %.2f, Hum: %.2f, Pres: %.2f\n", temp, humidity, pressure);

    return true;
  } else {
    Serial.println("Failed to get weather data. HTTP code: " + String(httpCode));
    return false;
  }

  http.end();
}

void updateThingSpeak(float t1, float h1, float p1, float t2, float h2, float p2) {
  HTTPClient http;
  String url = "https://api.thingspeak.com/update?api_key=" + String(thingspeakApiKey) +
               "&field1=" + String(t1, 2) +
               "&field2=" + String(h1, 2) +
               "&field3=" + String(p1, 2) +
               "&field4=" + String(t2, 2) +
               "&field5=" + String(h2, 2) +
               "&field6=" + String(p2, 2);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    Serial.println("ThingSpeak update successful");
  } else {
    Serial.println("ThingSpeak update failed. HTTP code: " + String(httpCode));
  }

  http.end();
}