//Structs version 1.5

//Format of data transmitted to UAV
typedef struct {
  bool manualMode; //Mode of operation, manual or PID
  float angle; //Commanded angle
  float p_gain; //Proportional gain
  float i_gain; //Integral gain
  float d_gain; //Derivative gain
  float alpha; //Weighting of accelerometer vs gyroscope measurement
  float tau; //Filter cutoff
} TXData;

//Format of data received from UAV
typedef struct{
  int timestamp; //Timestamp in millis
  float commandedAngle; //Angle commanded by PID
  float pitchAngle; //Pitch angle measured by IMU
  float pitchAngularVelocity; //Pitch angular velocity measured by IMU
  float totalError; //Total PID error
  float p_error; //Proportional error
  float i_error; //Integral error
  float d_error; //Derivative error
  char* message; //Any debug messages to pass along
} RXData;

//Format for replies
typedef struct{
  bool newData;
  TXData data;
} reply;