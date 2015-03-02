// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 5000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3500
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    

 #include <virtuabotixRTC.h> 
 // Creation of the Real Time Clock Object
//SCLK -> 4, I/O -> 5, CE -> 6
virtuabotixRTC myRTC(4, 5, 6);

int samples[NUMSAMPLES];

const int relepin = 8;
float temperatura;
float limite_temperatura =38.5;

void setup(void) {
  Serial.begin(9600);
  //analogReference(EXTERNAL);
  // seconds, minutes, hours, day of the week, day of the month, month, year
  //para ponerlo en hora alimentar el reloj a 5V
 //myRTC.setDS1302Time(00, 02, 10, 2, 2, 3, 2015);
  // myRTC.updateTime();
  pinMode(relepin, OUTPUT);
}
 
void loop(void) {
  myRTC.updateTime();
  print_datetime();
  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
 
  Serial.print("Average analog reading "); 
  Serial.println(average);
 
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  Serial.print("Thermistor resistance "); 
  Serial.println(average);
 
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
 
  Serial.print("Temperature "); 
  Serial.print(steinhart);
  Serial.println(" *C");
 
 temperatura= steinhart;
controla_rele(); 
  delay(1000);
}

void print_datetime(){
  // Start printing elements as individuals 
	
Serial.print("Current Date / Time: "); 
Serial.print(myRTC.dayofmonth); 
Serial.print("/"); 
Serial.print(myRTC.month); 
Serial.print("/");
Serial.print(myRTC.year);
Serial.print(" ");
Serial.print(myRTC.hours);
Serial.print(":");
Serial.print(myRTC.minutes);
Serial.print(":");
Serial.println(myRTC.seconds);
// Delay so the program doesn't print non-stop
}

void controla_rele(){
  if (temperatura > limite_temperatura) {
    // turn LED on:
    digitalWrite(relepin, LOW);
    Serial.println("RELE OFF"); //bomba conectada
  }
  else {
    // turn LED off:
    digitalWrite(relepin, HIGH);
     Serial.println("RELE ON"); //bomba desconectada

  }
  
 /*
  if (myRTC.hours > 10) {
    // turn LED on:
    digitalWrite(relepin, LOW);
    Serial.println("RELE OFF");
  }
  else {
    // turn LED off:
    digitalWrite(relepin, HIGH);
     Serial.println("RELE ON");

  }
  
  */
}
  
  
