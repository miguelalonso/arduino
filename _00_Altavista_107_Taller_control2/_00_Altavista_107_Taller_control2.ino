/*
  Arduino con ethernet shield para control del aire acondicionado del salon
  Miguel Alonso  Octubre 2014
  Fuentes: varios de internet
 */
 //https://api.xively.com/v2/feeds/130137.csv?key=WQ5gyvvWkWJ5oNLO2G7aOEb8Ex2EeoCWliPU4SjJB68ePyDO
 
#include "EmonLib.h"             // Include Emon Library
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <EthernetUdp.h>
#include <Time.h>

#define PV_PIN 7 //rele logica negativa
//#define FASE_PIN 6 //rele logica positiva LOW activado
#define PIN_MANUAL 6
#define NEUTRO_PIN 5 //rele logica negativa NO Utilizado
#define LED_PIN 8
//#define MAN_BOMBA_ON_PIN 7 //control por botones
//#define MAN_AC_ON_PIN 8
#define SERIAL_DEBUG 1 //para imprimir a puerto serie o no

byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEC};
// the dns server ip
IPAddress dnServer(192, 168, 1,1);
// the router's gateway address:
IPAddress gateway(192, 168, 1, 1);
// the subnet:
IPAddress subnet(255, 255, 255, 0);

//IPAddress ip(192, 168,2, 178);
IPAddress ip(192, 168,1, 184);
//byte gateway[] = { 192, 168, 1, 2 };                   // internet access via router
//byte dns[] = { 192, 168, 1, 1 };                   // internet access via router

//byte subnet[] = { 255, 255, 255, 0 };                  //subnet mask
EthernetServer server(84);
unsigned int localPort = 2391;      // local port to listen for UDP packets
EthernetClient client_emon;
IPAddress server_emon(163,117,157,189); //ordenador jaen-espectros2
//char server_emon[] = "emoncms.sytes.net";
const int  feedID          =    130137; // datos Xively
const int  streamCount     =      4; // Number of data streams to get
EthernetClient client_xiv;
char server_xiv[] = "api.xively.com"; 
int streamData[streamCount];    // change float to long if needed for your data
unsigned long lastConnectionTime_emon = 0;             // last time you connected to the server, in milliseconds
boolean lastConnected_emon = false;                    // state of the connection last time through the main loop
const unsigned long postingInterval_emon = 58000; // delay between updates, in milliseconds
unsigned long lastConnectionTime_xiv = 0;             // last time you connected to the server, in milliseconds
boolean lastConnected_xiv = false;                    // state of the connection last time through the main loop
const unsigned long postingInterval_xiv = 80000; // delay between updates, in milliseconds
float Pot_INV_auto; //potencia del bomba conectado a red Altavista 107 
float Pot_INV_red; //potencia del bomba conectado a red Altavista 107  
//String readString=String(100);

int j;
unsigned long t_ini; 
float datos_NRF[5];
EnergyMonitor emon1;             // Create an instance
float datos[6];
 
//IPAddress timeServer(64,113,32,5); // time-b.timefreq.bldrdoc.gov NTP server
IPAddress timeServer(132, 163, 4, 101); // time-c.timefreq.bldrdoc.gov NTP server
//IPAddress timeServer(158, 227, 98, 15); // time-c.timefreq.bldrdoc.gov NTP server
//IPAddress timeServer(216, 23, 247, 62);

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
const long timeZoneOffset = 0L;   
unsigned int ntpSyncTime = 600;
unsigned long ntpLastUpdate = 0;
unsigned long sensorLastUpdate = 0;
time_t prevDisplay = 0; 
EthernetUDP Udp;
unsigned long t_ant_internet=0;
int t_internet=200; //teimpo de espera para pedir la hora de internet

int estado_ntp=0;
float horaini1=12;
float horafin1=13;
float horaini2=16;
float horafin2=17;
bool manual=false;
bool manual_anterior=false;
float hora;
bool bombaon=false;
bool bombared=false;
bool manualbombaon=false;
bool manualbombared=false;

int ledState = LOW;             // ledState used to set the LED
unsigned long previousMillis = 0;        // will store last time LED was updated
 long interval = 1000;           // interval at which to blink (milliseconds)


void setup() {
  Serial.begin(9600);
  pinMode(PV_PIN, OUTPUT);
  pinMode(PIN_MANUAL, OUTPUT);
  pinMode(NEUTRO_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
//  pinMode(MAN_BOMBA_ON_PIN, INPUT);
 // pinMode( MAN_AC_ON_PIN, INPUT);
  digitalWrite(PV_PIN, HIGH); //PV a inversor
  digitalWrite(PIN_MANUAL, LOW);
  digitalWrite(NEUTRO_PIN, LOW);
 
  emon1.voltage(5, 102.7, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon1.current(0, 61.5);       // Current: input pin, calibration.
 
 // Ethernet.begin(mac, ip);
  //Ethernet.begin(mac, ip, dns, gateway); 
  Ethernet.begin(mac, ip, dnServer, gateway, subnet);
  server.begin();
  Serial.print(F("server IP "));
  Serial.println(Ethernet.localIP());
  
   //Try to get the date and time
   int trys=0;
   while(!getTimeAndDate() && trys<10) {
     trys++;
     //delay(1000);
       }
       //write_eprom(); //solo para incializar la primera vez
       //ler valore de eeprom
       read_eprom();
       if (manual==255){
       Serial.println("EEPROM no inicializada");
        float horaini1=12;
        float horafin1=13;
        float horaini2=16;
        float horafin2=17;
        bool manual=false;
        write_eprom();
       }
}

void loop() {
  emon1.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  //if(SERIAL_DEBUG){  emon1.serialprint();}           // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)
  float realPower       = emon1.realPower;        //extract Real Power into variable
  float apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
  float powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
  float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  float Irms            = emon1.Irms;             //extract Irms into Variable
  datos_NRF[0]    = emon1.realPower;        
  datos_NRF[1]    = emon1.apparentPower;    
  datos_NRF[2]    = emon1.powerFactor;      
  datos_NRF[3]    = emon1.Vrms;             
  datos_NRF[4]    = emon1.Irms; 
  datos_NRF[5]    = 2;      
  
 listenForEthernetClients();
 xively();
 emon();
 update_ntp();
 controla_bomba();
}
/// fin de loop
void xively(){
  if( (millis() - lastConnectionTime_xiv > postingInterval_xiv) ) {
     Xively2();
    lastConnectionTime_xiv=millis();
       }
}
void emon(){
  if( (millis() - lastConnectionTime_emon > postingInterval_emon) ) {
   envio_emon();
   lastConnectionTime_emon = millis();
    }
  }
  
void Xively2(void){
  
    if( (millis() - lastConnectionTime_xiv > postingInterval_xiv) ) {
    lastConnectionTime_xiv=millis();
      if( getFeed(feedID, streamCount) == true)
   {
      for(int id = 0; id < streamCount; id++){
        Serial.println( streamData[id]);
      }
    
   }
   else
   {
      Serial.println(F("Unable feed"));
       lastConnectionTime_xiv=millis();//para evitar bucles si falla
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
  //client_xiv.flush();
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
    if(!client_emon.connected() && (millis() - lastConnectionTime_emon > postingInterval_emon) ) {
    sendData_emon();
    lastConnectionTime_emon = millis();
    }
  lastConnected_emon = client_emon.connected();
}

//envio de datos a emoncms
void sendData_emon(void){
  // if there's a successful connection:
  if (client_emon.connect(server_emon, 80)) {
    //Serial.println("Connecting emoncms...");
    // send the HTTP PUT request:
    //client_emon.println(F("GET /emoncms/input/post.json?apikey=d3779ca1d8b1ae37c17b60012f585119&node=107&json={Pot_INV_red:1583.00,Pot_INV_auto:709.00}"));
    client_emon.print(F("GET /emoncms_spectrum/input/post.json?apikey=9bbe8aedfc502644f616dd98e3b4f737&node=107&json={Pot_INV_red"));
    client_emon.print(F(":"));
    client_emon.print(Pot_INV_red);
    client_emon.print(F(",Pot_INV_autoT:")); 
    client_emon.print(Pot_INV_auto); 
     client_emon.print(F(",Bombared:")); 
    client_emon.print(bombared); 
     client_emon.print(F(",Bombaon:")); 
    client_emon.print(bombaon); 
    client_emon.print(F(",Manual:")); 
    client_emon.print(manual); 
    
    client_emon.print(F(",PAC_taller:")); 
    client_emon.print(datos_NRF[0]); 
    client_emon.print(F(",S_taller:")); 
    client_emon.print(datos_NRF[1]);
    client_emon.print(F(",PF_taller:")); 
    client_emon.print(datos_NRF[2]);
    client_emon.print(F(",VAC_taller:")); 
    client_emon.print(datos_NRF[3]);
    client_emon.print(F(",IAC_taller:")); 
    client_emon.print(datos_NRF[4]);
       
    client_emon.println(F("}"));
    client_emon.println(" HTTP/1.1");
    //client_emon.println("Host: 163.117.157.189");
    //client_emon.println("User-Agent: Arduino-ethernet");
    client_emon.println(F("Connection: close"));
    client_emon.println();
Serial.println(F("Enviado emoncms"));
    // note the time that the connection was made:
    lastConnectionTime_emon = millis();
  } 
  else {
    // if you couldn't make a connection:
    Serial.println(F("Fallo emoncms"));
//    Serial.println("Disconnecting emoncms...");
    client_emon.stop();
    //client_emon.flush();
    delay(100);
  }
}
  
  
  //
  // web server
void listenForEthernetClients() {
   // listen for incoming clients
  EthernetClient client = server.available();
 client = server.available();
  if (client) {
   Serial.println(F("new client"));
    String readString = "";
    while (client.connected()) {   
      if (client.available()) {
        char c = client.read();
    Serial.print(c);
        //read char by char HTTP request//store characters to string
        if (readString.length() < 100) {
          readString += c;
          }

         //if HTTP request has ended
         if (c == '\n') {          
Serial.println("String:");Serial.println(readString.length());
if (readString.length() <=0) {readString = "";}
           Serial.println(readString); //print to serial monitor for debuging
     
           client.println("HTTP/1.1 200 OK"); //send new page
           client.println("Content-Type: text/html");
           client.println("Refresh: 20");  // refresh the page automatically every 5 sec
           client.println();     
           client.println("<HTML>");
            client.println(F("<BODY>"));
           
           client.println(F("<H1>WebServer Arduino Taller- Miguel Alonso</H1>"));
           client.println(F("<hr />"));
           client.println(F("<a href=\"/?bombaon\"\">Bomba ON   </a>"));
           client.println(F("<a href=\"/?bombaoff\"\">&nbsp;&nbsp;&nbsp;&nbsp;Bomba OFF</a><br />"));   
           client.println(F("<a href=\"/?manual\"\">Manual </a>"));
           client.println(F("<a href=\"/?auto\"\">&nbsp;&nbsp;&nbsp;&nbsp;Automatico </a><br />")); 
           client.println(F("<a href=\"/?bombaredon\"\">BombaRed ON  </a>"));
           client.println(F("<a href=\"/?bombaredoff\"\">&nbsp;&nbsp;&nbsp;&nbsp;BombaRed OFF</a><br />")); 
           client.print(F("Horas de inicio y fin de conexionado del bomba: "));
           client.println(F("<br />"));
           client.println(F("<FORM>Hora incicio 1. <input type=text name=horaini1 size=4 value="));
           client.println(horaini1);
           client.println(F(" > <input type=submit value=Enter> </form> "));
           client.println(F("<FORM>Hora fin 1. <input type=text name=horafin1 size=4 value="));
           client.println(horafin1);
           client.println(F(" > <input type=submit value=Enter> </form> "));
           client.println(F("<FORM>Hora incicio 2. <input type=text name=horaini2 size=4 value="));
           client.println(horaini2);
           client.println(F(" > <input type=submit value=Enter> </form> "));
           client.println(F("<FORM>Hora fin 2. <input type=text name=horafin2 size=4 value="));
           client.println(horafin2);
           client.println(F(" > <input type=submit value=Enter> </form> "));
                      
          client.print(F("Potencia Autoconsumo:"));
          client.print(F("P="));
          client.println(datos_NRF[0]);
          client.print(F(" S="));      
          client.println(datos_NRF[1]);
          client.print(F(" PF="));      
          client.println(datos_NRF[2]);
          client.print(F("V="));      
          client.println(datos_NRF[3]);
          client.print(F("I="));      
          client.println(datos_NRF[4]);
          client.print(F("Datos Xively (red/auto):"));
          client.println(Pot_INV_red);
          client.println(Pot_INV_auto);
           
           //client.println(millis());
              client.print(F("<p> Manual:"));
              client.print(manual); 
              client.print(F(" BOMBA_ON:"));
              client.print(bombaon); 
              client.print(F(" BOMBA_RED:"));
              client.print(bombared); 
              client.print(F(" horaini1:"));
              client.print(horaini1); 
              client.print(F(" horafin1:"));
              client.print(horafin1);
              client.print(F(" horaini2:"));
              client.print(horaini2); 
              client.print(F(" horafin2:"));
              client.print(horafin2);
              client.print(F("  Hora actual:"));
              client.print(hour());
              client.print(F(":"));
              client.print(minute());
              client.print(F(":"));
              client.print(second()); 
              
          client.println(F("</BODY>"));
           client.println(F("</HTML>"));
          //controls the Arduino if you press the buttons
           //break;
         //}
         
        
           delay(1);
           //stopping client
           client.stop();
           
          // Ejecuta_webserver_actions();
           //controls the Arduino if you press the buttons
           if (readString.indexOf(F("?bombaon")) >0){
              //bomba_ON();
                Serial.println("bombaON");
                manualbombaon=true;
           }
           if (readString.indexOf(F("?bombaoff")) >0){
              //bomba_OFF();
               Serial.println("bombaOFF");
               manualbombaon=false;
           }
           
           if (readString.indexOf(F("?bombaredon")) >0){
              //bomba_ON();
                Serial.println("bombaredON");
                manualbombared=true;
           }
           if (readString.indexOf(F("?bombaredoff")) >0){
              //bomba_OFF();
               Serial.println("bombaredOFF");
               manualbombared=false;
           }
           
           if (readString.indexOf(F("?manual")) >0){
              manual=true;
              digitalWrite (PIN_MANUAL,HIGH);
              Serial.println(F("Manual=1"));
              write_eprom();
           }
           if (readString.indexOf(F("?auto")) >0){
                manual=false;
                digitalWrite (PIN_MANUAL,LOW);
                 Serial.println(F("Manual=0"));
                 write_eprom();
           }
           if (readString.indexOf(F("?horaini1=")) >0){
                int colonPosition = readString.indexOf('=');
                int colonPosition2 = readString.indexOf('HTTP');
               String valor = readString.substring(colonPosition+1, colonPosition2);
               horaini1 = double(valor.toFloat());
               write_eprom();
           }
           if (readString.indexOf(F("?horaini2=")) >0){
                int colonPosition = readString.indexOf('=');
                int colonPosition2 = readString.indexOf('HTTP');
               String valor = readString.substring(colonPosition+1, colonPosition2);
               horaini2 = double(valor.toFloat());
               write_eprom();
           }
            if (readString.indexOf(F("?horafin1=")) >0){
                int colonPosition = readString.indexOf('=');
                int colonPosition2 = readString.indexOf('HTTP');
               String valor = readString.substring(colonPosition+1, colonPosition2);
               horafin1 = double(valor.toFloat());
               write_eprom();
           }
           if (readString.indexOf(F("?horafin2=")) >0){
                int colonPosition = readString.indexOf('=');
                int colonPosition2 = readString.indexOf('HTTP');
               String valor = readString.substring(colonPosition+1, colonPosition2);
               horafin2 = double(valor.toFloat());
               write_eprom();
           }

          ////////////////////////////////////////
          readString=""; 
             
         }
       }
    }
}
}
 
 
// Do not alter this function, it is used by the system
int getTimeAndDate() {
   int flag=0;
   Udp.begin(localPort);
   sendNTPpacket(timeServer);
   delay(1000);
   if (Udp.parsePacket()){
     Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
     unsigned long highWord, lowWord, epoch;
     highWord = word(packetBuffer[40], packetBuffer[41]);
     lowWord = word(packetBuffer[42], packetBuffer[43]);  
     epoch = highWord << 16 | lowWord;
     epoch = epoch - 2208988800 + timeZoneOffset;
     flag=1;
     setTime(epoch);
     ntpLastUpdate = now();
      estado_ntp=1;
   }
    ntpLastUpdate = now();//para evitar cc
   return flag;
}

// Do not alter this function, it is used by the system
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;		   
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}
void update_ntp(){
// Update the time via NTP server as often as the time you set at the top
    if(now()-ntpLastUpdate > ntpSyncTime) {
      int trys=0;
      while(!getTimeAndDate() && trys<10){
        trys++;
      }
      if(trys<10){
        Serial.println(F("ntp server update success"));
        estado_ntp=1;
      }
      else{
        Serial.println(F("ntp server update failed"));
        estado_ntp=0;
       }
    }
    
    // Display the time if it has changed by more than a second.
    if( (now() - prevDisplay) >5){ //print tiem every 5 s
      prevDisplay = now();
      clockDisplay();  
    }   
 }
 
 // Clock display of the time and date (Basic)
void clockDisplay(){
  if (SERIAL_DEBUG){
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  
  Serial.print(F(" Manual:"));
  Serial.print(manual); 
  Serial.print(F(" BOMBA_ON:"));
  Serial.print(bombaon); 
   Serial.print(F(" BOMBA_RED:"));
  Serial.print(bombared); 
  Serial.print(F(" horaini1:"));
  Serial.print(horaini1); 
  Serial.print(F(" horafin1:"));
  Serial.print(horafin1);
  Serial.print(F(" horaini2:"));
  Serial.print(horaini2); 
  Serial.print(F(" horafin2:"));
  Serial.print(horafin2); 
  Serial.println(); 
  }
}

// Utility function for clock display: prints preceding colon and leading 0
void printDigits(int digits){
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void controla_bomba(){
  /*
  //control por botones
  bool pin_bombaon=digitalRead( MAN_BOMBA_ON_PIN);
  bool pin_bombaac=digitalRead(  MAN_AC_ON_PIN);

      if (pin_bombaon || pin_bombaac){
        manual_anterior=manual;
        manual=true;
        estado_ntp=true;
        if(pin_bombaon){manualbombaon=true;}
        if(pin_bombaac){manualbombared=true;}
        if(pin_bombaon && pin_bombaac){manualbombared=true;manualbombaon=false;}
     }else{manual=manual_anterior;manualbombared=false;manualbombaon=false;}
    */  
      if (estado_ntp==false && !manual){  //no hay hora
        digitalWrite(PV_PIN, HIGH); //PV a inversor NC
       // digitalWrite(FASE_PIN, LOW);
        digitalWrite(NEUTRO_PIN, LOW);
      }
      else {
      hora=hour()+float(minute())/60;
      
        if(!manual){
          bool intervalo_1=(hora >horaini1 && hora<horafin1 && !manual);
          bool intervalo_2=(hora >horaini2 && hora<horafin2 && !manual);
          if (intervalo_1 || intervalo_2){ bomba_on();}else{bomba_off();}
        }
        
        if (manual){
          
          if (manualbombaon && manualbombared){manualbombared=true;manualbombaon=false;} 
          //si la PV está a la bomba entonces ponerla al inversor
          //prevalece el poner en manual la bomba a la red
          
           if (manualbombaon){  bomba_on();  interval=500; }
             else{bomba_off();interval=1000;}
             
            if (manualbombared){ bomba_red_on(); interval=100;} 
             else {bomba_red_off();interval=1000;}
         }
        } //fin nde else ntp==0




unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW && manual)
      ledState = HIGH;
    else
      ledState = LOW;
    digitalWrite(LED_PIN, ledState);
  }

}

void bomba_on(){
   if(!bombaon){
    bomba_red_off();
    delay(1000);
    digitalWrite(PV_PIN, LOW); //PV a bomba RELE activado NC
    bombaon=true;
    }
}


void bomba_off(){
  if (bombaon){
    digitalWrite(PV_PIN, HIGH); //PV al inversor OJo Rele logica negativa 
    bombaon=false;
    }
}


void bomba_red_on(){
  //para poner la red AC a la bomba, primero asegurarse de que PV está al bomba
  if (!bombared){
  bomba_off();
  delay(1000);
 // digitalWrite(FASE_PIN, HIGH);
  delay(100);
  digitalWrite(NEUTRO_PIN, HIGH);
  bombared=true;
  }
}

void bomba_red_off(){
  if(bombared){
  delay(1000);
 // digitalWrite(FASE_PIN, LOW);
  delay(100);
  digitalWrite(NEUTRO_PIN, LOW);
  bombared=false;
  }
}

void write_eprom(){
   EEPROM.write(4, manual);
   EEPROM.write(5, horaini1);
   EEPROM.write(6, horafin1);
   EEPROM.write(7, horaini2);
   EEPROM.write(8, horafin2);
    
  }
  void read_eprom(){
   manual= EEPROM.read(4);
   horaini1= EEPROM.read(5);
   horafin1= EEPROM.read(6);
   horaini2=EEPROM.read(7);
   horafin2=EEPROM.read(8);
    
  }
