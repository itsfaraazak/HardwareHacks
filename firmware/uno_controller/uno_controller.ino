/*
 * RhythmRide — Arduino Uno Controller (with 180°-rotated LCD)
 * ===========================================================
 * LCD physically mounted upside-down. A runtime font engine rotates every
 * glyph 180° via math (no per-char hardcoding). Each screen is kept to
 * <=8 distinct characters so it always fits the HD44780's 8 CGRAM slots.
 *
 * PINS:
 *   A0 throttle pot wiper   A1 brake pot wiper   A2 steer pot wiper
 *   D2 button0 KICK   D3 button1 SNARE   D4 button2 HIHAT  (other leg → GND)
 *   LCD: RS→D5  EN→D6  D4→A3  D5→D8  D6→D9  D7→D13
 *        LCD 1 GND→GND, 2 VCC→5V, 3 V0→contrast pot, 5 RW→GND,
 *        15 A→5V, 16 K→GND.  Pins 7-10 unconnected.
 *
 * SENDS (115200 baud, JSON lines):
 *   {"t":"throttle","v":..} {"t":"brake","v":..} {"t":"steer","v":..}
 *   {"t":"btn","i":0-2,"s":1/0}
 * RECEIVES: {"t":"bpm","v":N} {"t":"score","v":N}
 *
 * Library: LiquidCrystal (built into Arduino IDE).
 */

#include <LiquidCrystal.h>
#include <avr/pgmspace.h>

LiquidCrystal lcd(5, 6, A3, 8, 9, 13);

// ── 5x7 font, column-major, bit0=top. ASCII 0x20..0x5A ───────────────────────
const byte FONT[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00,
  0x14,0x7F,0x14,0x7F,0x14, 0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62,
  0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00, 0x00,0x1C,0x22,0x41,0x00,
  0x00,0x41,0x22,0x1C,0x00, 0x14,0x08,0x3E,0x08,0x14, 0x08,0x08,0x3E,0x08,0x08,
  0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x60,0x60,0x00,0x00,
  0x20,0x10,0x08,0x04,0x02, 0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00,
  0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31, 0x18,0x14,0x12,0x7F,0x10,
  0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03,
  0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E, 0x00,0x36,0x36,0x00,0x00,
  0x00,0x56,0x36,0x00,0x00, 0x08,0x14,0x22,0x41,0x00, 0x14,0x14,0x14,0x14,0x14,
  0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x51,0x09,0x06, 0x32,0x49,0x79,0x41,0x3E,
  0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
  0x7F,0x41,0x41,0x22,0x1C, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01,
  0x3E,0x41,0x49,0x49,0x7A, 0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00,
  0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40,
  0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
  0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46,
  0x46,0x49,0x49,0x49,0x31, 0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F,
  0x1F,0x20,0x40,0x20,0x1F, 0x7F,0x20,0x18,0x20,0x7F, 0x63,0x14,0x08,0x14,0x63,
  0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43
};

byte hmirror5(byte x){ byte r=0; for(byte c=0;c<5;c++) if((x>>c)&1) r|=(1<<(4-c)); return r; }

void buildRotated(char c, byte out[8]){
  if(c>='a'&&c<='z') c-=32;
  if(c<0x20||c>0x5A){ for(byte i=0;i<8;i++) out[i]=0; return; }
  const byte* f=&FONT[(c-0x20)*5];
  byte tmp[8];
  for(byte r=0;r<7;r++){ byte row=0;
    for(byte col=0;col<5;col++) if((pgm_read_byte(&f[col])>>r)&1) row|=(1<<(4-col));
    tmp[r]=row; }
  tmp[7]=0;
  for(byte r=0;r<8;r++) out[r]=hmirror5(tmp[7-r]);
}

byte glyphs[8][8]; byte glyphCount;
int findOrAddGlyph(byte g[8]){
  for(byte s=0;s<glyphCount;s++){ bool same=true;
    for(byte i=0;i<8;i++) if(glyphs[s][i]!=g[i]){ same=false; break; }
    if(same) return s; }
  if(glyphCount>=8) return -1;
  for(byte i=0;i<8;i++) glyphs[glyphCount][i]=g[i];
  return glyphCount++;
}

void renderFlipped(const char* line0, const char* line1){
  glyphCount=0;
  char phys[2][16]; const char* L[2]={line0,line1};
  for(byte lr=0;lr<2;lr++){
    char buf[17]="                ";
    byte n=strlen(L[lr]); if(n>16)n=16; memcpy(buf,L[lr],n);
    byte pr=1-lr;
    for(byte i=0;i<16;i++) phys[pr][i]=buf[15-i];
  }
  byte slotOf[2][16];
  for(byte r=0;r<2;r++) for(byte c=0;c<16;c++){
    char ch=phys[r][c];
    if(ch==' '){ slotOf[r][c]=255; continue; }
    byte g[8]; buildRotated(ch,g);
    int s=findOrAddGlyph(g); slotOf[r][c]=(s<0)?255:s;
  }
  for(byte s=0;s<glyphCount;s++) lcd.createChar(s,glyphs[s]);
  for(byte r=0;r<2;r++){ lcd.setCursor(0,r);
    for(byte c=0;c<16;c++){ byte s=slotOf[r][c]; if(s==255) lcd.write(' '); else lcd.write(s); }
  }
}

// ── Pins / tuning ─────────────────────────────────────────────────────────────
#define PIN_THROTTLE A0
#define PIN_BRAKE    A1
#define PIN_STEER    A2
const int BTN_N=3;
const int BTN_PINS[BTN_N]={2,3,4};
const char* BTN_NAMES[BTN_N]={"KICK","SNARE","HIHAT"};
const float EMA=0.15f, DELTA_T=0.015f, DELTA_B=0.004f, DELTA_S=0.004f;
const unsigned long POT_MS=40, SCREEN_MS=1600, FLASH_MS=650;

float thS=0,brS=0,stS=0, lT=-1,lB=-1,lS=-1;
int lastBtn[BTN_N];
unsigned long lastPot=0, lastScreen=0, flashUntil=0;
int remoteBpm=120, remoteScore=0;
byte screenIdx=0;

const char* NOTES[]={"C2","E2","G2","A2","C3","E3","G3","A3","C4","E4","G4","A4","C5","E5","G5","A5"};
const char* VOWELS[]={"A","E","I","O","U"};

void sendValue(const char* type, float v){
  char num[8]; dtostrf(v,4,2,num); char* p=num; while(*p==' ')p++;
  Serial.print("{\"t\":\""); Serial.print(type);
  Serial.print("\",\"v\":"); Serial.print(p); Serial.println("}");
}
void sendBtn(int i,int s){
  Serial.print("{\"t\":\"btn\",\"i\":"); Serial.print(i);
  Serial.print(",\"s\":"); Serial.print(s); Serial.println("}");
}

// ── Build current screen (each kept <=8 distinct chars) ──────────────────────
const byte NSCREENS=4;
void drawScreen(){
  char l0[17], l1[17];
  switch(screenIdx){
    case 0: snprintf(l0,17,"BPM");   snprintf(l1,17,"%d",remoteBpm);   break; // B,P,M+digits
    case 1: snprintf(l0,17,"SCR");   snprintf(l1,17,"%d",remoteScore); break; // S,C,R+digits
    case 2: { int ni=constrain((int)(thS*15.0f),0,15);
              snprintf(l0,17,"NOTE"); snprintf(l1,17,"%s",NOTES[ni]);  break; } // N,O,T,E+2
    default:{ int vi=constrain((int)(lS*5.0f),0,4);
              snprintf(l0,17,"VOWEL"); snprintf(l1,17,"%s",VOWELS[vi]);break; } // V,O,W,E,L+1
  }
  renderFlipped(l0,l1);
}

void parseIncoming(char* buf){
  if(strstr(buf,"\"bpm\"")){ char* p=strstr(buf,"\"v\":"); if(p) remoteBpm=atoi(p+4); }
  else if(strstr(buf,"\"score\"")){ char* p=strstr(buf,"\"v\":"); if(p) remoteScore=atoi(p+4); }
}

bool flashActive=false;

void setup(){
  Serial.begin(115200);
  lcd.begin(16,2);
  renderFlipped("RHYTHM","RIDE");   // R,H,Y,T,M,I,D,E = 8 distinct, fits
  for(int i=0;i<BTN_N;i++){ pinMode(BTN_PINS[i],INPUT_PULLUP); lastBtn[i]=HIGH; }
  Serial.println("{\"t\":\"hello\",\"role\":\"uno\"}");
  delay(1400);                       // hold boot splash
  lastScreen=millis();
}

static char rxBuf[128]; static byte rxLen=0;
void drainSerial(){
  while(Serial.available()){ char c=Serial.read();
    if(c=='\n'){ rxBuf[rxLen]='\0'; if(rxLen>0) parseIncoming(rxBuf); rxLen=0; }
    else if(rxLen<127) rxBuf[rxLen++]=c; }
}

void loop(){
  unsigned long now=millis();

  if(now-lastPot>=POT_MS){ lastPot=now;
    float t=EMA*(analogRead(PIN_THROTTLE)/1023.0f)+(1-EMA)*thS;
    float b=EMA*(analogRead(PIN_BRAKE)   /1023.0f)+(1-EMA)*brS;
    float s=EMA*(analogRead(PIN_STEER)   /1023.0f)+(1-EMA)*stS;
    thS=t; brS=b; stS=s;
    if(fabsf(t-lT)>=DELTA_T){ sendValue("throttle",t); lT=t; }
    if(fabsf(b-lB)>=DELTA_B){ sendValue("brake",   b); lB=b; }
    if(fabsf(s-lS)>=DELTA_S){ sendValue("steer",   s); lS=s; }
  }

  for(int i=0;i<BTN_N;i++){
    int st=digitalRead(BTN_PINS[i]);
    if(st!=lastBtn[i]){ lastBtn[i]=st; sendBtn(i, st==LOW?1:0);
      if(st==LOW){ renderFlipped(BTN_NAMES[i],"HIT"); flashUntil=now+FLASH_MS; flashActive=true; }
    }
  }

  // Flash ended → resume on next screen
  if(flashActive && now>=flashUntil){ flashActive=false; lastScreen=now; drawScreen(); }

  // Cycle screens when not flashing
  if(!flashActive && now-lastScreen>=SCREEN_MS){
    lastScreen=now; screenIdx=(screenIdx+1)%NSCREENS; drawScreen();
  }

  drainSerial();
}
