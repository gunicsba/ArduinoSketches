//WithMotion W901 test
#include "REG.h"
#include "wit_c_sdk.h"

#define ACC_UPDATE		0x01
#define GYRO_UPDATE		0x02
#define ANGLE_UPDATE	0x04
#define MAG_UPDATE		0x08
#define READ_UPDATE		0x80
static volatile char s_cDataUpdate = 0, s_cCmd = 0xff; 
static void SensorUartSend(uint8_t *p_data, uint32_t uiSize);
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum);
static void Delayms(uint16_t ucMs);

//BNO
#include "BNO08x_AOG.h"
BNO080 bno08x;
#define ImuWire Wire
#define RAD_TO_DEG_X_10 572.95779513082320876798154814105
#define REPORT_INTERVAL 20    //BNO report time, we want to keep reading it quick & offen. Its not timmed to anything just give constant data.
  float roll = 0;
  float yaw = 0;
  float pitch = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WitInit(WIT_PROTOCOL_NORMAL, 0x50);
	WitSerialWriteRegister(SensorUartSend);
	WitRegisterCallBack(SensorDataUpdata);
  WitDelayMsRegister(Delayms);
  delay(500);
  Serial1.begin(0,  SERIAL_8N1, 16, 17);
  Serial1.flush();
  WitReadReg(AX, 3);
	delay(200);
  while(Serial1.available()){
    WitSerialDataIn(Serial1.read());
  }
  if(s_cDataUpdate != 0) {
		Serial.print("0 baud find sensor\r\n\r\n");
	}

  WitWriteReg(KEY, KEY_UNLOCK);   delay(20);
  WitWriteReg(AXIS6, 0x01);       delay(20);
  WitWriteReg(SAVE, SAVE_PARAM);  delay(20);
  WitWriteReg(KEY, KEY_UNLOCK);   delay(20);
  WitWriteReg(ACCFILT, 100);      delay(20);
  WitWriteReg(SAVE, SAVE_PARAM);  delay(20);

  WitReadReg(ACCFILT, 3);      delay(20);
  WitReadReg(AXIS6, 3);        delay(20);
  Serial.print("ACCFILT: ");
  Serial.println(sReg[ACCFILT]);
  Serial.print(" sRegAxis6:");
  Serial.println(sReg[AXIS6]);
  
//  WitSetBandwidth(BANDWIDTH_21HZ); delay(20);
//  WitSetOutputRate(RRATE_20HZ);  delay(20);
  WitSetBandwidth(BANDWIDTH_94HZ); delay(20);
  WitSetOutputRate(RRATE_100HZ);  delay(20);
  WitSetContent(RSW_ANGLE);  delay(20);

  delay(300);
      Serial.print("%%%%%%%      ACCFILT: ");
      Serial.print(sReg[ACCFILT]);
      Serial.print("   sRegAxis6: ");
      Serial.println(sReg[AXIS6]);

  ImuWire.begin();

  scanI2CDevices();

  //delay(2000);


              // Initialize BNO080 lib
              if (bno08x.begin(0x4A, ImuWire)) //??? Passing NULL to non pointer argument, remove maybe ???
              {
                  //Increase I2C data rate to 400kHz
                  ImuWire.setClock(400000); 

                  delay(300);

                  // Use gameRotationVector and set REPORT_INTERVAL
                  bno08x.enableGameRotationVector(REPORT_INTERVAL);
              } else { Serial.println("Failed to init BNO!!!!"); delay(5000);}

}

int i;
float fAcc[3], fGyro[3], fAngle[3];
uint32_t LASTPRINT = 0;

void loop() {

  if( millis() > LASTPRINT + 5000){
    LASTPRINT = millis();
      WitReadReg(ACCFILT, 3);
      WitReadReg(AXIS6, 3);
      Serial.print(millis());
      Serial.print("%%%%%%%      ACCFILT: ");
      Serial.print(sReg[ACCFILT]);
      Serial.print("   sRegAxis6: ");
      Serial.println(sReg[AXIS6]);
/*
    for(int j = 0; j < REGSIZE; j++){
      Serial.print(j, HEX);
      Serial.print(" ");
      Serial.println(sReg[j], HEX);
    }*/
  }

  readBNO();
  // put your main code here, to run repeatedly:
  while(Serial1.available()){
    WitSerialDataIn(Serial1.read());
  }
  	if(s_cDataUpdate)
		{
			for(i = 0; i < 3; i++)
			{
				fAcc[i] = sReg[AX+i] / 32768.0f * 16.0f;
				fGyro[i] = sReg[GX+i] / 32768.0f * 2000.0f;
				fAngle[i] = sReg[Roll+i] / 32768.0f * 180.0f;
			}
			if(s_cDataUpdate & ACC_UPDATE & 0)
			{
				Serial.print("acc:");
				Serial.print(fAcc[0], 3);
				Serial.print(" ");
				Serial.print(fAcc[1], 3);
				Serial.print(" ");
				Serial.print(fAcc[2], 3);
				Serial.print("\r\n");
				s_cDataUpdate &= ~ACC_UPDATE;
			}
			if(s_cDataUpdate & GYRO_UPDATE & 0)
			{
				Serial.print("gyro:");
				Serial.print(fGyro[0], 1);
				Serial.print(" ");
				Serial.print(fGyro[1], 1);
				Serial.print(" ");
				Serial.print(fGyro[2], 1);
				Serial.print("\r\n");
				s_cDataUpdate &= ~GYRO_UPDATE;
			}
			if(s_cDataUpdate & ANGLE_UPDATE)
			{
//				Serial.print("WITanglePitch:");
//				Serial.print(fAngle[0], 3);
				Serial.print(" WITRoll:");
				Serial.print((fAngle[1]*10)-16, 3);
				Serial.print(" WITHeading:");
        float withHeading = fAngle[2]*-10;
        if(withHeading < 0) withHeading += 3600;
				Serial.print(withHeading, 3);
				Serial.print("\r\n");
				s_cDataUpdate &= ~ANGLE_UPDATE;
        printBNO();
			}
			if(s_cDataUpdate & MAG_UPDATE & 0)
			{
				Serial.print("mag:");
				Serial.print(sReg[HX]);
				Serial.print(" ");
				Serial.print(sReg[HY]);
				Serial.print(" ");
				Serial.print(sReg[HZ]);
				Serial.print("\r\n");
				s_cDataUpdate &= ~MAG_UPDATE;
			}
      s_cDataUpdate = 0;
		}
}



static void SensorUartSend(uint8_t *p_data, uint32_t uiSize)
{
  Serial1.write(p_data, uiSize);
  Serial1.flush();
}
static void Delayms(uint16_t ucMs)
{
  delay(ucMs);
}
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum)
{
	int i;
    for(i = 0; i < uiRegNum; i++)
    {
        switch(uiReg)
        {
            case AZ:
				s_cDataUpdate |= ACC_UPDATE;
            break;
            case GZ:
				s_cDataUpdate |= GYRO_UPDATE;
            break;
            case HZ:
				s_cDataUpdate |= MAG_UPDATE;
            break;
            case Yaw:
				s_cDataUpdate |= ANGLE_UPDATE;
            break;
            default:
				s_cDataUpdate |= READ_UPDATE;
			break;
        }
		uiReg++;
    }
}

void readBNO()
{
          if (bno08x.dataAvailable() == true)
        {
            float dqx, dqy, dqz, dqw, dacr;
            uint8_t dac;

            //get quaternion
            bno08x.getQuat(dqx, dqy, dqz, dqw, dacr, dac);
/*            
            while (bno08x.dataAvailable() == true)
            {
                //get quaternion
                bno08x.getQuat(dqx, dqy, dqz, dqw, dacr, dac);
                //Serial.println("Whiling");
                //Serial.print(dqx, 4);
                //Serial.print(F(","));
                //Serial.print(dqy, 4);
                //Serial.print(F(","));
                //Serial.print(dqz, 4);
                //Serial.print(F(","));
                //Serial.println(dqw, 4);
            }
            //Serial.println("End of while");
*/            
            float norm = sqrt(dqw * dqw + dqx * dqx + dqy * dqy + dqz * dqz);
            dqw = dqw / norm;
            dqx = dqx / norm;
            dqy = dqy / norm;
            dqz = dqz / norm;

            float ysqr = dqy * dqy;

            // yaw (z-axis rotation)
            float t3 = +2.0 * (dqw * dqz + dqx * dqy);
            float t4 = +1.0 - 2.0 * (ysqr + dqz * dqz);
            yaw = atan2(t3, t4);

            // Convert yaw to degrees x10
//            correctionHeading = -yaw;
            yaw = (int16_t)((yaw * -RAD_TO_DEG_X_10));
            if (yaw < 0) yaw += 3600;

            // pitch (y-axis rotation)
            float t2 = +2.0 * (dqw * dqy - dqz * dqx);
            t2 = t2 > 1.0 ? 1.0 : t2;
            t2 = t2 < -1.0 ? -1.0 : t2;
//            pitch = asin(t2) * RAD_TO_DEG_X_10;

            // roll (x-axis rotation)
            float t0 = +2.0 * (dqw * dqx + dqy * dqz);
            float t1 = +1.0 - 2.0 * (dqx * dqx + ysqr);
//            roll = atan2(t0, t1) * RAD_TO_DEG_X_10;

            if(false )//steerConfig.IsUseY_Axis)
            {
              roll = asin(t2) * RAD_TO_DEG_X_10;
              pitch = atan2(t0, t1) * RAD_TO_DEG_X_10;
            }
            else
            {
              pitch = asin(t2) * RAD_TO_DEG_X_10;
              roll = atan2(t0, t1) * RAD_TO_DEG_X_10;
            }
        }
}
void printBNO(){
              Serial.println();
            Serial.print("BNORoll:");
            Serial.print(roll*-1);
//            Serial.print(" BNOPitch:");
//            Serial.print(pitch);
            Serial.print(" BNOHeading:");
            Serial.print(yaw);
            Serial.println("");
}

String scanI2CDevices(){
  String forReturn="";
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");   /*ESP32 starts scanning available I2C devices*/
  forReturn += "Scanning...\n";
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {   /*for loop to check number of devices on 127 address*/
    
    ImuWire.beginTransmission(address);
    error = ImuWire.endTransmission();
    if (error == 0) {   /*if I2C device found*/
      Serial.print("I2C device found at address 0x");/*print this line if I2C device found*/
      forReturn += "I2C device found at address 0x";
      if (address<16) {
        Serial.print("0");
        forReturn += "0";
      }
      Serial.println(address,HEX);  /*prints the HEX value of I2C address*/
      forReturn += String(address, HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknown error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n"); /*If no I2C device attached print this message*/
    forReturn += "No I2C devices found\n";
  }
  else {
    Serial.println("done\n");
  }
  return forReturn;
}