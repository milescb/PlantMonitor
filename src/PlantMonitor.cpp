#include "PlantMonitor.h"

PlantMonitor::PlantMonitor(const char* ssid, const char* password,
                           const char* mqttBroker, int mqttPort,
                           const char* mqttClientId)
    : client(espClient), ssid(ssid), password(password),
      mqttBroker(mqttBroker), mqttPort(mqttPort), mqttClientId(mqttClientId) {}

void PlantMonitor::beginDHT(uint8_t pin, uint8_t type) {
    dht = new DHT(pin, type);
    dht->begin();
}

void PlantMonitor::beginMux(uint8_t a, uint8_t b, uint8_t c, uint8_t analogPin) {
    muxA = a; muxB = b; muxC = c; analogInput = analogPin;
    pinMode(muxA, OUTPUT);
    pinMode(muxB, OUTPUT);
    pinMode(muxC, OUTPUT);
}

void PlantMonitor::addPlant(const PlantConfig& config) {
    plants.push_back(config);
}

void PlantMonitor::connect() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    client.setServer(mqttBroker, mqttPort);

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void PlantMonitor::loop() {
    if (!client.connected()) reconnect();
    client.loop();
}

void PlantMonitor::readAndPublish() {
    // DHT
    if (dht) {
        float t = dht->readTemperature();
        float h = dht->readHumidity();
        if (!isnan(t) && !isnan(h)) {
            t = celsiusToFahrenheit(t);
            tempSum += t; tempCount++;
            humSum += h; humCount++;
            Serial.printf("Temp: %.2f F, Hum: %.2f %%\n", t, h);
        }
    }

    // Moisture
    for (auto& plant : plants) {
        float m = readMoisture(plant.muxChannel, plant.rawMin, plant.rawMax);
        plant.sum += m;
        plant.count++;
        Serial.printf("%s Moisture: %.2f %%\n", plant.name, m);
    }

    // Publish schedule
    if (intervalCounter % 10 == 0) {
        if (tempCount > 0) {
            publishData("sensor/temperature", tempSum / tempCount, "F");
            tempSum = 0; tempCount = 0;
        }
        if (humCount > 0) {
            publishData("sensor/humidity", humSum / humCount, "%");
            humSum = 0; humCount = 0;
        }
    }

    if (intervalCounter % 60 == 0) {
        for (auto& plant : plants) {
        if (plant.count > 0) {
            publishData(plant.topic, plant.sum / plant.count, "%");
            plant.sum = 0;
            plant.count = 0;
        }
        }
        intervalCounter = 0;
    }

    intervalCounter++;
}

void PlantMonitor::changeMux(int c, int b, int a) {
    digitalWrite(muxA, a);
    digitalWrite(muxB, b);
    digitalWrite(muxC, c);
}

float PlantMonitor::readMoisture(int muxChannel, float rawMin, float rawMax) {
    int select[8][3] = {
        {LOW, LOW, LOW}, {LOW, LOW, HIGH}, {LOW, HIGH, LOW}, {LOW, HIGH, HIGH},
        {HIGH, LOW, LOW}, {HIGH, LOW, HIGH}, {HIGH, HIGH, LOW}, {HIGH, HIGH, HIGH}
    };
    changeMux(select[muxChannel][2], select[muxChannel][1], select[muxChannel][0]);
    delay(10);
    int raw = analogRead(analogInput);
    return map(raw, rawMin, rawMax, 0, 100);
}

float PlantMonitor::celsiusToFahrenheit(float temp) {
    return temp * 9.0 / 5.0 + 32.0;
}

void PlantMonitor::reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting MQTT...");
        if (client.connect(mqttClientId)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.println(client.state());
            delay(5000);
        }
    }
}

void PlantMonitor::publishData(const char* topic, float value, const char* unit) {
    time_t now = time(nullptr);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    char msg[100];
    snprintf(msg, sizeof(msg), "{\"timestamp\":\"%s\",\"value\":%.2f,\"unit\":\"%s\"}",
             timestamp, value, unit);
    client.publish(topic, msg);
    Serial.printf("Published %s: %s\n", topic, msg);
}
