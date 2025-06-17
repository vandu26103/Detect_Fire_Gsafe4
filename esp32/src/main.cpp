#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <PubSubClient.h>
#include "wifiConfig.h"
#include <SoftwareSerial.h>
#define OTA_CURRENT_VERSION "24.02.11.001"

#define led_wifi 2
#define RXD_1 5
#define TXD_1 4
#define U2_TXD 17
#define U2_RXD 16
#define Sim_Status 32
#define PWRKEY 13
const char* ota_uri = "https://dev-api.lctech.vn/firmware/esp32/OTA";
const char* mqttServer = "dev-api.lctech.vn";
const int mqttPort = 1885;
const char* mqttUser = "lctech_dev";
const char* mqttPassword = "lctech_DEV@123";
const char* topicSever = "mmm/gsafe4/data";
//const char* topicSever = "mmm/g4/data";
char txt_subscribe[] = "mmm/000000000000/cmd";
int error_mqtt;
char ESPID[] = "000000000000";// Auto Read Mac Address make Device ID

SoftwareSerial ms51Serial(RXD_1, TXD_1);
//SoftwareSerial gsmSerial(U2_RXD, U2_TXD);
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

#define  numBytes (20)   // amount, ph, ec, tu, te, 
byte __attribute__((aligned(4))) dataload[numBytes]; // 
uint32_t amount, ph, ec, tu, temp;

#define TIME_LOOP 5000
#define OTA_LOOP 10000
uint32_t loop_sever;
uint32_t loop_ota;

const String root_ca = \
    "-----BEGIN CERTIFICATE-----\n" \
    "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\n" \
    "-----END CERTIFICATE-----\n";

void macRead();
void updateData();
void mqttConnect();
//void Wificonnect();
void ota_update();
void sendToSever();
void callback(char* topic, byte* payload, unsigned int length);
void checkSIMStatus();
void checkNetworkStatus();
void setup_mqtt();
void setup();

char du_lieu[20]= {
  0x0f,0x11,0x21,0x06,
  0x0f,0x11,0x21,0x06,
  0x0f,0x11,0x21,0x06,
  0x0f,0x11,0x21,0x06,
  0x0f,0x11,0x21,0x06,
};

char h2c(char c){  
  return "0123456789ABCDEF"[0x0F & (unsigned char)c];
}
void macRead(){
  uint8_t i;
  byte macAdd[6];
  WiFi.macAddress(macAdd);
  for(i=0;i<6;i++)
  { 
    ESPID[i*2]= h2c(macAdd[i]>>4);
    ESPID[i*2+1]= h2c(macAdd[i]);
    }     
  for(i=0;i<12;i++) txt_subscribe[i+4] = ESPID[i];
  Serial.println("id: "+String(ESPID));
}
void setup_mqtt(){
  espClient.setCACert(root_ca.c_str());
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(callback);
}

void setup()
{
    Serial.begin(115200);
    ms51Serial.begin(115200);
    //gsmSerial.begin(115200);
    
    pinMode(PWRKEY, OUTPUT);
    pinMode(Sim_Status, INPUT);
    pinMode(U2_RXD, INPUT);
    pinMode(U2_TXD, INPUT);
    digitalWrite(PWRKEY, 0);
    macRead();
    delay(500);
    ms51Serial.print("ESPID:");
    ms51Serial.println(String(ESPID));
    Serial.println("ESPID:" + String(ESPID));
    delay(100);
    wifiConfig.begin();
    digitalWrite(led_wifi, HIGH);
    if(digitalRead(Sim_Status)==1){
      digitalWrite(PWRKEY, 1);
      delay(1000);
      digitalWrite(PWRKEY, 0);
    }
  // checkSIMStatus();
  // checkNetworkStatus();
    /*while (gsmSerial.available()) {
      Serial.write(gsmSerial.read());
    }
    delay(1000);*/
    setup_mqtt();
    delay(10);
    mqtt.setServer(mqttServer,mqttPort);
    mqtt.setCallback(callback);
    delay(10);
    Serial.print("firmware: ");
    Serial.println(OTA_CURRENT_VERSION);
    
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("-----------------------");
}
 
void mqttConnect()
{
  if(mqtt.connected())return;
  while (!mqtt.connected()) {
    Serial.println("Connecting to MQTT...");

    if (mqtt.connect(ESPID, mqttUser, mqttPassword )) {
      Serial.println("connected");  
      Serial.print(" subscribe to: ");
      Serial.println(txt_subscribe);
      mqtt.subscribe(txt_subscribe);
    } else {
      Serial.print("failed with state ");
      Serial.println(mqtt.state());
      error_mqtt++;
      if(error_mqtt>10){
        error_mqtt=0;
        ESP.restart();
      }
    }
  }
}
void ota_update(){
  if(millis() - loop_ota > TIME_LOOP){
    loop_ota = millis();
  }else{
    return;
  }
  if ((WiFi.status() == WL_CONNECTED)) {
    Serial.println("start OTA update");
    WiFiClientSecure client;
    client.setCACert(root_ca.c_str());
    // t_httpUpdate_return ret = httpUpdate.update(client, ota_uri);
    t_httpUpdate_return ret = httpUpdate.update(client, ota_uri, OTA_CURRENT_VERSION);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }
}
void send_data(String dta){
   if(!mqtt.connected()) return;
    if(mqtt.publish(topicSever,dta.c_str(),dta.length())){

      Serial.println("Update\"" + dta + "\"to MQTT OK!");
      return;
    }
    Serial.println("Update MQTT Fail!");
}

uint32_t previousMillis_timeout;
bool time_out(unsigned long timeout_interval){
  unsigned long currentMillis = millis();
	if(currentMillis - previousMillis_timeout >= timeout_interval) 
	{
		previousMillis_timeout = currentMillis;
		return(true);
	}
	return(false);
}
long rssi;
String data1;  //data receive from ms51
void getData(){
  previousMillis_timeout = millis();
  String rcv;
  while(1){
    rcv = ms51Serial.readStringUntil('\n');
    if(rcv.length() >=50 )
		{
      data1 = rcv;
      return;
		}
		if(time_out(5000))
		{
			return ;
		}
  }
}

int rssiInit;
void getrssiInit(){
  previousMillis_timeout = millis();
  String rcv;
  char *ptr;
  while(1){
    rcv = ms51Serial.readStringUntil('\n');
    if(rcv.indexOf(F("RSSI:")) != -1){		
      Serial.println(rcv);
      int startIndex = rcv.indexOf(F("RSSI:")) + 5;
      rcv = rcv.substring(startIndex);
      rssiInit = rcv.toInt();
      return;
    }
		if(time_out(5000))
		{
			return ;
		}
  }
}
void SIM_PowerOn(){
  digitalWrite(PWRKEY, HIGH);
	delay(1000);
	digitalWrite(PWRKEY, LOW);
}
void SIM_PowerOff(){
  digitalWrite(PWRKEY, HIGH);
  delay(1000);
	digitalWrite(PWRKEY, LOW);
	delay(1000);
}
void SIM_Reset(){
  SIM_PowerOff();
  delay(1000);
  SIM_PowerOn();
}
String data; ///data publish to server
void loop(){
  wifiConfig.run();
  /* ESP - MS51 */
  rssi = abs(WiFi.RSSI());
  Serial.println("WF: " + String(rssi));
  getData();
  //getrssiInit(); 
  delay(500);
  ms51Serial.println("RSSIWF:" + String(rssi));
  /* PROCESS DATA*/
  if(data1.indexOf(F("Data:")) != -1){		
    Serial.println(data1);
    int startIndex = data1.indexOf(F("Data:")) + 5;
    String data_temp = data1.substring(startIndex);
    Serial.println("data_temp: "+ data_temp);
    int sum = 0;
    uint8_t checksum = (uint8_t)data_temp[69]; 
    for(int i=0;i<69;i++){
      sum = sum + (uint8_t)data_temp[i];
      
    }
    if((uint8_t)sum == checksum){
      data = data_temp.substring(0,69);
    }
	}
  //Serial.println("sum: " + String((uint8_t)sum));
  //Serial.println("crc: " + String((uint8_t)data_temp[69]));
  char Alarm_status = data.charAt(32);
  Serial.print("Alarm status:");
  Serial.println(Alarm_status);

  Serial.println("data pub: "+ data);
  //Serial.println("rssiInit: "+ String(rssiInit));
  /*if(rssiInit>100){//SIM POWER off
    SIM_Reset();
  }*/
  /* SEND TO SERVER */
  if(Alarm_status == '0'){
    if((WiFi.status() == WL_CONNECTED))
    {
      mqttConnect(); 
      if(millis()-loop_sever>10000){ //alarm
        
        loop_sever = millis();
        send_data(data);
      }
    }
  }
  else {
    if((WiFi.status() == WL_CONNECTED))
    {
      mqttConnect(); 
      if(millis()-loop_sever>30000){//normal
        
        loop_sever = millis();
        send_data(data);
      }
    }
  }
  if(digitalRead(Sim_Status)==1){
    SIM_PowerOn();
  }
  mqtt.loop();
  //delay(1000); 
}