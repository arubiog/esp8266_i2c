#include "sensors.h"

int val[32]={false};
//int valo[32]={false};


double BasicSensor::getValue(){
  double x;
  x = readFromHW();
  return (a*x+b);

}

bool BasicSensor::isOutOfRange(){
  double value = getValue();
  if (value < v_min or value > v_max)
    return true;
  return false;
  
}

double SimulatorSensor::readFromHW(){
  return 3.14;
}

double TC74Sensor::readFromHW(){
  //start the communication with IC with the address xx
  Wire.begin();
  Wire.beginTransmission((uint8_t)address); 
  //send a bit and ask for register zero
  Wire.write((uint8_t) 0);
  //end transmission
  int error = Wire.endTransmission();
  if (error != 0){
    return -1e9;
  }
  //request 1 byte from address xx
  Wire.requestFrom((uint8_t)address, (uint8_t)1);

  //To detect the error.
  int temp = 1e9;
  for (int i=0; i < 3; i++){
    if (Wire.available() == 0){
      delay(10);    
    }
    else{
      temp = Wire.read();
      if (temp > 127)
        temp -= 256;
    }
  }
  return double(temp);
    
}

double ADCSensor::readFromHW(){
  return analogRead(adc_channel); 
}

double MAX31855::readCelsius(void) {
  int16_t v;
  v = readFromHW();
   //if fault bit set // return 2000deg
  if (v & 0x1) 
            return 2000;    
  v&=0xfffc; // mask lower two bits
  return v / 16.0;
}

double MAX31855::readFarenheit(void) {
           return readCelsius() * 9.0/5.0 + 32;
}


double MAX31855::readFromHW(void) { 
  uint32_t d = 0; // we only need last 16 bits, first 16 will be discarded
  //uint32_t val[32]= {false};
  /* Read the chip and return the raw temperature value */
  /* Bring CS pin low to allow us to read the data from
   the conversion process */
  digitalWrite((uint8_t)cs,LOW);
  delay(10);
   /*
   Read bits 14-3 from MAX6675 for the Temp. Loop for each bit reading
   the value and storing the final value in 'temp'
   */
  for (int i=31; i>=0; i--) {
    digitalWrite((uint8_t)sclk,HIGH);
    delay(1);    
    d += digitalRead((uint8_t)miso) << i;
    val[i]= digitalRead((uint8_t)miso);

    //Serial.println(value,BIN);
    digitalWrite((uint8_t)sclk,LOW);
    delay(1);
    //Serial.println (val[i]) ;
  }
  digitalWrite((uint8_t)cs,HIGH);
  delay(20);

  
  for (int j=31; j>=0; j--){ // output each array element's value 
    Serial.print (val[j],BIN) ;
  }
  Serial.println ("uno") ;
  Serial.println ("next") ;
  // check bit D2 if HIGH no sensor
  if ((d & 0x04) == 0x04){ Serial.println("VCC"); return -1;}
  if ((d & 0x02) == 0x02){ Serial.println("GND"); return -1;}
  if ((d & 0x01) == 0x01){ Serial.println("OPEN CIRCUIT"); return -1;}
  // shift right three places
  return d >> 3;
}

