/*
AVDASI2 22/23 - Company B Fuselage Onboard Avionics Code
Main file version 1.6
Last updated for Structs 1.6, Radio 1.5, PID 2.8, Logging 1.6, WiredComms 1.1
Created by Arjun Katechia, Carlos Patron Martin, Isabel Sotirova, Julian Bill and Meghan Jantak
Designed for Teensy 4.1
*/

#include <SPI.h>
#include <RH_RF69.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <iostream>
#include "Structs.h"

//Default parameters
RXData currentState;
int onboardLoggingFrequency = 50; //Frequency of logging data to SD card in Hz
int groundStationLoggingFrequency = 10; //Frequency of logging data to ground station in Hz
//Variables storing time stamps for different events
unsigned long prev_time, curr_time;
unsigned long prev_tx = 0;
unsigned long prev_log = 0;
//Serial data output time period in miliseconds
int outputPeriod = 10000; //Serial logging period in miliseconds
String message = "";
bool radioOK = false;

void setup() {
  currentState.p_gain = 0; //Proportional gain
  currentState.i_gain = 0; //Interal gain
  currentState.d_gain = 0; //Derivative gain
  currentState.manualMode = false;

  loggingSetup();
  prev_time = millis();
  radioSetup();
  wiredSetup();
  pidSetup();
}

void loop() {
  //Time difference since last run calculation
  curr_time = millis();
  float dt = (curr_time - prev_time) / 1000.0;
  prev_time = curr_time;

  //Run PID and log data
  TXData PIDOutput = pidLoop(dt);
  logData(PIDOutput);
}

//Data logging for supplied data structure
void logData(TXData data){
  String dataString = String(millis());
  
  //Clear message after it was sent
  message = "";

  //Deconstruct data into a string
  dataString += "," + String(data.commandedAngle);
  dataString += "," + String(data.pitchAngle);
  dataString += "," + String(data.pitchAngularVelocity);
  dataString += "," + String(data.totalError);
  dataString += "," + String(data.p_error);
  dataString += "," + String(data.i_error);
  dataString += "," + String(data.d_error);
  dataString += "," + String(currentState.manualMode);
  dataString += "," + String(currentState.p_gain);
  dataString += "," + String(currentState.i_gain);
  dataString += "," + String(currentState.d_gain);
  dataString += "," + String(currentState.alpha);
  dataString += "," + String(currentState.tau);

  //Log data to SD
  if((prev_log+(1000/onboardLoggingFrequency))<curr_time){
    prev_log = millis();
    logToSD(dataString);
  }

  //Send data to ground station if enough time passed since last sent
  if((prev_tx+(1000/groundStationLoggingFrequency))<curr_time){
    prev_tx = millis();
    if(radioOK){
      sendDataToGroundStation(data);
    }else{
      wiredSendToGroundStation(data);
    }
  }
}

//Outputs sign of float x (-1/0/1)
float sign(float x){
  if(x<0) return -1;
  if(x>0) return 1;
  return 0;
}