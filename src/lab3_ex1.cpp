#include "Arduino.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT Broker settings
const char* mqtt_broker = "mqtt.iotserver.uz";  // Free public MQTT broker
const int mqtt_port = 1883;
const char* mqtt_username = "userTTPU";  // username given in the telegram group
const char* mqtt_password = "mqttpass";  // password given in the telegram group

// global declarations
#define RED_LED 26
#define GREEN_LED 27
#define BLUE_LED 14
#define YELLOW_LED 12

#define BUTTON 25
#define LIGHT_SENSOR 33

// topics
const char* topic_sensor = "ttpu/iot/narimon/sensors/light";
const char* topic_buttom = "ttpu/iot/narimon/events/button";

//time
unsigned long lastPublishTime = 0;
const long publishInterval = 5000;  // Publish every 5 seconds

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// functions
void publishEveryFive();
void connectMQTT();
void connectWiFi();
void checkMQTTconnection();
void check_WiFi_connection();
void SetUpPinMode();
void sentButtonState(const char* State);

void setup(){
  Serial.begin(9600);
  delay(1000);
  SetUpPinMode();

  configTime(18000, 0, "pool.ntp.org"); // UTC+5 for Tashkent

  // Connect to WiFi
  connectWiFi();
  
  // Setup MQTT
  mqtt_client.setServer(mqtt_broker, mqtt_port);

  // Connect to MQTT broker
  connectMQTT();
}


void loop() {
  check_WiFi_connection();
  checkMQTTconnection();

  // Process incoming MQTT messages
  mqtt_client.loop();

  publishEveryFive();  // Publish message every 5 seconds

  bool lastState = digitalRead(BUTTON);
  delay(50);
  bool currentState = digitalRead(BUTTON);

  if (lastState == LOW && currentState == HIGH) {
    sentButtonState("PRESSED");
  } 
  else if (lastState == HIGH && currentState == LOW){
    sentButtonState("RELEASED");
  }

}

void sentButtonState(const char* State){
    JsonDocument state;
    state["light"] = State;
    state["timestamp"] = time(nullptr);

    char buffer[256];
    serializeJson(state, buffer);

    Serial.print("Publishing message: ");
    Serial.println(buffer);
    
    if (mqtt_client.publish(topic_buttom, buffer)) {
      Serial.println("Message published successfully!");
    } else {
      Serial.println("Failed to publish message!");
    }
    Serial.println("---");
}

// Publish message every 5 seconds
void publishEveryFive(){
  unsigned long currentTime = millis();
  if (currentTime - lastPublishTime >= publishInterval) {
    lastPublishTime = currentTime;

    JsonDocument doc;
    doc["light"] = analogRead(LIGHT_SENSOR);
    doc["timestamp"] = time(nullptr);

    char buffer[256];
    serializeJson(doc, buffer);
    
    Serial.print("Publishing message: ");
    Serial.println(buffer);
    
    if (mqtt_client.publish(topic_sensor, buffer)) {
      Serial.println("Message published successfully!");
    } else {
      Serial.println("Failed to publish message!");
    }
    Serial.println("---");
  }
}

void check_WiFi_connection(){
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
}

void checkMQTTconnection(){
  if (!mqtt_client.connected()) {
    Serial.println("MQTT disconnected! Reconnecting...");
    connectMQTT();
  }
}

// Function to connect to WiFi
void connectWiFi() {
  Serial.println("\nConnecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect/reconnect to MQTT broker
void connectMQTT() {
  while (!mqtt_client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    
    String client_id = "esp32-client-" + String(WiFi.macAddress());
    
    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.println(mqtt_client.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void SetUpPinMode(){
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  pinMode(BUTTON, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);
}