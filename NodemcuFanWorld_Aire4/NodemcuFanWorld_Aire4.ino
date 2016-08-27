/*
 * IRremote: IRsendDemo - demonstrates sending IR codes with IRsend
 * An IR LED must be connected to Arduino PWM pin 3.
 * segundo arduino 
 *cuando recibe entrada digital4 encience el aire en IR3
 modificar en IRremote.h para poder enviar mensajes largos
 * #define RAWBUF 500 // Length of raw duration buffer
 * /*
#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0
#define D4 2 // Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO 
#define D7 13 // SPI Bus MOSI
#define D8 15 // SPI Bus SS (CS)
#define D9 3 // RX0 (Serial console)
#define D10 1 // TX0 (Serial console)
//const char* ssid = "MOVISTAR_C1EB";
//const char* password = "8uqH2jjiR6E2PsSfXmvt";

  */

#include <ESP8266WiFi.h>
#include <IRremoteESP8266.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <TimeLib.h> 
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h> //to store default data
////////////////////////////////////////////////////////
// The type of data that we want to extract from the page
#define EMON_SIZE 3
struct EMON_DATA {
  int feedid[EMON_SIZE];
  int feedvalue[EMON_SIZE];
  String feedname[EMON_SIZE];
};
EMON_DATA emonData;


/////////////////////////////////////////
WiFiServer server ( 80 );
WiFiManager wifiManager;
WiFiClient client_emon;
WiFiClient client_json;
IPAddress server_emon(163,117,157,189); //ordenador jaen-espectros2
String apikey_emon="9bbe8aedfc502644f616dd98e3b4f737";
unsigned long lastConnectionTime_emon = 0;             // last time you connected to the server, in milliseconds
boolean lastConnected_emon = false;                    // state of the connection last time through the main loop
const unsigned long postingInterval_emon = 28000; // delay between updates, in milliseconds

IRsend irsend(4);//an IR led is connected to GPIO4 (pin D2 on NodeMCU)
int j=0;
int manual=1;  //para control manual o automático
float umbral_PAC=1000; //umbral de potencia para arranque del aire acondicionado
const int ledPin =  2;      // the number of the LED pin
int aire_onoff = LOW;

unsigned long t_ant_print=0;
int t_print=3000;

//NTP
const int timeZone = 0;     // Central European Time
unsigned int localPort = 8888;      // local port to listen for UDP packets
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServer; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
//IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
time_t prevDisplay = 0; // when the digital clock was displayed
//NTP

const int  feedID          =    130137; // datos Xively
const int  streamCount     =      4; // Number of data streams to get
WiFiClient client_xiv;
char server_xiv[] = "api.xively.com"; 
int streamData[streamCount];    // change float to long if needed for your data

unsigned long lastConnectionTime_xiv = 0;             // last time you connected to the server, in milliseconds
boolean lastConnected_xiv = false;                    // state of the connection last time through the main loop
const unsigned long postingInterval_xiv = 60000; // delay between updates, in milliseconds
float Pot_INV_auto; //potencia del inversor conectado a red Altavista 107 
float Pot_INV_red; //potencia del inversor conectado a red Altavista 107
float PAC_casa; //potencia casa Altavista 107    
String cadena;
int emon_var_read = -1;
unsigned long readingEMONCMSinterval=90000;
unsigned long lastReadEMONCMSTime;
int emon_var = 9999;
int emon_var_id = 1653;//Pot_INV_auto autoconsumo
IPAddress ip; 
int i=0;

void setup()
{
  Serial.begin(9600);
  delay (1000);
  pinMode(ledPin, OUTPUT);
  Serial.println();
  Serial.println("Control Aire Fan World");
  irsend.begin();
    wifiManager.setBreakAfterConfig(true);
  //reset settings - for testing
  //wifiManager.resetSettings();
  if (!wifiManager.autoConnect("NodeMCU", "A12345678")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  ip=WiFi.localIP();
  /////////////////////////////////////////server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  digitalWrite(ledPin, HIGH);
  Serial.println("Waiting 30 seconds...");
  delay(10000);
  Serial.println("Inicializando variables");
  lastReadEMONCMSTime = millis();
    for (i=0;i<3;i++){
    read_emoncms_variables();
    envio_emon();
    Xively2();
    }
  Serial.println("Iniciado...");
  delay(5000);
}



void loop() {
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
     // digitalClockDisplay();  
    }
  }
  checkinternet();
  imprimirdatos(); 
  //2. client Xively
  if( (millis() - lastConnectionTime_xiv > postingInterval_xiv) ) {
     Xively2();
    lastConnectionTime_xiv=millis();
    ///control del aire
      if (!manual && Pot_INV_auto>umbral_PAC){
        digitalWrite(ledPin, LOW);
        ON();
        aire_onoff = HIGH;
      }
      if (!manual && Pot_INV_auto<umbral_PAC){
        digitalWrite(ledPin, HIGH);
        OFF();
        aire_onoff = LOW;
      }
      
    }
//3. read emoncms variables////////////////////////////////////////////////////////////////////////////////////
  read_emoncms_variables();
  //4. wifi reconnect/////////////////////////////////////////////////////////////////////////////////
  if (WiFi.status() != WL_CONNECTED){
      WIFI_Connect();
  } 
  
 
} ////END LOOP

void read_emoncms_variables(){
  //3. read emoncms variables////////////////////////////////////////////////////////////////////////////////////
  emonData.feedname[0]="Umbral_PAC"; //umbral PAC aire  //poner las feed id de emoncms:spectrum Altavista 107 que se quieran leer
  emonData.feedname[1]="manual"; //control_manual
  emonData.feedname[2]="PAC_casa"; //PAC_casa
  emonData.feedid[0]=2083; //umbral PAC aire  //poner las feed id de emoncms:spectrum Altavista 107 que se quieran leer
  emonData.feedid[1]=2084; //control_manual
  emonData.feedid[2]=1653; //PAC_casa
  
       if (millis() - lastReadEMONCMSTime > readingEMONCMSinterval)  {
          lastReadEMONCMSTime = millis();
          int lectura[EMON_SIZE];
          for(j=0;j<EMON_SIZE;j++){
            lectura[j]=readEMONCMS(j,emonData.feedid[j]);//read emoncms feedid
          }

          if (lectura[0]&& emonData.feedvalue[0]>500){umbral_PAC=emonData.feedvalue[0];}
          if (lectura[1]&& emonData.feedvalue[1]<2){
            //si estaba en automático y se pone en manual, parar el aire
            if (!manual && emonData.feedvalue[1]==1){digitalWrite(ledPin, HIGH);OFF();aire_onoff = LOW;}
            manual=emonData.feedvalue[1];
            }
          if (lectura[2]){PAC_casa=emonData.feedvalue[2];}
       }
  }

void WIFI_Connect() // por si acaso se desconecta el router, volver a conectar
{
    delay(3000);
    wifiManager.setBreakAfterConfig(true);
    if (!wifiManager.autoConnect("NodeMCU", "A12345678")) {
      Serial.println("failed to connect, we should reset as see if it connects");
      delay(3000);
      ESP.reset();
      delay(5000);
    }
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  /////////////////////////////////////////server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
} 

void ON(){
  int i=0;
Serial.println("enviando ON");
//16 grados verano
  // unsigned int raw_verano[228] = {3050,1650,550,1000,550,1000,550,300,550,300,550,350,500,1000,550,300,550,350,500,1000,550,1000,550,300,550,1000,550,300,550,300,550,1000,500,1050,550,300,550,1000,500,1050,500,350,500,350,500,1050,500,350,500,350,550,1000,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,350,550,300,550,1000,500,350,550,300,500,1050,500,350,500,350,550,1000,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,1050,500,350,500,350,550,300,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,550,300,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,450,400,500,350,500,350,550,350,500,350,500,350,500,350,500,350,550,300,450,400,450,1100,500,1050,500,350,500,1050,500,1000,550,1000,500,350,550,350,500,};
  unsigned int raw_verano[228] = {3100,1650,550,1050,500,1050,450,400,450,400,500,350,500,1050,450,400,500,350,500,1050,500,1000,500,400,450,1100,450,400,450,400,500,1000,500,1050,500,400,450,1100,450,1050,500,400,450,400,450,1050,500,400,450,400,500,1050,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,350,500,350,500,400,450,400,450,400,500,1050,450,1050,500,1050,500,1050,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,1050,500,1050,500,400,450,1050,500,400,450,400,450,1050,500,400,450,};
  //30 grados invierno
   unsigned int raw_invierno[228] = {3050,1650,550,1000,550,1000,550,300,550,300,550,350,500,1000,550,300,550,350,500,1000,550,1000,550,300,550,1000,550,300,550,300,550,1000,500,1050,550,300,550,1000,500,1050,500,350,500,350,500,1050,500,350,500,350,550,1000,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,350,550,300,550,1000,500,350,550,300,500,1050,500,350,500,350,550,1000,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,1050,500,350,500,350,550,300,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,550,300,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,450,400,500,350,500,350,550,350,500,350,500,350,500,350,500,350,550,300,450,400,450,1100,500,1050,500,350,500,1050,500,1000,550,1000,500,350,550,350,500,};
//irsend.sendRaw(raw,228,38);
 for(i=0;i<20; i++)
    {  
      if (month()>5 && month()<10){ 
        Serial.println("enviando ON verano 16 grados");
        irsend.sendRaw(raw_verano,sizeof(raw_verano)/sizeof(raw_verano[0]),38);
        delay(100);
      }else{
        Serial.println("enviando ON invierno 31 grados");
        irsend.sendRaw(raw_invierno,sizeof(raw_invierno)/sizeof(raw_invierno[0]),38);
        delay(100);
        }
    }
     envio_emon();
}

void ON_verano(){
  int i=0;
Serial.println("enviando ON"); 
unsigned int raw[228] = {3050,1650,550,1000,550,1000,550,300,550,300,550,350,500,1000,550,300,550,350,500,1000,550,1000,550,300,550,1000,550,300,550,300,550,1000,500,1050,550,300,550,1000,500,1050,500,350,500,350,500,1050,500,350,500,350,550,1000,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,350,550,300,550,1000,500,350,550,300,500,1050,500,350,500,350,550,1000,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,1050,500,350,500,350,550,300,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,550,300,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,450,400,500,350,500,350,550,350,500,350,500,350,500,350,500,350,550,300,450,400,450,1100,500,1050,500,350,500,1050,500,1000,550,1000,500,350,550,350,500,};
//irsend.sendRaw(raw,228,38);
 for(i=0;i<20; i++)
    {  
    irsend.sendRaw(raw,sizeof(raw)/sizeof(raw[0]),38);
     delay(100);
    }
}

void ON_invierno(){
Serial.println("enviando ON"); 
unsigned int raw[228] = {3050,1650,550,1000,550,1000,550,300,550,300,550,350,500,1000,550,300,550,350,500,1000,550,1000,550,300,550,1000,550,300,550,300,550,1000,500,1050,550,300,550,1000,500,1050,500,350,500,350,500,1050,500,350,500,350,550,1000,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,350,550,300,550,1000,500,350,550,300,500,1050,500,350,500,350,550,1000,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,1050,500,350,500,350,550,300,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,550,300,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,450,400,500,350,500,350,550,350,500,350,500,350,500,350,500,350,550,300,450,400,450,1100,500,1050,500,350,500,1050,500,1000,550,1000,500,350,550,350,500,};
//irsend.sendRaw(raw,228,38);
irsend.sendRaw(raw,sizeof(raw)/sizeof(raw[0]),38);
}
void OFF(){
Serial.println("enviando OFF"); 
int i=0;
unsigned int raw[228] = {3050,1700,500,1050,500,1050,500,350,500,350,500,400,450,1050,500,350,500,400,500,1000,500,1050,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,1050,500,1050,500,350,500,350,500,1050,500,350,500,400,450,1050,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,1050,500,400,450,400,500,1050,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,1050,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,1050,500,1050,500,1050,500,350,500,1050,500,1050,450,400,500,350,500,};
for(i=0;i<20; i++)
    { 
  //irsend.sendRaw(raw,228,38);
  irsend.sendRaw(raw,sizeof(raw)/sizeof(raw[0]),38);
 delay(100);
    }
    envio_emon();
}

void swing(){
unsigned int raw[228] = {3050,1700,500,1050,500,1050,500,350,500,350,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,1050,500,1050,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,1050,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,1050,450,1050,500,1050,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,1050,500,1050,450,400,500,350,500,1050,450,1050,500,1050,500,350,500,};
//irsend.sendRaw(raw,228,38);
irsend.sendRaw(raw,sizeof(raw)/sizeof(raw[0]),38);
}

void temp_up(){
 unsigned int raw[228] = {3050,1650,550,1000,550,1000,550,300,550,300,550,300,550,1000,550,300,550,350,500,1000,550,1000,550,300,550,1000,550,300,550,300,550,1000,550,1000,550,300,550,1000,550,1000,550,300,550,300,550,1000,550,300,550,300,550,1000,550,300,550,350,500,350,500,350,550,300,550,300,550,350,500,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,1000,550,300,550,300,550,1000,550,300,550,300,550,1000,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,300,550,350,500,350,550,300,550,300,550,1000,550,1000,500,1050,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,550,300,600,300,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,1000,550,300,550,350,500,1000,550,1000,550,1000,550,300,550,};
//irsend.sendRaw(raw,228,38);
irsend.sendRaw(raw,sizeof(raw)/sizeof(raw[0]),38);
}
/*
 void Aire_ON() {
 //31 grados invierno
unsigned int raw[228] = {3050,1700,550,1050,450,1050,500,400,450,400,450,400,500,1050,450,400,500,350,500,1050,500,1050,500,350,500,1050,500,350,500,350,500,1050,500,1050,500,350,500,1050,450,1100,450,400,450,400,500,1050,450,400,500,350,500,1050,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,500,350,500,1050,500,350,500,350,500,1050,500,350,500,400,450,1050,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,450,450,400,450,400,450,400,450,400,450,400,500,400,450,350,500,400,450,400,450,400,450,400,500,350,500,400,450,400,450,400,450,400,450,450,450,350,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,450,450,400,450,400,450,400,450,400,450,450,400,450,450,350,500,350,500,400,450,400,450,450,450,400,450,400,450,400,450,400,450,400,450,450,450,400,450,1050,500,400,450,1100,450,1050,450,1100,450,400,450,450,450,};
unsigned int raw[228] = {3050,1650,550,1050,500,1050,500,350,500,350,500,400,450,1050,500,400,450,400,450,1100,450,1050,500,400,450,1050,500,400,450,400,450,1050,500,1050,500,400,450,1050,500,1050,500,350,500,400,450,1050,500,400,450,400,450,1050,500,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,400,450,400,450,1050,500,400,450,400,450,1050,500,400,450,400,500,1050,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,1050,500,400,450,1050,500,1050,500,1050,500,350,500,400,450,};
unsigned int raw[228] = {3000,1850,350,1200,350,1200,350,500,350,500,350,500,400,1150,350,500,400,450,400,1150,400,1150,350,500,400,1150,350,500,400,450,400,1150,400,1150,400,450,400,1150,350,1200,350,500,350,500,400,1150,350,500,400,450,400,1150,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,400,450,400,1150,350,500,400,450,400,1150,400,450,400,500,350,1150,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,1150,400,450,400,1150,350,1200,350,1200,350,500,350,500,400,};
irsend.sendRaw(raw,228,38);
 }
  void Aire_ON() {
 16 grados verano
unsigned int raw[228] = {3050,1700,500,1050,500,1050,500,350,500,350,500,350,500,1050,500,350,500,350,500,1050,500,1050,500,350,500,1050,500,350,500,350,500,1050,500,1050,500,350,500,1050,500,1000,550,300,550,350,500,1000,550,300,550,350,500,1050,500,350,500,350,500,350,500,350,500,350,500,350,550,350,500,350,500,350,500,350,500,350,500,350,550,300,550,350,500,350,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,350,500,1050,500,1050,500,350,500,350,500,350,550,300,550,350,500,350,500,1000,550,1000,500,1050,500,1050,500,350,500,350,500,350,500,350,550,350,500,350,500,350,500,350,500,350,500,350,500,350,550,350,500,350,500,350,500,350,500,350,500,350,500,350,550,350,500,350,500,350,500,350,500,350,500,350,500,400,500,350,500,350,500,350,500,350,500,350,500,350,550,350,500,350,500,350,500,350,500,350,500,350,500,350,500,400,500,350,500,350,500,350,500,350,500,350,500,1050,500,1050,500,350,500,1050,500,350,500,350,500,1050,500,350,500,};
unsigned int raw[228] = {3100,1650,550,1050,500,1050,450,400,450,400,500,350,500,1050,450,400,500,350,500,1050,500,1000,500,400,450,1100,450,400,450,400,500,1000,500,1050,500,400,450,1100,450,1050,500,400,450,400,450,1050,500,400,450,400,500,1050,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,350,500,350,500,400,450,400,450,400,500,1050,450,1050,500,1050,500,1050,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,1050,500,1050,500,400,450,1050,500,400,450,400,450,1050,500,400,450,};
irsend.sendRaw(raw,228,38);
 }
 

 void Aire_ON() {
 // unsigned int raw[228]= {3050,1650,550,1000,550,1000,550,300,550,300,550,350,500,1000,550,300,550,350,500,1000,550,1000,550,300,550,1000,550,300,550,300,550,1000,500,1050,550,300,550,1000,500,1050,500,350,500,350,500,1050,500,350,500,350,550,1000,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,350,550,300,550,1000,500,350,550,300,500,1050,500,350,500,350,550,1000,500,350,550,300,550,300,550,300,550,350,500,350,500,350,500,1050,500,350,500,350,550,300,550,300,550,300,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,550,300,550,300,500,350,550,300,550,350,500,350,500,350,550,300,550,300,500,350,550,350,500,350,500,350,500,350,500,350,550,300,500,350,550,300,550,350,500,350,500,350,450,400,500,350,500,350,550,350,500,350,500,350,500,350,500,350,550,300,450,400,450,1100,500,1050,500,350,500,1050,500,1000,550,1000,500,350,550,350,500,};
//irsend.sendRaw(raw,228,38);
  delay(1000);
 }
 
void Aire_OFF() {
 
  // unsigned int raw[228] = {3050,1700,500,1050,500,1050,500,350,500,350,500,400,450,1050,500,350,500,400,500,1000,500,1050,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,1050,500,1050,500,350,500,350,500,1050,500,350,500,400,450,1050,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,1050,500,400,450,400,500,1050,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,1050,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,1050,500,1050,500,1050,500,350,500,1050,500,1050,450,400,500,350,500,};
//irsend.sendRaw(raw,228,38);
  delay(100);
}
/*
void swing(){
  unsigned int raw[228] = {3050,1700,500,1050,500,1050,500,350,500,350,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,1050,500,350,500,400,450,1050,500,1050,500,350,500,1050,500,1050,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,350,500,1050,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,1050,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,1050,450,1050,500,1050,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,400,450,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,350,500,350,500,350,500,400,450,400,500,350,500,1050,500,1050,450,400,500,350,500,1050,450,1050,500,1050,500,350,500,};
irsend.sendRaw(raw,228,38);
}
*/

/*-------- NTP code ----------*/

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


time_t getNtpTime()
{
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServer); 
  
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

//================EMONCMS====================
//envio de datos a emoncms
void sendData_emon(void){
  Serial.println(F("comienzo de envio a emoncms"));
  if (client_emon.connect(server_emon, 80)) {
    Serial.println(F("Sending emoncms..."));
    client_emon.print(F("GET /emoncms_spectrum/input/post.json?apikey="));
    client_emon.print(apikey_emon);
    client_emon.print(F("&node=107&json={aire_onoff"));
    client_emon.print(F(":"));
    int valor;
    if (aire_onoff){valor=1;}else{valor=0;}
    client_emon.print(valor);
    client_emon.print(F(",ip_3:")); 
    valor=ip[3];
    client_emon.print(valor);  
    /*client_emon.print(F(",Lux:")); 
    valor=lux;
    client_emon.print(valor);  
    client_emon.print(F(",PAC:")); client_emon.print(mydata.PAC);
    client_emon.print(F(",VDC:")); client_emon.print(mydata.VDC);
    client_emon.print(F(",IDC:")); valor=(float)mydata.IDC/100; client_emon.print(valor);
    client_emon.print(F(",VDCbus:")); client_emon.print(mydata.VDCbus);
    client_emon.print(F(",VAC:")); client_emon.print(mydata.VAC);
    client_emon.print(F(",IAC:")); valor=(float)mydata.IAC/100;client_emon.print(valor);
    client_emon.print(F(",f:")); valor=(float)mydata.f/100;client_emon.print(valor);
    client_emon.print(F(",cosf:")); valor=(float)mydata.cosf/1000;client_emon.print(valor);
    client_emon.print(F(",Etotal:")); client_emon.print(mydata.Etotal);
    client_emon.print(F(",Horas:")); client_emon.print(mydata.Horas);
    client_emon.print(F(",Nconex:")); client_emon.print(mydata.Nconex);
    client_emon.print(F(",A3:")); client_emon.print(mydata.A3);
    client_emon.print(F(",A4:")); client_emon.print(mydata.A4);
    client_emon.print(F(",A5:")); client_emon.print(mydata.A5);
    client_emon.print(F(",A6:")); client_emon.print(mydata.A6);
    client_emon.print(F(",A7:")); client_emon.print(mydata.A7);
    client_emon.print(F(",A8:")); client_emon.print(mydata.A8);
    */
    client_emon.println(F("} HTTP/1.1"));
    //client_emon.println(" HTTP/1.1");
    client_emon.println(F("Host: 163.117.157.189"));
    client_emon.println(F("User-Agent: Arduino-ethernet"));
    client_emon.println(F("Connection: close"));
    client_emon.println();

    // note the time that the connection was made:
    lastConnectionTime_emon = millis();
    Serial.println(F("Datos enviados a emoncms_spectrum"));
  } 
  else {
    // if you couldn't make a connection:
    Serial.println(F("Fallo"));
//    Serial.println("Disconnecting emoncms...");
    lastConnectionTime_emon = millis();
    client_emon.stop();
    delay(100);
  }
  
}


void envio_emon(void){
  if (client_emon.available()) {
    char c_emon = client_emon.read();
    //Serial.print(c_emon);
    }
  if (!client_emon.connected() && lastConnected_emon) {
    Serial.println();
    Serial.println("Disconnecting emoncms...");
    client_emon.stop();
    }
     if((millis() - lastConnectionTime_emon > postingInterval_emon) ) {
       delay(5);
        sendData_emon();
    }
  lastConnected_emon = client_emon.connected();
}

//=============================================

void imprimirdatos(void){
  if ((millis()-t_ant_print)>t_print){
         t_ant_print=millis();
  
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  ip=WiFi.localIP();
  Serial.print("Pot_INV_red:");
  Serial.print(Pot_INV_red);
  Serial.print(" Pot_INV_auto:");
  Serial.println(Pot_INV_auto);
  Serial.print(" umbral_PAC:");
  Serial.println(umbral_PAC);
  Serial.println("Datos emoncms ++++++++++");
   for (j=0;j<EMON_SIZE;j++){ 
    Serial.print(emonData.feedname[j]);
    Serial.print(" : ");
    Serial.print(emonData.feedid[j]);
    Serial.print("--> ");
    Serial.println(emonData.feedvalue[j]);
   }
 }
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year()); 
  Serial.println(); 
}


void checkinternet(){
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
   
  // Wait until the client sends some data
  Serial.println("new client");
  digitalClockDisplay();
  while(!client.available()){
    delay(1);
  }
   
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
   
  // Match the request
  if (request.indexOf("/MAN=ON") != -1) {
            manual = 1;
  } 
  if (request.indexOf("/MAN=OFF") != -1) {
            manual = 0;
  } 
  if (request.indexOf("/AIRE=ON") != -1) {
    digitalWrite(ledPin, LOW);
    ON();
    aire_onoff = HIGH;
  } 
  if (request.indexOf("AIRE=OFF") != -1){
   digitalWrite(ledPin, HIGH);
   OFF();
    aire_onoff = LOW;
  }

 if (request.indexOf("RESET") != -1){
    //reset settings - for testing
    wifiManager.resetSettings();
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  if (cadena.indexOf(F("?maxwatts=")) >0){
               int colonPosition = cadena.indexOf('=');
               int colonPosition2 = cadena.indexOf('HTTP');
               String valor = cadena.substring(colonPosition+1, colonPosition2);
               umbral_PAC = double(valor.toFloat());
           }
  
// Set ledPin according to the request
//digitalWrite(ledPin, aire_onoff);
   
 //client.flush();
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  String message = "<head>";
  message += "<meta http-equiv='refresh' content='120'/>";
  message += "<title>Aire Altavista 107 Nodemcu</title>";
  message += "<style>";
  message += "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  message += "</style></head><body>"; 
 client.print(message);
  
  client.print("Altavista107 - NodemCu, Aire acondicionado is now: ");
   
  if(aire_onoff == HIGH) {
    client.print("On");  
  } else {
    client.print("Off");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/MAN=ON\">here</a> turn the Control Manual<br>");
  client.println("Click <a href=\"/MAN=OFF\">here</a> turn the Control Auto<br>");
  client.println("<button type=\"button\" onClick=\"location.href='/MAN=ON'\">MAN ON</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/MAN=OFF'\">MAN OFF</button><br>");
  client.println("Click <a href=\"/AIRE=ON\">here</a> turn the AIRE on pin D2 ON<br>");
  client.println("Click <a href=\"/LED=OFF\">here</a> turn the AIRE on pin D2 OFF<br>");
  client.println("<button type=\"button\" onClick=\"location.href='/AIRE=ON'\">AIRE ON</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/AIRE=OFF'\">AIRE OFF</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/RESET'\">RESET WIFI</button><br>");
  client.println(F("</H2>"));
  client.println(F("<FORM>Umbral arranque aire Acond. <input type=text name=maxwatts size=4 value="));
  client.println(umbral_PAC);
  client.println(F(" > <input type=submit value=Enter> </form> <br/><br/>"));
  
  client.println("<p> UTC Time: ");
  client.print(hour());
  client.print(":");
  client.print(minute());
  client.print(":");
  client.print(second());
  client.print("</p><br>PAC Ingeteam: ");
  client.print(Pot_INV_red);
  client.print("<r>PAC Solarmax: ");
  client.print(Pot_INV_auto);
  client.print("<br>Casa: ");
  client.print(PAC_casa);
  //client.print("<br>lux: ");
  //client.print(lux);
  
  client.println("</html>");
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
 
}

/// Xively
void Xively2(void){
  
    if( (millis() - lastConnectionTime_xiv > postingInterval_xiv) ) {
    lastConnectionTime_xiv=millis();
      if( getFeed(feedID, streamCount) == true)
   {
      for(int id = 0; id < streamCount; id++){
        Serial.println( streamData[id]);
      }
     // Serial.println("--");
   }
   else
   {
      Serial.println(F("Unable feed"));
    }
    Pot_INV_red=streamData[1];
    Pot_INV_auto=streamData[3];
}
  lastConnected_xiv = client_xiv.connected();
}

// returns true if able to connect and get data for all requested streams
boolean getFeed(int feedId, int streamCount )
{
boolean result = false;
  if (client_xiv.connect(server_xiv, 80)>0) {
    client_xiv.print(F("GET /v2/feeds/"));
    client_xiv.print(130137);
    client_xiv.print(F(".csv HTTP/1.1\r\nHost: api.pachube.com\r\nX-PachubeApiKey: "));
    client_xiv.print(F("WQ5gyvvWkWJ5oNLO2G7aOEb8Ex2EeoCWliPU4SjJB68ePyDO"));
    client_xiv.print(F("\r\nUser-Agent: Arduino 1.0"));
    client_xiv.println(F("\r\n"));
  }
  else {
    Serial.println(F("failed"));
  }
  if (client_xiv.connected()) {
    Serial.println(F("Con"));
    if(  client_xiv.find("HTTP/1.1") && client_xiv.find("200 OK") )
       result = processCSVFeed(streamCount);
    else
       Serial.println(F("no 200 OK"));
  }
  else {
    Serial.println(F("Disc."));
  }
  client_xiv.stop();
  client_xiv.flush();
  return result;
}

int processCSVFeed(int streamCount)
{
  int processed = 0;
  client_xiv.find("\r\n\r\n"); // find the blank line indicating start of data
  for(int id = 0; id < streamCount; id++)
  {
     id = client_xiv.parseFloat(); // you can use this to select a specific id
    client_xiv.find("Z,"); // skip past timestamp
    streamData[id] = client_xiv.parseFloat();
    processed++;      
  }
  return(processed == streamCount );  // return true if got all data
}


//===============================
//Read emoncms feed

int readEMONCMS (int numvar,int emon_var_id)
{
  emon_var_read= -1;
  char buffer [200];
  memset (buffer, 0, sizeof(buffer));
  delay(100);
  Serial.println("===============================================");
  Serial.println("Reading EMONCMS...");
  delay(100);
   boolean salir = false; 
  if (client_emon.connect(server_emon, 80))
  {
    Serial.println("Connected to server emoncms.org");
    Serial.println("Making an HTTP Request");
    client_emon.print("GET /emoncms_spectrum/feed/value.json?apikey=");
    client_emon.print(apikey_emon);
    client_emon.print("&id=");
    client_emon.println(emon_var_id);
    client_emon.println(" HTTP/1.1");
    client_emon.println("Host:emoncms.org");
    client_emon.println("User-Agent: Arduino-ethernet");
    client_emon.println("Connection: close");
    //client_emon.println();
    client_emon.println(F("\r\n"));
    Serial.println("End of HTTP Request");
  }
  else 
  {
    Serial.println("Connection to server emoncms.org failed");
    return 0;
  }
  delay(1000);
  salir = false;
  while (salir == false)
  {
    if (client_emon.available())
    {
      char c = client_emon.read();
      Serial.print(c);
      buffer [strlen(buffer)]= c;
      if (c == '\n' )
      {
        Serial.print("FIN");
        Serial.println(buffer);
        memset (buffer, 0, 200);
      }    
    }
    else
    {
      Serial.println("Client emon not available");
      salir = true;
    }
    delay(1);
    // if the server's disconnected, stop the client:
    if (!client_emon.connected())
    {
      Serial.println("buffer: ");
      Serial.println(buffer);
      emon_var =  atoi( buffer+1);
      emon_var_read = emon_var;
      emonData.feedvalue[numvar]=emon_var;
      Serial.println("Disconnecting from server emoncms.");
      client_emon.stop();
      Serial.println("Disconnected from server emoncms");
       salir=true;
    }
  }
  Serial.print("Value of emon_var:");
  Serial.println(emon_var);
  Serial.print("Value of emon_var_read:");
  Serial.println(emon_var_read);
  Serial.println("End of read EMONCMS");
  Serial.println("===============================================");
  return 1;
}

//fin lectura emoncms


