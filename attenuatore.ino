#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <MD_KeySwitch.h>

#define USE_ENCODER

#ifdef USE_ENCODER
#include <Encoder.h>
const int ENC_A=3,ENC_B=2,BUTTON=11;
MD_KeySwitch button(BUTTON,LOW);
Encoder encoder(3,2);
#else
const int BTN_MENO=4, BTN_PIU=3;
MD_KeySwitch meno(BTN_MENO, LOW), piu(BTN_PIU, LOW);
#endif

const int V1=5, V2=6,V3=7, V4=8, V5=9, V6=10;

int val=0, oldVal=64;
Adafruit_SSD1306 disp;

void updateDisplay(uint8_t val) {
  char s[10];
  int16_t  x1, y1;
  uint16_t w, h;

  dtostrf(val/2.0, 3, 1, s);
  disp.getTextBounds(s,0,0,&x1, &y1, &w, &h);
  disp.clearDisplay();
  disp.setCursor((128-w)/2,28);
  disp.print(s);
  disp.display();
}

void updatePins(uint8_t val) {
  digitalWrite(V1,val&0x01?HIGH:LOW);
  digitalWrite(V2,val&0x02?HIGH:LOW);
  digitalWrite(V3,val&0x04?HIGH:LOW);
  digitalWrite(V4,val&0x08?HIGH:LOW);
  digitalWrite(V5,val&0x10?HIGH:LOW);
  digitalWrite(V6,val&0x20?HIGH:LOW);
}

void setup() {
  Serial.begin(115200);

  disp.begin();
  disp.clearDisplay();
  disp.setFont(&FreeSansBold18pt7b);
  disp.setTextSize(1);
  disp.setTextColor(WHITE);

#ifdef USE_ENCODER
  pinMode(BUTTON,INPUT_PULLUP);
#else
  pinMode(MENO,INPUT_PULLUP);
  pinMode(PIU,INPUT_PULLUP);

  meno.begin();
  meno.enableRepeat(true);
  meno.enableDoublePress(true);

  piu.begin();
  piu.enableRepeat(true);
  piu.enableDoublePress(true);
#endif
  pinMode(V1,OUTPUT);
  pinMode(V2,OUTPUT);
  pinMode(V3,OUTPUT);
  pinMode(V4,OUTPUT);
  pinMode(V5,OUTPUT);
  pinMode(V6,OUTPUT);
}

void loop() {
#ifdef USE_ENCODER
  int t=constrain(encoder.read(),0,255);
  val=constrain(t/4,0,63); //sadly necessary
  encoder.write(t);
#else
  switch (meno.read()) {
    case MD_KeySwitch::KS_PRESS:
    case MD_KeySwitch::KS_RPTPRESS:
      val--;
      break;
    case MD_KeySwitch::KS_DPRESS:
      val-=16;
      break;
  }

  switch (piu.read()) {
    case MD_KeySwitch::KS_PRESS:
    case MD_KeySwitch::KS_RPTPRESS:
      val++;
      break;
    case MD_KeySwitch::KS_DPRESS:
      val+=16;
      break;
  }

  val=constrain(val,0,63);
#endif

  switch (button.read()) {
    case MD_KeySwitch::KS_PRESS:
      val=val<32?63:0;
      encoder.write(val*4);
      break;
  }

  if (val!=oldVal) {
    Serial.println(val);
    oldVal=val;
    updateDisplay(val);
    updatePins(val);
  }
}
