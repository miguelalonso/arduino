/* http://192.168.1.50:90
 * Lee la potencia medida por el arduino con la pinza de energymonitor
 * pir softwareserial y envia los datos a emoncms
 * Agsoto 2016
 *
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

Conexiones del arduino nano
arduino ->Nodemcu
GND -> GND
D3(Tx) -> D4(Rx)  la salida nano D3(TX)al pin D4 (el mismo que el LED interno) del Nodemcu
D2(Rx) -> D6(Tx)  No hace falta conectarlo ya que el Nodemcu sólo lee datos

El Led interno de Nodemcu parpadea cada vez que recibe datos del Nano

  */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <TimeLib.h> 
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h> //to store default data
#include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>

////para no-ip////////////////
const char host[] = "altavista107.sytes.net";
const char usuarioClave[] = "bWlndWVsLmZvdG92b2x0YWljYUBnbWFpbC5jb206MTcyN29zZ3Y2Nw==";
char servidorIP[] = "checkip.dyndns.org";
char servidorActualizacion[] = "dynupdate.no-ip.com";
IPAddress ipActual;
IPAddress ipUltimaEnviada;
///////////////////////////////////////////////////////
//SoftwareSerial mySerial(2,12,false,64);// D4(RX), D6(TX) nodemcu
SoftwareSerial mySerial(14,12,false,64);// D4(RX), D6(TX) nodemcu

SoftEasyTransfer ET; 

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  uint32_t ms;
  float realPower; 
  float apparentPower;
  float powerFactor;
  float Irms;
  float Vrms;
};

RECEIVE_DATA_STRUCTURE mydata;
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
WiFiServer server (90);  //abrir puerto en portal alejandra para acceso externo
WiFiManager wifiManager;
WiFiClient client_emon;
WiFiClient client_json;
IPAddress server_emon(163,117,157,189); //ordenador jaen-espectros2
String apikey_emon="9bbe8aedfc502644f616dd98e3b4f737";
unsigned long lastConnectionTime_emon = 0;             // last time you connected to the server, in milliseconds
boolean lastConnected_emon = false;                    // state of the connection last time through the main loop
unsigned long postingInterval_emon = 28000; // delay between updates, in milliseconds

int j=0;
unsigned long t_ini; 
float umbral_PAC=1000; //umbral de potencia para arranque del aire acondicionado
const int ledPin =  14;      // the number of the LED pin
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
unsigned long postingInterval_xiv = 60000; // delay between updates, in milliseconds
float Pot_INV_auto; //potencia del inversor conectado a red Altavista 107 
float Pot_INV_red; //potencia del inversor conectado a red Altavista 107
float PAC_casa; //potencia casa Altavista 107    
String cadena;
int emon_var_read = -1;
unsigned long readingEMONCMSinterval=90000;//ver al final de begin
unsigned long lastReadEMONCMSTime;
int emon_var = 9999;
int emon_var_id = 1653;//Pot_INV_auto autoconsumo
IPAddress ip; 
int i=0;

void setup()
{
  Serial.begin(9600);
  delay (1000);
  Serial.print( "Esperando 2 minutos");
  delay(120000); //espera dos minutos para dar tiempo al router a coger w
  pinMode(ledPin, OUTPUT);
  Serial.println();
  Serial.println("Medida de potencia Altavista107");
  wifiManager.setBreakAfterConfig(true);
  //reset settings - for testing
  //wifiManager.resetSettings();
  //set static ip
  //block1 should be used for ESP8266 core 2.1.0 or newer, otherwise use block2

  //start-block1
  //IPAddress _ip,_gw,_sn;
  //_ip.fromString(static_ip);
  //_gw.fromString(static_gw);
  //_sn.fromString(static_sn);
  //end-block1

  //start-block2
  IPAddress _ip = IPAddress(192, 168, 1, 50);   ///IP fija 50
  IPAddress _gw = IPAddress(192, 168, 1, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);
  //end-block2
  
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
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
  Serial.println("Waiting 10 seconds...");
  delay(10000);
  Serial.println("Inicializando variables");
  postingInterval_emon = 0;
  readingEMONCMSinterval=0;
  postingInterval_xiv = 0;
    read_emoncms_variables();
    //envio_emon();
    Xively2();
    delay(5000);
    Serial.println("Iniciado...");
  delay(5000);
  postingInterval_emon = 48000;
  readingEMONCMSinterval=100000;
  postingInterval_xiv = 70000;

   //para la lectura del arduino nano por sofwareserial, pin D4
  mySerial.begin(9600);
  //start the library, pass in the data details and the name of the serial port.
  //ET.begin(details(mydata), &mySerial);
   ET.begin((byte*)&mydata,24, &mySerial); //24 es la longitud de la estructura de datos
   //ojo!! en arduino nano un int son 2 bytes en nodemcu son 4 bytes.
  Serial.print( "tamano:");
  int tam=sizeof(mydata);
  Serial.print( tam);
  
  for (byte n = 0; n <= 3; n++) ipUltimaEnviada[n] = EEPROM.read(n); // Cargamos de EEPROM la ultima IP que se envio.
  Serial.print(F("IP EEPROM:"));
  Serial.println(ipUltimaEnviada);
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
         
    }
  //3a. read emoncms variables////////////////////////////////////////////////////////////////////////////////////
  read_emoncms_variables();
  //3b. Datos a emoncms en leganes
   if( (millis() - lastConnectionTime_emon > postingInterval_emon) ) {
   envio_emon();
   lastConnectionTime_emon = millis();
    }
  //fin envio emoncms
  
  //4. wifi reconnect/////////////////////////////////////////////////////////////////////////////////
  if (WiFi.status() != WL_CONNECTED){
      WIFI_Connect();
  } 

  //5. lee los datos de potencias por puerto softwareserial
  //check and see if a data packet has come in. 
  if(ET.receiveData()){
    //chequeaIngeteam();
      Serial.print ("milisegundos: ");
      Serial.print(mydata.ms);   
      Serial.print (" realPower: ");
      Serial.println(mydata.realPower); 
    
 }

 //update no-ip
  update_no_ip();
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
          //if (lectura[1]&& emonData.feedvalue[1]<2){manual=emonData.feedvalue[1];}
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
    client_emon.print(F("&node=107&json={Pot_INV_red"));
    client_emon.print(F(":"));
    client_emon.print(Pot_INV_red);
    client_emon.print(F(",Pot_INV_auto:")); 
    client_emon.print(Pot_INV_auto); 
    client_emon.print(F(",PAC_casa:")); 
    client_emon.print(mydata.realPower); 
    client_emon.print(F(",S_casa:")); 
    client_emon.print(mydata.apparentPower);
    client_emon.print(F(",PF_casa:")); 
    client_emon.print(mydata.powerFactor);
    client_emon.print(F(",VAC_casa:")); 
    client_emon.print(mydata.Vrms);
    client_emon.print(F(",IAC_casa:")); 
    client_emon.print(mydata.Irms);
       
    client_emon.print(F(",ip_3_mide:")); 
    client_emon.print(ip[3]);
    
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
    Serial.println("Disconnecting emoncms...");
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
  
    Serial.print(Pot_INV_red);
    Serial.print(F(",Pot_INV_auto:")); 
    Serial.println(Pot_INV_auto); 
    Serial.print(F(",PAC_casa:")); 
    Serial.print(mydata.realPower); 
    Serial.print(F(",S_casa:")); 
    Serial.print(mydata.apparentPower);
    Serial.print(F(",PF_casa:")); 
    Serial.print(mydata.powerFactor);
    Serial.print(F(",VAC_casa:")); 
    Serial.print(mydata.Vrms);
    Serial.print(F(",IAC_casa:")); 
    Serial.println(mydata.Irms);
  
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
  
 if (request.indexOf("RESET") != -1){
    //reset settings - for testing
    wifiManager.resetSettings();
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  String message = "<head>";
  message += "<meta http-equiv='refresh' content='120'/>";
  message += "<title>Mide Altavista 107 Nodemcu</title>";
  message += "<style>";
  message += "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  message += "</style></head><body>"; 
  client.print(message);
  client.println("<button type=\"button\" onClick=\"location.href='/RESET'\">RESET WIFI</button><br>");
 
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
  client.print("<br>myData.realPower: ");
  client.print(mydata.realPower);
  client.print("<br>myData.apparentPower: ");
  client.print(mydata.apparentPower);
  client.print("<br>myData.powerFactor: ");
  client.print(mydata.powerFactor);
  client.print("<br>myData.Vrms: ");
  client.print(mydata.Vrms);
  client.print("<br>myData.Irms: ");
  client.print(mydata.Irms);
  
  
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

/////////////////////////////////////==== NO-IP=======
 void update_no_ip(){
  if ((millis()-t_ini)>999000L){
     t_ini=millis();
     ipActual = compruebaIP();
     Serial.print ("comprobando IP");
      if (ipActual != ipUltimaEnviada) {
      Serial.print(F("Actualizando IP "));
      Serial.print(servidorActualizacion);
      if (actualizaIPDynDNS(ipActual) == true){
      for (byte n = 0; n <= 3; n++) EEPROM.write(n, ipActual[n]);
      ipUltimaEnviada = ipActual;
    }
  } else {
    Serial.println(" nada....");
  }
  } 
}
 
 IPAddress compruebaIP() {
 
  WiFiClient cliente;
  String webIP;
  int desde, hasta;
  if (cliente.connect(servidorIP, 80)) {
    cliente.println(F("GET / HTTP/1.0"));
    cliente.println();
    webIP = "";
  } else {
    Serial.print(":( Conexion fallida con ");
    Serial.println(servidorIP);
  }
  while (cliente.connected()) { 
    while (cliente.available()) {
      webIP.concat((char)cliente.read());
    }
  }
  cliente.stop();
  desde = webIP.indexOf("Address: ") + 9;
  hasta = webIP.indexOf("</body>");
  return ipAIPAddress(webIP.substring(desde, hasta));
}
 
boolean actualizaIPDynDNS(IPAddress ip) {
  WiFiClient cliente;
  String webIP;
  if (cliente.connect(servidorActualizacion, 80)) {
    cliente.print("GET /nic/update?hostname=");
    cliente.print(host);
    cliente.print("&myip=");
    cliente.print(ip);
    cliente.println(" HTTP/1.0");
    cliente.println("Host: dynupdate.no-ip.com");
    cliente.print("Authorization: Basic ");
    cliente.println(usuarioClave);
    cliente.println("User-Agent: josematm.com - Wirduino - 0.1");
    cliente.println();
  } else {
    Serial.print(":( Conexion fallida con ");
    Serial.println(servidorActualizacion);
    return false;
  }
  while (cliente.connected()) { 
    while (cliente.available()) {
      webIP.concat((char)cliente.read());
    }
  }
  Serial.println(webIP);
  cliente.stop();
  if (webIP.lastIndexOf("badauth") != -1) {
    Serial.println("Error de de autentificacion. Comprueba usuario y clave");
    return false;
  } else {
    return true;  
  }
}
 
IPAddress ipAIPAddress(String ipEnCadena){
  IPAddress ipEnBytes;
  char digitoIP[4];
  byte cursorDigito = 0;
  byte cursorIP = 0;
  for (byte n = 0; n < ipEnCadena.length(); n++){
    if (ipEnCadena.charAt(n) != '.') {
      digitoIP[cursorDigito] = ipEnCadena.charAt(n);
      cursorDigito++;
    } else {
      digitoIP[cursorDigito +1] = '\n';
      ipEnBytes[cursorIP] = atoi(digitoIP);
      cursorDigito = 0;
      memset(digitoIP, 0, sizeof(digitoIP));
      cursorIP++;
    }
  }
  digitoIP[cursorDigito +1] = '\n';
  ipEnBytes[cursorIP] = atoi(digitoIP);
  return ipEnBytes;
}
/////////////////////////////////////==== NO-IP=======
 
