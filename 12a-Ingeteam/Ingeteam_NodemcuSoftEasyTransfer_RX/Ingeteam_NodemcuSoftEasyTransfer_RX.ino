/*
 * programa para el Nodemcu de ingeteam
 * recibe los datos por puerto serie softwareserial D2,D3
 * los datos los lee el arduino por modbus y los envia por serie
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
 
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
*/
//#include "FS.h"
#include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,12,false,64);// RX, TX nodemcu
SoftEasyTransfer ET; 

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  uint32_t ms;
  uint16_t PAC; 
  uint16_t VDC;
  uint16_t VDCbus;
  uint16_t IDC;
  uint16_t VAC;
  uint16_t IAC;
  uint16_t f;
  uint16_t cosf;
  uint16_t Etotal;
  uint16_t Horas;
  uint16_t Nconex;
  uint16_t A3;
  uint16_t A4;
  uint16_t A5;
  uint16_t A6;
  uint16_t A7;
  uint16_t A8;

};

RECEIVE_DATA_STRUCTURE mydata;

void setup(){
  Serial.begin(9600);
  mySerial.begin(9600);
  //start the library, pass in the data details and the name of the serial port.
  //ET.begin(details(mydata), &mySerial);
   ET.begin((byte*)&mydata,38, &mySerial);
  Serial.print( "tamano:");
  int tam=sizeof(mydata);
  Serial.print( tam);
}

void loop(){
  //check and see if a data packet has come in. 
  if(ET.receiveData()){
  Serial.print ("milisegundos: ");
  Serial.print(mydata.ms);   
  Serial.print (" PAC: ");
  Serial.println(mydata.PAC); 
 }
  //delay(50);
   
  
}
