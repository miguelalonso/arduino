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

IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
 //IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov NTP server
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

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
int datos_write[15];  // 2 element array holding Joystick readings
int datos_read[15];  // 2 element array holding Joystick readings

String horaS="";
String fechaS="";

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
   
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  Udp.begin(localPort);
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  //datos_write[0] = 0;//manual=101;man_giraNorte=102;man_giraSur=103;man_giraEste=104;man_giraOeste=105;
  /*datos_write[1] = 28;
  datos_write[2] = 28.25;
  datos_write[3] = 28.25;
  datos_write[4] = 28.25;
  datos_write[5] = 28.25;
  datos_write[6] = 28.25;
  datos_write[7] = 28.25;
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
      horainternet();
      internet();
     
     radio.powerDown();
     delay(500);
     radio.powerUp();
     delay(500);      
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
  }
}
  
void horainternet(void){
   if ((millis()-t_ant_internet)>t_internet){
           t_ant_internet=millis();
           sendNTPpacket(timeServer); // send an NTP packet to a time server
        delay(2000); 
        Serial.println(F("Pidiendo tiempo de internet.... ") );
        if ( Udp.parsePacket() ) {  
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    Serial.print(F("Seconds since Jan 1 1900 = ") );
    Serial.println(secsSince1900);               

    // now convert NTP time into everyday time:
    Serial.print(F("Unix time = "));
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    // print Unix time:
    Serial.println(epoch);                               


    // print the hour, minute and second:
    Serial.print(F("The UTC time is "));       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');  
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':'); 
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch %60); // print the second
  }
    }
}//fin de horainternet

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
            client.println("Refresh: 5");  // refresh the page automatically every 5 sec
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
           
           client.println("<p><br />");  
           client.println(F("<a href=\"/?button1Norte\"\">Manual giro Norte</a>"));
           client.println("<a href=\"/?button1Sur\"\">Manual Giro Sur</a><br />");   
           client.println("<br />");     
           client.println("<br />"); 
           client.println("<a href=\"/?button2Este\"\">Manual Giro Este</a>");
           client.println("<a href=\"/?button2Oeste\"\">Manual Giro Oeste</a><br />"); 
           client.println("<p>Seguidor solar PanTilt - Modificado para motores Stepper</p>"); 
           client.println("<p>Estado:"); 
           client.println(estado); 
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
      
         }
    
 String stringDigits(int digits){
  String number = "";
   if(digits < 10){   number="0";   }
   number+=String(digits);
  return number;
}

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress& address)
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
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}

