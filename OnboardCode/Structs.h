//Structs version 1.6

//Format of data received from ground station
typedef struct {
  bool manualMode; //Mode of operation, manual or PID
  float angle; //Commanded angle (degrees)
  float p_gain; //Proportional gain
  float i_gain; //Integral gain
  float d_gain; //Derivative gain
  float alpha; //Weighting of accelerometer vs gyroscope measurement
  float tau; //Filter cutoff
} RXData;

//Format of data sent to ground station
typedef struct{
  int timestamp; //Timestamp in millis
  float commandedAngle; //Angle commanded by PID (degrees)
  float pitchAngle; //Pitch angle measured by IMU (degrees)
  float pitchAngularVelocity; //Pitch angular velocity measured by IMU (derees per second)
  float totalError; //Total PID error
  float p_error; //Proportional error
  float i_error; //Integral error
  float d_error; //Derivative error
  char* message; //Any debug messages to pass along
} TXData;

//Format for replies
typedef struct{
  bool newData;
  RXData data;
} reply;

//Data send format
typedef union {
	TXData structuredData;
	uint8_t txData[sizeof(RXData)];
} unifiedTXData;

//Data receive format
typedef union {
	reply structuredData;
	uint8_t rxData[sizeof(reply)];
} unifiedRXData;