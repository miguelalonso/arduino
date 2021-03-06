
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

//#include <FS.h>       //this needs to be first, or it all crashes and burns...
 
#include <SoftwareSerial.h>

//SoftwareSerial mySerial(14, 12, false, 128);
//SoftwareSerial mySerial(2,12,false,128);// D4(RX), D6(TX) nodemcu
SoftwareSerial mySerial(13,15,true,64);// D4(RX), D5(TX) nodemcu

String potencia= "{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}";
//String potencia= "{FB;05;4E|64:E1D;E11;E1h;E1m;E1M;E2D;E21;E2h;E2m;E2M;E3D;E31;E3h;E3m;E3M|1270}";

String energia="{FB;05;36|64:CAC;KHR;KDY;KLD;KMT;KLM;KYR;KLY;KT0|0D34}";
String energia2="{FB;05;26|64:CAC;KHR;KDY;KLD;KMT|08AE}";
String energia3="{FB;05;22|64:KLM;KYR;KLY;KT0|07AD}";

String lectura="";
String lecturaenergia="";
char c;

float VDC;
float IDC;
float VAC;
float IAC;
float PAC;
float PRL; //da el % de potencia de operación
float CAC; //startups??
float KYR;//energy year
float KLY; //energy last year
float KMT;//energy month
float KLM; //energy last month
float KDY; //energy day
float KT0;//total energy
float TKK; //temperature
float KHR; //operating  hours
float KLD; //energy yesterday

void setup() {
  Serial.begin(9600);
  mySerial.begin(19200);

  Serial.println("\nSoftware serial test started");
  Serial.println("\nSolarMax 3000C + Nodemcu:");
}

void loop() {
  
  Serial.write("{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}");
  mySerial.write("{FB;05;2A|64:UDC;IDC;UL1;IL1;PAC;PRL|099F}\r\n");
  //mySerial.println();
  Serial.println();
  delay(2500);
  while (mySerial.available() > 0) {
   c= mySerial.read();
   lectura=lectura+char(c);
   }
  Serial.println("Lectura:");
  Serial.println(lectura);
  traducepotencia(lectura);

  Serial.println("Comando energia:");
  lectura="";
  delay (5000);
////////////////////////////////////////energía
  Serial.write("{FB;05;26|64:CAC;KHR;KDY;KLD;KMT|08AE}");
  mySerial.write("{FB;05;26|64:CAC;KHR;KDY;KLD;KMT|08AE}\r\n");
   //mySerial.println();
  Serial.println();
  delay(2500);
  while (mySerial.available() > 0) {
   c= mySerial.read();
   lecturaenergia=lecturaenergia+char(c);
   }
   
  Serial.println("Lecturaenergia:");
  Serial.println(lecturaenergia);
  traduceenergia(lecturaenergia);
  Serial.println("Comando energia 2");
  lecturaenergia="";
  delay (5000);

  ///////////////////////////////
  ////////////////////////////////////////energía2
  Serial.write("{FB;05;22|64:KLM;KYR;KLY;KT0|07AD}");
  mySerial.write("{FB;05;22|64:KLM;KYR;KLY;KT0|07AD}\r\n");
   //mySerial.println();
  Serial.println();
  delay(2500);
  while (mySerial.available() > 0) {
   c= mySerial.read();
   lecturaenergia=lecturaenergia+char(c);
   }
   
  Serial.println("Lecturaenergia:");
  Serial.println(lecturaenergia);
  traduceenergia2(lecturaenergia);
  Serial.println("Comando potencia");
  lecturaenergia="";
  delay (5000);
 
}

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
    VDC=hex2int(UDCs)/10;
    Serial.print ("VDC:");
    Serial.println(VDC);
    
    
    String IDCs = getValue(lectura, '=', 2);
    //Serial.print ("IDCs:");
    //Serial.println(IDCs);
    IDCs = getValue(IDCs, ';', 0);
    IDC=hex2int(IDCs)/100;
    Serial.print ("IDCs:");
    Serial.println(IDC);

    String UL1 = getValue(lectura, '=', 3);
    //Serial.print ("UL1:");
    //Serial.println(UL1);
    UL1 = getValue(UL1, ';', 0);
    VAC=hex2int(UL1)/10;
    Serial.print ("VAC:");
    Serial.println(VAC);

    String IL1 = getValue(lectura, '=', 4);
    //Serial.print ("IL1:");
    //Serial.println(IL1);
    IL1 = getValue(IL1, ';', 0);
    IAC=hex2int(IL1)/100;
    Serial.print ("IAC:");
    Serial.println(IAC);

    String PACs = getValue(lectura, '=', 5);
   // Serial.print ("PACs:");
   // Serial.println(PACs);
    PACs = getValue(PACs, ';', 0);
    PAC=hex2int(PACs)/2; //da el doble ??
    Serial.print ("PAC:");
    Serial.println(PAC);

    String PRLs = getValue(lectura, '=', 6);
    //Serial.print ("PRLs:");
    //Serial.println(PRLs);
    PRLs = getValue(PRLs, ';', 0);
    PRLs = getValue(PRLs, '|', 0);
    PRL=hex2int(PRLs);
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
    Serial.print ("CAC:");
    Serial.println(CAC);
    
    
    String KHRs = getValue(lectura, '=', 2);
    KHRs = getValue(KHRs, ';', 0);
    KHR=hex2int(KHRs);
    Serial.print ("KHR:");
    Serial.println(KHR);

    String KDYs = getValue(lectura, '=', 3);
    KDYs = getValue(KDYs, ';', 0);
    KDY=hex2int(KDYs);
    Serial.print ("KDY:");
    Serial.println(KDY);

    String KLDs = getValue(lectura, '=', 4);
    KLDs = getValue(KLDs, ';', 0);
    KLD=hex2int(KLDs);
    Serial.print ("KLD:");
    Serial.println(KLD);

    String KMTs = getValue(lectura, '=', 5);
    KMTs = getValue(KMTs, ';', 0);
    KMTs = getValue(KMTs, '|', 0);
    KMT=hex2int(KMTs); 
    Serial.print ("KMT:");
    Serial.println(KMT);

}

void traduceenergia2(String lectura){
    /*
    {FB;05;22|64:KLM;KYR;KLY;KT0|07AD}
    
        */
   
    String KLMs = getValue(lectura, '=', 1);
    KLMs = getValue(KLMs, ';', 0);
    KLM=hex2int(KLMs);
    Serial.print ("KLM:");
    Serial.println(KLM);

    String KYRs = getValue(lectura, '=', 2);
    KYRs = getValue(KYRs, ';', 0);
    KYR=hex2int(KYRs);
    Serial.print ("KYR:");
    Serial.println(KYR);
    
    String KLYs = getValue(lectura, '=', 3);
    KLYs = getValue(KLYs, ';', 0);
    KLY=hex2int(KLYs);
    Serial.print ("KLY:");
    Serial.println(KLY);

    String KT0s = getValue(lectura, '=', 4);
    KT0s = getValue(KT0s, ';', 0);
    KT0s = getValue(KT0s, '|', 0);
    KT0=hex2int(KT0s);
    Serial.print ("KT0:");
    Serial.println(KT0);

}

float hex2int(String a)
{
   int i;
   unsigned long val = 0;
   int len=a.length();

   for(i=0;i<len;i++)
      if(a[i] <= 57)
       val += (a[i]-48)*(1<<(4*(len-1-i)));
      else
       val += (a[i]-55)*(1<<(4*(len-1-i)));
   return (float) val;
}

