#include <SD.h>

// Defining SD card chip as Teensy onboard
const int chipSelect = BUILTIN_SDCARD;

//Initial variable definitions
int sessionNumber = 0;
int logNumber = 1;
int lastBackup = millis();
bool SDok = false;
String masterWarning;

void loggingSetup(){
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println();
    Serial.println();
    Serial.println("WARNING: SD Card failed or not present!!!");
    Serial.println();
    masterWarning = "GS SD card fail";
  }else{
    SDok = true;
    Serial.println("Card initialized.");

    //Get and print session number
    sessionNumber = getSessionNumber();
    Serial.println("Session number " + String(sessionNumber));
    
    //Set headers for log file
    logToSD("Time,Commanded angle,Pitch angle,Pitch angular velocity,Total PID error,Proportional error,Integral error,Derivative error,Manual mode,Proportional gain,Integral gain,Derivative gain,Alpha,Tau");
  }
}

void logToSD(String dataString)
{
  if(!SDok){return;}
  //Get file path based on current session
  String filePath = "Session" + String(sessionNumber) + "/log" + String(logNumber) + ".csv";
  //Open the file.
  File dataFile = SD.open(filePath.c_str(), FILE_WRITE);
  //If the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  } else {
    // if the file isn't open, pop up an error:
    Serial.println("Error opening " + filePath);
  }

  //Repeat same for a backup file
  String backupFilePath = "Session" + String(sessionNumber) + "/log" + String(logNumber) + "_backup.csv";
  File backupDataFile = SD.open(backupFilePath.c_str(), FILE_WRITE);
  if (backupDataFile) {
    backupDataFile.println(dataString);
    backupDataFile.close();
  } else {
    Serial.println("Error opening " + backupFilePath);
  }
}

//Logs specified string to serial and log file
void debugLog(String message){
  Serial.print(message);
  saveDebug(message, false);
}

//Logs specified string to serial and log file followed by a line break
void debugLogLn(String message){
  Serial.println(message);
  saveDebug(message, true);
}

//Prints message into the debug log file
void saveDebug(String message, bool newLineAfter){
  if(!SDok) {return;}

  String filePath = "Session" + String(sessionNumber) + "/debug.txt";
  // Open the file.
  File dataFile = SD.open(filePath.c_str(), FILE_WRITE);

  // If the file is available, write to it:
  if (dataFile) {
    dataFile.print(message);
    if(newLineAfter){
      dataFile.println();
    }
    dataFile.close();
  } else {
    // If the file isn't open, pop up an error:
    Serial.println("Error opening " + filePath);
  }
}

//Get current session number based on SD card contents
int getSessionNumber(){
  int sessionNumber = 1;
  File dir = SD.open("/");
  //Loop through all files on SD card to find latest session folder
  while(true) {
    File entry = dir.openNextFile();
    //Break if no more files
    if (! entry) {
      break;
    }
    String name = entry.name();
    //Only detect folders starting with "Sessions"
    if(name.startsWith("Session")){
      String currentNumberString = name.substring(7); //Get number after "Session" in folder name
      if(isNumber(currentNumberString)){
        int currentNumber = currentNumberString.toInt();
        //Update session number if higher than previously found
        if(currentNumber>sessionNumber){
          sessionNumber = currentNumber;
        }
      }
    }
    entry.close();
  }  
  sessionNumber++;

  //Create a number directory for session
  String dirName = "Session" + String(sessionNumber);
  SD.mkdir(dirName.c_str());
  
  return sessionNumber;
}

//Log received data from aircraft to SD card
void logAircraftData(RXData data){
  //Deconstruct data into a string
  String dataString = String(data.timestamp);
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
}

//Returns true if the input string is an integer
bool isNumber(const String s)
{
    bool number = true;
    for(char c:s){
      if(!isDigit(c)){
        number = false;
      }
    }
    return number;
}