#include "main.h"

PID armPID;

void asyncArmTask(void *ignore)
{
  int prevArmTarget = -1;
  pidInit(&armPID, 0.005, 0.001, 0.0);

  while (true)
  {
    // Precaution in case mutex is taken
    int currentArmTarget = prevArmTarget;
    int currentArmPot = getArmPot();

    // Take mutexes and set proper variables, depending on if operator control
    if (!isAutonomous() && isEnabled())
    {
      if (joystickGetDigital(1, 6, JOY_UP))
  		{
  			// Press buttons to set arm position
  			if (joystickGetDigital(1, 6, JOY_DOWN))
  				currentArmTarget = ARM_ZERO;
  			else if (joystickGetDigital(1, 5, JOY_DOWN))
  				currentArmTarget = ARM_LOW;
  			else if (joystickGetDigital(1, 5, JOY_UP))
  				currentArmTarget = ARM_MED;
  		}
      // If need to kill arm
      else if (joystickGetDigital(1, 8, JOY_UP))
      {
        currentArmTarget = -1;
        prevArmTarget = currentArmTarget;
        mutexTake(mutexes[MUTEX_ASYNC_ARM], 200);
        isArmAtTarget = true;
        mutexGive(mutexes[MUTEX_ASYNC_ARM]);
      }
    }
    else
    {
      mutexTake(mutexes[MUTEX_ASYNC_ARM], 200);
      currentArmTarget = nextArmTarget;
      mutexGive(mutexes[MUTEX_ASYNC_ARM]);
    }

    // Disengage if no target set
    if (currentArmTarget < 0)
      continue;

    // Calculate and set power for arm
    int power = pidCalculate(&armPID, currentArmTarget, currentArmPot) * 127;
    setArms(power);

    // Debug
    //printf("armPot: %d\tpower: %d\ttarget: %d\n", currentArmPot, power, currentArmTarget);

    // Set prev variables
    prevArmTarget = currentArmTarget;

    // Sets if arm is within target
    if (abs(currentArmTarget - currentArmPot) < 50)
    {
      mutexTake(mutexes[MUTEX_ASYNC_ARM], 200);
      isArmAtTarget = true;
      mutexGive(mutexes[MUTEX_ASYNC_ARM]);
    }
    else
    {
      mutexTake(mutexes[MUTEX_ASYNC_ARM], 200);
      isArmAtTarget = false;
      mutexGive(mutexes[MUTEX_ASYNC_ARM]);
    }

    // Signals tray if arms are within range to bring back tray
    if (abs(currentArmTarget - currentArmPot) < 1000)
    {
      mutexTake(mutexes[MUTEX_ASYNC_ARM], 200);
      isArmAtTargetTray = true;
      mutexGive(mutexes[MUTEX_ASYNC_ARM]);
    }
    else
    {
      mutexTake(mutexes[MUTEX_ASYNC_ARM], 200);
      isArmAtTargetTray = false;
      mutexGive(mutexes[MUTEX_ASYNC_ARM]);
    }

    delay(20);
  }
}
void waitUntilArmMoveComplete()
{
  while (!isArmAtTarget) { delay(40); }
}
void startAsyncArmController()
{
  startAsyncTrayController();
  // Only if task is not already running
  unsigned int asyncState = taskGetState(asyncArmHandle);
  if (asyncArmHandle != NULL && (asyncState != TASK_DEAD))
    return;

  // Reset variables
  mutexTake(mutexes[MUTEX_ASYNC_ARM], 200);
  nextArmTarget = -1;
  isArmAtTarget = true;
  mutexGive(mutexes[MUTEX_ASYNC_ARM]);

  // Stop the arm
  stopArms();

  // Create the task
  asyncArmHandle = taskCreate(asyncArmTask, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT + 1);
}
void stopAsyncArmController()
{
  // Stop task if needed
  unsigned int asyncState = taskGetState(asyncArmHandle);
  if (asyncArmHandle != NULL && (asyncState != TASK_DEAD))
    taskDelete(asyncArmHandle);

  // Reset variables
  mutexTake(mutexes[MUTEX_ASYNC_ARM], 500);
  nextArmTarget = -1;
  isArmAtTarget = true;
  mutexGive(mutexes[MUTEX_ASYNC_ARM]);

  // Stop the arms
  stopArms();
}
void moveArmsZeroAsync()
{
  // Start asyncArmController if needed
  startAsyncArmController();
  // Set the proper target
  mutexTake(mutexes[MUTEX_ASYNC_ARM], 1000);
  nextArmTarget = ARM_ZERO;
  isArmAtTarget = false;
  mutexGive(mutexes[MUTEX_ASYNC_ARM]);
}
void moveArmsScoreAsync()
{
  // Start asyncArmController if needed
  startAsyncArmController();
  // Set the proper target
  mutexTake(mutexes[MUTEX_ASYNC_ARM], 1000);
  nextArmTarget = ARM_SCORE;
  isArmAtTarget = false;
  mutexGive(mutexes[MUTEX_ASYNC_ARM]);
}
void moveArmsLowAsync()
{
  // Start asyncArmController if needed
  startAsyncArmController();
  // Set the proper target
  mutexTake(mutexes[MUTEX_ASYNC_ARM], 1000);
  nextArmTarget = ARM_LOW;
  isArmAtTarget = false;
  mutexGive(mutexes[MUTEX_ASYNC_ARM]);
}
void moveArmsMedAsync()
{
  // Start asyncArmController if needed
  startAsyncArmController();
  // Set the proper target
  mutexTake(mutexes[MUTEX_ASYNC_ARM], 1000);
  nextArmTarget = ARM_MED;
  isArmAtTarget = false;
  mutexGive(mutexes[MUTEX_ASYNC_ARM]);
}
int getArmPot()
{
  return analogRead(PORT_armPot);
}
void setArms(int power)
{
  motorSet(PORT_leftArm, power);
  motorSet(PORT_rightArm, -power);
}
void setRollers(int power)
{
  motorSet(PORT_leftRoller, power);
  motorSet(PORT_rightRoller, -power);
}
void stopArms()
{
  motorStop(PORT_leftArm);
  motorStop(PORT_rightArm);
}
void stopRollers()
{
  motorStop(PORT_leftRoller);
  motorStop(PORT_rightRoller);
}
