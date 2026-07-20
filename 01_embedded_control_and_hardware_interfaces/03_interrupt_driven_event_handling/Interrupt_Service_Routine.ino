/*
  Project: Interrupt-Driven Event Controller
  File: Interrupt_Service_Routine.ino

  Overview:
  This program uses an external hardware interrupt to detect a push-button
  event on Arduino Uno digital pin 2. Each accepted button press toggles an
  LED connected to digital pin 8 and increments an event counter.

  The interrupt service routine performs only the minimum required work:
  it records that an interrupt occurred and returns immediately. Button
  debouncing, LED control, counters, and Serial Monitor output are handled
  safely in the main program.

  Hardware:
  - Arduino Uno or compatible board
  - Push button connected between digital pin 2 and GND
  - Internal pull-up resistor enabled on digital pin 2
  - LED connected to digital pin 8 through a 220 ohm or 330 ohm resistor
  - LED cathode connected to Arduino GND

  Serial Monitor:
  - Baud rate: 115200
  - Line ending: Newline or Both NL & CR

  Commands:
  - status          Print the controller state
  - enable          Enable the external interrupt
  - disable         Disable the external interrupt
  - reset           Reset event counters and switch the LED off
  - led on          Switch the LED on
  - led off         Switch the LED off
  - led toggle      Toggle the LED state
  - debounce 50     Set the debounce interval in milliseconds
  - help            Print the command list

  Technical topics:
  - External hardware interrupts
  - Interrupt service routines
  - Volatile shared variables
  - Atomic access to multi-byte data
  - Software debouncing
  - Event-driven processing
  - Fixed-size serial command buffer
  - Input validation
*/

#include <Arduino.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

constexpr uint8_t BUTTON_PIN = 2;
constexpr uint8_t LED_PIN = 8;

constexpr unsigned long DEFAULT_DEBOUNCE_MS = 50;
constexpr unsigned long MIN_DEBOUNCE_MS = 5;
constexpr unsigned long MAX_DEBOUNCE_MS = 1000;

constexpr size_t COMMAND_BUFFER_SIZE = 48;

volatile bool interruptPending = false;
volatile uint32_t rawInterruptCount = 0;

bool interruptEnabled = true;
bool ledState = false;
bool discardInput = false;

uint32_t acceptedEventCount = 0;
uint32_t rejectedBounceCount = 0;

unsigned long debounceIntervalMs = DEFAULT_DEBOUNCE_MS;
unsigned long lastAcceptedEventTimeMs = 0;

char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;

void buttonInterruptServiceRoutine();
void processPendingInterrupt();
bool takeInterruptPendingFlag();
uint32_t readRawInterruptCount();

void enableButtonInterrupt();
void disableButtonInterrupt();
void setLedState(bool newState);

void readSerialInput();
void processCommand(char* command);
void convertToLowercase(char* text);

void setDebounceInterval(long requestedIntervalMs);
void resetController();
void printStatus();
void printHelp();
const __FlashStringHelper* ledStateToText();

void setup()
{
  // Use the Arduino internal pull-up resistor for the push button.
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configure the LED output and begin with the LED switched off.
  pinMode(LED_PIN, OUTPUT);
  setLedState(false);

  // Open the Serial Monitor connection.
  Serial.begin(115200);

  // Allow the first button press immediately after startup.
  lastAcceptedEventTimeMs = millis() - debounceIntervalMs;

  // Attach the external interrupt after initialization is complete.
  enableButtonInterrupt();

  Serial.println();
  Serial.println(F("Interrupt-driven event controller started."));
  Serial.println(F("Press the button or enter 'help' for commands."));
  printStatus();
}

void loop()
{
  // Process button events outside the interrupt service routine.
  processPendingInterrupt();

  // Keep Serial Monitor commands responsive.
  readSerialInput();
}

void buttonInterruptServiceRoutine()
{
  // Keep the ISR short: record the event and return.
  ++rawInterruptCount;
  interruptPending = true;
}

void processPendingInterrupt()
{
  // Copy and clear the shared event flag atomically.
  if (!takeInterruptPendingFlag())
  {
    return;
  }

  const unsigned long currentTimeMs = millis();

  // Reject transitions that occur inside the debounce interval.
  if ((currentTimeMs - lastAcceptedEventTimeMs) < debounceIntervalMs)
  {
    ++rejectedBounceCount;
    return;
  }

  lastAcceptedEventTimeMs = currentTimeMs;
  ++acceptedEventCount;

  // Toggle the LED after each accepted button event.
  setLedState(!ledState);

  Serial.print(F("Accepted button event "));
  Serial.print(acceptedEventCount);
  Serial.print(F(": LED "));
  Serial.println(ledStateToText());
}

bool takeInterruptPendingFlag()
{
  bool pending;

  // Prevent the ISR from changing the flag during the copy.
  noInterrupts();
  pending = interruptPending;
  interruptPending = false;
  interrupts();

  return pending;
}

uint32_t readRawInterruptCount()
{
  uint32_t countSnapshot;

  // Read the 32-bit ISR counter atomically on the 8-bit Arduino Uno.
  noInterrupts();
  countSnapshot = rawInterruptCount;
  interrupts();

  return countSnapshot;
}

void enableButtonInterrupt()
{
  // Remove any earlier attachment before enabling the interrupt again.
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));

  // INPUT_PULLUP keeps the pin HIGH until the button connects it to GND.
  attachInterrupt(
      digitalPinToInterrupt(BUTTON_PIN),
      buttonInterruptServiceRoutine,
      FALLING
  );

  interruptEnabled = true;
}

void disableButtonInterrupt()
{
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));
  interruptEnabled = false;

  // Clear an event that may have been recorded just before detaching.
  noInterrupts();
  interruptPending = false;
  interrupts();
}

void setLedState(bool newState)
{
  ledState = newState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
}

void readSerialInput()
{
  while (Serial.available() > 0)
  {
    const char receivedCharacter =
        static_cast<char>(Serial.read());

    // A newline or carriage return completes one command.
    if (receivedCharacter == '\n' ||
        receivedCharacter == '\r')
    {
      if (discardInput)
      {
        discardInput = false;
        commandLength = 0;
        continue;
      }

      if (commandLength > 0)
      {
        commandBuffer[commandLength] = '\0';
        processCommand(commandBuffer);
        commandLength = 0;
      }

      continue;
    }

    if (discardInput)
    {
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
  // Accept uppercase and lowercase command input.
  convertToLowercase(command);

  char* commandName = strtok(command, " ");

  if (commandName == nullptr)
  {
    return;
  }

  if (strcmp(commandName, "status") == 0)
  {
    printStatus();
  }
  else if (strcmp(commandName, "enable") == 0)
  {
    enableButtonInterrupt();
    Serial.println(F("Button interrupt enabled."));
    printStatus();
  }
  else if (strcmp(commandName, "disable") == 0)
  {
    disableButtonInterrupt();
    Serial.println(F("Button interrupt disabled."));
    printStatus();
  }
  else if (strcmp(commandName, "reset") == 0)
  {
    resetController();
    Serial.println(F("Controller counters reset."));
    printStatus();
  }
  else if (strcmp(commandName, "led") == 0)
  {
    char* ledCommand = strtok(nullptr, " ");

    if (ledCommand == nullptr)
    {
      Serial.println(F("Use: led <on|off|toggle>"));
      return;
    }

    if (strcmp(ledCommand, "on") == 0)
    {
      setLedState(true);
      Serial.println(F("LED switched ON."));
    }
    else if (strcmp(ledCommand, "off") == 0)
    {
      setLedState(false);
      Serial.println(F("LED switched OFF."));
    }
    else if (strcmp(ledCommand, "toggle") == 0)
    {
      setLedState(!ledState);
      Serial.println(F("LED state toggled."));
    }
    else
    {
      Serial.println(F("Use: led <on|off|toggle>"));
      return;
    }

    printStatus();
  }
  else if (strcmp(commandName, "debounce") == 0)
  {
    char* intervalText = strtok(nullptr, " ");

    if (intervalText == nullptr)
    {
      Serial.println(F("Use: debounce <5-1000>"));
      return;
    }

    char* endPointer = nullptr;
    const long requestedInterval =
        strtol(intervalText, &endPointer, 10);

    if (*endPointer != '\0')
    {
      Serial.println(F("The debounce interval must be a number."));
      return;
    }

    setDebounceInterval(requestedInterval);
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
  while (*text != '\0')
  {
    *text = static_cast<char>(
        tolower(static_cast<unsigned char>(*text))
    );

    ++text;
  }
}

void setDebounceInterval(long requestedIntervalMs)
{
  const unsigned long limitedInterval =
      static_cast<unsigned long>(
          constrain(
              requestedIntervalMs,
              static_cast<long>(MIN_DEBOUNCE_MS),
              static_cast<long>(MAX_DEBOUNCE_MS)
          )
      );

  debounceIntervalMs = limitedInterval;

  Serial.print(F("Debounce interval: "));
  Serial.print(debounceIntervalMs);
  Serial.println(F(" ms"));

  if (requestedIntervalMs !=
      static_cast<long>(limitedInterval))
  {
    Serial.println(
        F("The requested interval was limited to 5-1000 ms.")
    );
  }
}

void resetController()
{
  // Reset ISR-shared data atomically.
  noInterrupts();
  rawInterruptCount = 0;
  interruptPending = false;
  interrupts();

  acceptedEventCount = 0;
  rejectedBounceCount = 0;
  lastAcceptedEventTimeMs = millis() - debounceIntervalMs;

  setLedState(false);
}

void printStatus()
{
  Serial.println(F("Interrupt-driven event controller status"));

  Serial.print(F("  Interrupt: "));
  Serial.println(interruptEnabled ? F("ENABLED") : F("DISABLED"));

  Serial.print(F("  Button pin: "));
  Serial.println(BUTTON_PIN);

  Serial.print(F("  LED pin: "));
  Serial.println(LED_PIN);

  Serial.print(F("  LED state: "));
  Serial.println(ledStateToText());

  Serial.print(F("  Accepted events: "));
  Serial.println(acceptedEventCount);

  Serial.print(F("  Raw ISR count: "));
  Serial.println(readRawInterruptCount());

  Serial.print(F("  Rejected bounce events: "));
  Serial.println(rejectedBounceCount);

  Serial.print(F("  Debounce interval: "));
  Serial.print(debounceIntervalMs);
  Serial.println(F(" ms"));

  Serial.println();
}

void printHelp()
{
  Serial.println(F("Commands"));
  Serial.println(F("  status          Print controller status"));
  Serial.println(F("  enable          Enable the button interrupt"));
  Serial.println(F("  disable         Disable the button interrupt"));
  Serial.println(F("  reset           Reset counters and LED state"));
  Serial.println(F("  led on          Switch the LED on"));
  Serial.println(F("  led off         Switch the LED off"));
  Serial.println(F("  led toggle      Toggle the LED state"));
  Serial.println(F("  debounce 50     Set debounce interval"));
  Serial.println(F("  help            Print this command list"));
  Serial.println();
}

const __FlashStringHelper* ledStateToText()
{
  return ledState ? F("ON") : F("OFF");
}
