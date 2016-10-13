#include <SoftEasyTransfer.h>

/*   For Arduino 1.0 and newer, do this:   */
#include <SoftwareSerial.h>
SoftwareSerial mySerial(14, 12,false,128);
//create object
SoftEasyTransfer ET; 

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int cnt;
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
};

//give a name to the group of data
RECEIVE_DATA_STRUCTURE mydata;

void setup(){
  Serial.begin(19200);
  mySerial.begin(19200);
  //start the library, pass in the data details and the name of the serial port.
  ET.begin((byte*)&mydata,66, &mySerial);
  
  
  
}

void loop(){
  //check and see if a data packet has come in. 
  if(ET.receiveData()){
    //this is how you access the variables. [name of the group].[variable name]
    //since we have data, we will blink it out. 
    Serial.println(mydata.PAC);
    Serial.println(mydata.cnt);
  }
  //you should make this delay shorter then your transmit delay or else messages could be lost
  //delay(250);
}
