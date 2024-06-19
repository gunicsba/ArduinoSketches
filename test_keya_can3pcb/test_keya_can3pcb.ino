//Thank you manu for the skeleton :)
#include <FlexCAN_T4.h>
#include <Arduino.h>


const int led = LED_BUILTIN;

CAN_message_t msg;
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> keyabus;


int i=0;
int writeStatus=0;
int speed=0;
int encoder = 0;
long WASvalue = 0;


void sendKeyaMessage(uint8_t buf[8] = { 0 }){
    CAN_message_t msgs;
    msgs.id=0x06000001;
    msgs.flags.extended=1;
    msgs.len=8;
    msgs.buf[0]=buf[0];
    msgs.buf[1]=buf[1];
    msgs.buf[2]=buf[2];
    msgs.buf[3]=buf[3];
    msgs.buf[4]=buf[4];
    msgs.buf[5]=buf[5];
    msgs.buf[6]=buf[6];
    msgs.buf[7]=buf[7];
    keyabus.write(msgs);
}

void checkKeyaWas() {
  Serial.println("Sending WAS query!");
  sendKeyaMessage(new uint8_t[8] {0x40, 0x04, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00}); //Encoder count value query
  handleKeyaMsg();
}


void sendSpeed(){
  return;
    Serial.printf("%d RPM\n", speed);
    int encoded=speed*10;
    uint8_t lastByte=0x00;
    Serial.printf("%x %x\n", highByte(encoded), lowByte(encoded));

    if (speed<0){
        lastByte=0xFF;
    }
    //Speed = 0x23 00 20 01 00 00 00 00 
    sendKeyaMessage(new uint8_t[8] {0x23, 0x00, 0x20, 0x01, highByte(encoded), lowByte(encoded), lastByte, lastByte});
}
void sendPosition(){
//    Serial.printf("Position:%d\n", speed);
    int encoded=speed*10;
    uint8_t lastByte=0x00;
//    Serial.printf("%x %x\n", highByte(encoded), lowByte(encoded));

    if (speed<0){
        lastByte=0xFF;
    }
    //Position = 0x23 02 20 01 00 00 00 00 
    sendKeyaMessage(new uint8_t[8] {0x23, 0x02, 0x20, 0x01, highByte(encoded), lowByte(encoded), lastByte, lastByte });
}



void printMsg(const char order[], CAN_message_t &msg){
    Serial.print("CAN "); Serial.print(order);Serial.print(" ");
    Serial.print("  ID: 0x"); Serial.print(msg.id, HEX );
    Serial.print(" DATA: ");
    for ( uint8_t i = 0; i < msg.len; i++ ) {
        Serial.printf("%02X ", msg.buf[i]);
    }
    Serial.print("  TS: "); Serial.println(msg.timestamp);
}

void handleKeyaMsg() {
    if ( keyabus.read(msg) ) {
        if(msg.id ==0x7000001) {     // heartbeat encoder, speed, current, error
          encoder = msg.buf[1] | (msg.buf[0] << 8);
          if(encoder >= 30000) encoder -= 65535;
          Serial.print("low:-100,high:100,");
          Serial.print("buf0:");
          Serial.print(msg.buf[0]);
          Serial.print(",buf1:");
          Serial.print(msg.buf[1]);
          Serial.print(",Encoder:");
          Serial.println(encoder);
        } else 
        if(msg.id == 0x05800001){ 
          if(msg.buf[0] == 0x60 && msg.buf[1] == 0x04 && msg.buf[2] == 0x21 && msg.buf[3] == 0x01) {
            Serial.print("Raw encoder value:");
            WASvalue = msg.buf[4] | (msg.buf[5] << 8) | (msg.buf[6] << 16) | (msg.buf[7] << 24) ;
            Serial.print(msg.buf[7]);
            Serial.print(" ");
            Serial.print(msg.buf[6]);
            Serial.print(" ");
            Serial.print(msg.buf[5]);
            Serial.print(" ");
            Serial.print(msg.buf[4]);
            Serial.print(",WASvalue:");
            Serial.println(WASvalue);
          }
          else printMsg("1",msg);
        } else printMsg("1",msg);
return;        
        if (msg.id==0x7000001 && writeStatus!=100){
            writeStatus++;
            // Uncomment return if you want to have less message printed.
            //return;
        }else{
            writeStatus=0;
        }
        printMsg("1", msg);
    }
}


void setup() {
    pinMode(led, OUTPUT);
    Serial.begin(115200);
    keyabus.begin();
    keyabus.setBaudRate(250000);
delay(2000);
Serial.println("Setting Absolute Position!");
    digitalWrite(led, HIGH);
    if ( keyabus.read(msg) ) printMsg("1", msg);
    sendKeyaMessage(new uint8_t[8] {0x03, 0x0D, 0x20, 0x31, 0x00, 0x00, 0x00, 0x00}); //Absolute Position
    delay(2000);
    //sendKeyaMessage(new uint8_t[8] {0x03, 0x0D, 0x20, 0x11, 0x00, 0x00, 0x00, 0x00}); //Speed mode
    if ( keyabus.read(msg) ) printMsg("1", msg);
//    sendKeyaMessage(new uint8_t[8] {0x23, 0x0C, 0x2F, 0x31, 0x00, 0x00, 0x00, 0x00}); //Zero Position Calibration
Serial.println("Position Reset!");
    sendKeyaMessage(new uint8_t[8] {0x23, 0x0C, 0x20, 0x09, 0x00, 0x00, 0x00, 0x00}); //Position Reset
    sendKeyaMessage(new uint8_t[8] {0x23, 0x0C, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00}); //OFF
    if ( keyabus.read(msg) ) printMsg("1", msg);
    
}

void loop(){
  checkKeyaWas();
  delay(50);
  handleKeyaMsg();
  handleKeyaMsg();
}

void loop1(){
    sendKeyaMessage(new uint8_t[8] {0x23, 0x0C, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00}); //OFF
    delay(50);
    handleKeyaMsg();
}



void loop2() {

    i++;
    if (i==50000){
        digitalWrite(led, LOW);
    }
    if (i==100000){
        digitalWrite(led, HIGH);
        i=0;
    }

      //Enable = 0x23 0D 20 01 00 00 00 00
      sendKeyaMessage(new uint8_t[8] {0x23, 0x0D, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00}); //ON
      speed=0;
      sendPosition();
      handleKeyaMsg();
      delay(100);
      for(int i = 0 ; i < 250; i++){
        speed=i;
        sendPosition();
        handleKeyaMsg();
        delay(3);  
      }
  //    delay(100);
      for(int i = 0 ; i < 250; i++){
        speed=250-i;
        sendPosition();
        handleKeyaMsg();
        delay(3);  
      }
      delay(100);
      for(int i = 0 ; i < 250; i++){
        speed=-i;
        sendPosition();
        handleKeyaMsg();
        delay(3);  
      }
//      delay(100);
      for(int i = 0 ; i < 250; i++){
        speed=-250+i;
        sendPosition();
        handleKeyaMsg();
        delay(3);  
      }
      delay(100);
      //0x23 0C 20 09 -> position reset
      // Disable = 0x23 0C 20 01 00 00 00 00
      sendKeyaMessage(new uint8_t[8] {0x23, 0x0C, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00}); //OFF

}
