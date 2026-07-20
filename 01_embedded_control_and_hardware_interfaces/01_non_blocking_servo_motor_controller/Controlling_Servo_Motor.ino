/*
  Project: Non-Blocking Servo Position Controller
  File: Controlling_Servo_Motor.ino

  Overview:
  This program controls a hobby servo connected to an Arduino Uno. It has
  two operating modes. In automatic mode, the servo moves continuously
  between 40 and 180 degrees. In manual mode, a target angle is entered
  from the Serial Monitor and the servo moves gradually to that position.

  The movement is controlled with millis() instead of delay(). This keeps
  the program responsive while the servo is moving, so commands can be
  received at any time.

  Main functions:
  - Automatic sweep between configurable angle limits
  - Manual target-angle control
  - Smooth one-degree position updates
  - Adjustable movement speed
  - Pause and resume control
  - Current status reporting
  - Automatic status update after mode changes
  - Fixed-size command buffer instead of the Arduino String class

  Hardware:
  - Arduino Uno or compatible board
  - Standard hobby servo
  - Servo signal connected to digital pin 3
  - Servo VCC connected to a suitable regulated supply
  - Servo ground connected to Arduino GND

  Power connection:
  A servo may draw more current than the Arduino 5 V pin can provide.
  Use an external regulated 5 V supply when required. The external supply
  ground and Arduino ground must be connected together.

  Serial Monitor:
  - Baud rate: 115200
  - Line ending: Newline or Both NL & CR

  Commands:
  - auto          Start automatic sweep mode
  - manual        Select manual mode and hold the current position
  - angle 120     Move to a target angle
  - center        Move to 90 degrees
  - speed 15      Set the movement update interval in milliseconds
  - pause         Stop movement temporarily
  - resume        Continue movement
  - status        Print the current controller state
  - help          Print the command list

  Technical topics:
  - Servo library
  - Non-blocking timing
  - Operating-mode control
  - Serial command parsing
  - Input validation
  - Fixed-size character buffers
  - Modular embedded C++ functions
*/

#include <Arduino.h>
#include <Servo.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Arduino pin connected to the servo signal wire.
constexpr uint8_t SERVO_PIN = 3;

// Servo position limits and movement step.
constexpr int MIN_ANGLE_DEG = 40;
constexpr int MAX_ANGLE_DEG = 180;
constexpr int CENTER_ANGLE_DEG = 90;
constexpr int POSITION_STEP_DEG = 1;

// Allowed interval between one-degree position updates.
constexpr unsigned long DEFAULT_UPDATE_INTERVAL_MS = 20;
constexpr unsigned long MIN_UPDATE_INTERVAL_MS = 5;
constexpr unsigned long MAX_UPDATE_INTERVAL_MS = 100;

// Maximum accepted Serial Monitor command length.
constexpr size_t COMMAND_BUFFER_SIZE = 40;

enum class OperatingMode : uint8_t
{
  Automatic,
  Manual
};

Servo servoMotor;

OperatingMode operatingMode = OperatingMode::Automatic;

int currentAngleDeg = CENTER_ANGLE_DEG;
int targetAngleDeg = CENTER_ANGLE_DEG;
int automaticDirection = 1;

bool movementPaused = false;

// Prevents the "target reached" message from being printed repeatedly.
bool manualTargetReachedReported = true;

unsigned long updateIntervalMs = DEFAULT_UPDATE_INTERVAL_MS;
unsigned long previousUpdateTimeMs = 0;

char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;

// Servo control functions.
void updateServo();
void updateAutomaticMode();
void updateManualMode();

// Serial command functions.
void readSerialInput();
void processCommand(char* command);
void convertToLowercase(char* text);

// Parameter-setting functions.
void setTargetAngle(long requestedAngleDeg);
void setUpdateInterval(long requestedIntervalMs);

// User-information functions.
void printStatus();
void printHelp();
const __FlashStringHelper* modeToText();

void setup()
{
  // Start the Serial Monitor connection.
  Serial.begin(115200);

  // Connect the Servo library to pin 3 and start at 90 degrees.
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(currentAngleDeg);

  // Start the non-blocking movement timer.
  previousUpdateTimeMs = millis();

  Serial.println();
  Serial.println(F("Servo position controller started."));
  Serial.println(F("Enter 'help' to show the available commands."));
  printStatus();
}

void loop()
{
  // Serial input is checked continuously, even while the servo is moving.
  readSerialInput();

  // Servo movement is updated only when the timing interval has elapsed.
  updateServo();
}

void updateServo()
{
  // Pausing stops position changes without blocking Serial Monitor commands.
  if (movementPaused)
  {
    return;
  }

  const unsigned long currentTimeMs = millis();

  // Unsigned subtraction remains valid when millis() overflows.
  if ((currentTimeMs - previousUpdateTimeMs) < updateIntervalMs)
  {
    return;
  }

  previousUpdateTimeMs = currentTimeMs;

  // Run the movement routine for the selected operating mode.
  if (operatingMode == OperatingMode::Automatic)
  {
    updateAutomaticMode();
  }
  else
  {
    updateManualMode();
  }
}

void updateAutomaticMode()
{
  // Move one degree in the current sweep direction.
  currentAngleDeg += automaticDirection * POSITION_STEP_DEG;

  // Reverse the sweep direction at either configured angle limit.
  if (currentAngleDeg >= MAX_ANGLE_DEG)
  {
    currentAngleDeg = MAX_ANGLE_DEG;
    automaticDirection = -1;
  }
  else if (currentAngleDeg <= MIN_ANGLE_DEG)
  {
    currentAngleDeg = MIN_ANGLE_DEG;
    automaticDirection = 1;
  }

  // In automatic mode, the current sweep position is also the target.
  targetAngleDeg = currentAngleDeg;

  servoMotor.write(currentAngleDeg);
}

void updateManualMode()
{
  // Hold the requested position after the target has been reached.
  if (currentAngleDeg == targetAngleDeg)
  {
    if (!manualTargetReachedReported)
    {
      manualTargetReachedReported = true;

      Serial.println(F("Target position reached."));
      printStatus();
    }

    return;
  }

  // Approach the target gradually instead of jumping directly to it.
  if (currentAngleDeg < targetAngleDeg)
  {
    currentAngleDeg += POSITION_STEP_DEG;

    if (currentAngleDeg > targetAngleDeg)
    {
      currentAngleDeg = targetAngleDeg;
    }
  }
  else
  {
    currentAngleDeg -= POSITION_STEP_DEG;

    if (currentAngleDeg < targetAngleDeg)
    {
      currentAngleDeg = targetAngleDeg;
    }
  }

  servoMotor.write(currentAngleDeg);

  // Report the final manual position once the movement is complete.
  if (currentAngleDeg == targetAngleDeg &&
      !manualTargetReachedReported)
  {
    manualTargetReachedReported = true;

    Serial.println(F("Target position reached."));
    printStatus();
  }
}

void readSerialInput()
{
  // Read all characters currently waiting in the serial receive buffer.
  while (Serial.available() > 0)
  {
    const char receivedCharacter =
        static_cast<char>(Serial.read());

    // A line ending marks the end of one command.
    if (receivedCharacter == '\n' ||
        receivedCharacter == '\r')
    {
      if (commandLength > 0)
      {
        // Add the null terminator required by C string functions.
        commandBuffer[commandLength] = '\0';

        processCommand(commandBuffer);
        commandLength = 0;
      }

      continue;
    }

    // Leave one byte available for the terminating null character.
    if (commandLength < COMMAND_BUFFER_SIZE - 1)
    {
      commandBuffer[commandLength++] = receivedCharacter;
    }
    else
    {
      // Clear an input that cannot fit safely in the command buffer.
      commandLength = 0;
      Serial.println(F("Command is too long."));
    }
  }
}

void processCommand(char* command)
{
  // Commands can be entered with uppercase or lowercase letters.
  convertToLowercase(command);

  // Separate the command name from an optional argument.
  char* commandName = strtok(command, " ");

  if (commandName == nullptr)
  {
    return;
  }

  if (strcmp(commandName, "auto") == 0)
  {
    // Resume the continuous sweep from the current servo position.
    operatingMode = OperatingMode::Automatic;
    movementPaused = false;
    manualTargetReachedReported = true;
    previousUpdateTimeMs = millis();

    // Choose a direction that keeps the next step inside the valid range.
    if (currentAngleDeg >= MAX_ANGLE_DEG)
    {
      automaticDirection = -1;
    }
    else if (currentAngleDeg <= MIN_ANGLE_DEG)
    {
      automaticDirection = 1;
    }

    targetAngleDeg = currentAngleDeg;

    Serial.println(F("Automatic mode selected."));
    printStatus();
  }
  else if (strcmp(commandName, "manual") == 0)
  {
    // Manual mode holds the present position until another target is entered.
    operatingMode = OperatingMode::Manual;
    targetAngleDeg = currentAngleDeg;
    movementPaused = false;
    manualTargetReachedReported = true;

    Serial.println(F("Manual mode selected."));
    printStatus();
  }
  else if (strcmp(commandName, "angle") == 0)
  {
    // Read the number following the angle command.
    char* angleText = strtok(nullptr, " ");

    if (angleText == nullptr)
    {
      Serial.println(F("Use: angle <40-180>"));
      return;
    }

    char* endPointer = nullptr;
    const long requestedAngle =
        strtol(angleText, &endPointer, 10);

    // Reject an argument that contains non-numeric characters.
    if (*endPointer != '\0')
    {
      Serial.println(F("The angle must be a number."));
      return;
    }

    // An angle command always selects manual position control.
    operatingMode = OperatingMode::Manual;
    movementPaused = false;
    previousUpdateTimeMs = millis();

    setTargetAngle(requestedAngle);

    manualTargetReachedReported =
        (currentAngleDeg == targetAngleDeg);

    Serial.println(F("Manual mode selected."));
    printStatus();
  }
  else if (strcmp(commandName, "center") == 0)
  {
    // Centre the servo using the same manual-position routine.
    operatingMode = OperatingMode::Manual;
    movementPaused = false;
    previousUpdateTimeMs = millis();

    setTargetAngle(CENTER_ANGLE_DEG);

    manualTargetReachedReported =
        (currentAngleDeg == targetAngleDeg);

    Serial.println(F("Manual mode selected."));
    printStatus();
  }
  else if (strcmp(commandName, "speed") == 0)
  {
    // Read the requested interval between one-degree updates.
    char* speedText = strtok(nullptr, " ");

    if (speedText == nullptr)
    {
      Serial.println(F("Use: speed <5-100>"));
      return;
    }

    char* endPointer = nullptr;
    const long requestedInterval =
        strtol(speedText, &endPointer, 10);

    // Reject an argument that contains non-numeric characters.
    if (*endPointer != '\0')
    {
      Serial.println(F("The speed value must be a number."));
      return;
    }

    setUpdateInterval(requestedInterval);
    printStatus();
  }
  else if (strcmp(commandName, "pause") == 0)
  {
    // Keep the current position until movement is resumed.
    movementPaused = true;

    Serial.println(F("Movement paused."));
    printStatus();
  }
  else if (strcmp(commandName, "resume") == 0)
  {
    // Restart the timer so movement resumes from the current time.
    movementPaused = false;
    previousUpdateTimeMs = millis();

    Serial.println(F("Movement resumed."));
    printStatus();
  }
  else if (strcmp(commandName, "status") == 0)
  {
    printStatus();
  }
  else if (strcmp(commandName, "help") == 0)
  {
    printHelp();
  }
  else
  {
    Serial.println(F("Unknown command. Enter 'help'."));
  }
}

void convertToLowercase(char* text)
{
  // Convert the command in place until the null terminator is reached.
  while (*text != '\0')
  {
    *text = static_cast<char>(
        tolower(static_cast<unsigned char>(*text))
    );

    ++text;
  }
}

void setTargetAngle(long requestedAngleDeg)
{
  // Keep the requested target inside the configured servo range.
  const int limitedAngle = constrain(
      requestedAngleDeg,
      static_cast<long>(MIN_ANGLE_DEG),
      static_cast<long>(MAX_ANGLE_DEG)
  );

  targetAngleDeg = limitedAngle;

  Serial.print(F("Target angle: "));
  Serial.print(targetAngleDeg);
  Serial.println(F(" degrees"));

  if (requestedAngleDeg != limitedAngle)
  {
    Serial.println(
        F("The requested angle was limited to the safe range.")
    );
  }
}

void setUpdateInterval(long requestedIntervalMs)
{
  // Prevent movement updates from becoming excessively fast or slow.
  const unsigned long limitedInterval =
      static_cast<unsigned long>(
          constrain(
              requestedIntervalMs,
              static_cast<long>(MIN_UPDATE_INTERVAL_MS),
              static_cast<long>(MAX_UPDATE_INTERVAL_MS)
          )
      );

  updateIntervalMs = limitedInterval;

  Serial.print(F("Movement update interval: "));
  Serial.print(updateIntervalMs);
  Serial.println(F(" ms"));

  if (requestedIntervalMs !=
      static_cast<long>(limitedInterval))
  {
    Serial.println(
        F("The requested value was limited to 5-100 ms.")
    );
  }
}

void printStatus()
{
  Serial.println(F("Servo controller status"));

  Serial.print(F("  Mode: "));
  Serial.println(modeToText());

  Serial.print(F("  Current angle: "));
  Serial.print(currentAngleDeg);
  Serial.println(F(" degrees"));

  Serial.print(F("  Target angle: "));
  Serial.print(targetAngleDeg);
  Serial.println(F(" degrees"));

  Serial.print(F("  Update interval: "));
  Serial.print(updateIntervalMs);
  Serial.println(F(" ms"));

  Serial.print(F("  Movement: "));

  if (movementPaused)
  {
    Serial.println(F("PAUSED"));
  }
  else if (operatingMode == OperatingMode::Manual &&
           currentAngleDeg == targetAngleDeg)
  {
    Serial.println(F("HOLDING POSITION"));
  }
  else
  {
    Serial.println(F("MOVING"));
  }

  Serial.println();
}

void printHelp()
{
  Serial.println(F("Commands"));
  Serial.println(F("  auto          Automatic sweep mode"));
  Serial.println(F("  manual        Manual mode"));
  Serial.println(F("  angle 120     Set a target angle"));
  Serial.println(F("  center        Move to 90 degrees"));
  Serial.println(F("  speed 15      Set update interval"));
  Serial.println(F("  pause         Pause movement"));
  Serial.println(F("  resume        Resume movement"));
  Serial.println(F("  status        Print current status"));
  Serial.println(F("  help          Print this list"));
  Serial.println();
}

const __FlashStringHelper* modeToText()
{
  return operatingMode == OperatingMode::Automatic
      ? F("AUTOMATIC")
      : F("MANUAL");
}
