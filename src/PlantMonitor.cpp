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

void PlantMonitor::setTemperatureTopic(const char* topic) {
    temperatureTopic = topic;
}

void PlantMonitor::setHumidityTopic(const char* topic) {
    humidityTopic = topic;
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
            publishData(temperatureTopic, tempSum / tempCount, "F");
            tempSum = 0; tempCount = 0;
        }
        if (humCount > 0) {
            publishData(humidityTopic, humSum / humCount, "%");
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
    // Set the MUX to the desired channel
    switch(muxChannel) {
        case 0: changeMux(LOW, LOW, LOW); break;  // Channel 0: 000
        case 1: changeMux(LOW, LOW, HIGH); break; // Channel 1: 001
        case 2: changeMux(LOW, HIGH, LOW); break; // Channel 2: 010
        case 3: changeMux(LOW, HIGH, HIGH); break; // Channel 3: 011
        case 4: changeMux(HIGH, LOW, LOW); break; // Channel 4: 100
        case 5: changeMux(HIGH, LOW, HIGH); break; // Channel 5: 101
        case 6: changeMux(HIGH, HIGH, LOW); break; // Channel 6: 110
        case 7: changeMux(HIGH, HIGH, HIGH); break; // Channel 7: 111
        default: changeMux(LOW, LOW, LOW); break;  // Default
    }
    delay(10); // time to settle

    // read and convert 
    int rawValue = analogRead(analogInput);
    float moisturePercent = map(rawValue, rawMin, rawMax, 0, 100);
    // could constrain the value to 0-100, but want to see if not well calibrated  

    return moisturePercent;
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
