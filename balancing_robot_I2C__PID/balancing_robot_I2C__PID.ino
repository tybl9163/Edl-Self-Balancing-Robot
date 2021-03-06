double Kp = 150;   // Proportional Gain (Speed of Response)
double Kd = 0.001; // Differentiator Gain
double Ki = 10;    // Integrator Gain
#define max_speed 55  // Max Speed for self-balancing speed compensation
double originalSetpoint = 175;  // Makes the robot off center at the start of the loop so it begins correcting immediately


/* MPU9250 Basic Example Code
  by: Kris Winer
  date: April 1, 2014
  license: Beerware - Use this code however you'd like. If you
  find it useful you can buy me a beer some time.
  Modified by Brent Wilkins July 19, 2016

  Demonstrate basic MPU-9250 functionality including parameterizing the register
  addresses, initializing the sensor, getting properly scaled accelerometer,
  gyroscope, and magnetometer data out. Added display functions to allow display
  to on breadboard monitor. Addition of 9 DoF sensor fusion using open source
  Madgwick and Mahony filter algorithms. Sketch runs on the 3.3 V 8 MHz Pro Mini
  and the Teensy 3.1.

  SDA and SCL should have external pull-up resistors (to 3.3V).
  10k resistors are on the EMSENSR-9250 breakout board.

  Hardware setup:
  MPU9250 Breakout --------- Arduino
  VDD ---------------------- 3.3V
  VDDI --------------------- 3.3V
  SDA ----------------------- A4
  SCL ----------------------- A5
  GND ---------------------- GND
*/

#include "quaternionFilters.h"
#include "MPU9250.h"
#include <PID_v1.h>



// define pins
const int pinSpeed_Left = 9; // connect pin 9 to speed reference for left wheel
const int pinCW_Left = 7;    // connect pin 7 to clock-wise PMOS gate
const int pinCC_Left = 8;    // connect pin 8 to counter-clock-wise PMOS gate
const int pinSpeed_Right = 10; // connect pin 10 to speed reference for right wheel
const int pinCW_Right = 11; // connect pin 11 to CW direction for right wheel
const int pinCC_Right = 12; // connect pin 12 to CC direction for right wheel
char cnt = 0;

#define AHRS true         // Set to false for basic data read
#define SerialDebug true  // Set to true to get Serial output for debugging

// Pin definitions
int intPin = 2;  // These can be changed, 2 and 3 are the Arduinos ext int pins
int myLed  = 13;  // Set up pin 13 led for toggling

MPU9250 myIMU;

//PID
//double originalSetpoint = 190;
double setpoint = originalSetpoint;
double movingAngleOffset = 0.1;
double input, output = 0;

//adjust these values to fit your own design

PID pid(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT); // Takes in IMU measurements, constant gain values, the balncing setpoint  and calculates an output value depending on variation from the setpoint and the input measurements

// I2C scan function


void setup()
{

  //pinMode(pinON,INPUT);
  pinMode(pinCW_Left, OUTPUT); // CW Left is an output
  pinMode(pinCC_Left, OUTPUT); // CC Left is an Output
  pinMode(pinSpeed_Left, OUTPUT); // Vref Left is an output
  pinMode(13, OUTPUT);            // on-board LED
  digitalWrite(13, LOW);          // turn LED off
  digitalWrite(pinCW_Left, LOW);  // stop clockwise
  digitalWrite(pinCC_Left, LOW);  // stop counter-clockwise


  //right
  pinMode(pinCW_Right, OUTPUT); // CW Right is an output
  pinMode(pinCC_Right, OUTPUT); // CC Right is an output
  pinMode(pinSpeed_Right, OUTPUT); // Vreff Left is an output
  digitalWrite(pinCW_Right, LOW);  // stop clockwise
  digitalWrite(pinCC_Right, LOW);  // stop counter-clockwise
  Wire.begin();
  // TWBR = 12;  // 400 kbit/sec I2C speed
  //  <<<<<<< HEAD
  Serial.begin(38400);
  Serial.print(('y'));
  //=======
  Serial.begin(38400); // Sets bud rate to 38400 symbols/second
  //>>>>>>> cb5bf676a73978b250ef95dfad1ec129bf530d13
  //set bypass mode
  //myIMU.writeByte(MPU9250_ADDRESS, INT_PIN_CFG, 0x1);
  //Wire.beginTransmission(MPU9250_ADDRESS);
  //Wire.write(INT_PIN_CFG);
  //Wire.write(0b00000010);
  //Wire.endTransmission();

  //i2c.writeReg(INT_PIN_CFG, 0b00000010) # BYPASS_EN

  // Set up the interrupt pin, its set as active high, push-pull
  pinMode(intPin, INPUT); // Sets interrupt pin as an input
  digitalWrite(intPin, LOW); // Interrupt pin is by default low
  pinMode(myLed, OUTPUT); // LED is an output
  digitalWrite(myLed, HIGH); // LED is by default high

  // Read the WHO_AM_I register, this is a good test of communication
  byte c = myIMU.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  //Serial.println("fyehefuh");
  Serial.print(F("MPU9250 I AM 0x"));
  //char c = 0;
  Serial.print(c, HEX);
  Serial.print(F(" I should be 0x"));
  Serial.println(0x71, HEX);



  if (c == 0x71) // WHO_AM_I should always be 0x71
  {
    Serial.println(F("MPU9250 is online..."));

    // Start by performing self test and reporting values
    myIMU.MPU9250SelfTest(myIMU.selfTest);
    Serial.print(F("x-axis self test: acceleration trim within : "));
    Serial.print(myIMU.selfTest[0], 1); Serial.println("% of factory value");
    Serial.print(F("y-axis self test: acceleration trim within : "));
    Serial.print(myIMU.selfTest[1], 1); Serial.println("% of factory value");
    Serial.print(F("z-axis self test: acceleration trim within : "));
    Serial.print(myIMU.selfTest[2], 1); Serial.println("% of factory value");
    Serial.print(F("x-axis self test: gyration trim within : "));
    Serial.print(myIMU.selfTest[3], 1); Serial.println("% of factory value");
    Serial.print(F("y-axis self test: gyration trim within : "));
    Serial.print(myIMU.selfTest[4], 1); Serial.println("% of factory value");
    Serial.print(F("z-axis self test: gyration trim within : "));
    Serial.print(myIMU.selfTest[5], 1); Serial.println("% of factory value");

    // Calibrate gyro and accelerometers, load biases in bias registers
    myIMU.calibrateMPU9250(myIMU.gyroBias, myIMU.accelBias);



    myIMU.initMPU9250();
    // Initialize device for active mode read of acclerometer, gyroscope, and
    // temperature
    Serial.println("MPU9250 initialized for active data mode....");

    //activate by pass mode


    // Read the WHO_AM_I register of the magnetometer, this is a good test of
    // communication
    byte d = myIMU.readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
    //Serial.print("AK8963 ");
    //Serial.print("I AM 0x");
    //Serial.print(d, HEX);
    //Serial.print(" I should be 0x");
    //Serial.println(0x48, HEX);


    /*
        if (d != 0x48)
        {
          // Communication failed, stop here
          Serial.println(F("Communication failed, abort!"));
          Serial.flush();
          abort();
        }
    */


    // Get magnetometer calibration from AK8963 ROM
    myIMU.initAK8963(myIMU.factoryMagCalibration);
    // Initialize device for active mode read of magnetometer
    Serial.println("AK8963 initialized for active data mode....");

    if (SerialDebug)
    {
      //  Serial.println("Calibration values: ");
      Serial.print("X-Axis factory sensitivity adjustment value ");
      Serial.println(myIMU.factoryMagCalibration[0], 2);
      Serial.print("Y-Axis factory sensitivity adjustment value ");
      Serial.println(myIMU.factoryMagCalibration[1], 2);
      Serial.print("Z-Axis factory sensitivity adjustment value ");
      Serial.println(myIMU.factoryMagCalibration[2], 2);
    }



    // Get sensor resolutions, only need to do this once
    myIMU.getAres();
    myIMU.getGres();
    myIMU.getMres();

    // The next call delays for 4 seconds, and then records about 15 seconds of
    // data to calculate bias and scale.
    //myIMU.magCalMPU9250(myIMU.magBias, myIMU.magScale);
    Serial.println("AK8963 mag biases (mG)");
    Serial.println(myIMU.magBias[0]);
    Serial.println(myIMU.magBias[1]);
    Serial.println(myIMU.magBias[2]);

    Serial.println("AK8963 mag scale (mG)");
    Serial.println(myIMU.magScale[0]);
    Serial.println(myIMU.magScale[1]);
    Serial.println(myIMU.magScale[2]);
    delay(2000); // Add delay to see results before serial spew of data

    if (SerialDebug)
    {
      Serial.println("Magnetometer:");
      Serial.print("X-Axis sensitivity adjustment value ");
      Serial.println(myIMU.factoryMagCalibration[0], 2);
      Serial.print("Y-Axis sensitivity adjustment value ");
      Serial.println(myIMU.factoryMagCalibration[1], 2);
      Serial.print("Z-Axis sensitivity adjustment value ");
      Serial.println(myIMU.factoryMagCalibration[2], 2);
    }



  } // if (c == 0x71)
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);

    // Communication failed, stop here
    Serial.println(F("Communication failed, abort!"));
    Serial.flush();
    abort();
  }
  //setup PID

  pid.SetMode(AUTOMATIC); // Lets arduino change output by dynamically comparing input measurements and setpoint
  pid.SetSampleTime(10); // Samples error between setpoint and input every 10 milliseconds
  pid.SetOutputLimits(-255, 255); // Maps output limits to 8 bit resolution
}

void robot_move(float speed)
{
  speed = speed * 1; // Makes speed negative to start self-correcting right away

  //speed = speed / 64;
  //Serial.println("speed:");
  //Serial.println(speed);

  analogWrite(pinSpeed_Left, speed); // drive left wheel with speed parameter
  analogWrite(pinSpeed_Right, speed); // drive right wheel with speed parameter
  if (speed < 0) // If speed is negative make it positive and then
  {
    speed = speed * -1;
    if (speed > max_speed)
      speed = max_speed;
    analogWrite(pinSpeed_Left, speed);
    analogWrite(pinSpeed_Right, speed);
    digitalWrite(pinCW_Right, LOW);
    digitalWrite(pinCC_Right, HIGH);
    digitalWrite(pinCW_Left, LOW);
    digitalWrite(pinCC_Left, HIGH);
    /*
        digitalWrite(pinCW_Right,HIGH);
        digitalWrite(pinCC_Right,LOW);
        digitalWrite(pinCW_Left,HIGH);
        digitalWrite(pinCC_Left,LOW);
    */


  }
  else
  {
    if (speed > max_speed)
      speed = max_speed;
    analogWrite(pinSpeed_Left, speed);
    analogWrite(pinSpeed_Right, speed);

    /*digitalWrite(pinCW_Right,LOW);
      digitalWrite(pinCC_Right,HIGH);
      digitalWrite(pinCW_Left,LOW);
      digitalWrite(pinCC_Left,HIGH);
    */
    digitalWrite(pinCW_Right, HIGH);
    digitalWrite(pinCC_Right, LOW);
    digitalWrite(pinCW_Left, HIGH);
    digitalWrite(pinCC_Left, LOW);
  }

}
void loop()
{
  pid.Compute();

  robot_move(output);

  // If intPin goes high, all data registers have new data
  // On interrupt, check if data ready interrupt
  if (myIMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
  {
    //robot_move(output);
    myIMU.readAccelData(myIMU.accelCount);  // Read the x/y/z adc values


    // Now we'll calculate the accleration value into actual g's
    // This depends on scale being set
    myIMU.ax = (float)myIMU.accelCount[0] * myIMU.aRes; // - myIMU.accelBias[0];
    myIMU.ay = (float)myIMU.accelCount[1] * myIMU.aRes; // - myIMU.accelBias[1];
    myIMU.az = (float)myIMU.accelCount[2] * myIMU.aRes; // - myIMU.accelBias[2];

    myIMU.readGyroData(myIMU.gyroCount);  // Read the x/y/z adc values

    // Calculate the gyro value into actual degrees per second
    // This depends on scale being set
    myIMU.gx = (float)myIMU.gyroCount[0] * myIMU.gRes;
    myIMU.gy = (float)myIMU.gyroCount[1] * myIMU.gRes;
    myIMU.gz = (float)myIMU.gyroCount[2] * myIMU.gRes;

    //myIMU.readMagData(myIMU.magCount);  // Read the x/y/z adc values

    // Calculate the magnetometer values in milliGauss
    // Include factory calibration per data sheet and user environmental
    // corrections
    // Get actual magnetometer value, this depends on scale being set
    //myIMU.mx = (float)myIMU.magCount[0] * myIMU.mRes
    //           * myIMU.factoryMagCalibration[0] - myIMU.magBias[0];
    //myIMU.my = (float)myIMU.magCount[1] * myIMU.mRes
    //           * myIMU.factoryMagCalibration[1] - myIMU.magBias[1];
    //myIMU.mz = (float)myIMU.magCount[2] * myIMU.mRes
    //           * myIMU.factoryMagCalibration[2] - myIMU.magBias[2];
    //mx = -189 my = 355 mz = 127 mG
    myIMU.mx = -189; myIMU.my = 355; myIMU.mz = 127;
  } // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)


  // Must be called before updating quaternions!
  myIMU.updateTime();

  // Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
  // the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
  // (+ up) of accelerometer and gyro! We have to make some allowance for this
  // orientationmismatch in feeding the output to the quaternion filter. For the
  // MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
  // along the x-axis just like in the LSM9DS0 sensor. This rotation can be
  // modified to allow any convenient orientation convention. This is ok by
  // aircraft orientation standards! Pass gyro rate as rad/s
  MahonyQuaternionUpdate(myIMU.ax, myIMU.ay, myIMU.az, myIMU.gx * DEG_TO_RAD,
                         myIMU.gy * DEG_TO_RAD, myIMU.gz * DEG_TO_RAD, myIMU.my,
                         myIMU.mx, myIMU.mz, myIMU.deltat);

  if (!AHRS)
  {
    myIMU.delt_t = millis() - myIMU.count;
    if (myIMU.delt_t > 500)
    {
      if (SerialDebug)
      {
        // Print acceleration values in milligs!
        Serial.print("X-acceleration: "); Serial.print(1000 * myIMU.ax);
        Serial.print(" mg ");
        Serial.print("Y-acceleration: "); Serial.print(1000 * myIMU.ay);
        Serial.print(" mg ");
        Serial.print("Z-acceleration: "); Serial.print(1000 * myIMU.az);
        Serial.println(" mg ");

        // Print gyro values in degree/sec
        Serial.print("X-gyro rate: "); Serial.print(myIMU.gx, 3);
        Serial.print(" degrees/sec ");
        Serial.print("Y-gyro rate: "); Serial.print(myIMU.gy, 3);
        Serial.print(" degrees/sec ");
        Serial.print("Z-gyro rate: "); Serial.print(myIMU.gz, 3);
        Serial.println(" degrees/sec");

        // Print mag values in degree/sec
        Serial.print("X-mag field: "); Serial.print(myIMU.mx);
        Serial.print(" mG ");
        Serial.print("Y-mag field: "); Serial.print(myIMU.my);
        Serial.print(" mG ");
        Serial.print("Z-mag field: "); Serial.print(myIMU.mz);
        Serial.println(" mG");

        myIMU.tempCount = myIMU.readTempData();  // Read the adc values
        // Temperature in degrees Centigrade
        myIMU.temperature = ((float) myIMU.tempCount) / 333.87 + 21.0;
        // Print temperature in degrees Centigrade
        Serial.print("Temperature is ");  Serial.print(myIMU.temperature, 1);
        Serial.println(" degrees C");
      }



      myIMU.count = millis();
      //      digitalWrite(myLed, !digitalRead(myLed));  // toggle led
    } // if (myIMU.delt_t > 500)
  } // if (!AHRS)
  else
  {
    // Serial print and/or display at 0.5 s rate independent of data rates
    myIMU.delt_t = millis() - myIMU.count;

    // update LCD once per half-second independent of read rate
    if (myIMU.delt_t > 500)
    {
      if (SerialDebug)
      {
        Serial.print("ax = ");  Serial.print((int)1000 * myIMU.ax);
        Serial.print(" ay = "); Serial.print((int)1000 * myIMU.ay);
        Serial.print(" az = "); Serial.print((int)1000 * myIMU.az);
        Serial.println(" mg");

        Serial.print("gx = ");  Serial.print(myIMU.gx, 2);
        Serial.print(" gy = "); Serial.print(myIMU.gy, 2);
        Serial.print(" gz = "); Serial.print(myIMU.gz, 2);
        Serial.println(" deg/s");

        Serial.print("mx = ");  Serial.print((int)myIMU.mx);
        Serial.print(" my = "); Serial.print((int)myIMU.my);
        Serial.print(" mz = "); Serial.print((int)myIMU.mz);
        Serial.println(" mG");

        Serial.print("q0 = ");  Serial.print(*getQ());
        Serial.print(" qx = "); Serial.print(*(getQ() + 1));
        Serial.print(" qy = "); Serial.print(*(getQ() + 2));
        Serial.print(" qz = "); Serial.println(*(getQ() + 3));
      }

      // Define output variables from updated quaternion---these are Tait-Bryan
      // angles, commonly used in aircraft orientation. In this coordinate system,
      // the positive z-axis is down toward Earth. Yaw is the angle between Sensor
      // x-axis and Earth magnetic North (or true North if corrected for local
      // declination, looking down on the sensor positive yaw is counterclockwise.
      // Pitch is angle between sensor x-axis and Earth ground plane, toward the
      // Earth is positive, up toward the sky is negative. Roll is angle between
      // sensor y-axis and Earth ground plane, y-axis up is positive roll. These
      // arise from the definition of the homogeneous rotation matrix constructed
      // from quaternions. Tait-Bryan angles as well as Euler angles are
      // non-commutative; that is, the get the correct orientation the rotations
      // must be applied in the correct order which for this configuration is yaw,
      // pitch, and then roll.
      // For more see
      // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
      // which has additional links.
      myIMU.yaw   = atan2(2.0f * (*(getQ() + 1) * *(getQ() + 2) + *getQ()
                                  * *(getQ() + 3)), *getQ() * *getQ() + * (getQ() + 1)
                          * *(getQ() + 1) - * (getQ() + 2) * *(getQ() + 2) - * (getQ() + 3)
                          * *(getQ() + 3));
      myIMU.pitch = -asin(2.0f * (*(getQ() + 1) * *(getQ() + 3) - *getQ()
                                  * *(getQ() + 2)));
      myIMU.roll  = atan2(2.0f * (*getQ() * *(getQ() + 1) + * (getQ() + 2)
                                  * *(getQ() + 3)), *getQ() * *getQ() - * (getQ() + 1)
                          * *(getQ() + 1) - * (getQ() + 2) * *(getQ() + 2) + * (getQ() + 3)
                          * *(getQ() + 3));
      myIMU.pitch *= RAD_TO_DEG;
      myIMU.yaw   *= RAD_TO_DEG;

      // Declination of SparkFun Electronics (40Â°05'26.6"N 105Â°11'05.9"W) is
      // 	8Â° 30' E  Â± 0Â° 21' (or 8.5Â°) on 2016-07-19
      // - http://www.ngdc.noaa.gov/geomag-web/#declination
      myIMU.yaw  -= 8.5;
      myIMU.roll *= RAD_TO_DEG;

      input = myIMU.pitch + 180;
      if (cnt != 1)
      {
        cnt = 1;
        //setpoint = input;
      }
      Serial.println(input);
      if (SerialDebug)
      {
        Serial.print("Yaw, Pitch, Roll: ");
        Serial.print(myIMU.yaw, 2);
        Serial.print(", ");
        Serial.print(myIMU.pitch, 2);
        Serial.print(", ");
        Serial.println(myIMU.roll, 2);
        Serial.println(output);

        Serial.print("rate = ");
        Serial.print((float)myIMU.sumCount / myIMU.sum, 2);
        Serial.println(" Hz");
      }



      myIMU.count = millis();
      myIMU.sumCount = 0;
      myIMU.sum = 0;
    } // if (myIMU.delt_t > 500)
  } // if (AHRS)
}

