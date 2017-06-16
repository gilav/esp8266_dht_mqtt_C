#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


#define VERSION "V:0.5.0 Lavaux Gilles 06/2017"

// DHT sensor
#define DHTPIN 2        // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11

// MQTT settings and topics
#define AIO_SERVER      "MQTT_BROKER_ADDRESS"
#define AIO_SERVERPORT  7906
#define AIO_USERNAME    "MQTT_USER"
#define AIO_PASSWORD    "MQTT_PASSWORD"

// MQTT topic base path
const char *MQTT_TOPIC = "portable/esp_";

// Access Point lists and password
boolean apFound = false;
const char *apUsed;
const char *ssid[2] = {"AP_1", "AP_2"};
const char *pass[2] = {"password_AP1", "password_AP2"};
const int apCount = 2;

// loop interval: 5 sec
long loopInterval = 500;
long previousLoopMillis = 0;
// mqtt ping interval: 2 mins
long pingInterval = 120000;
long previousPingMillis = 0;
// mqtt publish interval: 30 sec
long publishInterval = 10000;
long previousPublishMillis = 0;
unsigned long publishCount = 0;
//
unsigned long count = 0;

//DHT dht(DHTPIN, DHTTYPE, 20);

// wifi client
WiFiClient client;
//WiFiClientSecure client;
// mqtt client
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_PASSWORD);
//Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);
// mqtt feeds
String fullTopicTemp = String("mobile/esp_") + String(ESP.getChipId(), HEX) + String("/temp");
Adafruit_MQTT_Publish feedTemp = Adafruit_MQTT_Publish(&mqtt, fullTopicTemp.c_str());
String fullTopicHumi = String("mobile/esp_") + String(ESP.getChipId(), HEX) + String("/humi");
Adafruit_MQTT_Publish feedHuni = Adafruit_MQTT_Publish(&mqtt, fullTopicHumi.c_str());
String fullTopicUp = String("mobile/esp_") + String(ESP.getChipId(), HEX) + String("/uptime");
Adafruit_MQTT_Publish feedUp = Adafruit_MQTT_Publish(&mqtt, fullTopicUp.c_str());
String fullTopicMem = String("mobile/esp_") + String(ESP.getChipId(), HEX) + String("/mem");
Adafruit_MQTT_Publish feedMem = Adafruit_MQTT_Publish(&mqtt, fullTopicMem.c_str());



// setup serial and wifi mode
void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.printf("The ESP8266 chip ID as a 32-bit integer: %08X\n", ESP.getChipId());
  Serial.printf("The flash chip ID as a 32-bit integer: %08X\n", ESP.getFlashChipId());
  Serial.printf("Flash chip frequency: %d (Hz)\n", ESP.getFlashChipSpeed());
  Serial.print("will publish on topic:");
  Serial.println(fullTopicMem);
  
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.println(VERSION);
  Serial.println(" WIFI set in station mode");
  WiFi.disconnect();
  Serial.println(" WIFI reset");
  delay(100);
}


// get free heap 
String getFreeHeap(){
  long  fh = ESP.getFreeHeap();
  char  fhc[20];
  ltoa(fh, fhc, 10);
  return String(fhc);
}


void loadCerts(){
  SPIFFS.begin();  
}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
boolean mqtt_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return true;
  }

  Serial.print("  connecting to MQTT");
  Serial.print(AIO_SERVER);
  Serial.print(":");
  Serial.print(AIO_SERVERPORT);
  Serial.print("... ");
  
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.print("  MQTT connect error: ");
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("  retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         return false;
       }
  }

  Serial.println("  MQTT Connected!");
  return true;
}


// publish something to MQTT
void doPublish(){
  if (! mqtt.connected()){
    return;
  }
  String heap = getFreeHeap();
  Serial.print("  publishing[");
  Serial.print(publishCount);
  Serial.print("]:");
  Serial.print(heap.c_str());
  if (! feedMem.publish(heap.c_str()) ) {
    Serial.println(F(" Failed"));
  } else {
    Serial.println(F(" OK!"));
  }
  publishCount++;
}


// use an access point
boolean useAp(const char *ssid, const char *password){
  Serial.println();
  Serial.print("   connecting to ");
  Serial.print(ssid);
  Serial.print(" with password:");
  Serial.print(password);
  Serial.print(" ");
  int limit=15;
  WiFi.begin(ssid, password);
  while ((WiFi.status() != WL_CONNECTED) && (limit > 0)) {
    limit--;
    delay(1000);
    Serial.print("_-");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("    WiFi connected");
    delay(10);
    apUsed=ssid;
    return true;
  }else if (limit==0){
    Serial.println("");
    Serial.println("    too many WiFi retry");
    delay(10);
    return false;
  }
}


// scan access points and connect to known one
void scanAp(){
  int n = WiFi.scanNetworks();
  Serial.println("  scan done");
  if (n == 0)
    Serial.println("  no networks found");
  else
  {
    Serial.print("  ");
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n && !apFound; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(" ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print("): ");
      if(WiFi.encryptionType(i) == ENC_TYPE_NONE){
        Serial.println("open");
      }else{
        Serial.println(WiFi.encryptionType(i));
      }
      for(int j = 0; j < apCount; ++j){
        Serial.print(" ");
        Serial.print(" testing known AP[");
        Serial.print(j);
        Serial.print("]:");
        Serial.print(ssid[j]);
        Serial.print(" VS ");
        Serial.print(WiFi.SSID(i));
        if(WiFi.SSID(i)==ssid[j]){
          Serial.print("  known AP found !!");
          apFound = useAp(ssid[j], pass[j]);
          if(apFound){
            break;
          }
        }else{
          Serial.println();
        }
      }
    }
  }
  Serial.println("");
}


// set WIFI and connect to MQTT
void wifiAndMqttConnect(){
    if (!apFound){
      scanAp();
    }else{
      if(WiFi.status() != WL_CONNECTED){
        Serial.println("  AP was found, but disconnected! rescan wifi...");
        apFound=false;
        scanAp();
      }else{
        //Serial.println("  AP already found and connected");
        mqtt_connect();
      }
    }
    count++;
}


// main loop
void loop(){
    unsigned long currentMillis = millis();
    if(currentMillis - previousLoopMillis > loopInterval) {
      previousLoopMillis = currentMillis;
      wifiAndMqttConnect();
    }else if(currentMillis - previousLoopMillis <0){
      Serial.println("### millisecond ROLLOVER !!!");
      previousLoopMillis=0;
      previousPublishMillis=0;
      previousPingMillis=0;
    }
    if(currentMillis - previousPublishMillis > publishInterval) {
      previousPublishMillis = currentMillis;
      doPublish();
    }
    if(currentMillis - previousPingMillis > pingInterval) {
      previousPingMillis = currentMillis;
      mqtt.ping();
    }
}
