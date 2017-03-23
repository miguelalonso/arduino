/*http://192.168.1.53:93
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
Si se usa sensor irradiancia TSL2561
TSL2561 -> Nodemcu
vcc -> 3V
gng -> G
SDA -> D2
SCL -> D1
Conexiones del arduino nano
Nano ->Nodemcu
GND -> GND
D3(Tx) -> D4(Rx)  la salida nano D3(TX)al pin D4 (el mismo que el LED interno) del Nodemcu
D2(Rx) -> D6(Tx)  No hace falta conectarlo ya que el Nodemcu sólo lee datos

Usa wifimanager para conectarse
1.la primera vez o después de un reset wifi, conectarse con el movil o el ordenador a la red Nodemcu
(introducir la clave A12345678)
2.después entrar en el explorador a la dirección http://192.168.4.1
3.seleccionar la wifi disponible y conectar introduciendo su clave correspondiente
4.desconectar el movil u ordenador de la nodemcu y conectar a la wifi local
5. por puerto serie se puede ver la ip local para acceder, suele ser http://192.168.1.43

 * */
 
#include <FS.h>       //this needs to be first, or it all crashes and burns...
                      //https://github.com/esp8266/Arduino
#include <ESP8266WiFi.h>
//#include <WiFiSerial.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <TimeLib.h> 
#include <SPI.h>
#include <Wire.h>

#include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>

SoftwareSerial inSerial(14,12,false,128);// D5(RX), D6(TX) nodemcu
SoftEasyTransfer ETin; 

SoftwareSerial outSerial(16,5);// D5(RX), D6(TX) nodemcu
SoftEasyTransfer ETout; 


struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
uint16_t cnt;
uint16_t dia;
uint16_t mes;
uint16_t ano;
uint16_t hora;
uint16_t minuto;
uint16_t segundo; 
uint16_t acimutsol; 
uint16_t cenitsol;
uint16_t acimut;
uint16_t cenit;
uint16_t manual;
uint16_t num_error; 
uint16_t milis;
uint16_t var4; 
uint16_t var5; 
uint16_t var6; 
};

RECEIVE_DATA_STRUCTURE mydatain;

struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
uint16_t cnt;
uint16_t dia;
uint16_t mes;
uint16_t ano;
uint16_t hora;
uint16_t minuto;
uint16_t segundo; 
uint16_t manual; 
uint16_t manual_giraNorte;
uint16_t manual_giraSur;
uint16_t manual_giraEste;
uint16_t manual_giraOeste;
uint16_t areposo; 
uint16_t milis;
uint16_t horaOK; 
uint16_t paro; 
uint16_t var6; 
};
SEND_DATA_STRUCTURE mydataout;

unsigned long t_ant_print=0;
unsigned long t_print=5000;
unsigned long t_ant_syncNTP=0;
unsigned long t_syncNTP=300000; //sincroniza con NTP cada 5 minutos
boolean datosOK; 
boolean debug=1;                                       



WiFiServer server ( 80 );
int ledPin = 13; // pin D7 del Nodemcu
WiFiManager wifiManager;

WiFiClient client_emon;
IPAddress server_emon(163,117,157,189); //ordenador jaen-espectros2
unsigned long lastConnectionTime_emon = 0;             // last time you connected to the server, in milliseconds
boolean lastConnected_emon = false;                    // state of the connection last time through the main loop
const unsigned long postingInterval_emon = 28000; // delay between updates, in milliseconds

 
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

IPAddress ip; 

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

        mydataout.dia=day();
        mydataout.mes=month();
        mydataout.ano=year();
        mydataout.hora=hour();
        mydataout.minuto=minute();
        mydataout.segundo=second();
        
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
  int j=0;
  while(!client.available() && j<1000){
    Serial.println("waiting...");
    delay(1);
    j++;
  }
   
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
   
  // Match the request
  int value = LOW;
  if (request.indexOf("/MANUAL=ON") != -1) {
    digitalWrite(ledPin, HIGH);
    value = HIGH;
    mydataout.manual=true;
    mydataout.manual_giraNorte=false;
    mydataout.manual_giraSur=false;
    mydataout.manual_giraEste=false;
    mydataout.manual_giraOeste=false;
  } 
  if (request.indexOf("MANUAL=OFF") != -1){
    digitalWrite(ledPin, LOW);
    value = LOW;
    mydataout.manual=false;
    mydataout.manual_giraNorte=false;
    mydataout.manual_giraSur=false;
    mydataout.manual_giraEste=false;
    mydataout.manual_giraOeste=false;
  }
  if (request.indexOf("NORTE=ON") != -1){
    mydataout.manual=true;
    mydataout.manual_giraNorte=true;
    mydataout.manual_giraSur=false;
    mydataout.manual_giraEste=false;
    mydataout.manual_giraOeste=false;
  }
  if (request.indexOf("SUR=ON") != -1){
    mydataout.manual=true;
    mydataout.manual_giraNorte=false;
    mydataout.manual_giraSur=true;
    mydataout.manual_giraEste=false;
    mydataout.manual_giraOeste=false;
  }
  if (request.indexOf("ESTE=ON") != -1){
    mydataout.manual=true;
    mydataout.manual_giraNorte=false;
    mydataout.manual_giraSur=false;
    mydataout.manual_giraEste=true;
    mydataout.manual_giraOeste=false;
  }
  if (request.indexOf("OESTE=ON") != -1){
    mydataout.manual=true;
    mydataout.manual_giraNorte=false;
    mydataout.manual_giraSur=false;
    mydataout.manual_giraEste=false;
    mydataout.manual_giraOeste=true;
  }

 if (request.indexOf("RESET") != -1){
    //reset settings - for testing
    wifiManager.resetSettings();
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  
// Set ledPin according to the request
//digitalWrite(ledPin, value);
   
 //client.flush();
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  String message = "<head>";
  message += "<meta http-equiv='refresh' content='120'/>";
  message += "<title>Panttilt Nodemcu</title>";
  message += "<style>";
  message += "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  message += "</style></head><body>"; 
 client.print(message);
 client.print("Seguidor PanTilt Ciemat: ");
 client.print("<p>Led pin is now: ");
  if(value == HIGH) {
    client.print("On");  
  } else {
    client.print("Off");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/MANUAL=ON\">here</a> Control Manual ON<br>");
  client.println("Click <a href=\"/MANUAL=OFF\">here</a> Control Automático<br>");
  client.println("<button type=\"button\" onClick=\"location.href='/MANUAL=ON'\">MANUAL ON</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/MANUAL=OFF'\">MANUAL OFF</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/NORTE=ON'\">GIRO NORTE</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/SUR=ON'\">GIRO SUR</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/ESTE=ON'\">GIRO ESTE</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/OESTE=ON'\">GIRO OESTE</button><br>");
  
  client.println("<button type=\"button\" onClick=\"location.href='/RESET'\">RESET WIFI</button><br>");
  
  client.println("<p> NodeMCU UTC Time: ");
  client.print(hour());
  client.print(":");
  client.print(minute());
  client.print(":");
  client.print(second());
  client.print("</p>acimutsol: ");
  client.print((float)mydatain.acimutsol/100);
  client.print("</p>acimut: ");
  client.print((float)mydatain.acimut/100);
  client.print("</p>cenitsol: ");
  client.print((float)mydatain.cenitsol/100);
  client.print("</p>cenit: ");
  client.print((float)mydatain.cenit/100);
  client.print("</p>manual: ");
  client.print(mydatain.manual);
  client.print("</p>num_error: ");
  client.print(mydatain.num_error);
  client.print("</p>Fin Carrera: ");
  client.print(mydatain.var5);

  client.println("<p> Pantilt Tacker Time: ");
  client.print(mydatain.hora);
  client.print(":");
  client.print(mydatain.minuto);
  client.print(":");
  client.print(mydatain.segundo);
  
  
  
  client.println("</html>");
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
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
      mydataout.horaOK=true;
      Serial.println(" NTP Response :Hora OK");
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  mydataout.horaOK=false;
  return 0; // return 0 if unable to get the time
}

//================EMONCMS====================

//envio de datos a emoncms
void sendData_emon(void){
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  //client_emon.stop();
   // if there's a successful connection:
   
  Serial.println(F("comienzo de envio a emoncms"));
  if (client_emon.connect(server_emon, 80)) {
    Serial.println(F("Connecting emoncms..."));
    // send the HTTP PUT request:
    client_emon.print(F("GET /emoncms_spectrum/input/post.json?apikey=9bbe8aedfc502644f616dd98e3b4f737&node=53&json={broadband"));
    client_emon.print(F(":"));
     float valor=32;
    client_emon.print(valor);
    client_emon.print(F(",infrared2:")); 
    valor=33;
    client_emon.print(F(",cntin:")); client_emon.print(mydatain.cnt);
      
    client_emon.print(F(",ip3_pantilt2:")); client_emon.print(ip[3]);
    
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

void imprimirdatos(void){
 
  Serial.println(mydataout.horaOK);

  Serial.println("<p> NodeMCU UTC Time: ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print("acimutsol: ");
  Serial.print(mydatain.acimutsol);
  Serial.print("acimut: ");
  Serial.print(mydatain.acimut);
  Serial.print("cenitsol: ");
  Serial.print(mydatain.cenitsol);
  Serial.print("cenit: ");
  Serial.print(mydatain.cenit);
  Serial.print("manual: ");
  Serial.print(mydatain.manual);
  Serial.print("num_error: ");
  Serial.print(mydatain.num_error);
  Serial.print("var5: ");
  Serial.print(mydatain.var5);

  Serial.println("<p> Pantilt Tacker Time: ");
  Serial.print(mydatain.hora);
  Serial.print(":");
  Serial.print(mydatain.minuto);
  Serial.print(":");
  Serial.print(mydatain.segundo);
  
  
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
 
}


void setup() {
  Serial.begin(9600);
  Serial.println();
  inSerial.begin(19200);
  ETin.begin((byte*)&mydatain,34, &inSerial);
  outSerial.begin(19200);
  ETout.begin((byte*)&mydataout,34, &inSerial);
  
  Serial.print( "Esperando 2 minutos");
  delay(1200); //espera dos minutos para dar tiempo al router a coger wifi.
  Serial.println("\nSoftware serial test started");
  Serial.println("\nSeguidor Panttilt + Nodemcu:");
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
  //IPAddress _ip = IPAddress(192, 168, 1, 53);   ///IP fija 53
  //IPAddress _gw = IPAddress(192, 168, 1, 1);
  //IPAddress _sn = IPAddress(255, 255, 255, 0);
  //end-block2
  
  //wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  
  if (!wifiManager.autoConnect("NodeMCU_PANTILT", "A12345678")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
 
  
  
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  /////////////////////////////////////////server
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
   ip=WiFi.localIP();
   
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
}


void loop() {
  // put your main code here, to run repeatedly:
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();  
    }
  }
  
  checkinternet();
  if (datosOK){
    Serial.println("datosOK:");
    envio_emon();
    }
 

//check and see if a data packet has come in. 
if(ETin.receiveData()){
    //this is how you access the variables. [name of the group].[variable name]
    //since we have data, we will blink it out. 
    Serial.println("Data received from Arduino, counter");
    Serial.println(mydatain.cnt);
    Serial.println(mydatain.milis);
    imprimirdatos();
    ETout.sendData();
  }
if ((millis()-t_ant_print)>t_print){
  Serial.println("serial:");
        t_ant_print=millis();
        mydataout.cnt++;
        mydataout.milis=millis();
       
      if (mydataout.cnt>999){mydataout.cnt=0;}
        digitalClockDisplay();
      
}
 
if ((millis()-t_ant_syncNTP)>t_syncNTP){
         t_ant_syncNTP=millis();
          setSyncProvider(getNtpTime);
}
  
 // if (debug==1)  {   imprimirdatos();  }

 //4. wifi reconnect
  if (WiFi.status() != WL_CONNECTED){
      WIFI_Connect();
  } 
  
  if (millis()> 86400000*2){
    ESP.restart(); //reinicia cada dos días
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


//////////////////////////////////////

