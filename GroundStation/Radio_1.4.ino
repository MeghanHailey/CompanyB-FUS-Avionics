#include <SPI.h>
#include <RH_RF69.h>

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0
#define RF_TXTIMEOUT 120

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers
#define RFM69_RST     3
#define RFM69_CS      10
// #define RFM69_IRQ     2 
#define RFM69_IRQN    digitalPinToInterrupt(2 )


// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS);
//----- END TEENSY CONFIG

int16_t packetnum = 0;  // packet counter, we increment per xmission

typedef struct {
	double    potx1;
	double    potx2;
	double    potxten;
	uint8_t   data4;
} payload;        // 1 + 4 + 4 + 4 = 13 bytes atmega368p / 1284p

typedef union {
	reply structuredData;
	uint8_t txData[sizeof(reply)];
} unifiedDataSend;

typedef union {
	RXData structuredData;
	uint8_t rxData[sizeof(reply)];
} unifiedDataReceive;

unifiedDataSend datatosend;

int txData_size = sizeof(datatosend.txData);

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
    radioOK = true;
  }
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

  datatosend.structuredData.newData = true;
}

//Set values based on data "received" from the ground station
void processData(RXData data){
  lastPitchAngle = data.pitchAngle;

  //Deconstruct data into a string
  String dataString = millis();
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

  //Log received data to SD card
  logToSD(dataString);
}

void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i=0; i<loops; i++)  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}

//Update variables before sending to aircraft
void updateValues(){
  if(datatosend.structuredData.newData){
    datatosend.structuredData.data = currentState;
  }else{
    TXData emptyData;
    datatosend.structuredData.data = emptyData;
  }
}

//Reply to trasmissions from aircraft with ground station info
void sendReply() {
  updateValues();
	
	rf69.waitPacketSent(RF_TXTIMEOUT); //Do not try to sent anything if modem not ready
	rf69.send(datatosend.txData, txData_size);
	rf69.waitPacketSent(RF_TXTIMEOUT); //Wait for the transmission done
	setNewData(false);

	rf69.setModeIdle();
}

void radioLoop(){
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  //Check for new tramsmissions
  if (rf69.recv(buf, &len)) {
    unifiedDataReceive datatoreceive;
    int rxData_size = sizeof(datatoreceive.rxData);
    //Decode received data
    for (uint8_t i = 0; i < rxData_size; i++) {
      datatoreceive.rxData[i] = buf[i];
    }
    //Update local variables based on received data
    lastPitchAngle = datatoreceive.structuredData.pitchAngle;
    processData(datatoreceive.structuredData);
    //Reply
    sendReply();
  }
}

//Sets the new data flag that determines if there is new data to be sent
//from the ground station to onboard
void setNewData(bool newDataValue){
  datatosend.structuredData.newData = newDataValue;
}