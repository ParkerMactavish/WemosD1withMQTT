#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <FS.h>
#include <SoftwareSerial.h>

/* Wifi Definition */
#define WIFI_CONF_FILE    "/wifiConfFile.txt"
#define WIFI_TRYOUT_TIMES (10)
/* End Wifi Definition */

/* MQTT Definition */
#define MQTT_CONF_FILE    "/mqttConfFile.txt"
#define MQTT_TRYOUT_TIMES (2)
/* End MQTT Definition */

/* Software Serial Definition */
#ifndef D5
#define D5 (14)
#define D6 (12)
#define D7 (13)
#define D8 (15)
#define TX (1)
#define softBaudrate  9600
#endif
/* End Software Serial Definition */

/* Global Object Declaration */
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
HTTPClient httpClient;
File configFile;
SoftwareSerial repeater;
/* End Global Object Declaration */

/* Global Buffer Declaration */
String ssidBuff,
       pswdBuff,
       mqttIPBuff,
       mqttTopicBuff,
       commandBuff,
       devicesBuff;
/* End Global Buffer Declaration */
//
///* Wifi Status Enumeration */
//typedef enum{
//  W_NO_CONFIG,
//  W_NO_CONNECTION,
//  W_CONNECTED
//} wifiStatus_t;
//wifiStatus_t wifiStatus = W_NO_CONFIG;
///* End Wifi Status Enumeration */
//
///* MQTT Status Enumeration */
//typedef enum {
//  M_NO_CONFIG,
//  M_NO_CONNECTION,
//  M_CONNECTED
//} mqttStatus_t;
//mqttStatus_t mqttStatus = M_NO_CONFIG;
///* End MQTT Status Enumeration */

/* Configuration Source Enumeration */
typedef enum{
  FROM_FILE,
  FROM_LOCALVAR
} configSource_t;
/* End Configuration Source Enumeration */

//uint8_t DFCommand[4][10]={
//  {0x7e, 0xff, 0x6, 0xc, 0x1, 0x0, 0x0, 0xfe, 0xee, 0xef},  // Initializing
//  {0x7e, 0xff, 0x6, 0x6, 0x1, 0x0, 0x14, 0xfe, 0xe0, 0xef}, // Setting Volume
//  {0x7e, 0xff, 0x6, 0x3, 0x1, 0x0, 0x1, 0xfe, 0xf6, 0xef},  // Play First Track
//  {0x7e, 0xff, 0x6, 0x3, 0x1, 0x0, 0x2, 0xfe, 0xf5, 0xef},  // Play Second Track
//  {0x7e, 0xff, 0x6, 0x3, 0x1, 0x0, 0x3, 0xfe, 0xf4, 0xef},  // Play Third Track
//};

uint8_t DFCommandBuff[10];

void setup_wifi(configSource_t from){
  if(from == FROM_FILE){
    configFile = SPIFFS.open(WIFI_CONF_FILE, "r");
    if(configFile){
      ssidBuff = configFile.readStringUntil('\n');
      pswdBuff = configFile.readStringUntil('\n');
      ssidBuff = ssidBuff.substring(0, ssidBuff.length()-1);
      pswdBuff = pswdBuff.substring(0, pswdBuff.length()-1);
//      Serial.println(ssidBuff.c_str());
//      Serial.println(pswdBuff.c_str());
      configFile.close();
      connect_wifi();
    }
  }
  else{
    if(connect_wifi() == 0){
      configFile = SPIFFS.open(WIFI_CONF_FILE, "w");
      configFile.println(ssidBuff);
      configFile.println(pswdBuff);
      configFile.close();
//      Serial.println("Saved");
    }
  }
}

void setup_mqtt(configSource_t from){
  if(from == FROM_FILE){
    configFile = SPIFFS.open(MQTT_CONF_FILE, "r");
    if(configFile){
      mqttIPBuff = configFile.readStringUntil('\n');
      mqttIPBuff = mqttIPBuff.substring(0, mqttIPBuff.length()-1);
      configFile.close();
      connect_mqtt();
    }
  }
  else{
    if(connect_mqtt() == 0){
      configFile = SPIFFS.open(MQTT_CONF_FILE, "w");
      configFile.println(mqttIPBuff);
      configFile.close();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

int connect_wifi(){
  uint8_t trial = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidBuff.c_str(), pswdBuff.c_str());
  while(WiFi.status() != WL_CONNECTED && trial++ < WIFI_TRYOUT_TIMES){
//    Serial.print(".");
    delay(500);
  }
  if(WiFi.status() != WL_CONNECTED){
    return -1;
  }
  return 0;
}

int connect_mqtt() {
  mqttClient.setServer(mqttIPBuff.c_str(), 3000);
  mqttClient.setCallback(callback);
  uint8_t trial = 0;
  while(!mqttClient.connected() && trial++ < MQTT_TRYOUT_TIMES){
//    Serial.print("Attempting MQTT connection...");
    String clientId = "Mactavish";
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
//      Serial.println("connected");
      // Once connected, publish an announcement...
      return 0;
    }
  }
  return -1;
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  SPIFFS.begin();
  Serial.begin(115200);
  repeater.begin(softBaudrate, SWSERIAL_8N1, D5, D6, false, 95, 90);
  setup_wifi(FROM_FILE);
  setup_mqtt(FROM_FILE);
}

void loop() {
  if(Serial.available()){
    if(Serial.peek() == ':'){
      delay(5);
      Serial.read();
//      repeater.print((char*)DFCommand[commandBuff.substring(1).toInt()]);
      for(int i = 0; i < 10; i ++){
        DFCommandBuff[i] = Serial.read();
      }
      for(int i = 0; i < 10; i ++){
        repeater.write(DFCommandBuff[i]);
      }
    }
    else {
      delay(50);
      commandBuff = Serial.readStringUntil('\n');
      digitalWrite(BUILTIN_LED, HIGH);
  //    Serial.println(commandBuff);
      if(strncmp(commandBuff.c_str(), "getCON", 6) == 0){
        if(WiFi.status() == WL_CONNECTED && mqttClient.connected()){
          Serial.println(2);
        }
        else if(WiFi.status() == WL_CONNECTED && !mqttClient.connected()){
          Serial.println(1);
        }
        else if(WiFi.status() != WL_CONNECTED && !mqttClient.connected()){
          Serial.println(0);
        }
      }
      else if(strncmp(commandBuff.c_str(), "ssid:", 5) == 0){
        ssidBuff = commandBuff.substring(5);
        Serial.println(ssidBuff);
      }
      else if(strncmp(commandBuff.c_str(), "pswd:", 5) == 0){
        pswdBuff = commandBuff.substring(5);
        Serial.println(pswdBuff);
      }
      else if(strncmp(commandBuff.c_str(), "setCON", 6) == 0){
        setup_wifi(FROM_LOCALVAR);
        if(WiFi.status() == WL_CONNECTED)
          Serial.println("OK");
        else
          Serial.println("ERROR");
      }
      else if(strncmp(commandBuff.c_str(), "init:", 5) == 0){
        if(mqttClient.connected()){
          mqttClient.disconnect();
        }
        if(httpClient.begin(wifiClient, commandBuff.substring(5).c_str())){
          uint16_t code = httpClient.GET();
          if(code>0){
            devicesBuff = httpClient.getString();
          }
        }
        connect_mqtt();
        if(mqttTopicBuff.length()){
          mqttClient.subscribe(mqttTopicBuff.c_str());
        }
        Serial.println(devicesBuff);
      }
      else if(strncmp(commandBuff.c_str(), "reachMQTT:", 10) == 0){
        mqttIPBuff = commandBuff.substring(10);
        setup_mqtt(FROM_LOCALVAR);
        if(!mqttClient.connected())
          Serial.println("ERROR");
        else
          Serial.println("OK");
      }
      else if(strncmp(commandBuff.c_str(), "sub:", 4) == 0){
        mqttTopicBuff = commandBuff.substring(4);
        Serial.println((mqttClient.subscribe(mqttTopicBuff.c_str()) == 1)?"OK":"ERROR");
      }
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
//
//  if(repeater.available()){
////    digitalWrite(LED_BUILTIN, LOW);
//    Serial.write(repeater.read());
////    digitalWrite(LED_BUILTIN, HIGH);
//  }

  if(WiFi.status() == WL_CONNECTED && !mqttClient.connected() && mqttIPBuff.length()){
    digitalWrite(LED_BUILTIN, LOW);
    mqttClient.connect("Mactavish");
    digitalWrite(LED_BUILTIN, HIGH);
  }

  mqttClient.loop();
}
