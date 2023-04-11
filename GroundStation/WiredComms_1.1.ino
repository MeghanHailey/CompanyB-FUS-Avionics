//RX and TX pin for serial communication
SoftwareSerial wiredSerial(16, 17);

//Initial setup of wired connection
void wiredSetup(){
  wiredSerial.begin(9600);
  delay(200);
}

//Wired comms code to loop
void wiredLoop(){
  wiredSerial.listen();

  //Check if wired serial is open with incoming data
  if(wiredSerial.available()){
    //Decode and process received data
    unifiedDataReceive dataToReceive;
    wiredSerial.readBytes(dataToReceive.rxData, sizeof(RXData));
    processData(dataToReceive.structuredData);
    //Reply
    sendReplyWired();
  }
}

//Reply to trasmissions from aircraft with ground station info
void sendReplyWired() {
  //Encode and send data
  datatosend.structuredData.data = currentState;
	wiredSerial.write(datatosend.txData, sizeof(reply));
  if (wiredSerial.overflow()){
    Serial.println("Overflow");
  }
}