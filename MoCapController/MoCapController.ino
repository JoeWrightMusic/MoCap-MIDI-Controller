/*
    NOTE: 
    In its current state, this code needs cleaning up! 
    There are lots of repeated lines of code that can be cleaned up 
    using arrays and for loops. 




    OPERATION: 
    Performing Mode: 
    The device will send xyz data from the 6050 via midi cc. 
    4 seperate streams of this xyz data can be sent out - triggered by the first
    4 capsens buttons. Pressing capsens0 will send xyz data on MIDIcc 1,2,3; 
    pressing capsens2 will send xyz data on MIDIcc 4,5,6 ...etc.

    Setup Mode:
    By pressing capsensor 4, you go into setup mode. this will let you map 
    MIDI values more easily without having continuous MIDI data across different 
    control numbers getting in the way. 

    When you first enter setup mode, you'll be sending the xyz values on MIDI 
    CC one at a time, to make mapping easier in software like Ableton Live.
    The workflow is like this: 

    Capsens4 -> Enter Setup mode, send X values
    - pressing capsens 0,1,2,3 will send x values for each button's cc numbers
    Press Capsens4 again -> send Y values 
    - pressing capsens 0,1,2,3 will send y values for each button's cc numbers
    Press Capsens4 again -> send Z values 
    - pressing capsens 0,1,2,3 will send z values for each button's cc numbers
    Press Capsens4 again -> back to performing mode
*/



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
uint16_t lasttouched = 0;//last cap sensor touched
uint16_t currtouched = 0;//current capsenseor touched
int presses[] = { 0, 0, 0, 0, 0 };//array to store on/off state of capsensors

// Library for MPR121
Adafruit_MPR121 cap = Adafruit_MPR121();

// Library for MPU6050
Adafruit_MPU6050 mpu;

// Weighted average filters for our gyro data
Ewma xFilter(0.1); 
Ewma yFilter(0.1); 
Ewma zFilter(0.1); 

// Variables for xyz values
// for current value
float x = 0;
float y = 0;
float z = 0;
// for minimums
float xMin = 0;
float yMin = 0;
float zMin = 0;
// for maximums 
float xMax = 0;
float yMax = 0;
float zMax = 0;
// this variable will be 1 the first time the code runs
int first = 1; 
// previous xyz values 
float xPrev = 0;
float yPrev = 0;
float zPrev = 0;

// flush will be used to trigger MIDIusb.flush when cc messages are ready
int flush = 0;

// These variables control logic for mapping the controller
int setupTouches = 1;
int setupCycle = 0;
int setupPre = 0;

// Function for sending a MIDI CC message
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// Setup function, runs before the main loop starts
void setup(void) {
  // Setup serial communication
  Serial.begin(115200);
  // Wait a bit...
  delay(5000);
 
  // // Try to initialize MPU6050!
  if (!mpu.begin()) {
    //will wait forever until mpu connects
    while (1) {
      delay(10);
    }
  }
  // // Try to initialize MPR121
  if (!cap.begin(0x5A)) {
    //will wait forever until mpr121 connects
    while (1);
    delay(10);
  }

  // MPU SETTINGS
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

//Main loop - runs over and over
void loop() {
  //get capsense values
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


  /* Get new movement sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  //The first time the loop runs, set the mins and maximums to whatever the sensor is at
  if (first) {
    xMin = a.acceleration.x;
    xMax = xMin;

    yMin = a.acceleration.y;
    yMax = yMin;

    zMin = a.acceleration.x;
    zMax = zMin;
    //set first to 0 so that this section doesn't run again
    first = 0;
  }

  // get xyz values from gyro (it says its reading acceleration but I think these are switched on cheap boards?)
  x = xFilter.filter(a.acceleration.x); 
  y = yFilter.filter(a.acceleration.y);
  z = zFilter.filter(a.acceleration.z);

  // if new min or maximum, store the value
  if(x<=xMin){xMin = x;};
  if(x>=xMax){xMax = x;};
  if(y<=yMin){yMin = y;};
  if(y>=yMax){yMax = y;};
  if(z<=zMin){zMin = z;};
  if(z>=zMax){zMax = z;};

  // scale xyz to midi range using min / max 
  x = (x-xMin) / (xMax-xMin) * 127;
  y = (y-yMin) / (yMax-yMin) * 127;
  z = (z-zMin) / (zMax-zMin) * 127;

  // check to see if we're in Setup Mode
  if(setupTouches){
    //send data on first xyz stream - MIDIcc 1,2,3
    if(presses[0] == 1){
      if(setupCycle==0){controlChange(0, 1, x);flush=1;}
      if(setupCycle==1){controlChange(0, 2, y);flush=1;}
      if(setupCycle==2){controlChange(0, 3, z);flush=1;}
    }
    //send data on second xyz stream - MIDIcc 4,5,6
    if(presses[1] == 1){
      if(setupCycle==0){controlChange(0, 4, x);flush=1;}
      if(setupCycle==1){controlChange(0, 5, y);flush=1;}
      if(setupCycle==2){controlChange(0, 6, z);flush=1;}
    }
    //send data on third xyz stream - MIDIcc 7,8,9
    if(presses[2] == 1){
      if(setupCycle==0){controlChange(0, 7, x);flush=1;}
      if(setupCycle==1){controlChange(0, 8, y);flush=1;}
      if(setupCycle==2){controlChange(0, 9, z);flush=1;}
    }
    //send data on fourth xyz stream - MIDIcc 10,11,12
    if(presses[3] == 1){
      if(setupCycle==0){controlChange(0, 10, x);flush=1;}
      if(setupCycle==1){controlChange(0, 11, y);flush=1;}
      if(setupCycle==2){controlChange(0, 12, z);flush=1;}
    }
    //setup mode button
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
  else {//if not in setup mode, run in performance mode...
    if(presses[0] == 1){//if capsens0 is pressed
      if(x!=xPrev){//send midicc if x has changed value
        controlChange(0, 1, x);
        flush = 1;
      }
      if(y!=yPrev){//send midicc if y has changed value
        controlChange(0, 2, y);
        flush = 1;
      }
      if(z!=zPrev){//send midicc if z has changed value... etc.
        controlChange(0, 3, z);
        flush = 1;
      }
    } 
    if(presses[1] == 1){//if capsens1 is pressed
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
    if(presses[2] == 1){//if capsens2 is pressed
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
    if(presses[3] == 1){//if capsens3 is pressed
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
    } //setup mode button
    if(presses[4] == 1){
      if(presses[4]!=setupPre){
        setupTouches = 1;
      }
      setupPre=presses[4];
    } else {setupPre=presses[4];}
  }

  //If we have midi cc to send, trigger the flush function
  if(flush){MidiUSB.flush();}

  //Record previous xyz values
  xPrev=x;
  yPrev=y;
  zPrev=z; 

  // Uncomment the following blocks to see what's happening with the data...
  // //REPORT TOUCHES
  // for(int i=0; i<5; i++){
  //   Serial.print(presses[i]);
  //   Serial.print("  ");
  // }
  // Serial.println("");

  // //REPORT GYRO
  // Serial.print(x);
  // Serial.print("   ");
  // Serial.print(y);
  // Serial.print("   ");
  // Serial.println(z);
  
  // //REPORT LOGIC 
  // Serial.print("presses4: ");
  // Serial.print(presses[4]);
  // Serial.print(",   setupPre:");
  // Serial.print(setupPre);
  // Serial.print(",   setup status: ");
  // Serial.print(setupTouches);
  // Serial.print(",   setupCycle ");
  // Serial.println(setupCycle);
}