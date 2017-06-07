/*
   MQTT Sensor - Brightness (photocell) for Home-Assistant - NodeMCU (ESP8266)
   https://home-assistant.io/components/sensor.mqtt/

   Libraries :
    - ESP8266 core for Arduino : https://github.com/esp8266/Arduino
    - PubSubClient : https://github.com/knolleary/pubsubclient
    - ArduinoJson : https://github.com/bblanchon/ArduinoJson

   Sources :
    - File > Examples > ES8266WiFi > WiFiClient
    - File > Examples > PubSubClient > mqtt_auth
    - File > Examples > PubSubClient > mqtt_esp8266
    - File > Examples > ArduinoJson > JsonGeneratorExample

   Schematic :
    - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_sensor_photocell/Schematic.png
    - Photocell leg 1 - VCC
    - Photocell leg 2 - A0 - Resistor 10K Ohms - GND
    - D0/GPIO16 - RST (wake-up purpose)

   Configuration (HA) :
    sensor 1:
      platform: mqtt
      state_topic: 'office/sensor1'
      name: 'Brightness'
      unit_of_measurement: '%'
      value_template: '{{ value_json.brightness }}'

   Samuel M. - v1.1 - 08.2016
   If you like this example, please add a star! Thank you!
   https://github.com/mertenats/open-home-automation
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1

// Wifi: SSID and password
const PROGMEM char* WIFI_SSID = "[Redacted]";
const PROGMEM char* WIFI_PASSWORD = "[Redacted]";

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "office_brightness";
const PROGMEM char* MQTT_SERVER_IP = "[Redacted]";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "[Redacted]";
const PROGMEM char* MQTT_PASSWORD = "[Redacted]";

// MQTT: topic
const PROGMEM char* MQTT_SENSOR_TOPIC = "office/sensor1";

// sleeping time
const PROGMEM uint16_t SLEEPING_TIME_IN_SECONDS = 600; // 10 minutes x 60 seconds

// Photocell: A0 
const PROGMEM uint8_t PHOTOCELL_PIN = 0;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// function called to publish the temperature and the humidity
void publishData(int p_analogRead) {
  // convert 0-1024 into a percentage
  uint8_t brightness = map(p_analogRead, 0, 1024, 0, 100);
  
  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["brightness"] = (String)brightness;
  root.prettyPrintTo(Serial);
  Serial.println("");
  /*
  {
    "brightness":  "75"
  }
  */
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(MQTT_SENSOR_TOPIC, data);
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // init the serial
  Serial.begin(115200);

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // read the value of the photocell
  uint16_t photocell = analogRead(PHOTOCELL_PIN);

  if (photocell < 0 || photocell > 1024) {
    Serial.println("ERROR: Failed to read from the photocell!");
    return;
  } else {
    publishData(photocell);
  }

  Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();

  Serial.println("INFO: Closing the Wifi connection");
  WiFi.disconnect();

  ESP.deepSleep(SLEEPING_TIME_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
  delay(500); // wait for deep sleep to happen
}
