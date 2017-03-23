//el que lee

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   9
#define CSN_PIN 10
#define JOYSTICK_X A0
#define JOYSTICK_Y A1

// NOTE: the "LL" at the end of the constant is "LongLong" type
const uint64_t pipe = 0xE8E8F0F0E1LL; // Define the transmit pipe
int j=0;

/*-----( Declare objects )-----*/
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
/*-----( Declare Variables )-----*/
int joystick[10];  // 2 element array holding Joystick readings

void setup()   /****** SETUP: RUNS ONCE ******/
{
  Serial.begin(9600);
  radio.begin();
  
  radio.setRetries(15,15);
  radio.setChannel(90);
  radio.setPayloadSize(20);
  //radio.setDataRate(RF24_250KBPS);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(1);
 
  radio.openWritingPipe(pipe);
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  joystick[0] = 555;
  joystick[1] = 28;
  joystick[2] = 28.25;
  joystick[3] = 28.25;
  joystick[4] = 28.25;
  joystick[5] = 28.25;
  joystick[6] = 28.25;
  joystick[7] = 28.25;
  joystick[8] = 30.34;
  joystick[9] = 2015;
  
  radio.powerUp();
  radio.write( joystick, sizeof(joystick) );
  Serial.println(joystick[9]);
haceralgo();
}//--(end main loop )---

void haceralgo(void){
  radio.powerDown();
 delay(6);
}
