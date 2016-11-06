/*
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

                   Nodemcu      ADS1115
                   GND----------GND   (ground in)
                   3V----------VCC  (puede ser 5V)
                   D1-----------SCL  
                   D2----------SDA 

                   //ADS115 multiplexor analógico de 16 bits
//https://learn.adafruit.com/adafruit-4-channel-adc-breakouts/assembly-and-wiring
//0x48 (1001000) ADR -> GND
//0x49 (1001001) ADR -> VDD
//0x4A (1001010) ADR -> SDA
//0x4B (1001011) ADR -> SCL
// NANO I2C: A4 (SDA) and A5 (SCL).

//conectamos ADR a GND --> Default (0x48)
                   
*/


#include <Wire.h>
#include <Adafruit_ADS1015.h>

 Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Hello!");
  
  Serial.println("Getting differential reading from AIN0 (P) and AIN1 (N)");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
   //ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
  ads.begin();
}

void loop(void)
{
  int16_t results;
   int16_t results1;
   /* Be sure to update this value based on the IC and the gain settings! */
   float multiplier = 0.125F; /*  +/- 4.096V  1 bit = 2mV      0.125mV */
  results = ads.readADC_Differential_0_1();  
  Serial.print("Differential01: "); Serial.print(results); Serial.print("("); Serial.print(results * multiplier); Serial.println("mV)");
  delay(1000);
  results1 = ads.readADC_Differential_2_3();  
 Serial.print("Differential23: "); Serial.print(results1); Serial.print("("); Serial.print(results1 * multiplier); Serial.println("mV)");
 delay(1000);
}
