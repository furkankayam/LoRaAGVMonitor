/*
    Author: Mehmet Furkan KAYA
    Description: Slave2
*/

#include <LoRa_E22.h>
#include <HardwareSerial.h>


#define M0 32
#define M1 33
#define RX 27
#define TX 35
#define RXP2 16
#define TXP2 17
#define address 4
#define channel 36
#define sendToAddress 3


HardwareSerial mySerial(1);
LoRa_E22 e22(TX, RX, &mySerial, UART_BPS_RATE_9600);


typedef struct {
  char command[50];
} STATUS_DATA;

STATUS_DATA statusData;


void loraSend(void);
void printInfo(void);
void loraE22Settings(void);


void setup() {
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);

  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXP2, TXP2);
  e22.begin();

  loraE22Settings();

  digitalWrite(M0, LOW);
  digitalWrite(M1, LOW);

  delay(500);
}

void loop() {
  if (Serial2.available() > 0) {
    String uartData = Serial2.readStringUntil('\n');

    strncpy(statusData.command, uartData.c_str(), sizeof(statusData.command) - 1);
    statusData.command[sizeof(statusData.command) - 1] = '\0';

    if (statusData.command == 0) {
      return;
    }

    printInfo();
    loraSend();
  }
}

// LoRa Send
void loraSend() {
  ResponseStatus rs = e22.sendFixedMessage(highByte(sendToAddress), lowByte(sendToAddress), channel, &statusData, sizeof(STATUS_DATA));
}

// LoRa E22 Settings
void loraE22Settings() {

  digitalWrite(M0, LOW);
  digitalWrite(M1, HIGH);

  ResponseStructContainer c;
  c = e22.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;

  // SETTINGS THAT CAN CHANGE
  configuration.ADDL = lowByte(address);
  configuration.ADDH = highByte(address);
  configuration.CHAN = channel;
  configuration.NETID = 0x00;

  // OPTIONAL SETTINGS
  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.OPTION.subPacketSetting = SPS_240_00;
  configuration.OPTION.transmissionPower = POWER_22;

  // ADVANCED SETTINGS
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.uartParity = MODE_00_8N1;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
  configuration.TRANSMISSION_MODE.WORTransceiverControl = WOR_RECEIVER;
  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_DISABLED;
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  configuration.TRANSMISSION_MODE.enableRepeater = REPEATER_DISABLED;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;

  // Save and Store SETTINGS
  ResponseStatus rs = e22.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  c.close();

}

// Print Info
void printInfo(){
  Serial.println("\n************************************");
  Serial.print("SEND DATA: ");
  Serial.println(statusData.command);
  Serial.println("\n************************************");
}
