
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
SoftwareSerial mySerial(10,11);// 10(RX), 11(TX) 
SoftwareSerial outSerial(2,3);// 4(RX), 5(TX) 
SoftEasyTransfer ET; 

String potencia= "{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}";
//String potencia= "{FB;05;4E|64:E1D;E11;E1h;E1m;E1M;E2D;E21;E2h;E2m;E2M;E3D;E31;E3h;E3m;E3M|1270}";
String energia="{FB;05;36|64:CAC;KHR;KDY;KLD;KMT;KLM;KYR;KLY;KT0|0D34}";
String energia2="{FB;05;26|64:CAC;KHR;KDY;KLD;KMT|08AE}";
//String energia3="{FB;05;22|64:KLM;KYR;KLY;KT0|07AD}";
String energia3="{FB;05;26|64:KLM;KYR;KLY;TKK;KT0|08D6}";
String lectura="";
String lecturaenergia="";
char c;
int VDC;
int IDC;
int VAC;
int IAC;
int PAC;
int PRL; //da el % de potencia de operación
int CAC; //startups??
int KYR;//energy year
int KLY; //energy last year
int KMT;//energy month
int KLM; //energy last month
int KDY; //energy day
int KT0;//total energy
int TKK; //temperature
int KHR; //operating  hours
int KLD; //energy yesterday


struct SEND_DATA_STRUCTURE{
uint16_t cnt;
uint16_t VDC;
uint16_t IDC;
uint16_t VAC;
uint16_t IAC;
uint16_t PAC;
uint16_t PRL; //da el % de potencia de operación
uint16_t CAC; //startups??
uint16_t KYR;//energy year
uint16_t KLY; //energy last year
uint16_t KMT;//energy month
uint16_t KLM; //energy last month
uint16_t KDY; //energy day
uint16_t KT0;//total energy
uint16_t TKK; //temperature
uint16_t KHR; //operating  hours
uint16_t KLD; //energy yesterday
};

SEND_DATA_STRUCTURE mydata;


void setup() {
  Serial.begin(19200);
  outSerial.begin(19200);
  mySerial.begin(19200);
  mydata.cnt=0;
  Serial.println("\nSoftware serial test started");
  Serial.println("\nSolarMax 3000C + Nodemcu:");
//  outSerial.begin(19200);
  //start the library, pass in the data details and the name of the serial port.
  ET.begin(details(mydata), &outSerial);
  Serial.print( "tamano:");
  int tam=sizeof(mydata);
  Serial.println( tam);
  delay (5000);
}

void loop() {
  Serial.write("{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}");
  mySerial.write("{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}\r\n");
  //mySerial.println();
  Serial.println();
  delay(1000);
  while (mySerial.available() > 0) {
   c= mySerial.read();
   lectura=lectura+char(c);
   }
  Serial.println("Lectura:");
  Serial.println(lectura);
  traducepotencia(lectura);
  ET.sendData();
  Serial.println("Comando energia1:");
  lectura="";
  delay (10000);
////////////////////////////////////////energía
  Serial.write("{FB;05;26|64:CAC;KHR;KDY;KLD;KMT|08AE}");
  mySerial.write("{FB;05;26|64:CAC;KHR;KDY;KLD;KMT|08AE}\r\n");
   //mySerial.println();
  Serial.println();
  delay(1000);
  while (mySerial.available() > 0) {
   c= mySerial.read();
   lecturaenergia=lecturaenergia+char(c);
   }
   
  Serial.println("Lecturaenergia:");
  Serial.println(lecturaenergia);
  traduceenergia(lecturaenergia);
   ET.sendData();
  Serial.println("Comando energia2");
  lecturaenergia="";
  delay (10000);

  ///////////////////////////////
  ////////////////////////////////////////energía2
  Serial.write("{FB;05;26|64:KLM;KYR;KLY;TKK;KT0|08D6}");
  mySerial.write("{FB;05;26|64:KLM;KYR;KLY;TKK;KT0|08D6}\r\n");
   //mySerial.println();
  Serial.println();
  delay(1000);
  while (mySerial.available() > 0) {
   c= mySerial.read();
   lecturaenergia=lecturaenergia+char(c);
   }
   
  Serial.println("Lecturaenergia:");
  Serial.println(lecturaenergia);
  traduceenergia2(lecturaenergia);
   ET.sendData();
  Serial.println("Comando potencia");
  lecturaenergia="";
  delay (10000);
  
  mydata.cnt++;
  if (mydata.cnt>999){mydata.cnt=0;}
  ET.sendData();
  Serial.print("Counter:");
  Serial.println(mydata.cnt);
} //fin loop

 String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}



void traducepotencia(String lectura){
    /*int firstEqual = lectura.indexOf('=');
    int secondEqual = firstEqual + 1;
    secondEqual = lectura.indexOf(';', secondEqual);
    String UDC=lectura.substring(firstEqual+1, secondEqual);
    Serial.print("VDC : ");
    Serial.println(UDC);
    unsigned long VDC= hex2int(UDC,UDC.length());
    Serial.println(VDC);
    int Equal3 = lectura.indexOf(';', secondEqual+1);
    String IDCs=lectura.substring(secondEqual+1, Equal3);
    unsigned long IDC= hex2int(IDCs,IDCs.length());
    Serial.print("IDC: ");
    Serial.println(IDC);
        */
        //String split = "hi this is a split test";
    //String split = lectura;
    String UDCs = getValue(lectura, '=', 1);
    //Serial.print ("UDCs:");
    //Serial.println(UDCs);
    UDCs = getValue(UDCs, ';', 0);
    VDC=hex2int(UDCs);
    mydata.VDC=VDC;
    Serial.print ("VDC:");
    Serial.println(VDC);
    
    
    String IDCs = getValue(lectura, '=', 2);
    //Serial.print ("IDCs:");
    //Serial.println(IDCs);
    IDCs = getValue(IDCs, ';', 0);
    IDC=hex2int(IDCs);
    mydata.IDC=IDC;
    Serial.print ("IDC:");
    Serial.println(IDC);

    String UL1 = getValue(lectura, '=', 3);
    //Serial.print ("UL1:");
    //Serial.println(UL1);
    UL1 = getValue(UL1, ';', 0);
    VAC=hex2int(UL1);
    mydata.VAC=VAC;
    Serial.print ("VAC:");
    Serial.println(VAC);

    String IL1 = getValue(lectura, '=', 4);
    //Serial.print ("IL1:");
    //Serial.println(IL1);
    IL1 = getValue(IL1, ';', 0);
    IAC=hex2int(IL1);
    mydata.IAC=IAC;
    Serial.print ("IAC:");
    Serial.println(IAC);

    String PACs = getValue(lectura, '=', 5);
   // Serial.print ("PACs:");
   // Serial.println(PACs);
    PACs = getValue(PACs, ';', 0);
    PAC=hex2int(PACs); //da el doble ??
    mydata.PAC=PAC;
    Serial.print ("PAC:");
    Serial.println(PAC);

    String PRLs = getValue(lectura, '=', 6);
    //Serial.print ("PRLs:");
    //Serial.println(PRLs);
    PRLs = getValue(PRLs, ';', 0);
    PRLs = getValue(PRLs, '|', 0);
    PRL=hex2int(PRLs);
    mydata.PRL=PRL;
    Serial.print ("PRL:");
    Serial.println(PRL);
    

}

void traduceenergia(String lectura){
    /*
    {FB;05;26|64:CAC;KHR;KDY;KLD;KMT|08AE}
    
        */
        //String split = "hi this is a split test";
    //String split = lectura;
    String CACs = getValue(lectura, '=', 1);
    CACs = getValue(CACs, ';', 0);
    CAC=hex2int(CACs);
    mydata.CAC=CAC;
    Serial.print ("CAC:");
    Serial.println(CAC);
    
    
    String KHRs = getValue(lectura, '=', 2);
    KHRs = getValue(KHRs, ';', 0);
    KHR=hex2int(KHRs);
    mydata.KHR=KHR;
    Serial.print ("KHR:");
    Serial.println(KHR);

    String KDYs = getValue(lectura, '=', 3);
    KDYs = getValue(KDYs, ';', 0);
    KDY=hex2int(KDYs);
    mydata.KDY=KDY;
    Serial.print ("KDY:");
    Serial.println(KDY);

    String KLDs = getValue(lectura, '=', 4);
    KLDs = getValue(KLDs, ';', 0);
    KLD=hex2int(KLDs);
    mydata.KLD=KLD;
    Serial.print ("KLD:");
    Serial.println(KLD);

    String KMTs = getValue(lectura, '=', 5);
    KMTs = getValue(KMTs, ';', 0);
    KMTs = getValue(KMTs, '|', 0);
    KMT=hex2int(KMTs); 
    mydata.KMT=KMT;
    Serial.print ("KMT:");
    Serial.println(KMT);

}

void traduceenergia2(String lectura){
    /*
    {FB;05;22|64:KLM;KYR;KLY;KT0|07AD}
    {FB;05;26|64:KLM;KYR;KLY;TKK;KT0|08D6}
    
        */
   
    String KLMs = getValue(lectura, '=', 1);
    KLMs = getValue(KLMs, ';', 0);
    KLM=hex2int(KLMs);
    mydata.KLM=KLM;
    Serial.print ("KLM:");
    Serial.println(KLM);

    String KYRs = getValue(lectura, '=', 2);
    KYRs = getValue(KYRs, ';', 0);
    KYR=hex2int(KYRs);
    mydata.KYR=KYR;
    Serial.print ("KYR:");
    Serial.println(KYR);
    
    String KLYs = getValue(lectura, '=', 3);
    KLYs = getValue(KLYs, ';', 0);
    KLY=hex2int(KLYs);
    mydata.KLY=KLY;
    Serial.print ("KLY:");
    Serial.println(KLY);

    String TKKs = getValue(lectura, '=', 4);
    TKKs = getValue(TKKs, ';', 0);
    mydata.TKK=hex2int(TKKs);
    Serial.print ("TKK:");
    Serial.println(mydata.TKK);
    
    String KT0s = getValue(lectura, '=', 5);
    KT0s = getValue(KT0s, ';', 0);
    KT0s = getValue(KT0s, '|', 0);
    KT0=hex2int(KT0s);
    mydata.KT0=KT0;
    Serial.print ("KT0:");
    Serial.println(KT0);

}

/*
int hex2int(String a)
{
   int i;
   unsigned long val = 0;
   int len=a.length();

   for(i=0;i<len;i++)
      if(a[i] <= 57)
       val += (a[i]-48)*(1<<(4*(len-1-i)));
      else
       val += (a[i]-55)*(1<<(4*(len-1-i)));
   return (int) val;
}
*/
unsigned int hex2int(String hexString) {
  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}


