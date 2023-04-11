/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0
#define RF_TXTIMEOUT 120

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers
#define RFM69_RST     3
#define RFM69_CS      10
//#define RFM69_IRQ     2 
#define RFM69_IRQN    digitalPinToInterrupt(2 )


// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS);
//----- END TEENSY CONFIG

unifiedTXData dataToSend;
unifiedRXData dataToReceive;

int txData_size = sizeof(dataToSend.txData);
int rxData_size = sizeof(dataToReceive.rxData);

void radioSetup(){
  pinMode(RFM69_IRQN, OUTPUT);        
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  debugLogLn("Feather RFM69 TX Test!");
  debugLogLn("");

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  //----- END TEENSY CONFIG

  if (!rf69.init()) {
    debugLogLn("RFM69 radio init failed");
    message = "OB radio fail";
  }else{
    radioOK = true;
    debugLogLn("RFM69 radio init OK!");
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
    // No encryption
    if (!rf69.setFrequency(RF69_FREQ)) {
      debugLogLn("setFrequency failed");
    }

    // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
    // ishighpowermodule flag set like this:
    rf69.setTxPower(20,true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

    // The encryption key has to be the same as the one in the server
    uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    rf69.setEncryptionKey(key);

    debugLog("RFM69 radio @");  debugLog((int)RF69_FREQ);  debugLogLn(" MHz");
  }
}

//Send specified data to the ground station for logging
void sendDataToGroundStation(TXData data){
  dataToSend.structuredData = data;
	
	rf69.waitPacketSent(RF_TXTIMEOUT); //Do not try to sent anything if modem not ready
	bool rfok = rf69.send(dataToSend.txData, txData_size);
	rf69.waitPacketSent(RF_TXTIMEOUT); //Wait for the transmission done

  if(!rfok){
    debugLogLn("There has been an error sending a radio trasmission!");
  }

  // Now wait for a reply
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf69.waitAvailableTimeout(5))  { 
    // Check for reply message
    if (rf69.recv(buf, &len)) {
      //Recode reply message
      for (uint8_t i = 0; i < rxData_size; i++) {
        dataToReceive.rxData[i] = buf[i];
      }

      //Check for new data in reply
      bool newData = dataToReceive.structuredData.newData;
      if(newData){
        //Update local variables based on data in reply
        RXData receivedData = dataToReceive.structuredData.data;
        if(receivedData.alpha>0){
          currentState.alpha = receivedData.alpha;
        }if(receivedData.tau>0){
          currentState.tau = receivedData.tau;
        }if(receivedData.p_gain>0){
          currentState.p_gain = receivedData.p_gain;
        }if(receivedData.i_gain>0){
          currentState.i_gain = receivedData.i_gain;
        }
        if(receivedData.d_gain>0){
          currentState.d_gain = receivedData.d_gain;
        }
        currentState.angle = receivedData.angle;
        currentState.manualMode = receivedData.manualMode;
      }
    } else {
      Serial.println("Receive failed");
    }
  }
}