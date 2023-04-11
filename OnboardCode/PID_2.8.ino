#include <Servo.h> 
//PID parameters
// Limit servo angle (-35 to +30) from zero position (zero position at around 90 deg)
float minAngle = -35;
float maxAngle = 30;
float maxPitch = 20; //Defines pitch angle range that can be commanded (+- maxPitch)

//Initial variable definitions
Adafruit_MPU6050 mpu;
Servo portServo;
Servo starboardServo;
int portServoPin = 36;
int starboardServoPin = 37;
float totalIntegralError = 0;
float command = 0;
float currentPitch = 0;
float startTime = millis();
float proportionalError = 0;
float derivativeError = 0;
float integralError = 0;
float totalError = 0;
float servoCommand = 0;

float pitch, pitch_velocity, pitch_acceleration, pitch_angle_acc, pitch_angle_gyro;
unsigned int lastRead = 0;

void pidSetup() {
  currentState.alpha = 0.75;
  //In this code, the alpha variable controls the weight of the accelerometer measurement. 
  //The larger the value of alpha, the more weight is given to the gyroscope measurement, and vice versa. 
  //The value of alpha can be tuned to achieve the best balance between accuracy and stability.
  currentState.tau = 0.8;
  //In this code, tau is the time constant of the low-pass filter, which determines the cutoff frequency of the filter. 
  //The smaller the value of tau, the stronger the filtering effect and the slower the response to changes in angular velocity. 
  //The value of tau can be adjusted to achieve the desired trade-off between smoothing and responsiveness.

  portServo.attach(portServoPin);
  starboardServo.attach(starboardServoPin);

  Serial.begin(115200);
  mpu.begin();
}

TXData pidLoop(float dt) {
  //Getting raw data from MPU
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  //Extract current values for tau and alpha
  float tau = currentState.tau;
  float alpha = currentState.alpha;
  //Low-pass filter the angular velocity
  pitch_acceleration = g.gyro.x;
  pitch_velocity = pitch_velocity + (pitch_acceleration - pitch_velocity) / tau * dt;
  //pitch_velocity = pitch_velocity + ((pitch_acceleration * dt) - pitch_velocity) * tau;
  pitch = pitch + pitch_velocity * dt;

  //Calculate pitch angle from accelerometer
  pitch_angle_acc = atan2(a.acceleration.y, a.acceleration.z);
  

  //Complementary filter
  pitch_angle_gyro = alpha * (pitch_angle_gyro + pitch_velocity * dt) + (1 - alpha) * pitch_angle_acc;
  float pitch_degrees = pitch_angle_gyro * 180/PI;
  float pitch_velocity_degrees = pitch_velocity * 180/PI;

  //IMU pitch angle offset
  pitch_degrees += 5.5;

  //Data output via serial
  bool print = false;
  if((lastRead+outputPeriod)<curr_time){
    lastRead = curr_time;
    print = true;
    Serial.print(String(currentState.angle) + "," + String(pitch_degrees) + ",");
  }
  
  //Commanding servo with claculated PID command (assuming servo zero position at 90 degrees)
  int beforePID = millis();
  TXData PIDOutput = getPIDOutput(pitch_degrees, dt, print);
  Serial.print("PID time: ");
  Serial.println(millis()-beforePID);
  PIDOutput.pitchAngularVelocity = pitch_velocity_degrees;
  servoCommand = PIDOutput.commandedAngle; 

  //Send signals to actuate both servos
  portServo.write(91 - servoCommand);
  starboardServo.write(77 + servoCommand);

  //Finish line of serial output
  if(print){
    Serial.println();
  }

  return PIDOutput;
}

//Returns a struct of values calculated by PID based on input parameters
//actual -> actual pitch angle, 
//dt -> time since last PID run in seconds
//print -> whether current values should be outputed to serial
TXData getPIDOutput(float actual, float dt, bool print){
  //Error terms and total error calculation
  proportionalError = actual - currentState.angle;
  derivativeError = actual - currentPitch;
  integralError = totalIntegralError;
  totalError = currentState.p_gain * proportionalError + currentState.d_gain * derivativeError + currentState.i_gain * integralError;
  
  //Serial data output
  if(print){
    Serial.print(String(proportionalError) + ",");
    Serial.print(String(integralError) + ",");
    Serial.print(String(derivativeError) + ",");
    Serial.print(totalError);
  }
  
  //Integral term anti-windup
  if ((totalError<=maxAngle&&totalError>=minAngle)||(sign(proportionalError)!=sign(command))){ 
      totalIntegralError += proportionalError * dt;
  }
  
  if(currentState.manualMode){
    command = currentState.angle;
  }else{
    command = constrain(totalError, minAngle, maxAngle);
  }
  
  //Storing pitch for next PID run calculations
  currentPitch = actual;

  //Construct and return the data structure
  TXData data;
  data.commandedAngle = command;
  data.pitchAngle = currentPitch;
  data.totalError = totalError;
  data.p_error = proportionalError;
  data.i_error = integralError;
  data.d_error = derivativeError;

  return data;
}