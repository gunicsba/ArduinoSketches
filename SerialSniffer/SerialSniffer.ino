#include <Bounce.h>
#include <elapsedMillis.h>
#include <EEPROM.h>
#include <SoftwareSerial.h> 


#define debug false


  // if not in eeprom, overwrite 
  #define EEP_Ident 0x5424

  //--------------------------- Switch Input Pins ------------------------
  Bounce steerSw = Bounce(8, 10); // pushbutton on pin 8
  Bounce workSw = Bounce(9, 10); // pushbutton on pin 9

  HardwareSerial* SerialOne = &Serial5;   //???
  //SoftwareSerial SerialOne = SoftwareSerial(21,20);// rxPin, txPin,
  HardwareSerial* SerialTwo = &Serial7;   //???

  #define SPEED_SIZE 5
  const int serialSpeed[SPEED_SIZE] = {9600,19200,38400,57600,115200};
  uint8_t lastSerialSpeed = 0;
    //EEPROM
  int16_t EEread = 0;

  uint8_t sniff = false;
  elapsedMillis emit;
  elapsedMillis ellapsedOne;
  elapsedMillis ellapsedTwo;
  //int incomingByte = 0; // for incoming serial data
  char incomingByte = 0; // for incoming serial data
  String strOne;
  String strTwo;

void setup() {
    //pinMode(21, INPUT);pinMode(20, OUTPUT);  
    delay(100);
    EEPROM.get(0, EEread);     // read identifier
      
    if (EEread != EEP_Ident)   // check on first start and write EEPROM
    {           
      EEPROM.put(0, EEP_Ident);
      EEPROM.put(6, lastSerialSpeed); //Machine
    }
    else {
      EEPROM.get(6, lastSerialSpeed); //Machine
    }
  
  Serial.begin(115200); //Console
  
  pinMode(8,INPUT_PULLUP);
  pinMode(9,INPUT_PULLUP);

  Serial.println("Available speeds: ");
  for(int i = 0 ; i < SPEED_SIZE ; i++){
    Serial.print(serialSpeed[i]);
    Serial.print(",");
  }
  Serial.println();
  if(lastSerialSpeed > SPEED_SIZE) lastSerialSpeed = 0;
  // put your setup code here, to run once:
  SerialOne->begin(serialSpeed[lastSerialSpeed]);
  delay(10);
  SerialTwo->begin(serialSpeed[lastSerialSpeed]);
  delay(10);
  
  Serial.print("Baud rate is set to ");
  Serial.println(serialSpeed[lastSerialSpeed]);
  delay(100);
}

void loop() {
  //Buttons
  // 1 PCB Button pressed?
  steerSw.update();
  workSw.update();
//  delay(100);
  if(steerSw.risingEdge()){    
    lastSerialSpeed++;
    if(lastSerialSpeed >= SPEED_SIZE) lastSerialSpeed = 0;
    Serial.print("SteerSwitch Pressed switching baud rate to ");
    Serial.println(serialSpeed[lastSerialSpeed]);
    EEPROM.put(6, lastSerialSpeed); //Machine
    SerialOne->flush();
    SerialTwo->flush();
    delay(1000);
    SerialOne->begin(serialSpeed[lastSerialSpeed]);
    SerialTwo->begin(serialSpeed[lastSerialSpeed]);
  }

  if(workSw.risingEdge()){
    sniff = !sniff;
    if(sniff){
      Serial.println("WorkSwitch pressed: Current mode is sniffing!");
    } else {
      Serial.println("WorkSwitch pressed: Current mode is emitting, we'll send some dummy message at 1Hz");
    }
  }

  if(!sniff && emit > 1000){
    emit = 0;
    Serial.print(millis());
    Serial.println("C This is a dummy message");
    SerialOne->print(millis());
    SerialOne->println(" ->5 This is a dummy message");
    SerialTwo->print(millis());
    SerialTwo->println(" ->7 This is a dummy message");
  }
  
  while(SerialOne->available() > 0){
    incomingByte = SerialOne->read();
    SerialTwo->write(incomingByte);
    if(debug) Serial.write(incomingByte);
    strOne.concat(incomingByte);
    ellapsedOne=0;
    if(incomingByte == '\n') {
      Serial.print("<<<<< ");
      Serial.println(strOne);
      strOne = "";
    }
  }
  if(ellapsedOne > 500 && !strOne.equals("")){
    ellapsedTwo=0;
    Serial.print("<<<<< ");
    Serial.print(strOne);
    strOne="";
  }

  while(SerialTwo->available() > 0){
    incomingByte = SerialTwo->read();
    SerialOne->write(incomingByte);
    if(debug) Serial.write(incomingByte);    
    strTwo.concat(incomingByte);
    ellapsedTwo=0;
    if(incomingByte == '\n') {
      Serial.print(millis());
      Serial.print(">>>>> ");
      Serial.print(strTwo);
      strTwo = "";
    }
  }
  if(ellapsedTwo > 500 && !strTwo.equals("")){
    ellapsedTwo=0;
    Serial.print(millis());
    Serial.print(">>>>> ");
    Serial.println(strTwo);
    strTwo="";
  }

  while(Serial.available() > 0){
    incomingByte = Serial.read();
    SerialOne->write(incomingByte);
    SerialTwo->write(incomingByte);    
  }
}
