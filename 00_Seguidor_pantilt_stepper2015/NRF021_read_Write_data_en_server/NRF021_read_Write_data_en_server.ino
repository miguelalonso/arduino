//el que lee

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   9
#define CSN_PIN 10


// NOTE: the "LL" at the end of the constant is "LongLong" type
const uint64_t pipe_write = 0xE8E8F0F0E1LL; // Define the transmit pipe
const uint64_t pipe_read =  0xE8E8F0F0E2LL; //pipes para RF24
int j=0;

/*-----( Declare objects )-----*/
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
/*-----( Declare Variables )-----*/
int datos_write[10];  // 2 element array holding Joystick readings
int datos_read[10];  // 2 element array holding Joystick readings

void setup()   /****** SETUP: RUNS ONCE ******/
{
  Serial.begin(9600);
  radio.begin();
  
  radio.setRetries(15,15);
  radio.setChannel(93);
  radio.setPayloadSize(20);
  //radio.setDataRate(RF24_250KBPS);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(0);
 
  radio.openWritingPipe(pipe_write);
  radio.openReadingPipe(1,pipe_read);
   radio.startListening();
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  datos_write[0] = 555;
  datos_write[1] = 28;
  datos_write[2] = 28.25;
  datos_write[3] = 28.25;
  datos_write[4] = 28.25;
  datos_write[5] = 28.25;
  datos_write[6] = 28.25;
  datos_write[7] = 28.25;
  datos_write[8] = 30.34;
  datos_write[9] = 2015;
  
 radio.stopListening();

    // Take the time, and send it.  This will block until complete
    unsigned long time = millis();
     Serial.print(" Enviando datos..."); 
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
      Serial.println("Failed, response timed out.\n\r");
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
       radio.read( datos_read, sizeof(datos_read) );

      Serial.print(" datos_read[9]= ");      
      Serial.println(datos_read[9]);
    }


  
  
haceralgo();
}//--(end main loop )---

void haceralgo(void){
  radio.powerDown();
 delay(1000);
   radio.powerUp();
   delay(1000);
}
