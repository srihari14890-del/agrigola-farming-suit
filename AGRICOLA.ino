#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET
);

// Sensors
const int SOIL_PIN = A0;
const int HEART_PIN = A1;
const int TOUCH_PIN = 2;

// L298N pump driver
const int ENA = 6;
const int IN1 = 7;
const int IN2 = 8;

// Soil threshold: calibrate for your sensor
const int DRY_THRESHOLD = 600;

// Touch detection
const unsigned long DEBOUNCE_MS = 40;
const unsigned long TOUCH_WINDOW_MS = 800;

int lastRawTouch = LOW;
int stableTouch = LOW;

unsigned long rawChangeTime = 0;
unsigned long lastTouchTime = 0;
unsigned long lastDisplayTime = 0;

byte touchCount = 0;
bool waitingForTouches = false;

// Pump state
bool pumpIsRunning = false;

// Turn pump ON
void pumpOn() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 255);
  pumpIsRunning = true;
}

// Turn pump OFF
void pumpOff() {
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  pumpIsRunning = false;
}

// Handle triple-touch
void handleTouches(byte count) {
  if (count == 3) {
    if (pumpIsRunning) {
      pumpOff();
      Serial.println("Triple touch: Pump OFF");
    } else {
      pumpOn();
      Serial.println("Triple touch: Pump ON");
    }
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Pump starts OFF
  pumpOff();

  // Start I2C
  Wire.begin();

  // Start OLED
  if (!display.begin(
        SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not detected!");
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("AGRICOLA");
  display.println("System Ready");
  display.display();

  delay(1000);
}

void loop() {
  unsigned long now = millis();

  // Read sensors
  int soilValue = analogRead(SOIL_PIN);
  int heartValue = analogRead(HEART_PIN);

  // Read touch sensor
  int rawTouch = digitalRead(TOUCH_PIN);

  if (rawTouch != lastRawTouch) {
    lastRawTouch = rawTouch;
    rawChangeTime = now;
  }

  // Debounce touch input
  if (now - rawChangeTime >= DEBOUNCE_MS &&
      stableTouch != rawTouch) {

    stableTouch = rawTouch;

    // Count a new touch when signal goes HIGH
    if (stableTouch == HIGH) {
      if (!waitingForTouches ||
          now - lastTouchTime > TOUCH_WINDOW_MS) {

        touchCount = 1;
        waitingForTouches = true;

      } else {
        touchCount++;
      }

      lastTouchTime = now;
    }
  }

  // Finish counting after the touch sequence
  if (waitingForTouches &&
      now - lastTouchTime > TOUCH_WINDOW_MS) {

    handleTouches(touchCount);

    touchCount = 0;
    waitingForTouches = false;
  }

  // Update OLED every 500 ms
  if (now - lastDisplayTime >= 500) {
    lastDisplayTime = now;

    display.clearDisplay();
    display.setCursor(0, 0);

    display.println("AGRICOLA");
    display.println("----------------");

    display.print("Soil raw: ");
    display.println(soilValue);

    display.print("Soil: ");
    display.println(
      soilValue > DRY_THRESHOLD ? "DRY" : "WET"
    );

    display.print("Heart raw: ");
    display.println(heartValue);

    display.display();

    // Serial Monitor readings
    Serial.print("Soil: ");
    Serial.print(soilValue);
    Serial.print(" | Heart raw: ");
    Serial.println(heartValue);
  }
}
