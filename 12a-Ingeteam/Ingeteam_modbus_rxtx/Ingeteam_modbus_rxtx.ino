/**
 *  Ingeteam altavista 107  Arduino nano
 *  arduino nano lee los datos
 *  los envía por software serial al Nodemcu 
 *  Se utiliza un max485
 *Adaptado Miguel Alonso 2016.
  
         MAX485EE
          ____
 1(Rx)-- 1|    |8 --- +5
 8 ----- 2|    |7 --- RS-485 B- Naranja al inversor
 8 ----- 3|    |6 --- RS-485 A+ Marron al inversor
 0(Tx) - 4|____|5 --- GND

 *  Nano    --->   Nodemcu o Max485
 *  0       --->   4(Tx) del Max485
 *  1       --->   1(Rx) del Max485
 *  2       --->   D6 del Nodemcu (softwareserial) No necesario
 *  3       --->   D4 del Nodemcu (softwareserial) 
 *  8       --->   2 y 3 del MAX485 
 * 	5V      --->   8 del del MAX485 
 * 	GND     --->   GND del Nodemcu y del MAX485 
 *		
 */
 //https://github.com/madsci1016/Arduino-EasyTransfer para enviar datos por puerto serie a Nodemcu
 // Modificar la librería, comentar io.h para que sea compatible con Nodemcu (funciona igual)
 //usar softwareserial para pines 2 y 3.

#include <SoftEasyTransfer.h>
#include <ModbusRtu.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);
SoftEasyTransfer ET; 

struct SEND_DATA_STRUCTURE{
  unsigned long ms;
  int PAC; 
  int VDC;
  int VDCbus;
  int IDC;
  int VAC;
  int IAC;
  int f;
  int cosf;
  int Etotal;
  int Horas;
  int Nconex;
  int A3;
  int A4;
  int A5;
  int A6;
  int A7;
  int A8;
};

SEND_DATA_STRUCTURE mydata;

uint16_t au16data[16];
uint8_t u8state;

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  u8serno : serial port (use 0 for Serial)
 *  u8txenpin : 0 for RS-232 and USB-FTDI 
 *               or any pin number > 1 for RS-485
 */
Modbus master(0,0,8); // PIN 8 es el RS485 enable


modbus_t telegram;

unsigned long u32wait;

void setup() {
  master.begin( 9600 ); // baud-rate at 19200
  master.setTimeOut( 2000 ); // if there is no answer in 2000 ms, roll over
  u32wait = millis() + 1000;
  u8state = 0; 

   mySerial.begin(9600);
  //start the library, pass in the data details and the name of the serial port.
  ET.begin(details(mydata), &mySerial);
  Serial.print( "tamano:");
  int tam=sizeof(mydata);
  Serial.println( tam);
  delay (5000);
}

void loop() {
 
 
  switch( u8state ) {
  case 0: 
    if (millis() > u32wait) u8state++; // wait state
    break;
  case 1: 
    telegram.u8id = 1; // slave address
    telegram.u8fct = 4; // function code (this one is registers read)
    telegram.u16RegAdd = 11; // start address in slave
    telegram.u16CoilsNo = 16; // number of elements (coils or registers) to read
    telegram.au16reg = au16data; // pointer to a memory array in the Arduino
    master.query( telegram ); // send query (only once)
    u8state++;
    break;
  case 2:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE) {
      u8state = 3;
      u32wait = millis() + 1000;
      Serial.println("ledio estado 3:"); 
        printP(); mydata.ms=millis();ET.sendData();
    }
    break;
   case 3: 
    if (millis() > u32wait) u8state++; // wait state
    break;     
   case 4: 
    telegram.u8id = 1; // slave address
    telegram.u8fct = 4; // function code (this one is registers read)
    telegram.u16RegAdd = 0; // start address in slave
    telegram.u16CoilsNo = 4; // number of elements (coils or registers) to read
    telegram.au16reg = au16data; // pointer to a memory array in the Arduino
    master.query( telegram ); // send query (only once)
    u8state++;
    break;
  case 5:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE) {
      u8state = 6;
      u32wait = millis() + 1000; 
      Serial.println("ledio estado 6:");
        printE();  mydata.ms=millis(); ET.sendData();
   }
    break;
    case 6: 
    if (millis() > u32wait) u8state++; // wait state
    break;     
   case 7: 
    telegram.u8id = 1; // slave address
    telegram.u8fct = 4; // function code (this one is registers read)
    telegram.u16RegAdd = 38; // start address in slave
    telegram.u16CoilsNo = 10; // number of elements (coils or registers) to read
    telegram.au16reg = au16data; // pointer to a memory array in the Arduino
    master.query( telegram ); // send query (only once)
    u8state++;
    break;
  case 8:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE) {
      u8state = 0;
      u32wait = millis() + 1000; 
      Serial.println("ledio estado 0:");
        printA();  
        //*mydata.au16dataA = au16data; 
        mydata.ms=millis();
        ET.sendData();//valores analógicos
   }
    break;
  }
}

void printP() {
        Serial.print(" Datos:");
        Serial.print("VDC:");
        Serial.print(au16data[0]);mydata.VDC=au16data[0];
        Serial.print(" IDC:");
        Serial.print(au16data[1]);mydata.IDC=au16data[1];
        Serial.print(" VDCbus:");
        Serial.print(au16data[2]);mydata.VDCbus=au16data[2];
        Serial.print(" IAC:");
        Serial.print(au16data[3]);mydata.IAC=au16data[3];
        Serial.print(" PAC:");
        Serial.print(au16data[4]); mydata.PAC=au16data[4];
        Serial.print(" cosf:");
        Serial.print(au16data[5]);mydata.cosf=au16data[5];
        Serial.print(" Nada:");
        Serial.print(au16data[6]);
        Serial.print(" VAC:");
        Serial.print(au16data[7]);mydata.VAC=au16data[7];
        Serial.print(" f:");
        Serial.print(au16data[8]);mydata.f=au16data[8];
        Serial.print(" ano:");
        Serial.print(au16data[9]);
        Serial.print(" mes:");
        Serial.print(au16data[10]);
        Serial.print(" dia:");
        Serial.print(au16data[11]);
        Serial.print(" h:");
        Serial.print(au16data[12]);
        Serial.print(" m:");
        Serial.print(au16data[13]);
        Serial.print(" s:");
        Serial.println(au16data[14]);
        
}

void printE() {
        Serial.print(" Energias:");
        Serial.print("Etotal:");
        Serial.print(au16data[1]);mydata.Etotal=au16data[1];
        Serial.print(" HorasFunc:");
        Serial.print(au16data[2]);mydata.Horas=au16data[2];
        Serial.print(" Nconex:");
        Serial.println(au16data[3]);mydata.Nconex=au16data[3];
}

void printA() {
        Serial.print(" Analogicas:");
        Serial.print("A1:");
        Serial.print(au16data[0]);
        Serial.print(" A2:");
        Serial.print(au16data[1]);
        Serial.print(" A3:");
        Serial.print(au16data[2]);
        Serial.print(" A4:");
        Serial.print(au16data[3]);mydata.A3=au16data[3];
        Serial.print(" 4:");
        Serial.print(au16data[4]);mydata.A4=au16data[4];
        Serial.print(" 5:");
        Serial.print(au16data[5]);mydata.A5=au16data[5];
        Serial.print(" 6:");
        Serial.print(au16data[6]);mydata.A6=au16data[6];
        Serial.print(" 7:");
        Serial.print(au16data[7]);mydata.A7=au16data[7];
        Serial.print(" 8:");
        Serial.print(au16data[8]);mydata.A8=au16data[8];
        Serial.print(" 9:");
        Serial.print(au16data[9]);
        Serial.print(" 10:");
        Serial.print(au16data[10]);
        Serial.print(" 11:");
        Serial.print(au16data[11]);
        Serial.print(" 12:");
        Serial.print(au16data[12]);
        Serial.print(" 13:");
        Serial.print(au16data[13]);
        Serial.print(" 14:");
        Serial.println(au16data[14]);
}
