/*
  Arduino con ethernet shield para control del aire acondicionado del salon
  Miguel Alonso  Octubre 2014
  Fuentes: varios de internet
  - CONNECTIONS: nRF24L01 Modules See:
 http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
   1 - GND
   2 - VCC 3.3V !!! NOT 5V
   3 - CE to Arduino pin 9 ->6
   4 - CSN to Arduino pin 10 ->7
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - UNUSED
 */
 //https://api.xively.com/v2/feeds/130137.csv?key=WQ5gyvvWkWJ5oNLO2G7aOEb8Ex2EeoCWliPU4SjJB68ePyDO
 

#include <SPI.h>
#include <Ethernet.h>
#include <nRF24L01.h>
#include <RF24.h>


float Pot_INV_auto; //potencia del inversor conectado a red Altavista 107 
float Pot_INV_red; //potencia del inversor conectado a red Altavista 107  
String cadena;
#define CE_PIN   9
#define CSN_PIN 10
const uint64_t pipe_read =  0xE8E8F0F0E1LL; //pipes para RF24
const uint64_t pipe_write =  0xE8E8F0F0E2LL; //pipes para RF24
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
int datos_read[10];  // 6 element array holding NRF24L01 readings
int datos_write[10];
 
void setup() {
  Serial.begin(9600);
  radio.begin();
  
  radio.setRetries(15,15);
  radio.setChannel(93);
  radio.setPayloadSize(20);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(0);
 
  
  radio.openReadingPipe(1,pipe_read);
  radio.openWritingPipe(pipe_write);
   radio.startListening();
    
}


void loop() {
 // 0 - Primero lee los datos NRF RF24   
 uint8_t pipe;
 
 datos_write[0] = 9993;
  datos_write[1] = 28;
  datos_write[2] = 28.25;
  datos_write[3] = 28.25;
  datos_write[4] = 28.25;
  datos_write[5] = 28.25;
  datos_write[6] = 28.25;
  datos_write[7] = 28.25;
  datos_write[8] = 9998;
  datos_write[9] = 9999;
  
  
  
  if ( radio.available() )
  {
   // Read the data payload until we've received everything
    bool done = false;
    while (!done){
      // Fetch the data payload
      done = radio.read( datos_read, sizeof(datos_read) );
      Serial.print(" datos_read[0]= ");      
      Serial.println(datos_read[0]);
      Serial.print(" datos_read[9]= ");      
      Serial.println(datos_read[9]);
                }
      radio.stopListening();
      radio.write( datos_write, sizeof(datos_write) );
     radio.startListening();
  }
  
   
 } //fin de loop 
