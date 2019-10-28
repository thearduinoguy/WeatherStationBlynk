//#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
//#define BLYNK_DEBUG

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "Seeed_BME280.h"
#include <Wire.h>
#include <EEPROM.h>

BME280 bme280;
char auth[] = "YourBlynkAuthTokenHere";
// EEPROM address
int addr = 0;

WiFiClient client;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "YourSSID";
char pass[] = "YourPassword";
const char* server = "api.thingspeak.com";
const char* api_key = "xxxx";

unsigned int raw = 0;
float volt = 0.0;

void setup()
{
  EEPROM.begin(4);
  Blynk.begin(auth, ssid, pass);
  if (!bme280.init()) {
    //Serial.println("Device error!");
  }
  WiFi.begin(ssid, pass);
  pinMode(A0, INPUT);

  // If we were in emergency power mode before the last reset, enter it again now
  if (EEPROM.read(addr) == 99)
  {
    EmergencyPowerMode();
  }
  else
  {
    EEPROM.write(addr, 0);
    EEPROM.commit();
  }
}

void postData(float temperature, float humidity, float pressure, float voltage) {
  // Send data to ThingSpeak
  if (client.connect(server, 80)) {
    //Serial.println("Connect to ThingSpeak - OK");

    String dataToThingSpeak = "";
    dataToThingSpeak += "GET /update?api_key=";
    dataToThingSpeak += api_key;

    dataToThingSpeak += "&field1=";
    dataToThingSpeak += String(temperature);

    dataToThingSpeak += "&field2=";
    dataToThingSpeak += String(humidity);

    dataToThingSpeak += "&field3=";
    dataToThingSpeak += String(pressure);

    dataToThingSpeak += "&field4=";
    dataToThingSpeak += String(voltage);

    dataToThingSpeak += " HTTP/1.1\r\nHost: a.c.d\r\nConnection: close\r\n\r\n";
    dataToThingSpeak += "";
    client.print(dataToThingSpeak);

    int timeout = millis() + 5000;
    while (client.available() == 0) {
      if (timeout - millis() < 0) {
        //Serial.println("Error: Client Timeout!");
        client.stop();
        return;
      }
    }
  }
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line);
  }
}

void EmergencyPowerMode()
{
  // if the last mode before reset was CHARGING then continue
  if (EEPROM.read(addr) == 99)
  {
    Blynk.notify("Battery Mode - Charging.");
    delay(1000);
  }
  else 
  {
    Blynk.notify("Battery Critical! Entered Emergency Power Mode!");
    // if this is the first time we have entered EPM then write thsis to eeprom
    EEPROM.write(addr, 99);
    EEPROM.commit();
  }

  //get voltage data
  raw = analogRead(A0);
  volt = raw / 1023.0;
  volt = (volt * 4.08);

  if (volt <= 4.0)
  {
    EEPROM.write(addr, 99);
    EEPROM.commit();
    ESP.deepSleep(60U * 60 * 1000000); //
  }
  else
  {
    Blynk.notify("Battery Fully Charged. Resuming Normal Power Mode!");
    // write normal mode to eeprom
    EEPROM.write(addr, 0);
    EEPROM.commit();
  }
}

void loop()
{
  Blynk.run();

  //get and print temperatures
  float temp = bme280.getTemperature();
  //Serial.print("Temp: ");
  //Serial.print(temp);
  //Serial.println("C");//The unit for  Celsius because original arduino don't support speical symbols
  Blynk.virtualWrite(0, temp); // virtual pin 0
  Blynk.virtualWrite(4, temp); // virtual pin 4

  //get and print atmospheric pressure data
  float pressure = bme280.getPressure(); // pressure in Pa
  float p = pressure / 100.0 ; // pressure in hPa
  //Serial.print("Pressure: ");
  //Serial.print(p);
  //Serial.println("hPa");
  Blynk.virtualWrite(1, p); // virtual pin 1

  //get and print humidity data
  float humidity = bme280.getHumidity();
  //Serial.println("Humidity: " + String(humidity) + "%");
  //Serial.println();
  Blynk.virtualWrite(3, humidity); // virtual pin 3

  //get and print voltage data
  raw = analogRead(A0);
  volt = raw / 1023.0;
  volt = (volt * 4.08);
  //Serial.print("Voltage: ");
  //Serial.print(volt);
  //Serial.println("v");
  Blynk.virtualWrite(2, volt); // virtual pin 2

  // Write the data to ThingSpeak
  postData(temp, humidity, pressure, volt);

  //Twitter stuff
  Blynk.tweet(String("Temp ") + temp + "  Humidity " + humidity);
  

  // If battery critical enter emergency power mode
  if (volt <= 3.7)
  {
    EmergencyPowerMode();
  }

  // deepSleep time is defined in microseconds. Multiply seconds by 1e6
  // MINUTES x SECONDS x MICROSECONDS
  ESP.deepSleep(20 * 60 * 1000000); //
}
