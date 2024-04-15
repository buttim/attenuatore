#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <MD_KeySwitch.h>
#include <LowPower.h>

#define USE_ENCODER

#ifdef USE_ENCODER
#include <Encoder.h>
const int ENC_A = 3, ENC_B = 4, BUTTON = 2;
MD_KeySwitch button(BUTTON, LOW);
Encoder encoder(ENC_A, ENC_B);
#else
const int BTN_MENO = 4, BTN_PIU = 3;
MD_KeySwitch meno(BTN_MENO, LOW), piu(BTN_PIU, LOW);
#endif

const int V1 = 5, V2 = 6, V3 = 7, V4 = 8, V5 = 9, V6 = 10;

int val = 0, oldVal = 64;
Adafruit_SSD1306 disp;

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
     ADMUX = _BV(MUX5) | _BV(MUX0) ;
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks bothstop
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void updateDisplay(uint8_t val) {
  char s[10];
  int16_t x1, y1;
  uint16_t w, h;

  if (val < 255)
    dtostrf(val / 2.0, 3, 1, s);
  else
    strcpy(s, "OFF");
  disp.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  disp.clearDisplay();
  disp.setCursor((128 - w) / 2, 28);
  disp.print(s);
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
  if (readVcc()<3000)
    while(true)
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void setup() { 
  checkBattery();
  Serial.begin(115200);

  disp.begin();
  disp.clearDisplay();
  disp.setFont(&FreeSansBold18pt7b);
  disp.setTextSize(1);
  disp.setTextColor(WHITE);

#ifdef USE_ENCODER
  pinMode(BUTTON, INPUT_PULLUP);
  button.enableRepeat(false);
  button.enableLongPress(true);
#else
  pinMode(MENO, INPUT_PULLUP);
  pinMode(PIU, INPUT_PULLUP);

  meno.begin();
  meno.enableRepeat(true);
  meno.enableDoublePress(true);

  piu.begin();
  piu.enableRepeat(true);
  piu.enableDoublePress(true);
#endif
  pinMode(V1, OUTPUT);
  pinMode(V2, OUTPUT);
  pinMode(V3, OUTPUT);
  pinMode(V4, OUTPUT);
  pinMode(V5, OUTPUT);
  pinMode(V6, OUTPUT);
}

uint64_t tLast=0;
void loop() {
  if (tLast==0 || millis()-tLast>5000) {
    tLast=millis();
    checkBattery();
  }
#ifdef USE_ENCODER
  int t = constrain(encoder.read(), 0, 255);
  val = constrain(t / 4, 0, 63);  //sadly necessary
  encoder.write(t);
#else
  switch (meno.read()) {
    case MD_KeySwitch::KS_PRESS:
    case MD_KeySwitch::KS_RPTPRESS:
      val--;
      break;
    case MD_KeySwitch::KS_DPRESS:
      val -= 16;
      break;
  }

  switch (piu.read()) {
    case MD_KeySwitch::KS_PRESS:
    case MD_KeySwitch::KS_RPTPRESS:
      val++;
      break;
    case MD_KeySwitch::KS_DPRESS:
      val += 16;
      break;
  }

  val = constrain(val, 0, 63);
#endif

  switch (button.read()) {
    case MD_KeySwitch::KS_PRESS:
      val = val < 32 ? 63 : 0;
      encoder.write(val * 4);
      break;
    case MD_KeySwitch::KS_LONGPRESS:
      updateDisplay(255);
      val = 0;
      disp.ssd1306_command(SSD1306_DISPLAYOFF);
      while (digitalRead(BUTTON) == LOW)
        ;
      updatePins(0);
      attachInterrupt(
        digitalPinToInterrupt(BUTTON), []() {}, LOW);
      delay(300);
      disp.dim(true);
      delay(300);
      disp.clearDisplay();
      disp.display();
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
      checkBattery();
      detachInterrupt(digitalPinToInterrupt(BUTTON));
      disp.ssd1306_command(SSD1306_DISPLAYON);
      updateDisplay(0);
      delay(300);
      disp.dim(false);
      while (digitalRead(BUTTON) == LOW)
        ;
      break;
  }

  if (val != oldVal) {
    Serial.println(val);
    oldVal = val;
    updateDisplay(val);
    updatePins(val);
  }
}
