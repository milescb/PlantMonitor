#pragma once
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>

struct PlantConfig {
    const char* name;
    int muxChannel;
    float rawMin;
    float rawMax;
    const char* topic;

    // running stats
    float sum = 0;
    int count = 0;
};

class PlantMonitor {
public:
    PlantMonitor(const char* ssid, const char* password,
                 const char* mqttBroker, int mqttPort,
                 const char* mqttClientId);

    void beginDHT(uint8_t pin, uint8_t type);
    void beginMux(uint8_t a, uint8_t b, uint8_t c, uint8_t analogPin);
    void addPlant(const PlantConfig& config);
    void connect();
    void loop();
    void readAndPublish();

private:
    void changeMux(int c, int b, int a);
    float readMoisture(int muxChannel, float rawMin, float rawMax);
    float celsiusToFahrenheit(float temp);
    void reconnect();
    void publishData(const char* topic, float value, const char* unit);

    WiFiClient espClient;
    PubSubClient client;
    const char* ssid;
    const char* password;
    const char* mqttBroker;
    int mqttPort;
    const char* mqttClientId;

    DHT* dht = nullptr;

    uint8_t muxA, muxB, muxC, analogInput;
    std::vector<PlantConfig> plants;

    int intervalCounter = 0;

    // Running averages
    float tempSum = 0; int tempCount = 0;
    float humSum = 0; int humCount = 0;
};
