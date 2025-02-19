/*
    Author: Mehmet Furkan KAYA
    Description: Master2
*/

#include <WiFi.h>
#include <LoRa_E22.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>

const char* WiFi_SSID = "<WIFI_NAME>";
const char* WiFi_PASS = "<WIFI_PASSWORD>";
const char* MQTT_SERVER = "<MQTT_SERVER_IP>";
const char* MQTT_CLIENT_ID = "status";
const char* MQTT_PUB_TOPIC = "status_publish";


WiFiClient mqttInstance;
PubSubClient mqtt(mqttInstance);


#define M0 32
#define M1 33
#define RX 27
#define TX 35
#define address 3
#define channel 36


typedef struct {
  char command[50];
} STATUS_DATA;

STATUS_DATA statusData;

typedef struct {
  float t;
  int l;
  float la;
  int c;
  int br;
  int bt;
} SensorData;

SensorData receivedSensorData;


bool LORA_AVAILABLE = false;

float t = 0.0, la = 0.0, bt = 0.0;
bool l = 0, c = 0, br = 0;

HardwareSerial mySerial(1);
LoRa_E22 e22(TX, RX, &mySerial, UART_BPS_RATE_9600);


void printInfo(void);
void loraListen(void);
void wifiConnect(void);
void mqttConnect(void);
void wifiMqttCheck(void);
void loraE22Settings(void);
String parseDataToJson(String input);
void publishMessage(const char* topic, const char* message);


void setup() {
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);

  Serial.begin(9600);
  e22.begin();

  loraE22Settings();

  digitalWrite(M0, LOW);
  digitalWrite(M1, LOW);

  delay(500);

  wifiConnect();
  mqtt.setServer(MQTT_SERVER, 1883);
  mqttConnect();
}

void loop() {
  wifiMqttCheck();

  loraListen();
  if (LORA_AVAILABLE == true) {
    LORA_AVAILABLE = false;
  }
}

// LoRa Listen
void loraListen() {
  while (e22.available() > 0) {
    ResponseStructContainer rsc = e22.receiveMessage(sizeof(STATUS_DATA));
    statusData = *(STATUS_DATA*) rsc.data;

    String data1 = parseDataToJson((char*)rsc.data);

    if (!data1.isEmpty()) {
      publishMessage(MQTT_PUB_TOPIC, data1.c_str());
    }

    rsc.close();
    LORA_AVAILABLE = true;
  }
}

// Parse Data To JSON
String parseDataToJson(String input) {
    int startIndex = 0;
    bool hasNonZeroValue = false;

    while (startIndex < input.length()) {
        int colonIndex = input.indexOf(':', startIndex);
        if (colonIndex == -1) break;
        
        String key = input.substring(startIndex, colonIndex);
        int endIndex = input.indexOf('|', colonIndex);
        if (endIndex == -1) endIndex = input.length();
        
        String value = input.substring(colonIndex + 1, endIndex);

        if (key == "t")  t  = value.toFloat();
        else if (key == "l")  l  = value.toInt();
        else if (key == "la") la = value.toFloat();
        else if (key == "c")  c  = value.toInt();
        else if (key == "br") br = value.toInt();
        else if (key == "bt") bt = value.toFloat();

        // Eğer herhangi bir değer 0'dan farklıysa bayrağı true yap
        if (t != 0.0 || l != 0 || la != 0.0 || c != 0 || br != 0 || bt != 0.0) {
            hasNonZeroValue = true;
        }

        startIndex = endIndex + 1;
    }

    if (!hasNonZeroValue) {
        return "";
    }

    String json = "{";
    json += "\"t\":" + String(t, 2) + ",";
    json += "\"l\":" + String(l) + ",";
    json += "\"la\":" + String(la, 2) + ",";
    json += "\"c\":" + String(c) + ",";
    json += "\"br\":" + String(br) + ",";
    json += "\"bt\":" + String(bt, 2);
    json += "}";

    return json;
}

// MQTT Publish
void publishMessage(const char* topic, const char* message) {
  if (!mqtt.publish(topic, message)) {
    Serial.println("Message could not be published!");
  } else {
    Serial.print("Message successfully published: ");
    printInfo();
    Serial.println(message);
  }
}

// Check WiFi and MQTT Connection
void wifiMqttCheck(void) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost, reconnecting...");
    wifiConnect();
  }

  if (!mqtt.connected()) {
    Serial.println("MQTT connection lost, reconnecting...");
    mqttConnect();
  }

  mqtt.loop();
}

// WiFi
void wifiConnect(void) {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WiFi_SSID, WiFi_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// MQTT
void mqttConnect(){
   while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

// LoRa E22 Settings
void loraE22Settings() {
  digitalWrite(M0, LOW);
  digitalWrite(M1, HIGH);

  ResponseStructContainer c;
  c = e22.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;

  configuration.ADDL = lowByte(address);
  configuration.ADDH = highByte(address);
  configuration.CHAN = channel;
  configuration.NETID = 0x00;

  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.OPTION.subPacketSetting = SPS_240_00;
  configuration.OPTION.transmissionPower = POWER_22;

  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.uartParity = MODE_00_8N1;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
  configuration.TRANSMISSION_MODE.WORTransceiverControl = WOR_TRANSMITTER;
  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_DISABLED;
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  configuration.TRANSMISSION_MODE.enableRepeater = REPEATER_DISABLED;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;

  ResponseStatus rs = e22.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  c.close();
}

// Print Info
void printInfo() {
  Serial.println("\n************************************");
  Serial.print("RECEIVE DATA: " + String(statusData.command));
  Serial.println("\n************************************");
}
