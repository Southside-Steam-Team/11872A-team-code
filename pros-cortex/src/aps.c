#include "main.h"
#include "math.h"
#include "aps.h"
#include "global.h"
#include "chassis.h"

// Physical parameters in inches
const double sL = 7.0625;                // distance from center to left tracking wheel
const double sR = 7.0625;                // distance from center to right tracking wheel
const double sB = 10.5;                // distance from center to back tracking wheel
const double sideWheelRadius = 2.0;   // radius of side wheels
const double backWheelRadius = 2.0;   // radius of back wheel
// Encoder counts
const int sideTicksPerRotation = 360; // side encoder ticks per 360 degrees of motion
const int backTicksPerRotation = 360; // back encoder ticks per 360 degrees of motion

// Previous positions
double prevPos[2] = {0.0, 0.0};
double prevAngle = 0.0;
double resetAngle = 0.0;
// Previous encoder values
int prevLeft = 0;
int prevRight = 0;
int prevBack = 0;

// Not sure if using v5 or cortex, just rewrite these functions to make code work
int getLeftEncoder()
{
  return 0;
  //return encoderGet(leftEncoder);
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
  prevLeft = 0;
}
void resetRightEncoder()
{
  encoderReset(rightEncoder);
  prevRight = 0;
}
void resetBackEncoder()
{
  encoderReset(backEncoder);
  prevBack = 0;
}

void initializeAPS(double startX, double startY, double startAngle)
{
  resetLeftEncoder();
  resetRightEncoder();
  resetBackEncoder();

  prevPos[X_COMP] = startX;
  prevPos[Y_COMP] = startY;
  resetAngle = degToRad(startAngle);

  taskCreate(startTracking, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT + 1);
}
void startTracking(void *ignore)
{
  long gyroTimer = millis();

  while (true)
  {
    // Get current encoder values
    //int currentLeft = getLeftEncoder();
    int currentRight = getRightEncoder();
    int currentBack = getBackEncoder();

    // Calculate traveled distance in inches
    //double deltaLeft = sideWheelRadius * encoderToRad(currentLeft - prevLeft, sideTicksPerRotation);
    double deltaRight = sideWheelRadius * encoderToRad(currentRight - prevRight, sideTicksPerRotation);
    double deltaBack = backWheelRadius * encoderToRad(currentBack - prevBack, backTicksPerRotation);

    //printf("%f\t%f\t%f\n", deltaLeft, deltaRight, deltaBack);

    // Update prev values;
    //prevLeft = currentLeft;
    prevRight = currentRight;
    prevBack = currentBack;

    // Calculate total change since last reset
    //double totalLeft = sideWheelRadius * encoderToRad(currentLeft, sideTicksPerRotation);
    double totalRight = sideWheelRadius * encoderToRad(currentRight, sideTicksPerRotation);

    // Calculate new absolute orientation
    // Calculate delta time from last iteration
    double deltaTime = (double) (millis() - gyroTimer) / 1000;
    // Reset loop timer
    gyroTimer = millis();
    // Calculate change in angle
    double deltaAngle = degToRad(gyro_get_rate(gyro) * deltaTime);

    // Calculate local offset
    double localOffset[] = {0.0, 0.0};

    // If drove straight (about < 1 deg diff)
    if (fabs(deltaAngle) < 0.01)
    {
      localOffset[X_COMP] = deltaBack;
      localOffset[Y_COMP] = deltaRight;
    }
    else
    {
      localOffset[X_COMP] = 2 * sin(deltaAngle / 2) * ((deltaBack / deltaAngle) + sB);
      localOffset[Y_COMP] = 2 * sin(deltaAngle / 2) * ((deltaRight / deltaAngle) + sR);
    }

    // Calculate average angle
    double avgAngle = prevAngle + (deltaAngle / 2);

    // Convert localOffset to polar
    double localPolar[] = {0.0, 0.0};
    convertPolar(localOffset, localPolar);

    // Shift angle
    localPolar[ANGLE] -= avgAngle;
    double globalOffset[] = {0.0, 0.0};

    // Converting back to cartesian gives the globalOffset
    convertCart(localPolar, globalOffset);

    // Calculate new absolute position
    prevPos[X_COMP] += globalOffset[X_COMP];
    prevPos[Y_COMP] += globalOffset[Y_COMP];

    // Update previous angle
    prevAngle += deltaAngle;

    delay(2);
  }
}
double nearestEquivalentAngle(double ref, double target)
{
  return round((ref - target) / (2 * M_PI)) * 2 * M_PI + target;
}
void convertPolar(double *source, double *target)
{
  target[MAGNITUDE] = sqrt(pow(source[X_COMP], 2) + pow(source[Y_COMP], 2));
  double tempAngle = atan(source[Y_COMP] / source[X_COMP]);

  if (source[X_COMP] < 0.0)
  {
    tempAngle += M_PI;
  }

  target[ANGLE] = tempAngle;
}
void convertCart(double *source, double *target)
{
  target[X_COMP] = source[MAGNITUDE] * cos(source[ANGLE]);
  target[Y_COMP] = source[MAGNITUDE] * sin(source[ANGLE]);
}
double encoderToRad(int count, int ticksPerRotation)
{
  return ((double) count / (double) ticksPerRotation) * 2 * M_PI;
}
double degToRad(double degrees)
{
  return degrees * (M_PI / 180);
}
double radToDeg(double rads)
{
  return rads * (180 / M_PI);
}