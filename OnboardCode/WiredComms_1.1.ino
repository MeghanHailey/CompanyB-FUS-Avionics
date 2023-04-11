//RX and TX pin for serial communication
SoftwareSerial wiredSerial(16, 17);

//Initial setup of wired connection
void wiredSetup(){
  wiredSerial.begin(9600);
  delay(200);
}

//Wired comms code to loop
void wiredLoop(){
  //Check if wired serial is open with incoming data
  if(wiredSerial.available()){
    //Decode and process received data
    unifiedRXData receivedReply;
    wiredSerial.readBytes(receivedReply.rxData, sizeof(reply));
    if(receivedReply.structuredData.newData){
      RXData receivedData = receivedReply.structuredData.data;
      if(receivedData.alpha>0){
          currentState.alpha = receivedData.alpha;
        }if(receivedData.tau>0){
          currentState.tau = receivedData.tau;
        }if(receivedData.p_gain>0){
          currentState.p_gain = receivedData.p_gain;
        }if(receivedData.i_gain>0){
          currentState.i_gain = receivedData.i_gain;
          integralError = 0;
        }if(receivedData.d_gain>0){           
          currentState.d_gain = receivedData.d_gain;
        }
        currentState.angle = receivedData.angle;
        currentState.manualMode = receivedData.manualMode;
    }
  }
}

//Send data using the wired connection
void wiredSendToGroundStation(TXData data){
  dataToSend.structuredData = data;
	wiredSerial.write(dataToSend.txData, sizeof(TXData));
}