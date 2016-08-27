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

const char host[] = "altavista107t.sytes.net";
const char usuarioClave[] = "bWlndWVsLmZvdG92b2x0YWljYUBnbWFpbC5jb206MTcyN29zZ3Y2Nw==";
char servidorIP[] = "checkip.dyndns.org";
char servidorActualizacion[] = "dynupdate.no-ip.com";
IPAddress ipActual;
IPAddress ipUltimaEnviada;

byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// the dns server ip
IPAddress dnServer(192, 168, 1, 1);
// the router's gateway address:
IPAddress gateway(192, 168, 1, 1);
// the subnet:
IPAddress subnet(255, 255, 255, 0);

IPAddress ip(192, 168,1, 179);
EthernetServer server(84);
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
const unsigned long postingInterval_emon = 35000; // delay between updates, in milliseconds
unsigned long lastConnectionTime_xiv = 0;             // last time you connected to the server, in milliseconds
boolean lastConnected_xiv = false;                    // state of the connection last time through the main loop
const unsigned long postingInterval_xiv = 60000; // delay between updates, in milliseconds
float Pot_INV_auto; //potencia del inversor conectado a red Altavista 107 
float Pot_INV_red; //potencia del inversor conectado a red Altavista 107  
String cadena;
int j;
unsigned long t_ini; 
float datos_NRF[5];
EnergyMonitor emon1;             // Create an instance
float datos[6];
 
void setup() {
  Serial.begin(9600);
  //98.7 37.4
  emon1.voltage(5, 102.7, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon1.current(0, 61.5);       // Current: input pin, calibration.
  //emon1.voltage(2, 361, 1.7);  // Voltage: input pin, calibration, phase_shift
  //emon1.current(1, 61.5);       // Current: input pin, calibration.
 
//  emon1.voltage(2, 361, 1.7);  // Voltage: input pin, calibration, phase_shift
//  emon1.current(1, 61.5);       // Current: input pin, calibration.

  //Ethernet.begin(mac, ip);
  Ethernet.begin(mac, ip, dnServer, gateway, subnet);
  server.begin();
  Serial.print(F("server IP "));
  Serial.println(Ethernet.localIP());
  
  for (byte n = 0; n <= 3; n++) ipUltimaEnviada[n] = EEPROM.read(n); // Cargamos de EEPROM la ultima IP que se envio.
  Serial.print(F("IP EEPROM:"));
  Serial.println(ipUltimaEnviada);
}


void loop() {
  
   emon1.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  emon1.serialprint();           // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)
  
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
    
  
  
   // 1. listen for incoming Ethernet connections:
 listenForEthernetClients();
  //2. client Xively
  if( (millis() - lastConnectionTime_xiv > postingInterval_xiv) ) {
     Xively2();
    lastConnectionTime_xiv=millis();
       }
  //3. Datos a emoncms en leganes
   if( (millis() - lastConnectionTime_emon > postingInterval_emon) ) {
   envio_emon();
   lastConnectionTime_emon = millis();
    }
  //fin envio emoncms
   
   //update no-ip
  update_no_ip();
  //fin de loop
}
/// fin de loop
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
    int id = client_xiv.parseFloat(); // you can use this to select a specific id
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
    //Serial.println("Disconnecting emoncms...");
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
    boolean currentLineIsBlank = true;
    while (client.connected()) {   
      if (client.available()) {
        char c = client.read();
    // Serial.print(c);
        //read char by char HTTP request//store characters to string
        if (cadena.length() < 50) {
          cadena += c;
          }

         //if HTTP request has ended
         if (c == '\n'&& currentLineIsBlank) {          
           //Serial.println(readString); //print to serial monitor for debuging
     
           client.println("HTTP/1.1 200 OK"); //send new page
           client.println("Content-Type: text/html");
            client.println("Refresh: 20");  // refresh the page automatically every 5 sec
           client.println();     
           client.println("<HTML>");
           client.println("<HEAD>");
           //client.println(F("<meta name='apple-mobile-web-app-capable' content='yes' />"));
           //client.println(F("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />"));
           client.println(F("<link rel='stylesheet' type='text/css' href='http://randomnerdtutorials.com/ethernetcss.css' />"));
           //client.println(F("<TITLE>WebServer Arduino - Miguel Alonso</TITLE>"));
           client.println(F("</HEAD>"));
           client.println(F("<BODY>"));
           client.println(F("<H1>WebServer Arduino Taller- Miguel Alonso</H1>"));
           client.println(F("<hr />"));
           client.println(F("<br />"));
           client.println(F("<a href=\"/?button1on\"\">Bomba ON</a>"));
           client.println(F("<a href=\"/?button1off\"\">Aire OFF</a><br />"));   
           client.println(F("<br />")); 
           client.println(F("<a href=\"/?manual\"\">Manual</a>"));
           client.println(F("<a href=\"/?auto\"\">Automat.</a><br />")); 
           client.println(F("<H2>"));
           client.println(F("</H2>"));
           // output the value of each analog input pin
          /*for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print(F("An input "));
            client.print(analogChannel);
            client.print(F(" is "));
            client.print(sensorReading);
            client.println(F("<br />"));       
          }*/
          // output the values read from remote sensor
          client.println(F("<br />"));       
          client.print("Sensor:");
          client.print("P=");
          client.println(datos_NRF[0]);
          client.print(" S=");      
          client.println(datos_NRF[1]);
          client.print(" PF=");      
          client.println(datos_NRF[2]);
          client.print("V=");      
          client.println(datos_NRF[3]);
          client.print("I=");      
          client.println(datos_NRF[4]);
                   
           
           client.print(F("Datos Xively (red/auto):"));
           client.println(Pot_INV_red);
           client.println(Pot_INV_auto);
           client.println(millis());
           
           
           client.println(F("</BODY>"));
           client.println(F("</HTML>"));
          //controls the Arduino if you press the buttons
            
           break;
         }
         if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
        } 
       }
           delay(1);
           //stopping client
           client.stop();
           Ejecuta_webserver_actions();
          
            //clearing string for next read
            cadena=""; 
    }
}
 
  
  void Ejecuta_webserver_actions( ){
    
    //controls the Arduino if you press the buttons
           if (cadena.indexOf(F("?button1on")) >0){

               //Aire_ON();
                Serial.println("SensorON");
                Serial.print("Estado sensor:");
           }
           if (cadena.indexOf(F("?button1off")) >0){

               //Aire_OFF();
               Serial.println("SensorOFF");
           }
           if (cadena.indexOf(F("?manual")) >0){

              Serial.println(F("Manual=1"));
           }
           if (cadena.indexOf(F("?auto")) >0){

                 Serial.println(F("Manual=0"));
           }
           if (cadena.indexOf(F("?maxwatts=")) >0){
                int colonPosition = cadena.indexOf('=');
                int colonPosition2 = cadena.indexOf('HTTP');
               String valor = cadena.substring(colonPosition+1, colonPosition2);
               //char buf[valor.length()];
              //valor.toCharArray(buf,valor.length()+1);
               //umbral_PAC=atof(buf);

           }
  }
 
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
    //Serial.println(" nada....");
  }
   
  } 
   
   }
 
 IPAddress compruebaIP() {
 
  EthernetClient cliente;
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
 
  EthernetClient cliente;
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
 
