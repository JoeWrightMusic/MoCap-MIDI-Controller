// Libraries for the MPU6050, MPR121, Weighted Average Filter, & MIDIUSB
// Install these from the libraries tab in the Arduino IDE
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "MIDIUSB.h"
#include <Ewma.h>
#include "Adafruit_MPR121.h"

// Variables etc. for MPR121...
#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif
uint16_t lasttouched = 0;
uint16_t currtouched = 0;
int presses[] = { 0, 0, 0, 0, 0 };


Adafruit_MPR121 cap = Adafruit_MPR121();
Adafruit_MPU6050 mpu;

Ewma xFilter(0.1); 
Ewma yFilter(0.1); 
Ewma zFilter(0.1); 

float x = 0;
float y = 0;
float z = 0;

float xMin = 0;
float yMin = 0;
float zMin = 0;

float xMax = 0;
float yMax = 0;
float zMax = 0;

int first = 1; 

float xPrev = 0;
float yPrev = 0;
float zPrev = 0;

int flush = 0;
int setupTouches = 1;
int setupCycle = 0;
int setupPre = 0;

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}


void setup(void) {
  Serial.begin(115200);
  // Serial.println("HELLPOOOO");
  delay(5000);
  // while (!Serial)
  //   delay(10); // will pause Zero, Leonardo, etc until serial console opens

  // // Serial.println("Adafruit MPU6050 test!");

  // // Try to initialize!
  if (!mpu.begin()) {
    // Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  // // Serial.println("MPU6050 Found!");

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // Serial.print("Accelerometer range set to: ");
  // switch (mpu.getAccelerometerRange()) {
  // case MPU6050_RANGE_2_G:
  //   // Serial.println("+-2G");
  //   break;
  // case MPU6050_RANGE_4_G:
  //   // Serial.println("+-4G");
  //   break;
  // case MPU6050_RANGE_8_G:
  //   // Serial.println("+-8G");
  //   break;
  // case MPU6050_RANGE_16_G:
  //   // Serial.println("+-16G");
  //   break;
  // }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  // Serial.print("Gyro range set to: ");
  // switch (mpu.getGyroRange()) {
  // case MPU6050_RANGE_250_DEG:
  //   // Serial.println("+- 250 deg/s");
  //   break;
  // case MPU6050_RANGE_500_DEG:
  //   // Serial.println("+- 500 deg/s");
  //   break;
  // case MPU6050_RANGE_1000_DEG:
  //   // Serial.println("+- 1000 deg/s");
  //   break;
  // case MPU6050_RANGE_2000_DEG:
  //   // Serial.println("+- 2000 deg/s");
  //   break;
  // }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  // // Serial.print("Filter bandwidth set to: ");
  // switch (mpu.getFilterBandwidth()) {
  // case MPU6050_BAND_260_HZ:
  //   // Serial.println("260 Hz");
  //   break;
  // case MPU6050_BAND_184_HZ:
  //   // Serial.println("184 Hz");
  //   break;
  // case MPU6050_BAND_94_HZ:
  //   // Serial.println("94 Hz");
  //   break;
  // case MPU6050_BAND_44_HZ:
  //   // Serial.println("44 Hz");
  //   break;
  // case MPU6050_BAND_21_HZ:
  //   // Serial.println("21 Hz");
  //   break;
  // case MPU6050_BAND_10_HZ:
  //   // Serial.println("10 Hz");
  //   break;
  // case MPU6050_BAND_5_HZ:
  //   // Serial.println("5 Hz");
  //   break;
  // }

  // Serial.printl/n("");

  // for(int i=0; i<50; i++){
  //   controlChange(0, 64, 64);
  //   MidiUSB.flush();
  //   delay(200);
  // }

  // for(int i=0; i<50; i++){
  //   controlChange(0, 65, 64);
  //   MidiUSB.flush();
  //   delay(200);
  // }

  // for(int i=0; i<50; i++){
  //   controlChange(0, 66, 64);
  //   MidiUSB.flush();
  //   delay(200);
  // }
  delay(1000);
}

void loop() {
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<5; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      presses[i] = 1;
      // Serial.print(i); Serial.println(" touched");
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      presses[i] = 0;
      // Serial.print(i); Serial.println(" released");
    }
  }
  
  // reset our state
  lasttouched = currtouched;
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (first) {
    xMin = g.gyro.x;
    xMax = xMin;

    yMin = g.gyro.y;
    yMax = yMin;

    zMin = g.gyro.x;
    zMax = zMin;

    first = 0;
  }

  x = xFilter.filter(a.acceleration.x); 
  y = yFilter.filter(a.acceleration.y);
  z = zFilter.filter(a.acceleration.z);

  if(x<=xMin){xMin = x;};
  if(x>=xMax){xMax = x;};
  if(y<=yMin){yMin = y;};
  if(y>=yMax){yMax = y;};
  if(z<=zMin){zMin = z;};
  if(z>=zMax){zMax = z;};


  x = (x-xMin) / (xMax-xMin) * 127;
  y = (y-yMin) / (yMax-yMin) * 127;
  z = (z-zMin) / (zMax-zMin) * 127;

  if(setupTouches){

    if(presses[0] == 1){
      if(setupCycle==0){controlChange(0, 1, x);flush=1;}
      if(setupCycle==1){controlChange(0, 2, y);flush=1;}
      if(setupCycle==2){controlChange(0, 3, z);flush=1;}
    }
    

    if(presses[1] == 1){
      if(setupCycle==0){controlChange(0, 4, x);flush=1;}
      if(setupCycle==1){controlChange(0, 5, y);flush=1;}
      if(setupCycle==2){controlChange(0, 6, z);flush=1;}
    }

    if(presses[2] == 1){
      if(setupCycle==0){controlChange(0, 7, x);flush=1;}
      if(setupCycle==1){controlChange(0, 8, y);flush=1;}
      if(setupCycle==2){controlChange(0, 9, z);flush=1;}
    }

    if(presses[3] == 1){
      if(setupCycle==0){controlChange(0, 10, x);flush=1;}
      if(setupCycle==1){controlChange(0, 11, y);flush=1;}
      if(setupCycle==2){controlChange(0, 12, z);flush=1;}
    }

    if(presses[4]==1){
      if(presses[4]!=setupPre){
        setupCycle++;
        if(setupCycle == 3){
          setupTouches=0;
          setupCycle=0;
        }  
      }
      setupPre=presses[4];
    } else {setupPre=presses[4];}
  } 
  else {
    if(presses[0] == 1){
      if(x!=xPrev){
        controlChange(0, 1, x);
        flush = 1;
      }
      if(y!=yPrev){
        controlChange(0, 2, y);
        flush = 1;
      }
      if(z!=zPrev){
        controlChange(0, 3, z);
        flush = 1;
      }
    } 
    if(presses[1] == 1){
      if(x!=xPrev){
        controlChange(0, 4, x);
        flush = 1;
      }
      if(y!=yPrev){
        controlChange(0, 5, y);
        flush = 1;
      }
      if(z!=zPrev){
        controlChange(0, 6, z);
        flush = 1;
      }
    } 
    if(presses[2] == 1){
      if(x!=xPrev){
        controlChange(0, 7, x);
        flush = 1;
      }
      if(y!=yPrev){
        controlChange(0, 8, y);
        flush = 1;
      }
      if(z!=zPrev){
        controlChange(0, 9, z);
        flush = 1;
      }
    } 
    if(presses[3] == 1){
      if(x!=xPrev){
        controlChange(0, 10, x);
        flush = 1;
      }
      if(y!=yPrev){
        controlChange(0, 11, y);
        flush = 1;
      }
      if(z!=zPrev){
        controlChange(0, 12, z);
        flush = 1;
      }
    } 
    if(presses[4] == 1){
      if(presses[4]!=setupPre){
        setupTouches = 1;
      }
      setupPre=presses[4];
    } else {setupPre=presses[4];}
  }


  if(flush){MidiUSB.flush();}
  xPrev=x;
  yPrev=y;
  zPrev=z;  
// REPORT TOUCHES
  // for(int i=0; i<5; i++){
  //   Serial.print(presses[i]);
  //   Serial.print("  ");
  // }
  // Serial.println("");

// REPORT GYRO
  // Serial.print(x);
  // Serial.print("   ");
  // Serial.print(y);
  // Serial.print("   ");
  // Serial.println(z);
  
//REPORT LOGIC 
  // Serial.print("presses4: ");
  // Serial.print(presses[4]);
  // Serial.print(",   setupPre:");
  // Serial.print(setupPre);
  // Serial.print(",   setup status: ");
  // Serial.print(setupTouches);
  // Serial.print(",   setupCycle ");
  // Serial.println(setupCycle);
  
  delay(15);
}