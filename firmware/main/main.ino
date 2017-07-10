

//**************************************************************//
// Pin descriptions
// 0 -> Alarm Reley
// 1 -> TODO  
// 2 -> Buzzer
// 3 -> TODO
// 4 -> MISO
// 5 -> CLK
// 6 .. 14-> TOTO  
// 15 -> Switch
// 16 -> LED WiFi
//**************************************************************//

#include <Wire.h>
#include <ESP8266WiFi.h>
//#include "max31855.h"

#include "com.h"
#include "sensors.h"
#include "pwds.h"

//**************************************************************//
//                 Gloabals declarations                        //
//**************************************************************//
#define DEBUG true
#define BUZZER_ON LOW
#define BUZZER_OFF HIGH
#define SERVER_PORT 23

#define GPIO_LED_WIFI_CONNECTED 16  
#define GPIO_BUZZ_ALARM 2  
#define GPIO_SCLK 14  
#define GPIO_MISO 12  
#define GPIO_CS1 13 
#define GPIO_CS2 0  
#define GPIO_I2C_SDA 4    // select the input pin for the Temerature
#define GPIO_I2C_SCLK 5 // select the input pin for the Temerature



String double2str(double value, int presition=100);
String readTemps(int presition=100);
String readStates();
void checkStates();
String readNrSensors();
void findStart();
void findStop();
void printDebug(String value, bool endl=false);
void printWifiInfo();


//**************************************************************//
//                Gloabals Variables                            //
//**************************************************************//

WiFiServer server(SERVER_PORT);
WifiCom com;
bool flg_alarm = false;
bool flg_finding = false;

ADCSensor st1(A0);
TC74Sensor st2(0x48);
TC74Sensor st3(0x4A);
TC74Sensor st4(0x4B);
TC74Sensor st5(0x4D);
MAX31855 st6(GPIO_SCLK,GPIO_CS1,GPIO_MISO);
MAX31855 st7(GPIO_SCLK,GPIO_CS2,GPIO_MISO);



bool valor=0;
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 1000;           // interval at which to blink (milliseconds)

BasicSensor* sensors[] = {&st1,&st2,&st3,&st4,&st5,&st6,&st7};
int nr_sensors = 7;


//**************************************************************//
//                Setup Method                                  //
//**************************************************************//

void setup() {
  bool wifi_searching = true;
  
  // Prepare logger for debug 
  if (DEBUG){
    Serial.begin(115200);
    delay(100);
  }
  
  // Configure LED_WIFI
  pinMode(GPIO_LED_WIFI_CONNECTED, OUTPUT);
 
  // Connect to the Wifi
  // TODO: Activate Wifi LED
  WiFi.begin(ssid, password);
  uint8_t i = 0;
  printDebug("Connecting");
  while(true){
    if (i < 20){ // Nr of intents 
      printDebug(".");
      if (WiFi.status() == WL_CONNECTED) break;
      digitalWrite(GPIO_LED_WIFI_CONNECTED, HIGH);
      delay(250);
      digitalWrite(GPIO_LED_WIFI_CONNECTED, LOW);
      delay(250);
      i++ ;
    } 
    else{
      digitalWrite(GPIO_LED_WIFI_CONNECTED, HIGH);
      if (wifi_searching){
        printDebug("Can not connect to:");
        printDebug(ssid, true);
        wifi_searching = false;
    
      } 
    }
  }
  digitalWrite(GPIO_LED_WIFI_CONNECTED, LOW);

  printWifiInfo();
  
  //Start Telnet server
  server.begin();
  server.setNoDelay(true);
  
  
  //Configure LED ALARM
  pinMode(GPIO_BUZZ_ALARM, OUTPUT);
  digitalWrite(GPIO_BUZZ_ALARM, BUZZER_OFF);
    
}

//**************************************************************//
//                Main Loop Method                              //
//**************************************************************//

void loop() { 
  bool has_client = false;
  checkStates();

  if (server.hasClient()){
    WiFiClient tmp = server.available();
    com = tmp;
    tmp.stop();
    has_client = true;
  }

  while(has_client){
    if (com.isConnected()){
      checkStates();
      Cmds cmd = com.getCommand();
      if (cmd != NONE)
        switch (cmd){
          case TEMP: 
            com.sendData(readTemps());
            break;
          case CONFIG: 
            com.sendData(String("Not implemented.\n")); 
            break;
          case FIND: 
            findStart();
            break;
          case FOUND:
            findStop();
            break;
          case STATE:
            com.sendData(readStates());
            break;           
          case SENSORS:
            com.sendData(readNrSensors());
            break;
        }
    }
    else has_client = false;
   }
   if (DEBUG){
    Serial.println(st6.readCelsius());
    Serial.println(readTemps());
    unsigned long currentMillis = millis();
    ledblink(valor);
    if (currentMillis - previousMillis >= interval) {
      if (valor!=true){
        valor=true;
      }else if(valor!=false){
        valor=false;
      }
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
    }
  }   
}

//**************************************************************//
//                Functions                                     //
//**************************************************************//

String double2str(double value, int precision){
  String str_buff;
  StringStream buff = StringStream(str_buff);
  buff.print(int(value),DEC);
  buff.print(".");
  unsigned int frac;
  if(value >= 0)
    frac = (value - int(value)) * precision;
  else
    frac = (int(value)- value ) * precision;
  buff.print(frac,DEC) ;
  
  return str_buff;
}
//**************************************************************//

String readTemps(int precision){
  String str_buff;
  StringStream buff = StringStream(str_buff);
  double value;
  for(int i=0; i< nr_sensors; i++){
    value = sensors[i]->getValue();
    buff.print("T");
    buff.print(i+1,DEC);
    buff.print(":");
    buff.print(double2str(value, precision));
    buff.print(";");
    
  }
  buff.print("\n");
  printDebug("readTemps: ");
  printDebug(str_buff);
  return str_buff;
}
//**************************************************************//

String readStates(){
  String str_buff;
  bool alarm = false;
  double value;
  StringStream buff = StringStream(str_buff);
  for(int i=0; i< nr_sensors; i++){
    if (sensors[i]->isOutOfRange()){
      alarm = true;
      buff.print("T");
      buff.print(i+1,DEC);
      buff.print(":ALARM (");
      value = sensors[i]->getValue();
      buff.print(double2str(value));
      buff.print("); ");
    }
    
  }
  if (!alarm)
    buff.print('OK');
  if(flg_finding)
    buff.print("; Finding: True");
  else
    buff.print("; Finding: False");
    
  buff.print("\n");
  printDebug("readStates: ");
  printDebug(str_buff);
  return str_buff;
}
//**************************************************************//
void checkStates(){
  flg_alarm = false;
  for(int i=0; i< nr_sensors; i++){
    if (sensors[i]->isOutOfRange()){
      //digitalWrite(GPIO_LED_ALARM, HIGH);
      flg_alarm = true;
      break;
    } 
  }
   
}
//**************************************************************//
String readNrSensors(){
  String str_buff;
  StringStream buff = StringStream(str_buff);
  buff.print("Nr. Sensors:");
  buff.print(nr_sensors,DEC);
  buff.print("\n");
  
  printDebug("readNrSensors");
  printDebug(str_buff);
  return str_buff;
}

//**************************************************************//
void ledblink(bool statu){
  digitalWrite(GPIO_LED_WIFI_CONNECTED, statu);
  if (statu!=1)
  printDebug("LED off", true);
  else
  printDebug("LED on", true);
}

//**************************************************************//
void findStart(){
  flg_finding = true;
  digitalWrite(GPIO_BUZZ_ALARM, BUZZER_ON);
  printDebug("findStart: Buzzer ON", true);
}
//**************************************************************//
void findStop(){
  flg_finding = false;
  digitalWrite(GPIO_BUZZ_ALARM, BUZZER_OFF);
  printDebug("findStop: Buzzer OFF", true);
}
//**************************************************************//
void printDebug(String value, bool endl){
  if (!DEBUG) return;
  Serial.print(value);
  if (endl)
    Serial.print("\n");
}
//**************************************************************//
void printWifiInfo(){
  printDebug("\n\n");
  printDebug("Connected to: ");
  printDebug(ssid, true);
  
  String str_buff;
  byte mac[6];  
  StringStream buff = StringStream(str_buff);
  WiFi.macAddress(mac);
  buff.print("MAC: ");
  for(int i=0; i<6; i++){
    buff.print(mac[i], HEX);
    if (i!=5)
      buff.print(':'); 
  }
  printDebug(str_buff, true);
  printDebug("IP: ");
  printDebug(WiFi.localIP().toString(), true);
  printDebug("To connect use: 'telnet IP 23'", true);
  
}
//**************************************************************//



