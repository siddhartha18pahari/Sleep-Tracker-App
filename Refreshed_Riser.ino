#include <Wire.h>
#include <rgb_lcd.h>
#include <TimeLib.h>

rgb_lcd lcd;

const int touchpadPin = 2;
const int buttonPin   = 3;

// Set true if your inputs are wired to GND when pressed (recommended wiring).
// If true: pinMode = INPUT_PULLUP and "pressed" reads LOW.
const bool USE_PULLUPS = false;

// -------------------- Timing --------------------
const unsigned long DEBOUNCE_MS     = 60;
const unsigned long WELCOME_MS      = 5000;
const unsigned long LCD_REFRESH_MS  = 200;
const unsigned long TIME_TICK_MS    = 1000;

// -------------------- State Machine --------------------
enum State {
  ST_WELCOME,
  ST_SET_USEC,
  ST_SET_TSEC,
  ST_SET_UMIN,
  ST_SET_TMIN,
  ST_SET_UHRS,
  ST_SET_THRS,
  ST_RUN
};

State state = ST_WELCOME;

// digits
uint8_t usec = 0, tsec = 0, umin = 0, tmin = 0, uhrs = 0, thrs = 0;

// run mode toggle
bool sleepMode = false;

// capture sleep/wake times
time_t timeValues[2] = {0, 0}; // [0]=bed, [1]=wake
uint8_t captureIndex = 0;

// -------------------- Debounced inputs --------------------
struct DebouncedInput {
  bool stable = false;
  bool lastStable = false;
  bool raw = false;
  unsigned long lastFlip = 0;

  void update(bool newRaw, unsigned long nowMs) {
    raw = newRaw;
    if (raw != stable) {
      if (nowMs - lastFlip >= DEBOUNCE_MS) {
        lastStable = stable;
        stable = raw;
        lastFlip = nowMs;
      }
    } else {
      lastStable = stable;
    }
  }

  bool rose() const { return (!lastStable && stable); }
};

DebouncedInput btn, touch;

// -------------------- Time / LCD throttling --------------------
unsigned long tWelcomeStart = 0;
unsigned long tLastLCD = 0;
unsigned long tLastTick = 0;

int lastH = -1, lastM = -1, lastS = -1;
State lastStatePrinted = (State)255;
bool lastSleepPrinted = false;

// -------------------- Helpers --------------------
bool readPressed(int pin) {
  int v = digitalRead(pin);
  if (USE_PULLUPS) return (v == LOW);
  return (v == HIGH);
}

void setBacklightForState(State s) {
  switch (s) {
    case ST_SET_USEC: lcd.setRGB(255, 0, 0); break;       // red
    case ST_SET_TSEC: lcd.setRGB(0, 255, 0); break;       // lime
    case ST_SET_UMIN: lcd.setRGB(0, 0, 255); break;       // blue
    case ST_SET_TMIN: lcd.setRGB(255, 165, 0); break;     // orange
    case ST_SET_UHRS: lcd.setRGB(0, 128, 0); break;       // green
    case ST_SET_THRS: lcd.setRGB(255, 255, 255); break;   // white
    case ST_RUN:
      if (sleepMode) lcd.setRGB(48, 25, 52);              // dark purple
      else lcd.setRGB(255, 255, 255);
      break;
    default:
      lcd.setRGB(255, 255, 255);
      break;
  }
}

void printTwoDigits(int x) {
  if (x < 10) lcd.print('0');
  lcd.print(x);
}

const char* stateLabel(State s) {
  switch (s) {
    case ST_SET_USEC: return "Set sec (1s)";
    case ST_SET_TSEC: return "Set sec (10s)";
    case ST_SET_UMIN: return "Set min (1m)";
    case ST_SET_TMIN: return "Set min (10m)";
    case ST_SET_UHRS: return "Set hr (1h)";
    case ST_SET_THRS: return "Set hr (10h)";
    default: return "";
  }
}

void printSetTimeRow() {
  int hrs  = thrs * 10 + uhrs;
  int mins = tmin * 10 + umin;
  int secs = tsec * 10 + usec;

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  printTwoDigits(hrs);
  lcd.print(':');
  printTwoDigits(mins);
  lcd.print(':');
  printTwoDigits(secs);
  lcd.print("   ");
}

void printSetHintRow() {
  lcd.setCursor(0, 1);
  lcd.print(stateLabel(state));
  lcd.print("   ");
}

void applySetTimeToClock() {
  int hrs  = thrs * 10 + uhrs;
  int mins = tmin * 10 + umin;
  int secs = tsec * 10 + usec;

  // Optional 24h clamp: prevent 24..29
  if (hrs > 23) hrs = 23;

  setTime(hrs, mins, secs, 1, 1, 2026);
}

void incrementCurrentField() {
  switch (state) {
    case ST_SET_USEC:
      usec = (usec + 1) % 10;
      break;
    case ST_SET_TSEC:
      tsec = (tsec + 1) % 6;     // 0..5
      break;
    case ST_SET_UMIN:
      umin = (umin + 1) % 10;
      break;
    case ST_SET_TMIN:
      tmin = (tmin + 1) % 6;     // 0..5
      break;
    case ST_SET_UHRS:
      uhrs = (uhrs + 1) % 10;
      // keep hours valid if tens already 2
      if (thrs == 2 && uhrs > 3) uhrs = 0;
      break;
    case ST_SET_THRS:
      thrs = (thrs + 1) % 3;     // 0..2
      if (thrs == 2 && uhrs > 3) uhrs = 0;
      break;
    default:
      break;
  }
}

void nextState() {
  switch (state) {
    case ST_SET_USEC: state = ST_SET_TSEC; break;
    case ST_SET_TSEC: state = ST_SET_UMIN; break;
    case ST_SET_UMIN: state = ST_SET_TMIN; break;
    case ST_SET_TMIN: state = ST_SET_UHRS; break;
    case ST_SET_UHRS: state = ST_SET_THRS; break;
    case ST_SET_THRS:
      applySetTimeToClock();
      state = ST_RUN;
      break;
    default:
      break;
  }
  setBacklightForState(state);
}

void printRunRow(int h, int m, int s) {
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  printTwoDigits(h);
  lcd.print(':');
  printTwoDigits(m);
  lcd.print(':');
  printTwoDigits(s);
  lcd.print("   ");
}

void printRunModeRow() {
  lcd.setCursor(0, 1);
  if (sleepMode) lcd.print("Good Night!     ");
  else lcd.print("Hello!          ");
}

void captureTimeStamp() {
  time_t currentTime = now();
  timeValues[captureIndex] = currentTime;

  int h = hour(currentTime);
  int m = minute(currentTime);
  int s = second(currentTime);

  Serial.print(captureIndex == 0 ? "Bed: " : "Wake: ");
  if (h < 10) Serial.print('0'); Serial.print(h); Serial.print(':');
  if (m < 10) Serial.print('0'); Serial.print(m); Serial.print(':');
  if (s < 10) Serial.print('0'); Serial.println(s);

  captureIndex = (captureIndex + 1) % 2;

  // Example JSON once both captured (after Wake logged -> captureIndex wraps to 0)
  if (captureIndex == 0) {
    Serial.print("{\"bed\":");
    Serial.print((unsigned long)timeValues[0]);
    Serial.print(",\"wake\":");
    Serial.print((unsigned long)timeValues[1]);
    Serial.println("}");
  }
}

// -------------------- Arduino setup/loop --------------------
void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setRGB(255, 255, 255);

  if (USE_PULLUPS) {
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(touchpadPin, INPUT_PULLUP);
  } else {
    pinMode(buttonPin, INPUT);
    pinMode(touchpadPin, INPUT);
  }

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Welcome");
  lcd.setCursor(2, 1);
  lcd.print("to WellRise!");

  tWelcomeStart = millis();
  state = ST_WELCOME;
}

void loop() {
  unsigned long nowMs = millis();

  // Update debounced inputs
  btn.update(readPressed(buttonPin), nowMs);
  touch.update(readPressed(touchpadPin), nowMs);

  // Welcome -> setting
  if (state == ST_WELCOME) {
    if (nowMs - tWelcomeStart >= WELCOME_MS) {
      lcd.clear();
      state = ST_SET_USEC;
      setBacklightForState(state);
      lastStatePrinted = (State)255;
    }
    return;
  }

  // Setting states
  if (state >= ST_SET_USEC && state <= ST_SET_THRS) {
    if (touch.rose()) incrementCurrentField();
    if (btn.rose()) {
      nextState();
      lastStatePrinted = (State)255;
    }

    if ((nowMs - tLastLCD >= LCD_REFRESH_MS) || (lastStatePrinted != state)) {
      tLastLCD = nowMs;
      lastStatePrinted = state;
      printSetTimeRow();
      printSetHintRow();
    }
    return;
  }

  // Run state
  if (state == ST_RUN) {
    if (nowMs - tLastTick >= TIME_TICK_MS) {
      tLastTick = nowMs;

      int h = hour();
      int m = minute();
      int s = second();

      if (h != lastH || m != lastM || s != lastS) {
        lastH = h; lastM = m; lastS = s;
        printRunRow(h, m, s);
      }

      if (sleepMode != lastSleepPrinted) {
        lastSleepPrinted = sleepMode;
        setBacklightForState(ST_RUN);
        printRunModeRow();
      }
    }

    if (touch.rose()) {
      sleepMode = !sleepMode;
      setBacklightForState(ST_RUN);
      printRunModeRow();
      captureTimeStamp();
    }
  }
}






