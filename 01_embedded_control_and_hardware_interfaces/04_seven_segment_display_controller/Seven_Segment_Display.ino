//Non-Blocking Seven-Segment Display Controller

/*
  Project: Non-Blocking Seven-Segment Display Controller
  File: Seven_Segment_Display.ino

  Overview:
  This project implements a configurable controller for a single-digit
  seven-segment display using an Arduino Uno. The display can operate in
  automatic counting mode, manual digit-selection mode, paused mode, or
  blank-display mode.

  The original basic implementation displayed only digits 1 and 2 and used
  delay()-based timing. This version replaces the repeated digit functions
  with a lookup-table-based display driver and uses millis() for non-blocking
  timing. As a result, the Arduino can continue processing serial commands
  and push-button events while the counter is running.

  Main Features:
  - Displays all decimal digits from 0 to 9.
  - Supports common-cathode and common-anode displays.
  - Counts upward or downward automatically.
  - Uses non-blocking timing based on millis().
  - Allows manual digit selection from the Serial Monitor.
  - Supports pause, resume, reset, blanking, and direction control.
  - Allows the counting interval to be changed while the program is running.
  - Includes optional push-button control with software debouncing.
  - Stores segment patterns in program memory to reduce SRAM use.
  - Reports the current operating state through the Serial Monitor.

  Hardware:
  - Arduino Uno or compatible ATmega328P board.
  - One single-digit seven-segment display.
  - Seven current-limiting resistors, typically 220 to 470 ohms.
  - Optional normally-open push button.

  Segment Connections:
  - Segment a -> Arduino digital pin 2
  - Segment b -> Arduino digital pin 3
  - Segment c -> Arduino digital pin 4
  - Segment d -> Arduino digital pin 5
  - Segment e -> Arduino digital pin 6
  - Segment f -> Arduino digital pin 7
  - Segment g -> Arduino digital pin 8

  Optional Button Connection:
  - One terminal of the push button -> Arduino digital pin 9
  - Other terminal of the push button -> GND
  - INPUT_PULLUP is enabled internally, so an external pull-up resistor is
    normally not required.
  - Each valid button press pauses or resumes automatic counting.

  Display Configuration:
  - Set COMMON_ANODE to false for a common-cathode display.
  - Set COMMON_ANODE to true for a common-anode display.
  - The segment-pattern bit order is: a, b, c, d, e, f, g.

  Serial Monitor Configuration:
  - Baud rate: 115200
  - Line ending: Any option is acceptable because CR and LF are ignored.

  Serial Commands:
  - '0' to '9' : Display the selected digit and enter manual mode.
  - 'a'        : Enter automatic counting mode.
  - 'p'        : Pause or resume automatic counting.
  - 'u'        : Select upward counting.
  - 'd'        : Select downward counting.
  - '+'        : Increase counting speed.
  - '-'        : Decrease counting speed.
  - 'r'        : Reset the displayed digit to zero.
  - 'b'        : Blank the display.
  - 's'        : Print the current system status.
  - 'h'        : Print the command help menu.

  Technical Concepts Demonstrated:
  - Modular embedded C++ functions.
  - Enumerations for operating modes and count direction.
  - Constant configuration using constexpr.
  - Lookup-table-based output encoding.
  - Program-memory storage using PROGMEM.
  - Non-blocking scheduling with millis().
  - Software button debouncing.
  - Serial command processing.
  - State management and runtime configuration.
  - Input validation and safe boundary handling.

  Expected Operation:
  After startup, the display begins at digit 0 and automatically counts
  upward once every second. The user can change the operating mode, counting
  direction, speed, or displayed digit without restarting the Arduino.

  Important Notes:
  - Each display segment must use its own current-limiting resistor.
  - Confirm whether the display is common-cathode or common-anode before use.
  - If the displayed segments are inverted, change COMMON_ANODE.
*/

#include <Arduino.h>
#include <avr/pgmspace.h>

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------

constexpr uint8_t SEGMENT_COUNT = 7;
constexpr uint8_t BUTTON_PIN = 9;

// Segment order: a, b, c, d, e, f, g.
constexpr uint8_t SEGMENT_PINS[SEGMENT_COUNT] =
{
  2, 3, 4, 5, 6, 7, 8
};

// Change this value to true when using a common-anode display.
constexpr bool COMMON_ANODE = false;

// -----------------------------------------------------------------------------
// Timing configuration
// -----------------------------------------------------------------------------

constexpr unsigned long DEFAULT_COUNT_INTERVAL_MS = 1000;
constexpr unsigned long MIN_COUNT_INTERVAL_MS = 100;
constexpr unsigned long MAX_COUNT_INTERVAL_MS = 5000;
constexpr unsigned long INTERVAL_STEP_MS = 100;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 40;

// -----------------------------------------------------------------------------
// Seven-segment digit patterns
// -----------------------------------------------------------------------------
//
// Bit order: a b c d e f g
// A binary 1 means that the corresponding segment should be active.
//
// Example for digit 0:
// a, b, c, d, e, and f are active; g is inactive.
// Pattern: 1111110
//
// PROGMEM stores the constant table in flash memory instead of SRAM.

const uint8_t DIGIT_PATTERNS[10] PROGMEM =
{
  0b1111110,  // 0
  0b0110000,  // 1
  0b1101101,  // 2
  0b1111001,  // 3
  0b0110011,  // 4
  0b1011011,  // 5
  0b1011111,  // 6
  0b1110000,  // 7
  0b1111111,  // 8
  0b1111011   // 9
};

// -----------------------------------------------------------------------------
// System state definitions
// -----------------------------------------------------------------------------

enum class DisplayMode : uint8_t
{
  Automatic,
  Manual,
  Blank
};

enum class CountDirection : int8_t
{
  Down = -1,
  Up = 1
};

DisplayMode displayMode = DisplayMode::Automatic;
CountDirection countDirection = CountDirection::Up;

uint8_t currentDigit = 0;
bool automaticCountingPaused = false;

unsigned long countIntervalMs = DEFAULT_COUNT_INTERVAL_MS;
unsigned long previousCountTimeMs = 0;

// Button-debouncing state.
bool previousRawButtonState = HIGH;
bool stableButtonState = HIGH;
unsigned long previousButtonChangeTimeMs = 0;

// -----------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------

void configureHardware();
void displayDigit(uint8_t digit);
void writeSegment(uint8_t segmentIndex, bool active);
void blankDisplay();

void updateAutomaticCounter();
void updateButton();
void handleButtonPress();

void processSerialCommand();
void printHelp();
void printStatus();

void resetCounter();
void increaseCountingSpeed();
void decreaseCountingSpeed();

const __FlashStringHelper* displayModeToText(DisplayMode mode);
const __FlashStringHelper* countDirectionToText(CountDirection direction);

// -----------------------------------------------------------------------------
// Arduino setup and loop
// -----------------------------------------------------------------------------

void setup()
{
  configureHardware();

  Serial.begin(115200);

  displayDigit(currentDigit);
  previousCountTimeMs = millis();

  Serial.println();
  Serial.println(F("Seven-segment display controller started."));
  printHelp();
  printStatus();
}

void loop()
{
  // These tasks run repeatedly without blocking one another.
  processSerialCommand();
  updateButton();
  updateAutomaticCounter();
}

// -----------------------------------------------------------------------------
// Hardware initialization
// -----------------------------------------------------------------------------

void configureHardware()
{
  for (uint8_t segmentIndex = 0;
       segmentIndex < SEGMENT_COUNT;
       ++segmentIndex)
  {
    pinMode(SEGMENT_PINS[segmentIndex], OUTPUT);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  blankDisplay();
}

// -----------------------------------------------------------------------------
// Display driver
// -----------------------------------------------------------------------------

void displayDigit(uint8_t digit)
{
  if (digit > 9)
  {
    blankDisplay();
    return;
  }

  const uint8_t pattern = pgm_read_byte(&DIGIT_PATTERNS[digit]);

  for (uint8_t segmentIndex = 0;
       segmentIndex < SEGMENT_COUNT;
       ++segmentIndex)
  {
    // The leftmost table bit represents segment a.
    const uint8_t bitPosition =
        static_cast<uint8_t>(SEGMENT_COUNT - 1U - segmentIndex);

    const bool segmentActive =
        ((pattern >> bitPosition) & 0x01U) != 0U;

    writeSegment(segmentIndex, segmentActive);
  }

  currentDigit = digit;
}

void writeSegment(uint8_t segmentIndex, bool active)
{
  if (segmentIndex >= SEGMENT_COUNT)
  {
    return;
  }

  // Common-anode displays use inverted electrical logic.
  const uint8_t outputLevel =
      COMMON_ANODE
          ? (active ? LOW : HIGH)
          : (active ? HIGH : LOW);

  digitalWrite(SEGMENT_PINS[segmentIndex], outputLevel);
}

void blankDisplay()
{
  for (uint8_t segmentIndex = 0;
       segmentIndex < SEGMENT_COUNT;
       ++segmentIndex)
  {
    writeSegment(segmentIndex, false);
  }
}

// -----------------------------------------------------------------------------
// Automatic counter
// -----------------------------------------------------------------------------

void updateAutomaticCounter()
{
  if (displayMode != DisplayMode::Automatic ||
      automaticCountingPaused)
  {
    return;
  }

  const unsigned long currentTimeMs = millis();

  // Unsigned subtraction remains safe when millis() overflows.
  if ((currentTimeMs - previousCountTimeMs) < countIntervalMs)
  {
    return;
  }

  previousCountTimeMs = currentTimeMs;

  if (countDirection == CountDirection::Up)
  {
    currentDigit = static_cast<uint8_t>((currentDigit + 1U) % 10U);
  }
  else
  {
    currentDigit =
        (currentDigit == 0U)
            ? 9U
            : static_cast<uint8_t>(currentDigit - 1U);
  }

  displayDigit(currentDigit);
}

// -----------------------------------------------------------------------------
// Push-button processing
// -----------------------------------------------------------------------------

void updateButton()
{
  const bool rawButtonState = digitalRead(BUTTON_PIN);
  const unsigned long currentTimeMs = millis();

  if (rawButtonState != previousRawButtonState)
  {
    previousRawButtonState = rawButtonState;
    previousButtonChangeTimeMs = currentTimeMs;
  }

  if ((currentTimeMs - previousButtonChangeTimeMs) <
      BUTTON_DEBOUNCE_MS)
  {
    return;
  }

  if (rawButtonState != stableButtonState)
  {
    stableButtonState = rawButtonState;

    // INPUT_PULLUP means that a pressed button reads LOW.
    if (stableButtonState == LOW)
    {
      handleButtonPress();
    }
  }
}

void handleButtonPress()
{
  if (displayMode != DisplayMode::Automatic)
  {
    displayMode = DisplayMode::Automatic;
    automaticCountingPaused = false;
    previousCountTimeMs = millis();
    displayDigit(currentDigit);

    Serial.println(F("Button: automatic mode enabled."));
  }
  else
  {
    automaticCountingPaused = !automaticCountingPaused;

    Serial.print(F("Button: automatic counting "));
    Serial.println(
        automaticCountingPaused ? F("paused.") : F("resumed.")
    );
  }

  printStatus();
}

// -----------------------------------------------------------------------------
// Serial command interface
// -----------------------------------------------------------------------------

void processSerialCommand()
{
  if (Serial.available() <= 0)
  {
    return;
  }

  const char command = static_cast<char>(Serial.read());

  // Numeric commands directly select a digit.
  if (command >= '0' && command <= '9')
  {
    displayMode = DisplayMode::Manual;
    automaticCountingPaused = false;

    displayDigit(static_cast<uint8_t>(command - '0'));

    Serial.println(F("Manual digit selected."));
    printStatus();
    return;
  }

  switch (command)
  {
    case 'a':
    case 'A':
      displayMode = DisplayMode::Automatic;
      automaticCountingPaused = false;
      previousCountTimeMs = millis();
      displayDigit(currentDigit);

      Serial.println(F("Automatic counting mode enabled."));
      printStatus();
      break;

    case 'p':
    case 'P':
      if (displayMode != DisplayMode::Automatic)
      {
        displayMode = DisplayMode::Automatic;
        automaticCountingPaused = true;
        displayDigit(currentDigit);
      }
      else
      {
        automaticCountingPaused = !automaticCountingPaused;
      }

      Serial.println(
          automaticCountingPaused
              ? F("Automatic counting paused.")
              : F("Automatic counting resumed.")
      );
      printStatus();
      break;

    case 'u':
    case 'U':
      countDirection = CountDirection::Up;
      Serial.println(F("Count direction set to UP."));
      printStatus();
      break;

    case 'd':
    case 'D':
      countDirection = CountDirection::Down;
      Serial.println(F("Count direction set to DOWN."));
      printStatus();
      break;

    case '+':
      increaseCountingSpeed();
      printStatus();
      break;

    case '-':
      decreaseCountingSpeed();
      printStatus();
      break;

    case 'r':
    case 'R':
      resetCounter();
      Serial.println(F("Counter reset to zero."));
      printStatus();
      break;

    case 'b':
    case 'B':
      displayMode = DisplayMode::Blank;
      automaticCountingPaused = false;
      blankDisplay();

      Serial.println(F("Display blanked."));
      printStatus();
      break;

    case 's':
    case 'S':
      printStatus();
      break;

    case 'h':
    case 'H':
    case '?':
      printHelp();
      break;

    case '\n':
    case '\r':
    case ' ':
      // Ignore line endings and spaces from the Serial Monitor.
      break;

    default:
      Serial.print(F("Unknown command: "));
      Serial.println(command);
      Serial.println(F("Enter 'h' to display the command list."));
      break;
  }
}

// -----------------------------------------------------------------------------
// Runtime configuration functions
// -----------------------------------------------------------------------------

void resetCounter()
{
  currentDigit = 0;
  previousCountTimeMs = millis();

  if (displayMode != DisplayMode::Blank)
  {
    displayDigit(currentDigit);
  }
}

void increaseCountingSpeed()
{
  if (countIntervalMs <=
      (MIN_COUNT_INTERVAL_MS + INTERVAL_STEP_MS))
  {
    countIntervalMs = MIN_COUNT_INTERVAL_MS;
  }
  else
  {
    countIntervalMs -= INTERVAL_STEP_MS;
  }

  Serial.println(F("Counting speed increased."));
}

void decreaseCountingSpeed()
{
  if (countIntervalMs >=
      (MAX_COUNT_INTERVAL_MS - INTERVAL_STEP_MS))
  {
    countIntervalMs = MAX_COUNT_INTERVAL_MS;
  }
  else
  {
    countIntervalMs += INTERVAL_STEP_MS;
  }

  Serial.println(F("Counting speed decreased."));
}

// -----------------------------------------------------------------------------
// User information and diagnostics
// -----------------------------------------------------------------------------

void printHelp()
{
  Serial.println(F("--------------------------------------------"));
  Serial.println(F("Seven-Segment Controller Commands"));
  Serial.println(F("  0-9 : Select a digit manually"));
  Serial.println(F("  a   : Automatic counting mode"));
  Serial.println(F("  p   : Pause or resume automatic counting"));
  Serial.println(F("  u   : Count upward"));
  Serial.println(F("  d   : Count downward"));
  Serial.println(F("  +   : Increase counting speed"));
  Serial.println(F("  -   : Decrease counting speed"));
  Serial.println(F("  r   : Reset the counter to zero"));
  Serial.println(F("  b   : Blank the display"));
  Serial.println(F("  s   : Print current status"));
  Serial.println(F("  h   : Print this help menu"));
  Serial.println(F("--------------------------------------------"));
}

void printStatus()
{
  Serial.println(F("System status"));
  Serial.print(F("  Mode: "));
  Serial.println(displayModeToText(displayMode));

  Serial.print(F("  Digit: "));
  Serial.println(currentDigit);

  Serial.print(F("  Direction: "));
  Serial.println(countDirectionToText(countDirection));

  Serial.print(F("  Automatic counter: "));
  Serial.println(
      automaticCountingPaused ? F("PAUSED") : F("RUNNING")
  );

  Serial.print(F("  Count interval: "));
  Serial.print(countIntervalMs);
  Serial.println(F(" ms"));

  Serial.print(F("  Display type: "));
  Serial.println(
      COMMON_ANODE ? F("COMMON ANODE") : F("COMMON CATHODE")
  );

  Serial.println();
}

const __FlashStringHelper* displayModeToText(DisplayMode mode)
{
  switch (mode)
  {
    case DisplayMode::Automatic:
      return F("AUTOMATIC");

    case DisplayMode::Manual:
      return F("MANUAL");

    case DisplayMode::Blank:
      return F("BLANK");

    default:
      return F("UNKNOWN");
  }
}

const __FlashStringHelper* countDirectionToText(
    CountDirection direction)
{
  return direction == CountDirection::Up
      ? F("UP")
      : F("DOWN");
}
