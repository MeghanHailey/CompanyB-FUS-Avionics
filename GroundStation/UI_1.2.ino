String data;
String subs;
char d1;

void UIloop() {
  //Check for data received via serial
  if(Serial.available()){
    String strArr[6];
    data = Serial.readString();
    Serial.print(data);
    d1 = data.charAt(0);
    subs = data.substring(1);
    //Check what type of update was received
    switch(d1){
      case '!':
        currentState.manualMode = true;
        break;
      case '*':
        currentState.manualMode = false;
        break;
      case '@':
        int stringStart = 0;
        int arrayIndex = 0;
        for (unsigned int i=0; i < subs.length(); i++){
          //Get character and check if it's our "special" character.
          if(subs.charAt(i) == ','){
            //Clear previous values from array.
            strArr[arrayIndex] = "";
            //Save substring into array.
            strArr[arrayIndex] = subs.substring(stringStart, i);
            //Set new string starting point.
            stringStart = (i+1);
            arrayIndex++;
          }
        }
        //Update local variables based on values received from UI
        strArr[arrayIndex] = subs.substring(stringStart);
        currentState.p_gain = strArr[0].toFloat();
        currentState.i_gain = strArr[1].toFloat();
        currentState.d_gain = strArr[2].toFloat();
        currentState.alpha = strArr[3].toFloat();
        currentState.tau = strArr[4].toFloat();
        currentState.angle = strArr[5].toFloat();
        setNewData(true);
        Serial.println((String)"Recieved: " + currentState.p_gain + "," + currentState.i_gain + "," + currentState.d_gain + "," + currentState.alpha + "," + currentState.tau+"," + currentState.angle);
        break;
    }
  }
}
