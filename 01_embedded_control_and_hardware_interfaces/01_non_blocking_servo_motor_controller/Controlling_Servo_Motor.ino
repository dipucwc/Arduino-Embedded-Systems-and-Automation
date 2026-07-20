/*
  Project: Non-Blocking Servo Position Controller
  File: Controlling_Servo_Motor(2).ino

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
  - manual        Select manual mode
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

// Hardware connection used by the Servo library.
constexpr uint8_t SERVO_PIN = 3;

// Allowed servo positions and the amount moved during each update.
constexpr int MIN_ANGLE_DEG = 40;
constexpr int MAX_ANGLE_DEG = 180;
constexpr int CENTER_ANGLE_DEG = 90;
constexpr int POSITION_STEP_DEG = 1;

// Timing limits are kept in milliseconds for use with millis().
constexpr unsigned long DEFAULT_UPDATE_INTERVAL_MS = 20;
constexpr unsigned long MIN_UPDATE_INTERVAL_MS = 5;
constexpr unsigned long MAX_UPDATE_INTERVAL_MS = 100;

// The fixed buffer avoids dynamic memory allocation on the Arduino.
constexpr size_t COMMAND_BUFFER_SIZE = 40;

// The controller operates either as an automatic sweep or manual position control.
enum class OperatingMode : uint8_t
{
  Automatic,
  Manual
};

// Servo object and controller state.
Servo servoMotor;

OperatingMode operatingMode = OperatingMode::Automatic;

int currentAngleDeg = CENTER_ANGLE_DEG;
int targetAngleDeg = CENTER_ANGLE_DEG;
int automaticDirection = 1;

bool movementPaused = false;

unsigned long updateIntervalMs = DEFAULT_UPDATE_INTERVAL_MS;
unsigned long previousUpdateTimeMs = 0;

// Incoming serial characters are collected here until a line ending is received.
char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;

// Function declarations keep the program layout easy to follow.
void updateServo();
void updateAutomaticMode();
void updateManualMode();

void readSerialInput();
void processCommand(char* command);
void convertToLowercase(char* text);

void setTargetAngle(long requestedAngleDeg);
void setUpdateInterval(long requestedIntervalMs);

void printStatus();
void printHelp();
const __FlashStringHelper* modeToText();

void setup()
{
  // Open the Serial Monitor connection used for commands and status messages.
  Serial.begin(115200);

  // Attach the servo signal to pin 3 and place the horn at the centre position.
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(currentAngleDeg);

  // Start the non-blocking timer from the current system time.
  previousUpdateTimeMs = millis();

  // Print a short startup message after the controller is ready.
  Serial.println();
  Serial.println(F("Servo position controller started."));
  Serial.println(F("Enter 'help' to show the available commands."));
  printStatus();
}

void loop()
{
  // Check for new commands on every pass through loop().
  readSerialInput();

  // Update the servo only when the selected interval has elapsed.
  updateServo();
}

void updateServo()
{
  // Pausing stops movement without blocking serial command handling.
  if (movementPaused)
  {
    return;
  }

  // Read the clock once so the same timestamp is used for this update.
  const unsigned long currentTimeMs = millis();

  // Unsigned subtraction also works correctly after millis() overflows.
  if ((currentTimeMs - previousUpdateTimeMs) < updateIntervalMs)
  {
    return;
  }

  // Store the time of this movement step.
  previousUpdateTimeMs = currentTimeMs;

  // Run the movement routine for the active operating mode.
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
  // Move one step in the current sweep direction.
  currentAngleDeg += automaticDirection * POSITION_STEP_DEG;

  // Reverse direction whenever an angle limit is reached.
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
  // No output update is needed after the requested position is reached.
  if (currentAngleDeg == targetAngleDeg)
  {
    return;
  }

  // Approach the target gradually instead of jumping directly to it.
  if (currentAngleDeg < targetAngleDeg)
  {
    ++currentAngleDeg;
  }
  else
  {
    --currentAngleDeg;
  }

  // Send the next position to the servo after changing the stored angle.
  servoMotor.write(currentAngleDeg);
}

void readSerialInput()
{
  // Read every character currently waiting in the serial receive buffer.
  while (Serial.available() > 0)
  {
    const char receivedCharacter =
        static_cast<char>(Serial.read());

    // A newline or carriage return marks the end of one command.
    if (receivedCharacter == '\n' || receivedCharacter == '\r')
    {
      if (commandLength > 0)
      {
        // Terminate the character array before passing it to string functions.
        commandBuffer[commandLength] = '\0';
        processCommand(commandBuffer);
        commandLength = 0;
      }

      continue;
    }

    // Keep one byte free for the terminating null character.
    if (commandLength < COMMAND_BUFFER_SIZE - 1)
    {
      commandBuffer[commandLength++] = receivedCharacter;
    }
    else
    {
      // Reset the buffer when the input cannot fit safely.
      commandLength = 0;
      Serial.println(F("Command is too long."));
    }
  }
}

void processCommand(char* command)
{
  // Commands are handled without requiring a specific letter case.
  convertToLowercase(command);

  // Split the first word from any optional numeric argument.
  char* commandName = strtok(command, " ");

  if (commandName == nullptr)
  {
    return;
  }

  // Select the requested action from the first command word.
  if (strcmp(commandName, "auto") == 0)
  {
    // Automatic mode always resumes the sweep immediately.
    operatingMode = OperatingMode::Automatic;
    movementPaused = false;
    previousUpdateTimeMs = millis();

    Serial.println(F("Automatic mode selected."));
  }
  else if (strcmp(commandName, "manual") == 0)
  {
    // Hold the present position until a manual target is entered.
    operatingMode = OperatingMode::Manual;
    targetAngleDeg = currentAngleDeg;

    Serial.println(F("Manual mode selected."));
  }
  else if (strcmp(commandName, "angle") == 0)
  {
    // Read the angle argument that follows the command name.
    char* angleText = strtok(nullptr, " ");

    if (angleText == nullptr)
    {
      Serial.println(F("Use: angle <40-180>"));
      return;
    }

    char* endPointer = nullptr;
    // strtol() also reports whether the complete argument was numeric.
    const long requestedAngle =
        strtol(angleText, &endPointer, 10);

    if (*endPointer != '\0')
    {
      Serial.println(F("The angle must be a number."));
      return;
    }

    // An angle command changes to manual mode and starts moving at once.
    operatingMode = OperatingMode::Manual;
    movementPaused = false;
    setTargetAngle(requestedAngle);
  }
  else if (strcmp(commandName, "center") == 0)
  {
    // Centre is a convenient manual command for returning to 90 degrees.
    operatingMode = OperatingMode::Manual;
    movementPaused = false;
    setTargetAngle(CENTER_ANGLE_DEG);
  }
  else if (strcmp(commandName, "speed") == 0)
  {
    // The speed command sets the delay between one-degree updates.
    char* speedText = strtok(nullptr, " ");

    if (speedText == nullptr)
    {
      Serial.println(F("Use: speed <5-100>"));
      return;
    }

    char* endPointer = nullptr;
    const long requestedInterval =
        strtol(speedText, &endPointer, 10);

    if (*endPointer != '\0')
    {
      Serial.println(F("The speed value must be a number."));
      return;
    }

    setUpdateInterval(requestedInterval);
  }
  else if (strcmp(commandName, "pause") == 0)
  {
    // Only the movement state changes; serial input remains active.
    movementPaused = true;
    Serial.println(F("Movement paused."));
  }
  else if (strcmp(commandName, "resume") == 0)
  {
    // Restart timing from the resume command to avoid an immediate old update.
    movementPaused = false;
    previousUpdateTimeMs = millis();
    Serial.println(F("Movement resumed."));
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
  // Convert the command in place until the terminating null is reached.
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
  // Keep the requested position inside the configured servo range.
  const int limitedAngle = constrain(
      requestedAngleDeg,
      static_cast<long>(MIN_ANGLE_DEG),
      static_cast<long>(MAX_ANGLE_DEG)
  );

  // Store the checked value used by the manual movement routine.
  targetAngleDeg = limitedAngle;

  Serial.print(F("Target angle: "));
  Serial.print(targetAngleDeg);
  Serial.println(F(" degrees"));

  if (requestedAngleDeg != limitedAngle)
  {
    Serial.println(F("The requested angle was limited to the safe range."));
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

  // A smaller interval produces faster motion; a larger interval is slower.
  updateIntervalMs = limitedInterval;

  Serial.print(F("Movement update interval: "));
  Serial.print(updateIntervalMs);
  Serial.println(F(" ms"));

  if (requestedIntervalMs !=
      static_cast<long>(limitedInterval))
  {
    Serial.println(F("The requested value was limited to 5-100 ms."));
  }
}

void printStatus()
{
  // Print the main controller variables in a readable block.
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
  Serial.println(movementPaused ? F("PAUSED") : F("RUNNING"));

  Serial.println();
}

void printHelp()
{
  // Keep the command list beside the parser so both are easy to maintain.
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
}

const __FlashStringHelper* modeToText()
{
  // Store text in flash memory and return the label for the active mode.
  return operatingMode == OperatingMode::Automatic
      ? F("AUTOMATIC")
      : F("MANUAL");
}