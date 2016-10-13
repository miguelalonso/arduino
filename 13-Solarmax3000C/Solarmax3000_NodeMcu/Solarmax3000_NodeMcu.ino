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

El Led interno de Nodemcu parpadea cada vez que recibe datos del Nano

Usa wifimanager para conectarse
1.la primera vez o después de un reset wifi, conectarse con el movil o el ordenador a la red Nodemcu
(introducir la clave A12345678)
2.después entrar en el explorador a la dirección http://192.168.4.1
3.seleccionar la wifi disponible y conectar introduciendo su clave correspondiente
4.desconectar el movil u ordenador de la nodemcu y conectar a la wifi local
5. por puerto serie se puede ver la ip local para acceder, suele ser http://192.168.1.43
NOTA: Cuando se conecta uno con la ip local a veces se queda "colgado", hay que resetear apagando y volviendo a encender el Nodemcu
 8uqH2jjiR6E2PsSfXmvt
AC output PAC Power
Operating hours KHR
Date year DYR
Date month DMT
Date day DDY
Energy year KYR
Energy month KMT
Energy day KDY
Energy total KT0
Installed capacity PIN
Mains cycle duration TNP
Network address ADR
Relative output PRL
Software version SWV
Solar energy year RYR
Solar energy day RDY
Solar energy total RT0
Solar radiation RAD
Voltage DC UDC
Voltage phase 1 UL1
Voltage phase 2 UL2
Voltage phase 3 UL3
Current DC IDC
Current phase 1 IL1
Current phase 2 IL2
Current phase 3 IL3
Temperature power unit 1 TKK
Temperature power unit 2 TK2
Temperature power unit 3 TK3
Temperature solar cells TSZ
Type Type
Time minute TMI
Time hour THR

 Operating hours (KHR): 9953
Energy today [Wh] (KDY): 12900
Energy yesterday [Wh] (KLD): 14200
Energy this month [kWh] (KMT): 154
Energy last monh [kWh] (KLM): 248
Energy this year [kWh] (KYR): 558
Energy last year [kWh] (KLY): 2703
Energy total [kWh] (KT0): 6356

 * */
 
#include <FS.h>       //this needs to be first, or it all crashes and burns...
                      //https://github.com/esp8266/Arduino
#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <TimeLib.h> 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h> //D0,D1,D2 con este sensor


#include <SoftwareSerial.h>

SoftwareSerial mySerial(14,12,false,256);// D5(RX), D6(TX) nodemcu

unsigned long t_ant_solarmax=0;
unsigned long t_solarmax=5000;
String potencia= "{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}";
String energia="{FB;05;36|64:CAC;KHR;KDY;KLD;KMT;KLM;KYR;KLY;KT0|0D34}";
String lectura="";
char c;
float VDC;
float IDC;
float VAC;
float IAC;
float PAC;
float PRL; //da el % de potencia de operación
float CAC; //startups??
float KYR;//energy year
float KLY; //energy last year
float KMT;//energy month
float KLM; //energy last month
float KDY; //energy day
float KT0;//total energy
float TKK; //temperature
float KHR; //operating  hours
float KLD; //energy yesterday
unsigned long cnt=0;

boolean datosOK; //chequea si los datos de ingeteam no son todos cero (por la noche) para no enviar a emoncms.

boolean debug=1;                                       //Set to 1 to few debug serial output, turning debug off increases battery life

unsigned long t_ant_print=0;
int t_print=3000;

uint16_t broadband = 0;
uint16_t infrared = 0; 
float lux;
float luxCS;
float luxCL;
float Ccalib=1000; //cuentas a 1000 W/m2
float irradiancia;
  
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);


WiFiServer server ( 93 );
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
  int value = LOW;
  if (request.indexOf("/LED=ON") != -1) {
    digitalWrite(ledPin, HIGH);
    value = HIGH;
  } 
  if (request.indexOf("LED=OFF") != -1){
    digitalWrite(ledPin, LOW);
    value = LOW;
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
  message += "<title>Solarmax3000 Nodemcu</title>";
  message += "<style>";
  message += "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  message += "</style></head><body>"; 
 client.print(message);
  
   
  client.print("Led pin is now: ");
   
  if(value == HIGH) {
    client.print("On");  
  } else {
    client.print("Off");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/LED=ON\">here</a> turn the LED on pin D5 ON<br>");
  client.println("Click <a href=\"/LED=OFF\">here</a> turn the LED on pin D5 OFF<br>");
  client.println("<button type=\"button\" onClick=\"location.href='/LED=ON'\">LED ON</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/LED=OFF'\">LED OFF</button><br>");
  client.println("<button type=\"button\" onClick=\"location.href='/RESET'\">RESET WIFI</button><br>");
  
  client.println("<p> UTC Time: ");
  client.print(hour());
  client.print(":");
  client.print(minute());
  client.print(":");
  client.print(second());
  client.print("</p><br>PAC Solarmax: ");
  client.print(PAC);
  client.print("</p><br>broadband: ");
  client.print(broadband);
  client.print("<br>infrared: ");
  client.print(infrared);
  client.print("<br>lux: ");
  client.print(lux);
  
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
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
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
     float valor=broadband;
    client_emon.print(valor);
    client_emon.print(F(",infrared:")); 
    valor=infrared;
    client_emon.print(valor);  
    client_emon.print(F(",Lux:")); 
    valor=lux;
    client_emon.print(valor);  
    client_emon.print(F(",PAC:")); client_emon.print(PAC);
    client_emon.print(F(",VDC:")); client_emon.print(VDC);
    client_emon.print(F(",IDC:")); client_emon.print(IDC);
    client_emon.print(F(",VAC:")); client_emon.print(VAC);
    client_emon.print(F(",IAC:")); client_emon.print(IAC);
    client_emon.print(F(",KYR:")); client_emon.print(KYR);
    client_emon.print(F(",KMT:")); client_emon.print(KMT);
    client_emon.print(F(",KDY:")); client_emon.print(KDY);
    client_emon.print(F(",KT0:")); client_emon.print(KT0);
    client_emon.print(F(",TKK:")); client_emon.print(TKK);
    
    client_emon.print(F(",ip3_solarmax:")); client_emon.print(ip[3]);
    
    
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

void calularLuxCS(){
  //datos obtenidos de hoja de características técnicas del TSL2561
  float Lux=0;
  float CH0=broadband;
  float CH1=infrared;
  float ratio=CH1/CH0;
  float pot=pow(ratio,1.4);


  if(ratio>0 && ratio<= 0.52){ Lux=0.0315*CH0-0.0593*CH0*pot;}
  if( ratio>0.52 && ratio<= 0.65){ Lux = 0.0229*CH0-0.0291*CH1;}
  if( ratio>0.65 &&ratio<= 0.80){ Lux = 0.0157 * CH0-0.0180*CH1;}
  if( ratio>0.80 && ratio<= 1.30){ Lux = 0.00338*CH0-0.00260*CH1;}
  if( ratio>1.30 ){ Lux = 0;}

  luxCS=Lux;
  
}

void calularLuxCL(){
  float Lux=0;
  float CH0=broadband;
  float CH1=infrared;
  float ratio=CH1/CH0;
  float pot=pow(ratio,1.4);


  if(ratio>0 && ratio<= 0.50){ Lux=0.0304*CH0-0.062*CH0*pot;}
  if( ratio>0.50 && ratio<= 0.61){ Lux = 0.0224*CH0-0.031*CH1;}
  if( ratio>0.61 &&ratio<= 0.80){ Lux = 0.0128 * CH0-0.0153*CH1;}
  if( ratio>0.80 && ratio<= 1.30){ Lux = 0.00146*CH0-0.00112*CH1;}
  if( ratio>1.30 ){ Lux = 0;}
 
  luxCL=Lux;
  
}

void medir(){
       if (debug==1){ Serial.println ("Midiendo....");}
  sensors_event_t event;
  tsl.getEvent(&event);
  tsl.getLuminosity (&broadband, &infrared);
  if (event.light)
  {
    lux =event.light;
         if (debug==1)  { 
             Serial.print(event.light); Serial.println(" lux");
             Serial.print(broadband); Serial.println(" broadband;");
             Serial.print(infrared); Serial.println(" infrared;");
         }
  }  else  {    if (debug==1)  {Serial.println("Sensor overload"); } }
    delay(1000);
   calularLuxCS();
   calularLuxCL(); //el bueno en este caso es el CL
   irradiancia=(float)broadband*1000/Ccalib;
  
}

void imprimirdatos(void){
  if ((millis()-t_ant_print)>t_print){
         t_ant_print=millis();
  Serial.print("Lux: ");
  Serial.println(lux);
  Serial.print("LuxCL: ");
  Serial.println(luxCL);
  Serial.print("LuxCS: ");
  Serial.println(luxCS);
  Serial.print("broadband: ");
  Serial.println(broadband);
  Serial.print("infrared: ");
  Serial.println(infrared);
  Serial.print("irradiancia: ");
  Serial.println(irradiancia);
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
 }
}

 void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}



/**************************************************************************/
/*
    Configures the gain and integration time for the TSL2561
*/
/**************************************************************************/
void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
   tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  //tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
   //tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("402 ms");
  Serial.println("------------------------------------");
}

void chequeaSolarmax(){
  if (PAC==0 && IDC==0 && IAC==0){
    datosOK=false;
  }
  else{
    datosOK=true;
    }
    if (PAC>4000 ){
      datosOK=false;
     }
  }


void setup() {
  Serial.begin(19200);
  Serial.println();
  mySerial.begin(19200);
  Serial.print( "Esperando 2 minutos");
  delay(1200); //espera dos minutos para dar tiempo al router a coger wifi.
  Serial.println("\nSoftware serial test started");
  Serial.println("\nSolarMax 3000C + Nodemcu:");
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
  IPAddress _ip = IPAddress(192, 168, 1, 53);   ///IP fija 53
  IPAddress _gw = IPAddress(192, 168, 1, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);
  //end-block2
  
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  if (!wifiManager.autoConnect("NodeMCU_3000C", "A12345678")) {
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
  displaySensorDetails();
  configureSensor();
}


void loop() {
  // put your main code here, to run repeatedly:
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
     // digitalClockDisplay();  
    }
  }
  checkinternet();
  if (datosOK){envio_emon();}
  medir();

//check and see if a data packet has come in. 
if ((millis()-t_ant_solarmax)>t_solarmax){
         t_ant_solarmax=millis();
          readpower();
          readenergy();
          chequeaSolarmax();
           datosOK=true;
}
  
  if (debug==1)  {   imprimirdatos();  }

 //4. wifi reconnect
  if (WiFi.status() != WL_CONNECTED){
      WIFI_Connect();
  } 
  
  if (millis()> 86400*2){
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

void readpower(){
  Serial.write("{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}");
  mySerial.write("{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}\r\n");
  //mySerial.println();
  Serial.println();
  delay(1);
  while (mySerial.available() > 0  || cnt<9999) {
   cnt++;
     if (mySerial.available() > 0){
       c= mySerial.read();
       lectura=lectura+char(c);
     }
   }
   cnt=0;
   Serial.print("Millis;");
   Serial.println(millis());
  Serial.println("Lectura:");
  Serial.println(lectura);
  traducepotencia(lectura);

  Serial.println("Otro comando");
  lectura="";
  delay (5000);
  }

void readenergy(){
  
  }
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}





void traducepotencia(String lectura){
    /*int firstEqual = lectura.indexOf('=');
    int secondEqual = firstEqual + 1;
    secondEqual = lectura.indexOf(';', secondEqual);
    String UDC=lectura.substring(firstEqual+1, secondEqual);
    Serial.print("VDC : ");
    Serial.println(UDC);
    unsigned long VDC= hex2int(UDC,UDC.length());
    Serial.println(VDC);
    int Equal3 = lectura.indexOf(';', secondEqual+1);
    String IDCs=lectura.substring(secondEqual+1, Equal3);
    unsigned long IDC= hex2int(IDCs,IDCs.length());
    Serial.print("IDC: ");
    Serial.println(IDC);
        */
        //String split = "hi this is a split test";
    //String split = lectura;
    String UDCs = getValue(lectura, '=', 1);
    //Serial.print ("UDCs:");
    //Serial.println(UDCs);
    UDCs = getValue(UDCs, ';', 0);
    VDC=hex2int(UDCs)/10;
    Serial.print ("VDC:");
    Serial.println(VDC);
    
    
    String IDCs = getValue(lectura, '=', 2);
    //Serial.print ("IDCs:");
    //Serial.println(IDCs);
    IDCs = getValue(IDCs, ';', 0);
    IDC=hex2int(IDCs)/100;
    Serial.print ("IDCs:");
    Serial.println(IDC);

    String UL1 = getValue(lectura, '=', 3);
    //Serial.print ("UL1:");
    //Serial.println(UL1);
    UL1 = getValue(UL1, ';', 0);
    VAC=hex2int(UL1)/10;
    Serial.print ("VAC:");
    Serial.println(VAC);

    String IL1 = getValue(lectura, '=', 4);
    //Serial.print ("IL1:");
    //Serial.println(IL1);
    IL1 = getValue(IL1, ';', 0);
    IAC=hex2int(IL1)/100;
    Serial.print ("IAC:");
    Serial.println(IAC);

    String PACs = getValue(lectura, '=', 5);
   // Serial.print ("PACs:");
   // Serial.println(PACs);
    PACs = getValue(PACs, ';', 0);
    PAC=hex2int(PACs)/2; //da el doble ??
    Serial.print ("PAC:");
    Serial.println(PAC);

    String PRLs = getValue(lectura, '=', 6);
    //Serial.print ("PRLs:");
    //Serial.println(PRLs);
    PRLs = getValue(PRLs, ';', 0);
    PRLs = getValue(PRLs, '|', 0);
    PRL=hex2int(PRLs);
    Serial.print ("PRL:");
    Serial.println(PRL);
    

}


float hex2int(String a)
{
   int i;
   unsigned long val = 0;
   int len=a.length();

   for(i=0;i<len;i++)
      if(a[i] <= 57)
       val += (a[i]-48)*(1<<(4*(len-1-i)));
      else
       val += (a[i]-55)*(1<<(4*(len-1-i)));
   return (float) val;
}


