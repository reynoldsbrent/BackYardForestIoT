// Transmitter.ino

#include <HardwareSerial.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


Adafruit_BME280 bme;
HardwareSerial LoRaSerial(2);  

void setup() {
  Serial.begin(115200);

  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME280 sensor");
    while (1);
  }

  LoRaSerial.begin(115200, SERIAL_8N1, 16, 17);

  delay(2000);

  // Set LoRa Parameters
  LoRaSerial.println("AT+BAND=915900000");
  delay(100);
  LoRaSerial.println("AT+ADDRESS=9");
  delay(100);
  LoRaSerial.println("AT+NETWORKID=26");
  delay(100);
  LoRaSerial.println("AT+POWER=20");
  delay(100);

  Serial.println("Transmitter ready.");
}

void loop() {
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  String message = "TEMP:" + String(temp, 2) +
                   "|HUM:" + String(humidity, 2) +
                   "|PRES:" + String(pressure, 2);
  String command = "AT+SEND=10," + String(message.length()) + "," + message;
  LoRaSerial.println(command);

  Serial.println("Sent: " + message);

  delay(10000); // Send every 10 seconds
}
