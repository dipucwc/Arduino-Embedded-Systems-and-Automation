/*
  Project: Non-Blocking Digital Output Controller
  File: Digital_Writing_Code.ino

  Overview:
  This program controls an LED or another low-current digital load connected
  to an Arduino Uno. The controller supports automatic blinking, direct manual
  control, output toggling, and timed pulse generation.

  Timing is handled with millis() instead of delay(). Serial commands therefore
  remain responsive while the output is blinking or while a pulse is active.

  Hardware:
  - Arduino Uno or compatible board
  - LED connected to digital pin 8 through a current-limiting resistor
  - LED cathode connected to Arduino GND

  Serial Monitor:
  - Baud rate: 115200
  - Line ending: Newline or Both NL & CR

  Commands:
  - auto            Start automatic blinking
  - manual          Select manual control
  - on              Set the output HIGH
  - off             Set the output LOW
  - toggle          Invert the current output state
  - pulse 500       Generate a 500 ms HIGH pulse
  - interval 1000   Set the automatic blink interval to 1000 ms
  - status          Print the controller state
  - help            Print the command list

  Technical topics:
  - Digital output control
  - Non-blocking timing
  - Operating-mode management
  - Timed pulse generation
  - Serial command parsing
  - Input validation
  - Fixed-size character buffers
*/

#include <Arduino.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Digital pin connected to the LED or external low-current load.
constexpr uint8_t OUTPUT_PIN = 8;

// Automatic blinking interval limits.
constexpr unsigned long DEFAULT_BLINK_INTERVAL_MS = 500;
constexpr unsigned long MIN_BLINK_INTERVAL_MS = 100;
constexpr unsigned long MAX_BLINK_INTERVAL_MS = 5000;

// Timed pulse duration limits.
constexpr unsigned long MIN_PULSE_DURATION_MS = 50;
constexpr unsigned long MAX_PULSE_DURATION_MS = 10000;

// Maximum command length accepted from the Serial Monitor.
constexpr size_t COMMAND_BUFFER_SIZE = 48;

enum class OperatingMode : uint8_t
{
  Automatic,
  Manual
};

OperatingMode operatingMode = OperatingMode::Automatic;

bool outputState = false;
bool pulseActive = false;
bool discardInput = false;

unsigned long blinkIntervalMs = DEFAULT_BLINK_INTERVAL_MS;
unsigned long previousBlinkTimeMs = 0;

unsigned long pulseStartTimeMs = 0;
unsigned long pulseDurationMs = 0;

char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;

// Output-control functions.
void updateController();
void updateAutomaticMode();
void updatePulse();
void setOutputState(bool newState);

// Serial-input functions.
void readSerialInput();
void processCommand(char* command);
void convertToLowercase(char* text);

// Parameter-setting functions.
void setBlinkInterval(long requestedIntervalMs);
void startPulse(long requestedDurationMs);

// User-information functions.
void printStatus();
void printHelp();
const __FlashStringHelper* modeToText();
const __FlashStringHelper* outputStateToText();

void setup()
{
  // Configure the selected Arduino pin as a digital output.
  pinMode(OUTPUT_PIN, OUTPUT);

  // Start with the output switched off.
  setOutputState(false);

  // Open the Serial Monitor connection.
  Serial.begin(115200);

  // Start the automatic timing reference from the current system time.
  previousBlinkTimeMs = millis();

  Serial.println();
  Serial.println(F("Digital output controller started."));
  Serial.println(F("Enter 'help' to show the available commands."));
  printStatus();
}

void loop()
{
  // Check for new Serial Monitor commands on every loop cycle.
  readSerialInput();

  // Update blinking or pulse timing without blocking the program.
  updateController();
}

void updateController()
{
  // A timed pulse has priority over automatic blinking.
  if (pulseActive)
  {
    updatePulse();
    return;
  }

  if (operatingMode == OperatingMode::Automatic)
  {
    updateAutomaticMode();
  }
}

void updateAutomaticMode()
{
  const unsigned long currentTimeMs = millis();

  // Unsigned subtraction continues to work correctly after millis() overflows.
  if ((currentTimeMs - previousBlinkTimeMs) < blinkIntervalMs)
  {
    return;
  }

  previousBlinkTimeMs = currentTimeMs;

  // Invert the output whenever the selected interval has elapsed.
  setOutputState(!outputState);
}

void updatePulse()
{
  const unsigned long currentTimeMs = millis();

  // Keep the output HIGH until the requested pulse duration has elapsed.
  if ((currentTimeMs - pulseStartTimeMs) < pulseDurationMs)
  {
    return;
  }

  pulseActive = false;
  setOutputState(false);

  Serial.println(F("Pulse completed."));
  printStatus();
}

void setOutputState(bool newState)
{
  outputState = newState;

  // Convert the stored Boolean state into an Arduino logic level.
  digitalWrite(OUTPUT_PIN, outputState ? HIGH : LOW);
}

void readSerialInput()
{
  while (Serial.available() > 0)
  {
    const char receivedCharacter =
        static_cast<char>(Serial.read());

    // A line ending marks the end of the current command.
    if (receivedCharacter == '\n' ||
        receivedCharacter == '\r')
    {
      if (discardInput)
      {
        // Resume normal input collection after the oversized command ends.
        discardInput = false;
        commandLength = 0;
        continue;
      }

      if (commandLength > 0)
      {
        // Add the null terminator required by C string functions.
        commandBuffer[commandLength] = '\0';

        processCommand(commandBuffer);
        commandLength = 0;
      }

      continue;
    }

    if (discardInput)
    {
      // Ignore the remaining characters of an oversized command.
      continue;
    }

    // Keep one byte free for the terminating null character.
    if (commandLength < COMMAND_BUFFER_SIZE - 1)
    {
      commandBuffer[commandLength++] = receivedCharacter;
    }
    else
    {
      commandLength = 0;
      discardInput = true;

      Serial.println(F("Command is too long and was discarded."));
    }
  }
}

void processCommand(char* command)
{
  // Accept commands written with uppercase or lowercase letters.
  convertToLowercase(command);

  // Separate the command name from an optional numeric argument.
  char* commandName = strtok(command, " ");

  if (commandName == nullptr)
  {
    return;
  }

  if (strcmp(commandName, "auto") == 0)
  {
    operatingMode = OperatingMode::Automatic;
    pulseActive = false;
    previousBlinkTimeMs = millis();

    Serial.println(F("Automatic blinking selected."));
    printStatus();
  }
  else if (strcmp(commandName, "manual") == 0)
  {
    operatingMode = OperatingMode::Manual;
    pulseActive = false;

    Serial.println(F("Manual control selected."));
    printStatus();
  }
  else if (strcmp(commandName, "on") == 0)
  {
    operatingMode = OperatingMode::Manual;
    pulseActive = false;
    setOutputState(true);

    Serial.println(F("Output switched ON."));
    printStatus();
  }
  else if (strcmp(commandName, "off") == 0)
  {
    operatingMode = OperatingMode::Manual;
    pulseActive = false;
    setOutputState(false);

    Serial.println(F("Output switched OFF."));
    printStatus();
  }
  else if (strcmp(commandName, "toggle") == 0)
  {
    operatingMode = OperatingMode::Manual;
    pulseActive = false;
    setOutputState(!outputState);

    Serial.println(F("Output toggled."));
    printStatus();
  }
  else if (strcmp(commandName, "pulse") == 0)
  {
    char* durationText = strtok(nullptr, " ");

    if (durationText == nullptr)
    {
      Serial.println(F("Use: pulse <50-10000>"));
      return;
    }

    char* endPointer = nullptr;
    const long requestedDuration =
        strtol(durationText, &endPointer, 10);

    if (*endPointer != '\0')
    {
      Serial.println(F("The pulse duration must be a number."));
      return;
    }

    startPulse(requestedDuration);
    printStatus();
  }
  else if (strcmp(commandName, "interval") == 0)
  {
    char* intervalText = strtok(nullptr, " ");

    if (intervalText == nullptr)
    {
      Serial.println(F("Use: interval <100-5000>"));
      return;
    }

    char* endPointer = nullptr;
    const long requestedInterval =
        strtol(intervalText, &endPointer, 10);

    if (*endPointer != '\0')
    {
      Serial.println(F("The interval must be a number."));
      return;
    }

    setBlinkInterval(requestedInterval);
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

void setBlinkInterval(long requestedIntervalMs)
{
  // Limit the interval to a useful and safe operating range.
  const unsigned long limitedInterval =
      static_cast<unsigned long>(
          constrain(
              requestedIntervalMs,
              static_cast<long>(MIN_BLINK_INTERVAL_MS),
              static_cast<long>(MAX_BLINK_INTERVAL_MS)
          )
      );

  blinkIntervalMs = limitedInterval;
  previousBlinkTimeMs = millis();

  Serial.print(F("Blink interval: "));
  Serial.print(blinkIntervalMs);
  Serial.println(F(" ms"));

  if (requestedIntervalMs !=
      static_cast<long>(limitedInterval))
  {
    Serial.println(
        F("The requested interval was limited to 100-5000 ms.")
    );
  }
}

void startPulse(long requestedDurationMs)
{
  // Limit the pulse duration before starting the timed output.
  const unsigned long limitedDuration =
      static_cast<unsigned long>(
          constrain(
              requestedDurationMs,
              static_cast<long>(MIN_PULSE_DURATION_MS),
              static_cast<long>(MAX_PULSE_DURATION_MS)
          )
      );

  operatingMode = OperatingMode::Manual;
  pulseDurationMs = limitedDuration;
  pulseStartTimeMs = millis();
  pulseActive = true;

  setOutputState(true);

  Serial.print(F("Pulse started for "));
  Serial.print(pulseDurationMs);
  Serial.println(F(" ms"));

  if (requestedDurationMs !=
      static_cast<long>(limitedDuration))
  {
    Serial.println(
        F("The requested duration was limited to 50-10000 ms.")
    );
  }
}

void printStatus()
{
  Serial.println(F("Digital output controller status"));

  Serial.print(F("  Mode: "));
  Serial.println(modeToText());

  Serial.print(F("  Output pin: "));
  Serial.println(OUTPUT_PIN);

  Serial.print(F("  Output state: "));
  Serial.println(outputStateToText());

  Serial.print(F("  Blink interval: "));
  Serial.print(blinkIntervalMs);
  Serial.println(F(" ms"));

  Serial.print(F("  Pulse: "));
  Serial.println(pulseActive ? F("ACTIVE") : F("INACTIVE"));

  if (pulseActive)
  {
    Serial.print(F("  Pulse duration: "));
    Serial.print(pulseDurationMs);
    Serial.println(F(" ms"));
  }

  Serial.println();
}

void printHelp()
{
  Serial.println(F("Commands"));
  Serial.println(F("  auto            Start automatic blinking"));
  Serial.println(F("  manual          Select manual control"));
  Serial.println(F("  on              Set output HIGH"));
  Serial.println(F("  off             Set output LOW"));
  Serial.println(F("  toggle          Invert output state"));
  Serial.println(F("  pulse 500       Generate a timed HIGH pulse"));
  Serial.println(F("  interval 1000   Set automatic blink interval"));
  Serial.println(F("  status          Print controller status"));
  Serial.println(F("  help            Print this command list"));
  Serial.println();
}

const __FlashStringHelper* modeToText()
{
  return operatingMode == OperatingMode::Automatic
      ? F("AUTOMATIC")
      : F("MANUAL");
}

const __FlashStringHelper* outputStateToText()
{
  return outputState ? F("HIGH / ON") : F("LOW / OFF");
}
