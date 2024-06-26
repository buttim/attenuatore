#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <MD_KeySwitch.h>
#include <LowPower.h>

//FUSES: L=E2 H=DA E=0E

#include <Encoder.h>
const int ENC_A = 3, ENC_B = 4, BUTTON = 2, MAX_VCC = 4200, MIN_VCC = 2600;
MD_KeySwitch button(BUTTON, LOW);
Encoder encoder(ENC_A, ENC_B);

const int V1 = 5, V2 = 6, V3 = 7, V4 = 8, V5 = 9, V6 = 10;

uint16_t vcc;
int val = 0, oldVal = 64;
Adafruit_SSD1306 disp;
uint64_t tLastCheck = 0, tLastInput = 0;

//From https://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
//Secret Arduino Voltmeter – Measure Battery Voltage by Provide Your Own is licensed
//under a Creative Commons Attribution-ShareAlike 4.0 International License.
uint16_t readVcc() {
// Read 1.1V reference against AVcc
// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2);             // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);  // Start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;  // measuring

  uint8_t low = ADCL;   // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH;  // unlocks bothstop

  long result = (high << 8) | low;

  result = 1387069L / result;  // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return (uint16_t)result;     // Vcc in millivolts
}

void show(const char *s, const GFXfont *f = &FreeSansBold12pt7b) {
  int16_t x1, y1;
  uint16_t w, h;

  disp.setFont(f);
  disp.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  disp.clearDisplay();
  disp.setCursor((128 - w) / 2, 20);
  disp.print(s);
  disp.display();
}

void showOff() {
  show("OFF");
}

void updateDisplay(uint8_t val, uint16_t vcc) {
  int16_t x1, y1;
  uint16_t w, h;
  char s[10];

  dtostrf(val / 2.0, 3, 1, s);
  disp.setFont(&FreeSansBold12pt7b);
  disp.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  disp.clearDisplay();
  disp.setCursor((128 - w) / 2, 18);
  disp.print(s);

  w = constrain(map(vcc, MIN_VCC, MAX_VCC, 0, 128), 0, 128);
  disp.drawRect(0, 24, 128, 8, WHITE);
  disp.fillRect(0, 24, w, 8, WHITE);

  disp.display();
}

void updatePins(uint8_t val) {
  digitalWrite(V1, val & 0x01 ? HIGH : LOW);
  digitalWrite(V2, val & 0x02 ? HIGH : LOW);
  digitalWrite(V3, val & 0x04 ? HIGH : LOW);
  digitalWrite(V4, val & 0x08 ? HIGH : LOW);
  digitalWrite(V5, val & 0x10 ? HIGH : LOW);
  digitalWrite(V6, val & 0x20 ? HIGH : LOW);
}

void checkBattery() {
  if ((vcc = readVcc()) < MIN_VCC) {
    powerOff();
    while (true)
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}

void powerOff() {
  showOff();
  while (digitalRead(BUTTON) == LOW)
    ;
  disp.clearDisplay();
  disp.display();
  disp.ssd1306_command(SSD1306_DISPLAYOFF);
  attachInterrupt(
    digitalPinToInterrupt(BUTTON), []() {}, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(digitalPinToInterrupt(BUTTON));
  checkBattery();
  tLastInput = 0;
  initDisplay();
  updateDisplay(val, vcc);
  delay(300);
  while (digitalRead(BUTTON) == LOW)
    ;
}

void initDisplay() {
  disp.begin();
  disp.setRotation(2);
  disp.clearDisplay();
  disp.setFont(&FreeSansBold12pt7b);
  disp.setTextSize(1);
  disp.setTextColor(WHITE);
}

void setup() {
  checkBattery();
  Serial.begin(115200);

  initDisplay();

  pinMode(BUTTON, INPUT_PULLUP);
  button.enableRepeat(false);
  button.enableLongPress(true);
  pinMode(V1, OUTPUT);
  pinMode(V2, OUTPUT);
  pinMode(V3, OUTPUT);
  pinMode(V4, OUTPUT);
  pinMode(V5, OUTPUT);
  pinMode(V6, OUTPUT);
}

void loop() {
  if (tLastCheck == 0 || millis() - tLastCheck > 5000) {
    tLastCheck = millis();
    checkBattery();
  }
  if (tLastInput != 0 && millis() - tLastInput > 60 * 1000UL)
    powerOff();

  int t = constrain(encoder.read(), 0, 255);
  val = constrain(t / 4, 0, 63);  //sadly necessary
  encoder.write(t);

  switch (button.read()) {
    case MD_KeySwitch::KS_PRESS:
      val = val < 32 ? 63 : 0;
      encoder.write(val * 4);
      break;
    case MD_KeySwitch::KS_LONGPRESS:
      powerOff();
      break;
  }

  if (val != oldVal) {
    tLastInput = millis();
    Serial.println(val);
    oldVal = val;
    updateDisplay(val, vcc);
    updatePins(val);
  }
}
