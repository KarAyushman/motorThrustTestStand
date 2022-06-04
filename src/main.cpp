/***
 * Monarch Aerospace 2022 ©
 * Programmed by:- Ayushman LP. Kar
 * 
 * This is the program for the Model Rocket Motor Test Stand.
 * 
 ***/

#include <HX711_ADC.h>
#include "HX711.h"
#include "Arduino.h"


//Pins:
const int HX711_dout = 9;
const int HX711_sck = 13;

//HX711 Constructors:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
HX711 scale;

//Variables
unsigned long t = 0;
double mtime;
double force;
double dt;
double dp;
double impulse;
double last;
double ig;

void setup() {
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(1.0);
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  calibrate();
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0;

  Serial.println("Please select enter the respective option");
  Serial.println("Press 't' to begin Taring Sequence");
  Serial.println("Press 'r' to begin Calibration Sequence");
  Serial.println("Press 'c' to edit Calibration Factor");
  Serial.println("Press 's' to begin Load Test Sequence");


  // Serial Input for Sequence Initialization
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); //Taring Sequence
    else if (inByte == 'r') calibrate(); //Calibration Sequence
    else if (inByte == 'c') changeSavedCalFactor(); //Edit Calibration Factor
    else if (inByte == 's') loadTest(); //loadTest Sequence
  }

  // Tare Sequence Acknowledgement
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");
  delay(5000);

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCell.tareNoDelay();
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); //Refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //Pull new Calibration Value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}

void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}

void loadTest(){
    float calVal = LoadCell.getCalFactor();
    scale.begin(HX711_dout, HX711_sck);
    Serial.println("Please take off the Calibration Weight and clear the Load Cell."
    "Affix the Motor in the Upright Position and get ready to start");
    delay(1000);
    Serial.println("Column 1 - Time Elasped (in secs)");
    delay(1000);
    Serial.println("Column 2 - Thrust (in N)");
    delay(1000);
    Serial.println("Column 3 - Total Impulse (in Nsecs)");
    delay(500);
    Serial.print("Current Calibration Factor is: ");
    Serial.println(calVal);
    Serial.println("Press s to begin sequence or e to edit Calibration Factor");

    int done = 0;
    while(done == 0)
    {
        while (Serial.available() > 0){
            if (Serial.read() == 's'){
                done = 1;
            }
            else if (Serial.read() == 'e'){
                changeSavedCalFactor();
            }
        }
    }
    while(Serial.available() > 0){
        byte dummyread = Serial.read();
    }

    double taret = 80;
    scale.tare(taret);
    scale.set_scale(calVal);


    while(!Serial.available()>0){
        mtime = micros()/1000000.f;
        force = scale.get_units()*0.009806f;
        dt = mtime-last;
        dp = force*dt;
        impulse = impulse + dp;
        last = mtime;
        Serial.print(mtime,10);
        Serial.print(",");
        Serial.print(force,10);
        Serial.print(",");
        Serial.println(impulse,10);
    }

    Serial.println("Calibration Stopped!");
    delay(5000);
    loop();
}