//el que lee

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //physical mac address
byte ip[] = { 192, 168, 1, 177 };                      // ip in lan (that's what you need to use in your browser. ("192.168.1.178")
byte gateway[] = { 192, 168, 1, 1 };                   // internet access via router
byte subnet[] = { 255, 255, 255, 0 };                  //subnet mask
EthernetServer server(82);                             //server port     
unsigned int localPort = 8888;      // local port to listen for UDP packets
String readString;
String estado="Automatico";
String estado_ntp="updated";

IPAddress timeServer(132, 163, 4, 102); // time-a.timefreq.bldrdoc.gov NTP server
 //IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov NTP server
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
const long timeZoneOffset = 0L;   
unsigned int ntpSyncTime = 60;
unsigned long ntpLastUpdate = 0;   
time_t prevDisplay = 0; 

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
unsigned long t_ant_internet=0;
int t_internet=20000; //teimpo de espera para pedir la hora de internet
unsigned long t_ant_print=0;
int t_print=3000;

/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   6 //para que funcione la ethernet shield
#define CSN_PIN 7


// NOTE: the "LL" at the end of the constant is "LongLong" type
const uint64_t pipe_write = 0xE8E8F0F0E1LL; // Define the transmit pipe
const uint64_t pipe_read =  0xE8E8F0F0E2LL; //pipes para RF24
int j=0;

/*-----( Declare objects )-----*/
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
/*-----( Declare Variables )-----*/
int datos_write[15];  // 2 element array holding  readings
int datos_read[15];  // 2 element array holding  readings

String horaS="";
String fechaS="";
String horaWeb="";
String fechaWeb="";

void setup()   /****** SETUP: RUNS ONCE ******/
{
  Serial.begin(9600);
  radio.begin();
  
  radio.setRetries(15,15);
  radio.setChannel(93);
  radio.setPayloadSize(128);
  //radio.setDataRate(RF24_250KBPS);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(1);
 
  radio.openWritingPipe(pipe_write);
  radio.openReadingPipe(1,pipe_read);
  radio.startListening();
  
  Ethernet.begin(mac,ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
   //Try to get the date and time
   int trys=0;
   while(!getTimeAndDate() && trys<10) {
     trys++;
   }
   
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  //datos_write[0] = 0;//manual=101;man_giraNorte=102;man_giraSur=103;man_giraEste=104;man_giraOeste=105;
  /*datos_write[1] = 28; //Timeupdated
  datos_write[2] = 28.25; //horaWeb
  datos_write[3] = 28.25; //minWeb
  datos_write[4] = 28.25; //secWeb
  datos_write[5] = 28.25; //día Web
  datos_write[6] = 28.25; //mes Web
  datos_write[7] = 28.25; //añoWeb
  datos_write[8] = 30.34;
  datos_write[9] = 2015;
  */
 radio.stopListening();

    // Take the time, and send it.  This will block until complete
    unsigned long time = millis();
     //Serial.print(" Enviando datos..."); 
     radio.write( datos_write, sizeof(datos_write) );

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 1+(radio.getMaxTimeout()/1000) )
        timeout = true;

    // Describe the results
    if ( timeout )
    {
      //Serial.println("Failed, response timed out.\n\r");
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
       radio.read( datos_read, sizeof(datos_read) );
       fechahoraseguidor();
       imprimirdatos();
    }
    internet();
     
     radio.powerDown();
     delay(500);
     radio.powerUp();
     delay(500);  
 
 // Update the time via NTP server as often as the time you set at the top
    if(now()-ntpLastUpdate > ntpSyncTime) {
      int trys=0;
      while(!getTimeAndDate() && trys<10){
        trys++;
      }
      if(trys<10){
        Serial.println(F("ntp server update success"));
        estado_ntp=F("Updated");
        datos_write[1] = 1;
      }
      else{
        Serial.println(F("ntp server update failed"));
        estado_ntp=F("Failed");
        datos_write[1] = 0;
      }
    }
    
    // Display the time if it has changed by more than a second.
    if( now() != prevDisplay){
      prevDisplay = now();
      clockDisplay();  
    }    
}//--(end main loop )---

void imprimirdatos(void){
  if ((millis()-t_ant_print)>t_print){
             t_ant_print=millis();
    for(j=0;j<9;j++){
        Serial.print(F(" datos_read[")); 
         Serial.print(j);  
         Serial.print("]:");     
         Serial.print(datos_read[j]);
        }
        Serial.print(F(" datos_read[9]:"));      
        Serial.println(datos_read[9]);
        Serial.println(datos_write[1]);
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
   }
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

// Clock display of the time and date (Basic)
void clockDisplay(){
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
  
 if( datos_write[1] == 1){
    datos_write[2] = hour();
    datos_write[3] = minute();
    datos_write[4] = second();
    datos_write[5] = day();
    datos_write[6] = month();
    datos_write[7] = year();
   }
}

// Utility function for clock display: prints preceding colon and leading 0
void printDigits(int digits){
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void internet(void){
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {   
      if (client.available()) {
        char c = client.read();
     
        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          //Serial.print(c);
         }

         //if HTTP request has ended
         if (c == '\n') {          
           Serial.println(readString); //print to serial monitor for debuging
     
           client.println("HTTP/1.1 200 OK"); //send new page
           client.println("Content-Type: text/html");
           client.println(F("Connnection: close"));
           client.println();
           client.println(F("<!DOCTYPE HTML>"));
            // add a meta refresh tag, so the browser pulls again every x seconds:
          client.print(F("<meta http-equiv=\"refresh\" content=\""));
          client.print("5");
          client.println(F("; url=/\">"));
          client.println();     
           client.println("<HTML>");
           client.println("<HEAD>");
           client.println(F("<meta name='apple-mobile-web-app-capable' content='yes' />"));
           client.println(F("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />"));
           client.println(F("<link rel='stylesheet' type='text/css' href='http://randomnerdtutorials.com/ethernetcss.css' />"));
           client.println(F("<TITLE>WebServer Arduino SunTracker PanTilt - Miguel Alonso - Ciemat</TITLE>"));
           client.println("</HEAD>");
           client.println("<BODY>");
           client.println("<H1>WebServer Arduino SunTracker PanTilt - Miguel Alonso</H1>");
           client.println("<hr />");
           client.println("<br />");  
           client.println("<H2>Hora En Seguidor  ");
           client.println(horaS);
           client.println(" ");
           client.println(fechaS);
           client.println("</H2>");
           client.println("<a href=\"/?button1Auto\"\">Autom&#224tico</a>");
           client.println("<a href=\"/?sincronizetime\"\">Sincronizar reloj</a>");
           client.println("<p><br />");  
           client.println(F("<a href=\"/?button1Norte\"\">Manual giro Norte</a>"));
           client.println("<a href=\"/?button1Sur\"\">Manual Giro Sur</a><br />");   
           client.println("<br />");     
           client.println("<br />"); 
           client.println("<a href=\"/?button2Este\"\">Manual Giro Este</a>");
           client.println("<a href=\"/?button2Oeste\"\">Manual Giro Oeste</a><br />"); 
           client.println(F("<p>Seguidor solar PanTilt - Modificado para motores Stepper</p>")); 
           client.println("<p>Estado:"); 
           client.println(estado);
           client.println(F(" - Web Time:")); 
           client.println(estado_ntp);
            client.println(F(" :: ")); 
           client.println(horaWeb);
           client.println(" ");
           client.println(fechaWeb);
           client.println(F(" Finales Carrera: "));
           client.println(datos_read[10]); //esta´n los finales de carrera 0010
           client.println("<br />"); 
           client.println("<p>");
           client.println("<H2>");
           client.println("Acimut Sol:");
           client.println(datos_read[6]/100.0,2);
           client.println("Cenit Sol:");
           client.println(datos_read[7]/100.0,2); 
           client.println("</H2>");
           client.println("</p>");
           client.println("Acimut Seg:");
           client.println(datos_read[8]/100.0,2);
           client.println("Cenit Seg:");
           client.println(datos_read[9]/100.0,2); 
           client.println("</BODY>");
           client.println("</HTML>");
     
           delay(1);
           //stopping client
           client.stop();
           //controls the Arduino if you press the buttons
           if (readString.indexOf("?button1Auto") >0){
                datos_write[0]=0;
                estado="AUTO";
           }
           if (readString.indexOf("?button1Norte") >0){
                datos_write[0]=102;
                estado="GIRO NORTE";
           }
           if (readString.indexOf("?button1Sur") >0){
              datos_write[0]=103;
               estado="GIRO SUR";
           }
           if (readString.indexOf("?button2Este") >0){
             datos_write[0]=104;
             estado="GIRO ESTE";
               
           }
           if (readString.indexOf("?button2Oeste") >0){
             datos_write[0]=105;
             estado="GIRO OESTE";
               
           }
           if (readString.indexOf("?sincronizetime") >0){
             datos_write[1]=2;
             estado="GIRO OESTE";
               
           }else{
             if (datos_write[1]==2){datos_write[1]=0;}
             }
            //clearing string for next read
            readString="";  
           
         }
       }
    }
}
  }
  
  void fechahoraseguidor(void){
      horaS=String(datos_read[0]);
      horaS+=":";
      horaS+=stringDigits(datos_read[1]);
      horaS+=":";
      horaS+=stringDigits(datos_read[2]);
      
      fechaS=stringDigits(datos_read[3]);;
      fechaS+="/";
      fechaS+=stringDigits(datos_read[4]);
      fechaS+="/";
      fechaS+=stringDigits(datos_read[5]);
      
      horaWeb=String(hour());
      horaWeb+=":";
      horaWeb+=stringDigits(minute());
      horaWeb+=":";
      horaWeb+=stringDigits(second());
      
      fechaWeb=stringDigits(day());;
      fechaWeb+="/";
      fechaWeb+=stringDigits(month());
      fechaWeb+="/";
      fechaWeb+=stringDigits(year());
 }
    
 String stringDigits(int digits){
  String number = "";
   if(digits < 10){   number="0";   }
   number+=String(digits);
  return number;
}


