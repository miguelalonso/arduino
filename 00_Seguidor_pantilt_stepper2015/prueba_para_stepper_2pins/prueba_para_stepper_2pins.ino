
/*
 Stepper Motor Control - one revolution

 This program drives a unipolar or bipolar stepper motor.
 The motor is attached to digital pins 8 - 11 of the Arduino.

 The motor should revolve one revolution in one direction, then
 one revolution in the other direction.


 Created 11 Mar. 2007
 Modified 30 Nov. 2009
 by Tom Igoe

 */

#include <Stepper.h>

 float acimut_sol=0;
 float acimut_actual=-90;
  
const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution


// for your motor
// for your motor
float grados_paso=1.8/(19.2*7.5); //para acimut
//float grados_paso=1.8/(19.2*5.23); //para cenit
int motorPin1 = 2;
int motorPin2 = 3;
int mortorEN1= 4;
int motorPin3 = 5;
int motorPin4 = 6;
int mortorEN2= 7;
// initialize the stepper library on pins 8 through 11:
Stepper myStepper1(stepsPerRevolution, motorPin1,motorPin2);
Stepper myStepper2(stepsPerRevolution, motorPin3,motorPin4);

int j=0;
float grados=15;
long pasos=long(grados/grados_paso);
float one_step_acimut=1.8; // 360/200=1.8 grados por paso
float reduccion_acimut=19.2*7.5;
float grados_one_step_acimut= 2*one_step_acimut/reduccion_acimut; //cada paso equivale a 1.8/144=0.0125 grados



void setup() {
  pinMode(mortorEN1, OUTPUT);
  digitalWrite(mortorEN1, LOW);
  pinMode(mortorEN2, OUTPUT);
  digitalWrite(mortorEN2, LOW);
  
  // set the speed at 60 rpm:
  myStepper1.setSpeed(120);
   myStepper2.setSpeed(120);
  // initialize the serial port:
  Serial.begin(9600);
  acimut_actual=-90;
    acimut_sol=0;
   pasos=long((acimut_sol-acimut_actual)/grados_one_step_acimut);
    Serial.print("pasos: ");
    Serial.println(pasos);
}

void loop() {
  /*
  // step one revolution  in one direction:
  Serial.print("clockwise: ");
  Serial.println(pasos);
  digitalWrite(mortorEN1, HIGH);
    myStepper1.step(pasos);
  descanso();
  delay(1000);

  // step one revolution in the other direction:
  Serial.print("counterclockwise: ");
    Serial.println(pasos);
  digitalWrite(mortorEN1, HIGH);
  myStepper1.step(-pasos); 
  descanso();
  delay(1000);
 
  
  ///=====================
  digitalWrite(mortorEN2, HIGH);
    myStepper2.step(pasos);
  descanso();
  delay(1000);

  // step one revolution in the other direction:
  Serial.print("counterclockwise: ");
    Serial.println(pasos);
  digitalWrite(mortorEN2, HIGH);
  myStepper2.step(-pasos); 
  descanso();
  delay(1000);
  */
  
  acimut_sol=0;
  
  seguir_al_sol();
  descanso();
}

void descanso(){
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(mortorEN1, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
  digitalWrite(mortorEN2, LOW);
      }
      
void seguir_al_sol(){
    long pasos_min=10;
    long pasos_max=40;
    int pasito;
    int micropaso=3600;
    
    digitalWrite(mortorEN2, HIGH);
   while(pasito<pasos){
   pasito=pasito+micropaso;
     myStepper2.step(micropaso);
     Serial.println(pasito);
    }
    descanso();
     Serial.println("Finalizado +");
    delay (5000);
    pasito=0;
           
      digitalWrite(mortorEN2, HIGH);
   while(pasito<pasos){
   pasito=pasito+micropaso;
    Serial.println(-pasito);
     myStepper2.step(-micropaso);
    }
    descanso();
     Serial.println("Finalizado -");
    delay (5000);
    pasito=0;
     
  }
  
 
