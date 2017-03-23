/* 
 * Seguidor solar PanTilt modificado por Enrique con los motores paso a paso
 * Control de motores a 2 hilos (más el enable). 6 señales para los dos motores
 * Reloj RTC Tyny RTC I2C 
                 - SCL a A4
                 - DS a A5
 * NRF24 - CONNECTIONS: nRF24L01 Modules See:
 http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
   1 - GND
   2 - VCC 3.3V !!! NOT 5V
   3 - CE to Arduino pin 9
   4 - CSN to Arduino pin 10
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - UNUSED
   ==============================
   Finales de carrera
   6 Este
   7 Oeste
   8 Norte
   A3 Sur //entrada analógica (no hay más
   
   
   */
  //programa corregido despues de romper los FC
  //para programarlo hay que quitar los pines 0 y 1, sino da error y no se puede subir el programa
 
#include <math.h>
#include <SunPos.h>
#include <Stepper.h>
#include <Wire.h>                       // For some strange reasons, Wire.h must be included here
#include <DS1307new.h>
#include "RTClib.h" //para Tyny RTC I2C 
#include <SPI.h>
#include <Ethernet.h>
#include <nRF24L01.h>
#include <RF24.h>

RTC_DS1307 rtc;

 // definimos el lugar, sun y time de acuerdo con sunpos.h
 cLocation lugar;
 cSunCoordinates sun;
 cTime fecha_hora_PSA;

float grados_paso_acimut=1.8/(19.2*7.5); //para acimut
float grados_paso_cenit=1.8/(19.2*5.23); //para cenit

const int stepsPerRevolution_acimut = 200;  // change this to fit the number of steps per revolution
float one_step_acimut=1.8; // 360/200=1.8 grados por paso
float reduccion_acimut=19.2*4;
float grados_one_step_acimut= one_step_acimut/reduccion_acimut; //cada paso equivale a 1.8/144=0.0125 grados

const int stepsPerRevolution_cenit = 200;  // change this to fit the number of steps per revolution
float one_step_cenit=1.8; // 360/200=1.8 grados por paso
float reduccion_cenit=19.2*6;
float grados_one_step_cenit= one_step_cenit/reduccion_cenit; //cada paso equivale a 1.8/100.416=0.01792543021 grados

// for your motor
int motor1Pin1 = 5;
int motor1Pin2 = 6;
int mortor1EN= 7;
int motor2Pin1 = 2;
int motor2Pin2 = 3;
int mortor2EN= 4;

int FC_este_pin=8;
int FC_oeste_pin=A3;
int FC_norte_pin=A1;
int FC_sur_pin=A2; //como no quedan entradas digitales se usa la analógica 3

boolean FC_este;
boolean FC_oeste;
boolean FC_norte;
boolean FC_sur; //como no quedan entradas digitales se usa la analógica 3
boolean error_FC_acimut=false;
boolean error_FC_cenit=false;
boolean error_FC=false;
boolean error=false;
int num_error=0;
boolean manual=false;
boolean manual_giraNorte=false;
boolean manual_giraSur=false;
boolean manual_giraEste=false;
boolean manual_giraOeste=false;

Stepper motor_acimut(stepsPerRevolution_acimut, motor1Pin1, motor1Pin2);
Stepper motor_cenit(stepsPerRevolution_cenit, motor2Pin1, motor2Pin2);

/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   9
#define CSN_PIN 10
const uint64_t pipe_read =  0xE8E8F0F0E4LL; //pipes para RF24
const uint64_t pipe_write = 0xE8E8F0F0E5LL; //pipes para RF24
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
int datos_read[15];  // 6 element array holding NRF24L01 readings
int datos_write[15];

DateTime now;


float acimut_sol; //referido como 0 a mediodía, negativo al este(mañana)
float cenit_sol; //referido como 0 a mediodía, negativo al este(mañana)
float angulo_FC_este=-177.5;
float angulo_FC_sur=89.0;
float acimut_actual=0.0;
float cenit_actual=0.0;
float acimut_max=120;
float cenit_max=85;
float cenit_sol2;
float acimut_sol2;
boolean iniciado=false;
long pasos;
int j;

float Irradiancia;
int reference;
int pinIrrad=A0;
float angulo_retorno=98.0; //angulo cenital para decir cuando vuelve a reposo
int noche=0;
int noche_anterior=0;
unsigned long t_anterior=0;
int t_print=2000;
unsigned long t_anterior_updatetime=0;
unsigned long t_updatetime=600000; //si recibe la hora Web actualiza cada 10 minutos


void setup()  {
  Serial.begin(9600);
  pinMode(mortor1EN, OUTPUT);
  pinMode(mortor2EN, OUTPUT);
  pinMode(FC_oeste_pin, INPUT);
  pinMode(FC_norte_pin, INPUT);
  pinMode(FC_sur_pin, INPUT);
  
  digitalWrite(mortor1EN, LOW);
  digitalWrite(mortor2EN, LOW);
    descanso_acimut();
     descanso_cenit();
   
   Serial.println("comienzo");
   
  radio.begin();
  
  radio.setRetries(15,15);
  radio.setChannel(93);
  radio.setPayloadSize(128);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(1);
 
    radio.openReadingPipe(1,pipe_read);
    radio.openWritingPipe(pipe_write);
    radio.startListening();
  Serial.println("radio inicializado");
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();
//rtc.adjust(DateTime(2015, 9, 25, 13, 56, 30));
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  Serial.println("reloj inicializado");
  // set the speed at 60 rpm:
  motor_acimut.setSpeed(120);
  motor_cenit.setSpeed(120);
  
 lugar.dLongitude=-3.728244;
 lugar.dLatitude=40.454988;
 sun.dZenithAngle=0;
 sun.dAzimuth=0;
 
  Serial.println("Inicio de loop");
}

void loop(){ 
  finales_carrera();
  reference=analogRead(pinIrrad);
  Irradiancia=reference*3.3/1023;
  now = rtc.now();
  //datosnrf24();
  
   //=======================================
   // fecha de sunpos PSA
  fecha_hora_PSA.iYear=now.year();
  fecha_hora_PSA.iMonth=now.month();
  fecha_hora_PSA.iDay=now.day();
  fecha_hora_PSA.dHours=(double)now.hour();
  fecha_hora_PSA.dMinutes=(double)now.minute();
  fecha_hora_PSA.dSeconds=(double)now.second();
  sunpos(fecha_hora_PSA, lugar, &sun);
  acimut_sol=sun.dAzimuth-180;
  cenit_sol=sun.dZenithAngle;  
  
 
  
  if(cenit_sol>angulo_retorno){ // de noche a reposo
    cenit_sol=0;
    acimut_sol=0;
    noche=1;
  }
  else {noche=0;}
  
  if (noche_anterior==1 && noche ==0){iniciado=false;} //por la mañana inicializa.
  noche_anterior=noche;  
  
  if (!iniciado && !manual) {
    inicializa();   //solo para pruebas, no inicializa
    iniciado=true;}  
    
    finales_carrera();  
    if (!error && !manual){
      Seguir_sol();
    }
    if (manual){
      long num_pasos=5; //número de pasos a girar en cada comando
      if (manual_giraNorte){gira_Norte(num_pasos);}
      if (manual_giraSur){gira_Sur(num_pasos);}
      if (manual_giraEste){gira_Este(num_pasos);}
      if (manual_giraOeste){gira_Oeste(num_pasos);}
      }
      
     descanso_acimut();
     descanso_cenit(); 
     
 if ((millis()-t_anterior)>t_print){
      digitalClockDisplay(); 
      printangles();
      t_anterior=millis();
    }
  //delay(2000); //si se pone un delay el receptor NRF remoto no funciona bien!!!!!
}

void printangles(){
  Serial.print("Acimut_actual: ");
  Serial.print(acimut_actual); 
  Serial.print(" Acimut Sol: ");
  Serial.print(acimut_sol); 
  Serial.print(" err: ");
  Serial.print(acimut_sol-acimut_actual); 
  Serial.print("   Cenit_actual: ");
  Serial.print(cenit_actual); 
  Serial.print(" Cenit Sol: ");
  Serial.println(cenit_sol);
 Serial.print(" err: "); 
  Serial.println(cenit_sol-cenit_actual); 

  Serial.print("cenit_sol: ");
 Serial.println(sun.dZenithAngle);
 
    //Serial.print("datos_read[1]: ");
    //Serial.print(datos_read[1]);
    //Serial.print("resta ");
    //Serial.println((millis()-t_anterior_updatetime));
    
  
  if (manual){
      Serial.println(" Manual : ");
      if (manual_giraNorte){Serial.println(F("Manual gira Norte:"));}
      if (manual_giraSur){Serial.println(F("Manual gira Sur:"));}
      if (manual_giraEste){Serial.println(F("Manual gira Este:"));}
      if (manual_giraOeste){Serial.println(F("Manual gira Oeste:"));}
  }
  if (error){
      Serial.println(" Error : ");
        }
}
  
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.println("Hora actual");
  Serial.print(now.hour());
  Serial.print(":");
  printDigits(now.minute());
  Serial.print(":");
  printDigits(now.second());
  Serial.print(" ");
  printDigits(now.day());
  Serial.print("/");
  printDigits(now.month());
  Serial.print("/");
  Serial.print(now.year()); 
  Serial.println(); 
   
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  //Serial.print(":");
  if(digits < 10)
   Serial.print('0');
   Serial.print(digits);
}

void descanso_acimut(){
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(mortor1EN, LOW);
}

void descanso_cenit(){
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(mortor2EN, LOW);
 }

void datosnrf24(){
  datos_write[0] = now.hour();
  datos_write[1] = now.minute();
  datos_write[2] = now.second();
  datos_write[3] = now.day();
  datos_write[4] = now.month();
  datos_write[5] = now.year();
  datos_write[6] = acimut_sol*100;
  datos_write[7] = cenit_sol*100;
  datos_write[8] = acimut_actual*100;
  datos_write[9] = cenit_actual*100;
  datos_write[10] = FC_este*1000+FC_oeste*100+FC_norte*10+FC_sur; 
  
  if ( radio.available() )
  {
   // Read the data payload until we've received everything
     radio.startListening();                                    // Now, continue listening
    
    unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
    boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not
    
    while ( ! radio.available() ){                             // While nothing is received
      if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
          timeout = true;
          break;
      }      
    }
        
    if ( timeout ){                                             // Describe the results
        //Serial.println(F("Failed, response timed out."));
    }else{ radio.read( datos_read, sizeof(datos_read) );        }
                  
      radio.stopListening();
      radio.write( datos_write, sizeof(datos_write) );
      radio.startListening();
  }
   //Serial.print("datos_read[0]:");
  //Serial.println(datos_read[0]);
  if (datos_read[0]>100){manual=true;}else{manual=false;}
  if (manual){
    if (datos_read[0]==102){manual_giraNorte=true;}else{manual_giraNorte=false;}
    if (datos_read[0]==103){manual_giraSur=true;}else{manual_giraSur=false;}
    if (datos_read[0]==104){manual_giraEste=true;}else{manual_giraEste=false;}
    if (datos_read[0]==105){manual_giraOeste=true;}else{manual_giraOeste=false;}
    }
    if(datos_read[1]>0){ //si datos_read[1]=2 actualiza por mandato del server
    //rtc.adjust(DateTime(2015, 1, 29, 18, 35, 30));
      if ((millis()-t_anterior_updatetime)>t_updatetime || datos_read[1]==2){
            t_anterior_updatetime=millis();
            int trys=0;
            while(now.day()==0 && trys<10){
              rtc.adjust(DateTime(datos_read[7], datos_read[6], datos_read[5], datos_read[2], datos_read[3], datos_read[4]));
              trys++;
            }
            Serial.println(F("Tiempo Actualizado...."));
      }
    }else{t_anterior_updatetime=millis();}
}
   
void finales_carrera(){
   
      int k=0;
      FC_este = digitalRead(FC_este_pin);
      FC_oeste = digitalRead(FC_oeste_pin);
      FC_norte = digitalRead(FC_norte_pin);
      FC_sur = digitalRead(FC_sur_pin);
/*
      Serial.print("Final carrera Este: ");
      Serial.println(FC_este);
      Serial.print("Final carrera OEste: ");
      Serial.println(FC_este);
      Serial.print("Final carrera Norte: ");
      Serial.println(FC_norte);
      Serial.print("Final carrera Sur: ");
      Serial.println(FC_sur);
  */    
      error_FC_acimut=false;
      error_FC_cenit=false;
      error_FC=false;
      error=false;
      if(!FC_este && !FC_oeste){error_FC_acimut=true;num_error=1;}
      if(!FC_norte && !FC_sur){error_FC_cenit=true;num_error=2;}
      if (error_FC_acimut || error_FC_cenit){error_FC=true;error=true;}
}   

void inicializa(){
  Serial.println("incializando...");
  FC_norte = digitalRead(FC_norte_pin);
  FC_este = digitalRead(FC_este_pin);
  
  // 40 pasos son 0.5 grados
  int pasos_giro=40;
     Serial.print(F("grados_one_step_acimut...."));
     Serial.println(grados_one_step_acimut,6);
     Serial.print(F("grados_one_step_cenit...."));
     Serial.println(grados_one_step_cenit,6);
     Serial.println(F("Inicializando...."));
    
    //INICIALIZA CENIT, primero en cenit, luego en acimut     // busca el final de carrerra    //moverse al este hasta que encuentre el final carrera
    Serial.println(F("Buscando final carrera Sur:")); //gira al Sur
    digitalWrite(mortor2EN, HIGH);
    while(FC_sur){
    FC_sur = digitalRead(FC_sur_pin);
    motor_cenit.step(pasos_giro);
    }
    descanso_cenit();
    cenit_actual=angulo_FC_sur; 
    
    // INICIALIZA ACIMUT
    // busca el final de carrerra
    //moverse al este hasta que encuentre el final carrera
    Serial.println(F("Buscando final carrera Este:")); //gira al Este
    digitalWrite(mortor1EN, HIGH);
    while(FC_este){
      FC_este = digitalRead(FC_este_pin);
      motor_acimut.step(pasos_giro);
    }
    descanso_acimut();
    acimut_actual=angulo_FC_este;
    
    iniciado=true;
     Serial.println(F("Inicializado....OK"));
     pasos=long((acimut_sol-acimut_actual)/grados_one_step_acimut);
     Serial.println(F("Pasos acimut:"));
     Serial.println(pasos);
          
     delay(5000);
     // Pimer movimiento, todo seguido, se puede eliminar ya que en este movimiento no detecta posibles errores en finales de carrera.
     if (cenit_sol>cenit_max){cenit_sol2=cenit_max;} else{cenit_sol2=cenit_sol;}
    if (abs(acimut_sol)>acimut_max && acimut_sol>0){acimut_sol2=acimut_max;} else{acimut_sol2=acimut_sol;}
    if (abs(acimut_sol)>acimut_max && acimut_sol<0){acimut_sol2=acimut_max;} else{acimut_sol2=acimut_sol;}
    
    pasos=long((acimut_sol2-acimut_actual)/grados_one_step_acimut);
    //pasos=200;
    Serial.print(F("Buscando en acimut: "));
     Serial.println(acimut_sol2);
     Serial.print(F("Numero de pasos: "));
     Serial.println(pasos);
    if(acimut_sol>acimut_actual){ gira_Oeste(pasos);}
    if(acimut_sol<acimut_actual){gira_Este(pasos);}
    acimut_actual= acimut_actual+float(pasos)*grados_one_step_acimut;
    Serial.println(F("Posicionado en acimut: "));
    delay(10000);
    
    pasos=long((cenit_sol2-cenit_actual)/grados_one_step_cenit);
    Serial.println(F("Posicionando en cenit:"));
    Serial.print(cenit_sol2);
    if (cenit_sol>cenit_actual) {gira_Sur(pasos);}
    if (cenit_sol<cenit_actual){gira_Norte(pasos);}
    cenit_actual= cenit_actual+float(pasos)*grados_one_step_cenit;
    delay(5000);
    Serial.println(F("Siguiendo al sol:"));
       
} //fin de inicializa
  //==================================GIROS=============================================
  void gira_Este(long pasos){
    Serial.println("girando al este:");
    Serial.print(pasos);
   if(FC_este){digitalWrite(mortor1EN, HIGH); motor_acimut.step(-pasos);}
  descanso_acimut();
  }
  
  void gira_Oeste(long pasos){
    Serial.println("girando al Oeste:");
    Serial.print(pasos);
     if(FC_oeste){digitalWrite(mortor1EN, HIGH);motor_acimut.step(-pasos);}
  descanso_acimut();
  }
  
  void gira_Norte(long pasos){
    Serial.println("girando al Norte:");
    Serial.print(pasos);
       if(FC_norte){digitalWrite(mortor2EN, HIGH);motor_cenit.step(pasos);}
  descanso_cenit();
  }
  
   void gira_Sur(long pasos){
   Serial.println("girando al Sur:");
    Serial.print(pasos);
       if(FC_sur){digitalWrite(mortor2EN, HIGH);motor_cenit.step(pasos);}
  descanso_cenit();
  }
  //==================================GIROS=============================================
/// función poara seguir al sol
  void Seguir_sol(){
    //seguimiento acimutal 
    //40 pasos son 0.5 grados  // que se mueva a tramos para evitar un movimiento continuo descontrolado
    int pasos_min=10;
    int pasos_max=40;
   
  if (cenit_sol>cenit_max){cenit_sol2=cenit_max;} else{cenit_sol2=cenit_sol;}
  if (abs(acimut_sol)>acimut_max && acimut_sol>0){acimut_sol2=acimut_max;} else{acimut_sol2=acimut_sol;}
  if (abs(acimut_sol)>acimut_max && acimut_sol<0){acimut_sol2=acimut_max;} else{acimut_sol2=acimut_sol;}
   
    pasos=long((acimut_sol2-acimut_actual)/grados_one_step_acimut);
    if (abs(pasos)>pasos_max && pasos>0){pasos=pasos_max;}
    if (abs(pasos)>pasos_max && pasos<0){pasos=-pasos_max;}
    
   if(abs(pasos)>=pasos_min){
    pasos=2*pasos; // se mueve el doble de lo necesario, la mitad de movimientos
    if(acimut_sol>acimut_actual){ gira_Oeste(pasos);}
    if(acimut_sol<acimut_actual){gira_Este(pasos);}
    acimut_actual= acimut_actual+float(pasos)*grados_one_step_acimut;
    Serial.print("pasos acimut: ");
    Serial.println(pasos);
    }
  
   //seguimiento cenital
   if(pasos<pasos_min){ //si está en acimut, esperar
  
  pasos=long((cenit_sol2-cenit_actual)/grados_one_step_cenit);
  if (abs(pasos)>pasos_max && pasos>0){pasos=pasos_max;}
  if (abs(pasos)>pasos_max && pasos<0){pasos=-pasos_max;}
     
  if(abs(pasos)>=pasos_min){ 
    pasos=2*pasos; // se mueve el doble de lo necesario, la mitad de movimientos 
    if (cenit_sol>cenit_actual) {gira_Sur(pasos);}
    if (cenit_sol<cenit_actual){gira_Norte(pasos);}
    cenit_actual= cenit_actual+float(pasos)*grados_one_step_cenit;
    Serial.print("pasos cenit: ");
    Serial.println(pasos);
   }
 
 }
  
// Serial.print("pasos....  ");
//  Serial.println(pasos);
  
} // fin de seguir al sol
