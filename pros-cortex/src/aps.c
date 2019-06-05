#include "main.h"

// Physical parameters in inches
const double sL = 4.5;               // distance from center to left tracking wheel
const double sR = 4.5;               // distance from center to right tracking wheel
const double sB = 7.0;                 // distance from center to back tracking wheel
const double sideWheelDiameter = 4.0;   // diameter of side wheels
const double backWheelDiameter = 4.0;   // diameter of back wheel
// Encoder counts
const int sideEncoderResolution = 360;  // side encoder ticks per 360 degrees of motion
const int backEncoderResolution = 360;  // back encoder ticks per 360 degrees of motion

// Current angle calculated by gyro
double gyroAngle = 0.0;

// Previous position and orientation
double robotPose[3] = {0.0, 0.0, 0.0};
double resetAngle = 0.0;
// Previous encoder values
int prevLeftEncoder = 0;
int prevRightEncoder = 0;
int prevBackEncoder = 0;

// Not sure if using v5 or cortex, just rewrite these functions to make code work
int getLeftEncoder()
{
  //return encoderGet(leftEncoder);
  return 0;
}
int getRightEncoder()
{
  return encoderGet(rightEncoder);
}
int getBackEncoder()
{
  return encoderGet(backEncoder);
}
void resetLeftEncoder()
{
  //encoderReset(leftEncoder);
  prevLeftEncoder = 0;
}
void resetRightEncoder()
{
  encoderReset(rightEncoder);
  prevRightEncoder = 0;
}
void resetBackEncoder()
{
  encoderReset(backEncoder);
  prevBackEncoder = 0;
}
void initializeAPS(double startX, double startY, double startAngle)
{
  // Reset to starting positions
  resetPosition(startX, startY, startAngle);

  // Create new tasks to track position
  taskCreate(startGyroIntegral, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT + 2);
  taskCreate(startTracking, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT + 1);
}
void resetPosition(double resetX, double resetY, double resetAngle)
{
  // Reset all the encoders
  resetLeftEncoder();
  resetRightEncoder();
  resetBackEncoder();

  // Set reset pose
  robotPose[POSE_X] = resetX;
  robotPose[POSE_Y] = resetY;
  robotPose[POSE_ANGLE] = resetAngle;

  // Set reset orientation
  resetAngle = degToRad(resetAngle);
  gyroAngle = resetAngle;
}
void startGyroIntegral(void *ignore)
{
  unsigned long gyroTimer = millis();

  while (true)
  {
    // Calculate delta time from last iteration
    double deltaTime = ((double) (millis() - gyroTimer)) / 1000;

    //printf("%f\n", gyroAngle);
    // Reset loop timer
    gyroTimer = millis();

    double gyroRate = gyro_get_rate(&mainGyro);
    double deltaAngle = gyroRate * deltaTime;

    //printf("RATE: %f\tTIME: %f\t DANGLE: %f\tANGLE: %f\n", gyroRate, deltaTime, deltaAngle, gyroAngle);

    // Add to angle with rate from gyro
    gyroAngle += degToRad(gyro_get_rate(&mainGyro) * deltaTime);

    delay(20);
  }
}
void startTracking(void *ignore)
{
  while (true)
  {
    // Get current encoder values
    //int currentLeftEncoder = getLeftEncoder();
    int currentRightEncoder = getRightEncoder();
    int currentBackEncoder = getBackEncoder();

    // Calculate traveled distance in inches
    //double deltaLeft = calculateTravelDistance(currentLeftEncoder - prevLeftEncoder, sideWheelDiameter, sideEncoderResolution);
    double deltaRightDistance = calculateTravelDistance(currentRightEncoder - prevRightEncoder, sideWheelDiameter, sideEncoderResolution);
    double deltaBackDistance = calculateTravelDistance(currentBackEncoder - prevBackEncoder, backWheelDiameter, backEncoderResolution);

//printf("%f\t%f\t%f\n", deltaLeft, deltaRight, deltaBack);

    // Update prev values;
    //prevLeftEncoder = currentLeftEncoder;
    prevRightEncoder = currentRightEncoder;
    prevBackEncoder = currentBackEncoder;

    // Calculate total change since last reset
    //double totalLeftDistance = calculateTravelDistance(currentLeftEncoder, sideWheelDiameter, sideEncoderResolution);
    //double totalRightDistance = calculateTravelDistance(currentRightEncoder, sideWheelDiameter, sideEncoderResolution);

    // Calculate new absolute orientation
    //double newAngle = resetAngle + (totalLeftDistance - totalRightDistance) / (sL + sR);

    // Calculate change in angle
    //double deltaAngle = newAngle - prevAngle;
    double deltaAngle = gyroAngle - robotPose[POSE_ANGLE];

    // Calculate local offset vector
    double localOffset[] = {0.0, 0.0};

    // If drove straight (about < 1 deg diff)
    if (fabs(deltaAngle) < 0.01)
    {
      localOffset[X_COMP] = deltaBackDistance;
      localOffset[Y_COMP] = deltaRightDistance;
    }
    else
    {
      localOffset[X_COMP] = 2 * sin(deltaAngle / 2) * ((deltaBackDistance / deltaAngle) + sB);
      localOffset[Y_COMP] = 2 * sin(deltaAngle / 2) * ((deltaRightDistance / deltaAngle) + sR);
    }

    // Calculate average angle
    double avgAngle = robotPose[POSE_ANGLE] + (deltaAngle / 2);

    // Convert localOffset to a polar vector
    double localPolar[] = {0.0, 0.0};
    cartToPolar(localOffset, localPolar);

    // Shift angle
    localPolar[ANGLE] -= avgAngle;

    // Converting back to cartesian gives the globalOffset
    double globalOffset[] = {0.0, 0.0};
    polarToCart(localPolar, globalOffset);

    // Calculate new absolute position and orientation
    robotPose[POSE_X] += globalOffset[X_COMP];
    robotPose[POSE_Y] += globalOffset[Y_COMP];
    robotPose[POSE_ANGLE] = gyroAngle;

    delay(10);
  }
}
double nearestEquivalentAngle(double reference, double target)
{
  return round((reference - target) / (2 * M_PI)) * 2 * M_PI + target;
}
void cartToPolar(double *cartVector, double *polarVector)
{
  // Calculates magnitude of vector with distance formula
  polarVector[MAGNITUDE] = sqrt(pow(cartVector[X_COMP], 2) + pow(cartVector[Y_COMP], 2));
  // Calculates angle with arctan, automatically gives angle in correct quadrant
  polarVector[ANGLE] = atan2(cartVector[Y_COMP], cartVector[X_COMP]);
}
void polarToCart(double *polarVector, double *cartVector)
{
  // Calculate x component with cosine
  cartVector[X_COMP] = polarVector[MAGNITUDE] * cos(polarVector[ANGLE]);
  // Calculate y component with sine
  cartVector[Y_COMP] = polarVector[MAGNITUDE] * sin(polarVector[ANGLE]);
}
double calculateTravelDistance(int encoderCount, double wheelDiameter, int encoderResolution)
{
  return ((double) encoderCount * M_PI * wheelDiameter) / ((double) encoderResolution);
}
double degToRad(double degrees)
{
  return degrees * (M_PI / 180);
}
double radToDeg(double radians)
{
  return radians * (180 / M_PI);
}
