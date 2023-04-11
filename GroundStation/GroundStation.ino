/*
AVDASI2 22/23 - Company B Fuselage Ground Station Code
Main file version 1.6
Last updated for Structs 1.5, Radio 1.4, Logging 1.5, WiredComms 1.1, UI 1.2
Created by Carlos Patron Martin, Filip Ramian, Isabel Sotirova, Julian Bill and Meghan Jantak
Designed for Teensy 4.1
*/

#include <BufferedPrint.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <RingBuf.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <sdios.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <RH_RF69.h>
#include "Structs.h"
#include <SoftwareSerial.h>
#include <iostream>


//Default parameters
TXData currentState;
float lastPitchAngle = 0;
float maxElevatorAngle = 60; 
float maxPitchAngle = 20;
float knobIncrementPitch = 0.1;
float knobIncrementElevator = 1;

// Initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 0, en = 1, d4 = 7, d5 = 8, d6 = 5, d7 = 6;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int BUTTON2_PIN = 34;  // Pin number of the button for set angle 2
const int BUTTON3_PIN = 14;  // Pin number of the button that zeroes the angle
const int KNOB1_PIN = 15;  // Pin number of the knob to set commanded angle
const int KNOB_BUTTON = 38; // Pin for knob press button (mode change)
const int KNOB_A = 35; // Pin A for knob (rotary encoder)
const int KNOB_B = 33; // Pin B for knob (rotary encoder)

int button1State = 0;  // Current state of button 1 - mode selector
int button2State = 0;  // Current state of button 2
int button3State = 0;  // Current state of button 3

bool button1Pressed = false;  // Flag to keep track of button press
bool button2Pressed = false;  // Flag to keep track of button press
bool button3Pressed = false;  // Flag to keep track of button press
bool knobAlast = false;
bool knobBlast = false;
int lastKnobValue;
unsigned int lastDisplayUpdate = millis();
unsigned int lastKnobUpdate = millis();
int knobDirection = 0;

bool radioOK = true;

void setup() {
  currentState.p_gain = 2; //Proportional gain
  currentState.i_gain = 1; //Interal gain
  currentState.d_gain = 3; //Derivative gain
  currentState.alpha = 0.75;
  currentState.tau = 0.8;
  currentState.manualMode = false;
  currentState.angle = 0;

  loggingSetup();
  radioSetup();
  wiredSetup();
  Serial.println("Setup");
  Serial.setTimeout(5);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(BUTTON3_PIN, INPUT);
  pinMode(KNOB1_PIN, INPUT);
  pinMode(KNOB_BUTTON, INPUT);
  pinMode(KNOB_A, INPUT);
  pinMode(KNOB_B, INPUT);

  lastKnobValue = analogRead(KNOB1_PIN);
}

void loop() {
  UIloop();

  //Check if radio initialised correctly and used wired communication if not
  if(radioOK){
    radioLoop();
  } else{
    wiredLoop();
  }
  
  // Read the state of the first button
  button1State = digitalRead(KNOB_BUTTON);
  // Read the state of the second button
  button2State = digitalRead(BUTTON2_PIN);
  // Read the state of the third button
  button3State = digitalRead(BUTTON3_PIN);
  // Read the values of the knob
  int knobAState = digitalRead(KNOB_A);
  int knobBState = digitalRead(KNOB_B);

  // Check if the first button is pressed
  if (button1State == LOW && !button1Pressed) {
    // Update mode
    currentState.manualMode = !currentState.manualMode;
    setNewData(true);
    // Set the button press flag
    button1Pressed = true;
    currentState.angle = 0;
  }
  // Check if the first button is released
  if (button1State == HIGH && button1Pressed) {
    // Reset the button press flag
    button1Pressed = false;
  }

  // Check if the second button is pressed
  if (button2State == LOW && !button2Pressed) {
    // Update the elevator angle to 10 degrees
    currentState.angle = 0;
    setNewData(true);
    // Set the button press flag
    button2Pressed = true;
  }

  //Rotary knob - angle setting
  //Detect if knob was rotated and if yes in which direction
  float increment = 0;
  if(knobAState == 1 && knobBState == 1){
    if(knobAlast==0){
      knobDirection = -1;      
    }
    else if(knobBlast==0){
      knobDirection = 1;      
    }
  }else if(knobAState == 0&&knobAlast == 1&&knobBState==1&&knobDirection==1){
    increment = 1;
  }else if(knobBState == 0&&knobBlast == 1&&knobAState==1&&knobDirection==-1){
    increment = -1;
  }
  knobAlast = knobAState;
  knobBlast = knobBState;

  //Change angle setting based on knob rotation
  if(increment!=0){
    int maxAngle = 0;
    float newAngle = currentState.angle;
    if(currentState.manualMode){
      maxAngle = maxElevatorAngle;
      newAngle += knobIncrementElevator * increment;
    }else{
      maxAngle = maxPitchAngle;
      newAngle += knobIncrementPitch * increment;
    }
    currentState.angle = constrain(newAngle, -maxAngle, maxAngle);
    lastKnobUpdate = millis();
    setNewData(true);
  }

  // Check if the second button is released
  if (button2State == HIGH && button2Pressed) {
    // Reset the button press flag
    button2Pressed = false;
  }

  // Check if the second button is pressed
  if (button3State == LOW && !button3Pressed) {
    // Update the elevator angle to 10 degrees
    currentState.angle = 10;
    setNewData(true);
    // Set the button press flag
    button3Pressed = true;
  }
  // Check if the second button is released
  if (button3State == HIGH && button3Pressed) {
    // Reset the button press flag
    button3Pressed = false;
  }

  //Update display
  if (millis()>(lastDisplayUpdate+100)){
    // Clear the LCD display
    lcd.clear();
    // Print the appropriate message on the first line
    if(currentState.manualMode){
      lcd.print("Elev. cmd: ");
      lcd.print(currentState.angle, 0);
    }else{
      lcd.print("Pitch cmd: ");
      lcd.print(currentState.angle, 1);
    }
    
    lcd.setCursor(0, 1);
    lcd.print("A/C pitch: ");
    if(lastPitchAngle<-100){
      lcd.print(lastPitchAngle, 0);      
    }else{
      lcd.print(lastPitchAngle, 1);
    }

    lastDisplayUpdate = millis();
  }
}