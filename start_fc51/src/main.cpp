#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPIFFS.h>

#define WIFI_SSID "E308"
#define WIFI_PASSWORD "98806829"
#define MQTT_SERVER "f08089bc943a49cd9c62cb94a58e946d.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "hivemq.webclient.1732612694947"
#define MQTT_PASSWORD "e&D2?FR5qh0Tod<1VH>m"
#define ONE_WIRE_BUS 5

WiFiClientSecure espClient;
PubSubClient client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
    Serial.begin(115200);
    sensors.begin();

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Use insecure mode for SSL/TLS connection
    espClient.setInsecure();

    // Connect to MQTT
    client.setServer(MQTT_SERVER, MQTT_PORT);
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("Connected to MQTT!");
        } else {
            Serial.print("Failed with state: ");
            Serial.println(client.state());
            delay(5000);
        }
    }

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS. Formatting...");
        SPIFFS.format();
        if (!SPIFFS.begin(true)) {
            Serial.println("Failed to initialize SPIFFS after formatting.");
            return;
        }
    }
    Serial.println("SPIFFS initialized successfully");
}

void logToSPIFFS(String data) {
    File file = SPIFFS.open("/temperature_log.txt", FILE_APPEND);
    if (file) {
        file.println(data);
        file.close();
        Serial.println("Logged to SPIFFS: " + data);
    } else {
        Serial.println("Failed to log data to SPIFFS.");
    }
}

void loop() {
    // Check MQTT connection
    if (!client.connected()) {
        while (!client.connected()) {
            Serial.println("Reconnecting to MQTT...");
            if (client.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
                Serial.println("Reconnected to MQTT!");
            } else {
                Serial.print("Failed with state: ");
                Serial.println(client.state());
                delay(5000);
            }
        }
    }

    // Read temperature and process
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);

    if (temperature == DEVICE_DISCONNECTED_C) {
        Serial.println("Failed to read from DS18B20 sensor!");
        return;
    }

    // Format the temperature as a string
    char tempString[8];
    dtostrf(temperature, 6, 2, tempString);
    String data = String(tempString);

    // Log to SPIFFS
    logToSPIFFS(data);

    // Publish to MQTT
    if (client.publish("iot/temperature", data.c_str())) {
        Serial.print("Published to MQTT: ");
        Serial.println(data);
    } else {
        Serial.println("Failed to publish to MQTT.");
    }

    delay(20000); // Wait 20 seconds before the next reading
}
