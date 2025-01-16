#include <DHT.h>        // Include library for DHT sensor
#include "MQ135.h"      // Include library for MQ135 sensor
#include <ESP8266WiFi.h>

// ThingSpeak configuration
String apiKey = "Q4M7MV6LSAGFXDN6"; // Updated ThingSpeak Write API key
const char *ssid = "SHUKLA G-4G";    // Updated Wi-Fi SSID
const char *pass = "shukla1234";     // Updated Wi-Fi password
const char* server = "api.thingspeak.com";

// Sensor configuration
#define DHTPIN 4                    // GPIO2 on NodeMCU for DHT11
DHT dht(DHTPIN, DHT11);
MQ135 gasSensor = MQ135(A0);        // MQ135 sensor connected to analog pin A0

// Variables for storing sensor data
float temperature = 0.0;
float humidity = 0.0;
int air_quality = 0;

// WiFi client
WiFiClient client;

// Timing variables for sensor readings and ThingSpeak updates
unsigned long lastSensorReadTime = 0;
unsigned long lastThingSpeakUpdateTime = 0;
const unsigned long sensorReadInterval = 2000; // 2 seconds for DHT11 sensor readings
const unsigned long thingspeakUpdateInterval = 15000; // 15 seconds for ThingSpeak updates

void setup() {
    Serial.begin(115200);
    delay(10);

    // Initialize DHT sensor
    dht.begin();
    Serial.println("Initializing sensors...");

    // Connect to Wi-Fi
    Serial.println("Connecting to Wi-Fi..."); 
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    unsigned long currentMillis = millis();

    // Read sensors every 2 seconds
    if (currentMillis - lastSensorReadTime >= sensorReadInterval) {
        lastSensorReadTime = currentMillis;

        // Read DHT sensor values
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();

        // Validate sensor readings
        if (isnan(humidity) || isnan(temperature)) {
            Serial.println("Failed to read from DHT sensor! Retrying...");
            return; // Skip this loop iteration if DHT fails
        }

        // Read air quality from MQ135 sensor
        air_quality = gasSensor.getPPM();

        // Print sensor data
        Serial.println("-------------------------------------------------");
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print(" °C | Humidity: ");
        Serial.print(humidity);
        Serial.print(" % | Air Quality: ");
        Serial.print(air_quality);
        Serial.println(" PPM");
        Serial.println("-------------------------------------------------");
    }

    // Send data to ThingSpeak every 15 seconds
    if (currentMillis - lastThingSpeakUpdateTime >= thingspeakUpdateInterval) {
        lastThingSpeakUpdateTime = currentMillis;

        if (client.connect(server, 80)) {
            String postStr = apiKey;
            postStr += "&field1=" + String(temperature);   // Send temperature to field1
            postStr += "&field2=" + String(humidity);      // Send humidity to field2
            postStr += "&field3=" + String(air_quality);   // Send air quality to field3
            postStr += "\r\n\r\n";

            client.print("POST /update HTTP/1.1\r\n");
            client.print("Host: api.thingspeak.com\r\n");
            client.print("Connection: close\r\n");
            client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\r\n");
            client.print("Content-Type: application/x-www-form-urlencoded\r\n");
            client.print("Content-Length: " + String(postStr.length()) + "\r\n\r\n");
            client.print(postStr);

            // Read the response from ThingSpeak
            String response = client.readString();
            Serial.println("Response from ThingSpeak:");
            Serial.println(response);

            Serial.println("Data sent to ThingSpeak.");
            client.stop();
        } else {
            Serial.println("Failed to connect to ThingSpeak.");
        }
    }
}
