
/*Nodemcu Solarmax3000C Serial
 * http://192.168.1.55:95
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

AC output PAC Power
Operating hours KHR
Date year DYR
Date month DMT
Date day DDY
Energy year KYR
Energy month KMT
Energy day KDY
Energy total KT0
Installed capacity PIN
Mains cycle duration TNP
Network address ADR
Relative output PRL
Software version SWV
Solar energy year RYR
Solar energy day RDY
Solar energy total RT0
Solar radiation RAD
Voltage DC UDC
Voltage phase 1 UL1
Voltage phase 2 UL2
Voltage phase 3 UL3
Current DC IDC
Current phase 1 IL1
Current phase 2 IL2
Current phase 3 IL3
Temperature power unit 1 TKK
Temperature power unit 2 TK2
Temperature power unit 3 TK3
Temperature solar cells TSZ
Type Type
Time minute TMI
Time hour THR

*/
//https://github.com/madsci1016/Arduino-EasyTransfer para enviar datos por puerto serie a Nodemcu
 // Modificar la librería, comentar io.h para que sea compatible con Nodemcu (funciona igual)
 //usar softwareserial para pines 2 y 3.

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
uint16_t acimusol; 
uint16_t cenitsol;
uint16_t acimut;
uint16_t cenit;
uint16_t astop;
uint16_t areposo; 
uint16_t milis;
uint16_t horaOK; 
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
uint16_t acimusol; 
uint16_t cenitsol;
uint16_t acimut;
uint16_t cenit;
uint16_t astop;
uint16_t areposo; 
uint16_t milis;
uint16_t horaOK; 
uint16_t manual; 
uint16_t var6; 
};

RECEIVE_DATA_STRUCTURE mydatain;


void setup() {
  Serial.begin(19200);
  outSerial.begin(19200);
  mydataout.cnt=0;
  Serial.println("\nSoftware serial test started");
  Serial.println("\nPantilt + Nodemcu:");

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
}

void loop() {
  set_out_values();
  ETout.sendData();
  delay(1000);
  
  for(int i=0; i<5; i++){
     if(ETin.receiveData()){
        //this is how you access the variables. [name of the group].[variable name]
        //since we have data, we will blink it out. 
        Serial.println("Data received from NodeMCU, PAC, counter");
        Serial.println(mydatain.milis);
        Serial.println(mydatain.cnt);
       if (mydatain.horaOK){
      Serial.println("Hora remota:");
      Serial.print(mydatain.hora);
      printDigits(mydatain.minuto);
      printDigits(mydatain.segundo);
      Serial.print(" ");
      Serial.print(mydatain.dia);
      Serial.print(".");
      Serial.print(mydatain.mes);
      Serial.print(".");
      Serial.print(mydatain.ano); 
      Serial.println(); 
       }else
       {Serial.println("HORA REMOTA NO ok");
        delay(10);
      }

      if (mydatain.manual){
        Serial.println("Manual");
        }else{
          Serial.println("auto");
        }
    }
}
  
} //fin loop

void set_out_values(){
  mydataout.cnt++;
  mydataout.milis=millis();
  if (mydataout.cnt>999){mydataout.cnt=0;}
  Serial.print("Counter:");
  Serial.println(mydataout.cnt);
  
mydataout.dia=1;
mydataout.mes=2;
mydataout.ano=3;
mydataout.hora=4;
mydataout.minuto=5;
mydataout.segundo=6; 
mydataout.acimusol=7; 
mydataout.cenitsol=8;
mydataout.acimut=9;
mydataout.cenit=10;
mydataout.astop=11;
mydataout.areposo=12; 
mydataout.milis=13;
mydataout.horaOK=14; 
mydataout.var5=15; 
mydataout.var6=16; 

}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
