// Thermometer and digital clock based on a DS18B20 sensor, a TM1637 4-digit 7-segment display and an ESP8266
// Creation date: 2023-08-11
// Last update date: 2024-01-02
// v1.0

#include <time.h>
#include <ESP8266WiFi.h>
#include <TM1637Display.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <config.h>

// TM1637 pins
const int CLK = D5;
const int DIO = D6;
TM1637Display display(CLK, DIO);

const uint32_t updateInterval = 15 * 1000; // milliseconds
time_t now;
struct tm tm;

int hours = 0;
int minutes = 0;

OneWire ow(D7);
DallasTemperature dt(&ow);
float temperature;

WiFiClient wifiClient;

void setup();
void loop();
void updateTime();
void updateDisplayTime();
void updateTemperature();
void updateDisplayTemperature();
void sendTemperature();

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid, wifiPassword);
    configTime(0, 0, ntpServer);
    setenv("TZ", MY_TZ, 1); // Set environment variable with your time zone
    tzset();
    display.setBrightness(10);
    dt.begin();
    dt.setResolution(12);
}

void loop()
{
    updateTime();
    updateDisplayTime();
    if (tm.tm_sec % 30 == 5)
    {
        updateTemperature();
        updateDisplayTemperature();
    }
}

void updateTime()
{
    time(&now);             // read the current time
    localtime_r(&now, &tm); // update the structure tm with current time
    hours = tm.tm_hour;
    minutes = tm.tm_min;
}

void updateDisplayTime()
{
    uint8_t data[4];

    int hour10 = hours / 10;
    int hour1 = hours % 10;
    int min10 = minutes / 10;
    int min1 = minutes % 10;

    data[0] = hour10 == 0 ? 0 : display.encodeDigit(hour10);
    data[1] = display.encodeDigit(hour1) | 0x80;
    data[2] = display.encodeDigit(min10);
    data[3] = display.encodeDigit(min1);
    display.setSegments(data);
}

void updateTemperature()
{
    dt.requestTemperatures();
    delay(1000);
    temperature = dt.getTempCByIndex(0);
    sendTemperature();
}

void updateDisplayTemperature()
{
    static uint8_t data[] = {0, 0, 0, 0b01100011};
    static int tens, ones;
    static bool neg;
    tens = static_cast<int>(temperature) / 10;
    ones = static_cast<int>(temperature) % 10;
    neg = temperature < 0;
    data[0] = neg ? 0b01000000 : 0;
    data[1] = display.encodeDigit(tens);
    data[2] = display.encodeDigit(ones);
    display.setSegments(data);
    delay(10000);
}

void sendTemperature()
{
    String data = String(temperature, 4);
    HTTPClient client;

    client.begin(wifiClient, url + data);
    client.addHeader("Content-Type", "text/plain");

    int httpResponseCode = client.POST("");

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
}