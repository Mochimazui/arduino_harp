/********************************************************************************
 *    ||   c |  c# |   d |  d# |   e |   f |  f# |   g |  g# |   a |  a# |   b
 * -----------------------------------------------------------------------------
 *  0 ||   0 |   1 |   2 |   3 |   4 |   5 |   6 |   7 |   8 |   9 |  10 |  11
 *  1 ||  12 |  13 |  14 |  15 |  16 |  17 |  18 |  19 |  20 |  21 |  22 |  23
 *  2 ||  24 |  25 |  26 |  27 |  28 |  29 |  30 |  31 |  32 |  33 |  34 |  35
 *  3 ||  36 |  37 |  38 |  39 |  40 |  41 |  42 |  43 |  44 |  45 |  46 |  47
 *  4 ||  48 |  49 |  50 |  51 |  52 |  53 |  54 |  55 |  56 |  57 |  58 |  59
 *  5 ||  60 |  61 |  62 |  63 |  64 |  65 |  66 |  67 |  68 |  69 |  70 |  71
 *  6 ||  72 |  73 |  74 |  75 |  76 |  77 |  78 |  79 |  80 |  81 |  82 |  83
 *  7 ||  84 |  85 |  86 |  87 |  88 |  89 |  90 |  91 |  92 |  93 |  94 |  95
 *  8 ||  96 |  97 |  98 |  99 | 100 | 101 | 102 | 103 | 104 | 105 | 106 | 107
 *  9 || 108 | 109 | 110 | 111 | 112 | 113 | 114 | 115 | 116 | 117 | 118 | 119
 * 10 || 120 | 121 | 122 | 123 | 124 | 125 | 126 | 127 |
 ********************************************************************************/
/*****************************************
 * LCD RS pin to digital pin 8
 * LCD Enable pin to digital pin 9
 * LCD D4 pin to digital pin 4
 * LCD D5 pin to digital pin 5
 * LCD D6 pin to digital pin 6
 * LCD D7 pin to digital pin 7
 * LCD BL pin to digital pin 10
 * KEY pin to analogl pin 0
 *****************************************/

#include <LiquidCrystal.h>

#define PSI  3
#define PCLR 13
#define PRCK 11
#define PSCK 12
#define PINPUT A3

#define BUTTON_N 5
#define KEY_N 13
#define NOTE_N 12
#define MODE_N 3

#define GET_KEY_DELAY 500 
#define SCALES_ARRAY_OFFSET 14

#define MAX_BUTTON_VALUE 800

typedef enum 
{
  FULL,
  SCALES,
  CUSTOM
}
_T_KeyboardMode;

typedef enum
{
  KEY_UP = -1,
  KEY_DOWN = 1,
  NO_EVENT = 0
}
_T_KeyEvent;

typedef enum
{
  LIGHT_ON = 0,
  LIGHT_OFF = 1
}
_T_KeyStatus;

LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7);

int keyStatus[KEY_N] = {0};
int keyNotes[KEY_N]  = {0};
int keyEvent[KEY_N] = {0};
int scalesOffset[] = 
{
  -24,-22,-20,-19,-17,-15,-13,-12,-10, -8, -7, -5, -3, -1,
    0, // scalesOffset[14]
    2,  4,  5,  7,  9, 11, 12, 14, 16, 17, 19, 21, 23
};
int selectedKey = 1;

int adc_button_val[BUTTON_N] ={ 50, 200, 400, 600, 800 };
int oldButton = -1;

const char keyboardModes[MODE_N][15] = { "Full","Scales","Custom" };
const char noteNames[NOTE_N][3] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
int currentMode;
int do_note;
int do_pos;

char outputStr[32];

void setup()
{
  Serial.begin(9600);

  pinMode(PSI,OUTPUT);
  pinMode(PCLR,OUTPUT);
  pinMode(PRCK,OUTPUT);
  pinMode(PSCK,OUTPUT);  

  lcd.clear(); 
  lcd.begin(16, 2);

  currentMode = FULL;
  do_note = 60;
  do_pos = 1;

  updateKeyNotes();
  updateLcd();
}

void loop()
{
#ifdef _DEBUG
  testKeys(); 
  return;
#endif
  checkKeyboard();  
  outputMidi();
  checkButtons();  
}

void shiftBit()
{
  delayMicroseconds(1);

  digitalWrite(PSCK,HIGH);
  delayMicroseconds(1);
  digitalWrite(PSCK,LOW);

  digitalWrite(PRCK,HIGH);
  delayMicroseconds(1);
  digitalWrite(PRCK,LOW);

  delayMicroseconds(1);
}

int getFirstKey()
{
  // clear all output
  digitalWrite(PCLR,HIGH);
  delayMicroseconds(1);
  digitalWrite(PCLR,LOW);

  digitalWrite(PSI,HIGH);
  shiftBit();
  digitalWrite(PSI,LOW);
  delayMicroseconds(GET_KEY_DELAY);  
  return analogRead(PINPUT);
}

int getNextKey()
{
  shiftBit();
  delayMicroseconds(GET_KEY_DELAY);
  return analogRead(PINPUT);
}

int getKey(int first)
{
  if(first)
    return getFirstKey();
  else
    return getNextKey();
}

void updateKeyNotes()
{
  switch(currentMode)
  {
  case FULL: 
    for(int i=0;i<KEY_N;++i)
      keyNotes[KEY_N-(i+1)] = i+do_note-do_pos+1;    
    break;
  case SCALES: 
    for(int i=0;i<KEY_N;++i)
    {
      int n = (i-do_pos-1)/7;
      int offset = (i-do_pos)%7;
      keyNotes[KEY_N-(i+1)] = 
        do_note + 
        scalesOffset[SCALES_ARRAY_OFFSET+(i-do_pos+1)];
    }
    break;
  case CUSTOM: 
    selectedKey = 1;
    break;
  default:
    break;
  }
}

void updateLcd()
{

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(keyboardModes[currentMode]);
  lcd.setCursor(0,1);
  if(currentMode==CUSTOM)
  {
    sprintf(outputStr,
      "Key %d = %s%d",
      selectedKey,
      noteNames[keyNotes[selectedKey]%12],
      keyNotes[selectedKey]/12);
  }
  else
  {
    sprintf(outputStr,
      "1=%s%d Key %d",
      noteNames[do_note%12],
      do_note/12,do_pos);
  }
  lcd.print(outputStr);
}

void onCustomButtonDown(int button)
{
  bool flag = true;
  int &cknote = keyNotes[selectedKey];
  switch(button)
  {
  case 0: // right
    if(selectedKey!=KEY_N)++selectedKey;
    break;
  case 1: // up
    if(cknote!=127)++cknote;
    break;
  case 2: // down
    if(cknote!=0)--cknote;
    break;
  case 3: // left
    if(selectedKey!=1)--selectedKey;
    break;
  case 4: // select
    ++currentMode;
    if(currentMode>=MODE_N)
      currentMode=0;
    break;    
  default:
    flag = false;
    break;
  }
  if(flag)
  {
    updateKeyNotes();
    updateLcd();
  }
}

void onButtonDown(int button)
{
  bool flag = true;
  switch(button)
  {
  case 0: // right
    if(do_pos!=KEY_N)++do_pos;
    break;
  case 1: // up
    if(do_note!=127)++do_note;
    break;
  case 2: // down
    if(do_note!=0)--do_note;
    break;
  case 3: // left
    if(do_pos!=1)--do_pos;
    break;
  case 4: // select
    ++currentMode;
    if(currentMode>=MODE_N)
      currentMode=0;
    break;    
  default:
    flag = false;
    break;
  }
  if(flag)
  {
    updateKeyNotes();
    updateLcd();
  }
}

int getButton(unsigned int input)
{
  int k;
  for (k=0;k<BUTTON_N;++k)
  {
    if (input<adc_button_val[k])
    {
      return k;
    }
  }
  if (k >= BUTTON_N)
    k = -1;
  return k;
}

void checkButtons()
{
  int tval;
  int tb1,tb2;

  tval = analogRead(0);    
  if(tval>MAX_BUTTON_VALUE)
    return;

  tb1 = getButton(tval);  
  if (tb1 == oldButton)   
    return;
  delay(50);  
  tval = analogRead(0);   
  tb2 = getButton(tval);
  if(tb1!=tb2)
    return;
  oldButton = tb1;
  if(currentMode==CUSTOM)
    onCustomButtonDown(tb1);
  else
    onButtonDown(tb1);       
}

void checkKeyboard()
{
  int oldStatus;
  int newStatus;

  for(int i=0;i<KEY_N;++i)
  {
    newStatus = getKey(!i);

    oldStatus = keyStatus[i];
    if(newStatus <=50)
      newStatus=LIGHT_OFF;
    else
      newStatus=LIGHT_ON;  

    if(oldStatus==newStatus)
    {
      keyEvent[i] = NO_EVENT;
    }
    else
    {
      if(newStatus == LIGHT_OFF)
        keyEvent[i] = KEY_DOWN;
      else
        keyEvent[i] = KEY_UP;
      keyStatus[i] = newStatus;
    }
  }
}

void outputMidi()
{
  // 1000nnnn 0kkkkkkk 0vvvvvvv Note Off event.
  // 1001nnnn 0kkkkkkk 0vvvvvvv Note On event.	
  // nnnn channel 
  // kkkkkkk key (note) number
  // vvvvvvv velocity.

  for(int i=0;i<KEY_N;++i)
  {
    if(keyEvent[i] == KEY_DOWN)
      Serial.write(B10000001);
    else if(keyEvent[i] == KEY_UP)
      Serial.write(B10010001);
    else
      continue;
    Serial.write(keyNotes[i]);
    Serial.write(B01111111); 
    keyEvent[i] = NO_EVENT;
  }
  /*******************************
   * // old message
   * Serial.print(" @ ");
   * Serial.print(keyNotes[i]);
   * Serial.print(" ");
   * Serial.print(keyStatus[i]);
   * Serial.print(" # ");
   * Serial.println("");      
   ******************************/
}

#ifdef _DEBUG

void outputKeyStatus(int kid,int val)
{
  char tstr[16];
  sprintf(tstr,"# %d : %-3d #",kid,val);
  Serial.print(tstr);
}

void testKeys()
{
  int tval;
  tval = getFirstKey();
  outputKeyStatus(1,tval);
  for(int i=2;i<=13;++i)
  {
    tval = getNextKey();
    outputKeyStatus(i,tval);
  }
  Serial.println();
}

#endif

