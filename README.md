# Plant Monitor 

An Arduino library for home temperature and humidity measurements with a DHT sensor and plant moisture level measurements with a capacitive moisture sensor. Measurements are sent via MQTT to any host where MQTT messages can be sent. 

This library is used for obtaining readings on [rain.crabdance.com](https://rain.crabdance.com). 

## Requirements

### Hardware

- ESP8266 dev board
- CD4051BE multiplexer
- Breadboard / jumper cables etc.

### Software

- Arduino IDE
- ESP8266 board software and drivers

## Wiring the device

### ESP8266 to DHT Sensor

| ESP8266 Pin | DHT Pin | Description |
|-------------|---------|-------------|
| D4          | Out     | Data signal |
| G           | -       | Ground      |
| 3.3V        | +       | Power       |

### ESP8266 to CD4051BE Multiplexer

| ESP8266 Pin | CD4051BE Pin | Description        |
|-------------|--------------|-------------------|
| D0          | S0 (A)       | Address bit A     |
| D1          | S1 (B)       | Address bit B     |
| D2          | S2 (C)       | Address bit C     |
| A0          | Common       | Analog input      |
| 3.3V        | VCC          | Power             |
| G           | GND          | Ground            |
| G           | Inhibit      | Disable (tie low) |
| G           | VEE          | Negative supply   |  

Connect the capacitive moisture sensors to any of the I/O channels of the multiplexer as well as ground and 3.3v as appropriate. 

## Configuration

See the BasicUsage.ino script for detailed configuration. 
For calibrating the max dry and wet readings, take a few measurements with the sensor in air and with water and mark these down. 