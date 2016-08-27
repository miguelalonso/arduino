// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3
/*
 - CONNECTIONS: nRF24L01 Modules See:
 http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
   1 - GND
   2 - VCC 3.3V !!! NOT 5V
   3 - CE to Arduino pin 6
   4 - CSN to Arduino pin 7
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - UNUSED
   */
#include "EmonLib.h"             // Include Emon Library
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   6
#define CSN_PIN 7

// NOTE: the "LL" at the end of the constant is "LongLong" type
const uint64_t pipe = 0xE8E8F0F0E2LL; // Define the transmit pipe
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

EnergyMonitor emon1;             // Create an instance
float datos[6];
unsigned long t_inicio = millis();
 
void setup()
{  
  Serial.begin(9600);
  
  emon1.voltage(5, 206.7, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon1.current(1, 37.4);       // Current: input pin, calibration.
  radio.begin();
   radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(pipe);
}

void loop()
{
  emon1.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  emon1.serialprint();           // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)
  
  float realPower       = emon1.realPower;        //extract Real Power into variable
  float apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
  float powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
  float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  float Irms            = emon1.Irms;             //extract Irms into Variable
  
  datos[0]    = (datos[0]+emon1.realPower)/2;        //extract Real Power into variable
  datos[1]    = (datos[1]+emon1.apparentPower)/2;    //extract Apparent Power into variable
  datos[2]    = (datos[2]+emon1.powerFactor)/2;      //extract Power Factor into Variable
  datos[3]    = (datos[3]+emon1.Vrms)/2;             //extract Vrms into Variable
  datos[4]    = (datos[4]+emon1.Irms)/2;        //extract Irms into Variable
  datos[5]    = 2;    
   
  if (millis() - t_inicio > 7500 ){ //envia datos cada 5 s
   radio.write( datos, sizeof(datos) );
   t_inicio = millis();
  datos[0]    = emon1.realPower;        
  datos[1]    = emon1.apparentPower;    
  datos[2]    = emon1.powerFactor;      
  datos[3]    = emon1.Vrms;             
  datos[4]    = emon1.Irms; 
 datos[5]    = 2;      
    }
  
    
  
}
