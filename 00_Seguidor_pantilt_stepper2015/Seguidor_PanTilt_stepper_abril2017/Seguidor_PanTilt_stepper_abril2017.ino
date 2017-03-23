/* 
 *  Modificado para comunicarse con un NodemCU
 *  Serial de arduino 9 y 19
 *  Ya no hay NRF24
 
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
#include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>
SoftwareSerial outSerial(9,10);// 10(RX), 11(TX) 
SoftwareSerial inSerial(11,12);// 10(RX), 11(TX) 
SoftEasyTransfer ETout; 
SoftEasyTransfer ETin; 
struct SEND_DATA_STRUCTURE{
uint16_t cnt;
uint16_t dia;
uint16_t mes;
uint16_t ano;
uint16_t hora;
uint16_t minuto;
uint16_t segundo; 
uint16_t acimutsol; 
uint16_t cenitsol;
uint16_t acimut;
uint16_t cenit;
uint16_t manual;
uint16_t num_error; 
uint16_t milis;
uint16_t var4; 
uint16_t var5; 
uint16_t var6; 
};

SEND_DATA_STRUCTURE mydataout;

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
uint16_t cnt;
uint16_t dia;
uint16_t mes;
uint16_t ano;
uint16_t hora;
uint16_t minuto;
uint16_t segundo; 
uint16_t manual; 
uint16_t manual_giraNorte;
uint16_t manual_giraSur;
uint16_t manual_giraEste;
uint16_t manual_giraOeste;
uint16_t areposo; 
uint16_t milis;
uint16_t horaOK; 
uint16_t paro; 
uint16_t var6; 
};

RECEIVE_DATA_STRUCTURE mydatain;


RTC_DS1307 rtc;
unsigned long  delayTime;

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



DateTime now;


float acimut_sol; //referido como 0 a mediodía, negativo al este(mañana)
float cenit_sol; //referido como 0 a mediodía, negativo al este(mañana)
float angulo_FC_este=-177.5;
float angulo_FC_sur=77.5;
float acimut_actual=0.0;
float cenit_actual=0.0;
float acimut_max=120;
float cenit_max=77.0;
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

unsigned long t_anterior_out=0;
unsigned long t_out=1000; //si recibe la hora Web actualiza cada 10 minutos


void setup()  {
  Serial.begin(9600);
   outSerial.begin(19200);
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

  ETout.begin(details(mydataout), &outSerial);
  Serial.print( "tamano:");
  int tamout=sizeof(mydataout);
  Serial.println( tamout);
  delay (1000);

  ETin.begin(details(mydatain), &outSerial);
  Serial.print( "tamano:");
  int tamin=sizeof(mydatain);
  Serial.println( tamin);
  delay (1000);
  
   
  
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();
//rtc.adjust(DateTime(2017, 3, 17, 16, 23, 0));
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
   //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     rtc.adjust(DateTime(2014, 3, 15, 16, 34, 0));
  }
  Serial.println("reloj inicializado");
  digitalClockDisplay(); 
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
  
  
  if (Serial.available()) {
     procesaSerie();
     }
     
  finales_carrera();
  reference=analogRead(pinIrrad);
  Irradiancia=reference*3.3/1023;
  now = rtc.now();
  
  
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

     if(ETin.receiveData()){
      read_in_values();
     }
 if ((millis()-t_anterior)>t_print){
      digitalClockDisplay(); 
      printangles();
      set_out_values();
      ETout.sendData();
      t_anterior=millis();
    }
}

void printangles(){
  Serial.print("Acimut_actual: ");
  Serial.print(acimut_actual); 
  Serial.print(" Acimut Sol: ");
  Serial.print(acimut_sol); 
  Serial.print(" error_acimut: ");
  Serial.print(acimut_sol-acimut_actual); 
  Serial.print("   Cenit_actual: ");
  Serial.print(cenit_actual); 
  Serial.print(" Cenit Sol: ");
  Serial.print(cenit_sol);
  Serial.print(" error_cenit: "); 
  Serial.print(cenit_sol-cenit_actual); 
  Serial.print("cenit_sol: ");
  Serial.println(sun.dZenithAngle);
  
  if (manual){
      Serial.println(" Manual : ");
      if (manual_giraNorte){Serial.println(F("Manual gira Norte:"));}
      if (manual_giraSur){Serial.println(F("Manual gira Sur:"));}
      if (manual_giraEste){Serial.println(F("Manual gira Este:"));}
      if (manual_giraOeste){Serial.println(F("Manual gira Oeste:"));}
  }
  if (error){
      Serial.print(" Seguidor en Error ");
      Serial.println(num_error);
        }
}
  
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print("Hora actual  ");
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

void read_in_values(){
    if(mydatain.horaOK){    
      if (mydatain.manual){
        manual=true;
        Serial.println("control Manual");
      }else{manual=false;}
      if (manual){
        if (mydatain.manual_giraNorte){manual_giraNorte=true;}else{manual_giraNorte=false;}
        if (mydatain.manual_giraSur){manual_giraSur=true;}else{manual_giraSur=false;}
        if (mydatain.manual_giraEste){manual_giraEste=true;}else{manual_giraEste=false;}
        if (mydatain.manual_giraOeste){manual_giraOeste=true;}else{manual_giraOeste=false;}
       }
        //rtc.adjust(DateTime(2015, 1, 29, 18, 35, 30));
       if ((millis()-t_anterior_updatetime)>t_updatetime && mydatain.horaOK){
                  t_anterior_updatetime=millis();
                  int trys=0;
                  while(now.day()==0 && trys<10){
                    rtc.adjust(DateTime(mydatain.ano, mydatain.mes, mydatain.dia, mydatain.hora, mydatain.minuto, mydatain.segundo));
                    trys++;
                  }
                  Serial.println(F("Tiempo Actualizado...."));
            }else{t_anterior_updatetime=millis();}
    Serial.print("NodeMCU OK, horaOK, ");
   }else{Serial.println("NodeMCU NOT WORKING, Please reset NodeMCU");}

  Serial.print(" NodeMCU Time: ");
  Serial.print(mydatain.hora);
  Serial.print(":");
  Serial.print(mydatain.minuto);
  Serial.print(":");
  Serial.println(mydatain.segundo);
}

void set_out_values(){
  mydataout.cnt++;
  mydataout.milis=millis();
  if (mydataout.cnt>999){mydataout.cnt=0;}
  //Serial.print("Counter:");
  //Serial.println(mydataout.cnt);
    mydataout.dia=now.day();;
    mydataout.mes=now.month();
    mydataout.ano=now.year();
    mydataout.hora=now.hour();;
    mydataout.minuto=now.minute();
    mydataout.segundo=now.second();
    float val=0;
    if (acimut_sol>0){val=acimut_sol;}else{val=-acimut_sol;}
   float x=val;
    float y=100.0*x;
   mydataout.acimutsol=y;
     x=cenit_sol;
     y=100.0*x;
    mydataout.cenitsol=y;
    
    if (acimut_actual>0){val=acimut_actual;}else{val=-acimut_actual;}
     x=val;
     y=100.0*x;
    mydataout.acimut=y;
     x=cenit_actual;
     y=100.0*x;
    mydataout.cenit=y;
    mydataout.manual=manual;
    mydataout.num_error=num_error; 
    mydataout.milis=millis();
    mydataout.var4=14; 
    mydataout.var5=FC_este*1000+FC_oeste*100+FC_norte*10+FC_sur; 
    mydataout.var6=sun.dAzimuth; 
}

void procesaSerie(){
   char buf = '\0';
   
   buf = Serial.read();
    if (buf == 't' || buf == 'T'){
      while(Serial.available())
         buf = Serial.read();
          Serial.println( "________> TIME AND DATE SETTING <_______________" );
          Serial.println(F( " FORMAT  YYYY,MM,DD,HH,DD,SS,   DO NOT FORGET THE LAST ','" ));
          Serial.println(F( "Set the date and time by entering the current date/time in" ));
          Serial.println(F( "YYYY,MM,DD,HH,DD,SS," ));
          Serial.println(F("example: 2000,12,31,23,59,59, would be DEC 31st, 2000  11:59:59 pm") );
          
      // while(Serial.available())
       //  buf = Serial.read();
       setTimeFunction();
    }
}
void setTimeFunction(){
   delayTime = millis() + 45000UL;
  while (delayTime >= millis() && !Serial.available()) {
    delay(10);
  }
  digitalClockDisplay();
  if (Serial.available()) {
        //note that the tmElements_t Year member is an offset from 1970,
        //but the RTC wants the last two digits of the calendar year.
        //use the convenience macros from Time.h to do the conversions.
            int Year = Serial.parseInt();
            int Month = Serial.parseInt();
            int Day = Serial.parseInt();
            int Hour = Serial.parseInt();
            int Minute = Serial.parseInt();
            int Second = Serial.parseInt();
           rtc.adjust(DateTime(Year, Month, Day, Hour, Minute, Second));
     //use the time_t value to ensure correct weekday is set
            

       Serial.println("RTC set OK! please wait..." );
       digitalClockDisplay();
            while (Serial.available() > 0) Serial.read();
  }
  else 
    Serial.println( F("timed out, please try again"));

    delay(1000);
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
