#include <PlantMonitor.h>

PlantMonitor monitor("YourSSID", "YourPassword", 
                    "10.0.0.32", 1883, // MQTT IP and port 
                    "esp8266_bookshelf" // MQTT device name
                );

void setup() {
    Serial.begin(115200);

    // Optionally setup the DHT device for temperature and moisture readings
    monitor.beginDHT(D4, DHT22);
    // Configure MQTT topics for temperature and humidity (optional - defaults shown)
    monitor.setTemperatureTopic("sensor/temperature/bookshelf");
    monitor.setHumidityTopic("sensor/humidity/bookshelf");
    
    // Setup the multiplexer to read analogue signals from moisture sensors.
    // The first three inputs are the digital pins connecting to the multiplexer chip
    // while the last is the analogue pin. 
    monitor.beginMux(D0, D1, D2, A0);

    // add up to eight plants. The configuration takes the form:
    //  {"name", MUX_VALUE, MAX_DRY_READING, MAX_WET_READING, MQTT_NAME}
    monitor.addPlant({"prayer", 0, 712, 345, "sensor/moisture/prayer"});
    monitor.addPlant({"begonia", 1, 725, 355, "sensor/moisture/begonia"});
    monitor.addPlant({"myrtle", 2, 721, 360, "sensor/moisture/myrtle"});
    monitor.addPlant({"pinkprayer", 3, 724, 340, "sensor/moisture/pinkprayer"});
    monitor.addPlant({"kissbegonia", 4, 715, 330, "sensor/moisture/kissbegonia"});
    monitor.addPlant({"nerve", 6, 723, 0, "sensor/moisture/nerve"});

    monitor.connect();
}

void loop() {
    monitor.loop();
    monitor.readAndPublish();
    delay(60000);
}
