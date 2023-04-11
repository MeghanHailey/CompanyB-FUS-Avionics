#include <SD.h>

// Defining SD card chip as Teensy onboard
const int chipSelect = BUILTIN_SDCARD;

//Initial variables definitions
int sessionNumber = 0;
int logNumber = 1;
int lastBackup = millis();
bool SDok = false;

void loggingSetup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.print("Initializing SD card...");

  // See if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println();
    Serial.println();
    Serial.println("WARNING: SD Card failed or not present!!!");
    Serial.println();
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
}

//Prints message into the debug log file
void saveDebug(String message, bool newLineAfter){
  if(!SDok){return;}
  String filePath = "Session" + String(sessionNumber) + "/debug.txt";
  // Open the file.
  File dataFile = SD.open(filePath.c_str(), FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.print(message);
    if(newLineAfter){
      dataFile.println();
    }
    dataFile.close();
  } else {
    // if the file isn't open, pop up an error:
    Serial.println("Error opening " + filePath);
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
     if (entry.isDirectory()) {
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
     }
     entry.close();
   }  
   sessionNumber++;

   //Create a number directory for session
   String dirName = "Session" + String(sessionNumber);
   SD.mkdir(dirName.c_str());
   
   return sessionNumber;
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