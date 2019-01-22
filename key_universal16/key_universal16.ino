/* 
 *  Universal16 Keyboard Input sample
 *  
 *  Pin assign
 *  LED
 *    Col0    9
 *    Col1    8
 *    Col2    7
 *    Col3    6
 *    Row0    5
 *    Row1    4
 *    Row2    3
 *    Row3    2
 *  Keys
 *    Col3    10
 *    Col2    16
 *    Col1    14
 *    Col0    15
 */

#include "Keyboard.h"
#include <MsTimer2.h>
#include <TimerOne.h>

#define KEY_NUM_MAX   16

#define LED_NUM_X     4
#define LED_NUM_Y     4

#define KEY_NUM_COL   4
#define KEY_NUM_ROW   4

#define KEY_INPUT_ACTIVE    0
#define KEY_INPUT_DEACTIVE  1

unsigned char key_status_current[KEY_NUM_MAX];
unsigned char key_status_before[KEY_NUM_MAX];

unsigned char led_cols[4] = {9,8,7,6};

// ---------------------------------------------------------------------------------
//
// KEY MAP table
//  1キーにつき4バイト(=32bit)割り当てる。同時に4キー分を割り当てることが出来る。
//  bit31-bit24     KEY1
//  bit23-bit16     KEY2
//  bit15-biit8     KEY3
//  bit7 - bit0     KEY4
// 
//  未割当の場合は0を割り当てる。とりあえず今はキー一つだけ。
//
// ---------------------------------------------------------------------------------
#define KEYTABLE_PRESET_NUM   4
const unsigned long key_code_preset[KEYTABLE_PRESET_NUM][KEY_NUM_MAX] = {
  {
    // Preset1 for VisualStudio 2017
    KEY_F1, 
    KEY_F2, 
    KEY_F3, 
    KEY_F4, 
    KEY_F5, 
    KEY_F6, 
    KEY_F7, 
    KEY_F8, 
    0x00818000 | 'B',       //  Build
    KEY_F5,                 // Run
    KEY_F10,                // Step over
    KEY_F11,                // Step in
    0x008182B3,             // KEY_LEFT_SHIFT << 16 |  KEY_LEFT_ALT  << 8 |  KEY_TAB,  
    0x008180B3,             // KEY_LEFT_SHIFT << 16 |  KEY_LEFT_CTRL << 8 |  KEY_TAB,  
    0x000080B3,             // KEY_LEFT_ALT  << 8 |  KEY_TAB,  
    0x000082B3              // KEY_LEFT_CTRL << 8 |  KEY_TAB,  
  },
  {
    // Preset 2:Android Studio
    KEY_F1, 
    KEY_F2, 
    KEY_F3, 
    KEY_F4, 
    KEY_F5, 
    KEY_F6, 
    KEY_F7, 
    KEY_F8, 
    0x00008000 | KEY_F9,    // Build : KEY_LEFT_SHIFT | KEY_F9
    0x00818200 | KEY_F9,    // Debug... : KEY_LEFT_SHIFT |  KEY_LEFT_ALT | KEY_F9,  
    KEY_F7,                 // Step over
    KEY_F8,                 // Step in
    0x008182B3,             // KEY_LEFT_SHIFT << 16 |  KEY_LEFT_ALT  << 8 |  KEY_TAB,  
    0x008180B3,             // KEY_LEFT_SHIFT << 16 |  KEY_LEFT_CTRL << 8 |  KEY_TAB,  
    0x000080B3,             // KEY_LEFT_ALT  << 8 |  KEY_TAB,  
    0x000082B3              // KEY_LEFT_CTRL << 8 |  KEY_TAB,  
  },
  {
    // Preset 3
    KEY_F1, 
    KEY_F2, 
    KEY_F3, 
    KEY_F4, 
    KEY_F5, 
    KEY_F6, 
    KEY_F7, 
    KEY_F8, 
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8'
  },
  {
    // Preset 4
    KEY_F1, 
    KEY_F2, 
    KEY_F3, 
    KEY_F4, 
    KEY_F5, 
    KEY_F6, 
    KEY_F7, 
    KEY_F8, 
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8'
  }

};

// カレントキーテーブル
unsigned long key_code[KEY_NUM_MAX];

// チャタリング除去用のバッファ
volatile unsigned char key_buffer[KEY_NUM_MAX] = {0};

// LED点灯状態設定
unsigned char led_state[LED_NUM_Y][LED_NUM_X] = {
  { 0, 0, 0, 0},
  { 0, 0, 0, 0},
  { 0, 0, 0, 0},
  { 0, 0, 0, 0}
};

// LEDカラム制御用変数
int g_nLedCol = 0;


// プロトタイプ宣言
void setKeyCol(int nLine);
void KeyScanHandler();

// Keytableをセットする
void SetKeyTable(int nPreset)
{
  if ( nPreset < 0 || nPreset >= KEYTABLE_PRESET_NUM ) return;
  
  for( int i=0; i < KEY_NUM_MAX; i++) {
    key_code[i] = key_code_preset[nPreset][i];
  }
}

void SetLed(int x, int y, bool onoff)
{
  if ( x < 0 || x >= LED_NUM_X ) return;
  if ( y < 0 || y >= LED_NUM_X ) return;
  
  if ( onoff == true ) {
    led_state[y][x] = 1;
  } else {
    led_state[y][x] = 0;    
  }
}

//
//  10ms タイマーハンドラ
//  キーのスキャン及びチャタリング除去処理を行う
//
void KeyScanHandler()
{
  int i, n = 0;
  for( i=1, n=0; i<=4;i++, n+=4) {
    setKeyCol(i);
    key_buffer[n+0] <<= 1; key_buffer[n+0] |= digitalRead(15);
    key_buffer[n+1] <<= 1; key_buffer[n+1] |= digitalRead(14);
    key_buffer[n+2] <<= 1; key_buffer[n+2] |= digitalRead(16);
    key_buffer[n+3] <<= 1; key_buffer[n+3] |= digitalRead(10);
    setKeyCol(0);
  }
}

// LED制御ハンドラ

void LedCtrlHandler()
{
  unsigned char port_d;
  unsigned char port_c;

  digitalWrite(led_cols[g_nLedCol], HIGH);
  g_nLedCol++;
  g_nLedCol &= 0x03;

  port_d = PORTD;
  port_c = PORTC;

  port_d &= ~(_BV(0) | _BV(1) | _BV(4));
  port_c &= ~(_BV(6));

  if ( led_state[0][g_nLedCol] != 0 ) port_c |= _BV(6);
  if ( led_state[1][g_nLedCol] != 0 ) port_d |= _BV(4);
  if ( led_state[2][g_nLedCol] != 0 ) port_d |= _BV(0);
  if ( led_state[3][g_nLedCol] != 0 ) port_d |= _BV(1);

  PORTD = port_d;
  PORTC = port_c;

  digitalWrite(led_cols[g_nLedCol], LOW);
  
}

//
// Keyのカラム制御を行う
//
void setKeyCol(int nLine)
{
  unsigned char port_f = PORTF;
  
  port_f |= ( _BV(4) | _BV(5) | _BV(6) | _BV(7)); 
  switch(nLine) {
    case 0:
      break;

    case 1:
      port_f &= ~_BV(4);
      break;

    case 2:
      port_f &= ~_BV(5);
      break;

    case 3:
      port_f &= ~_BV(6);
      break;

    case 4:
      port_f &= ~_BV(7);
      break;
    
  }
  PORTF = port_f;
}


void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);

  // Key Matrix
  pinMode(A3,OUTPUT);
  pinMode(A2,OUTPUT);
  pinMode(A1,OUTPUT);
  pinMode(A0,OUTPUT);

  pinMode(15,INPUT_PULLUP);
  pinMode(14,INPUT_PULLUP);
  pinMode(16,INPUT_PULLUP);
  pinMode(10,INPUT_PULLUP);

  setKeyCol(0);

  // LED Matrix
  pinMode(5,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(2,OUTPUT);

  pinMode(9,OUTPUT);
  pinMode(8,OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(6,OUTPUT);
  
  digitalWrite(5,HIGH);
  digitalWrite(4,HIGH);
  digitalWrite(3,HIGH);
  digitalWrite(2,HIGH);

  digitalWrite(9,HIGH);
  digitalWrite(8,HIGH);
  digitalWrite(7,HIGH);
  digitalWrite(6,HIGH);

  digitalWrite(9,LOW);
  
  for( int i=0; i<KEY_NUM_MAX; i++ ) {
    key_status_current[i] = KEY_INPUT_DEACTIVE;
    key_status_before[i] = KEY_INPUT_DEACTIVE;
  }

  SetKeyTable(0);
  SetLed(0,0,true);

  Keyboard.begin();

  MsTimer2::set(10, KeyScanHandler);
  MsTimer2::start();
  Timer1.initialize(100);
  Timer1.attachInterrupt(LedCtrlHandler);
  Timer1.start();
  
}

void loop() {  
  // Key input check
  unsigned long keycode;

  // チャタリング除去
  for( int i=0; i < KEY_NUM_MAX; i++) {
    if (( key_buffer[i] & 0x0f) == 0x0f ){
      key_status_current[i] = KEY_INPUT_DEACTIVE;
    } else if ( (key_buffer[i] & 0x0f) == 0x00 ) {
      key_status_current[i] = KEY_INPUT_ACTIVE;
    }
  }

  // キーイベントの発生
  for( int i=0; i<KEY_NUM_MAX; i++) {
    if ( key_status_before[i] != key_status_current[i] ){
      if ( key_status_current[i] == KEY_INPUT_ACTIVE) {
        
        keycode = (key_code[i] >> 24) & 0xff;   Keyboard.press(keycode);
        keycode = (key_code[i] >> 16) & 0xff;   Keyboard.press(keycode);
        keycode = (key_code[i] >> 8) & 0xff;    Keyboard.press(keycode);
        keycode = (key_code[i] >> 0) & 0xff;    Keyboard.press(keycode);
        
      } else {
        keycode = (key_code[i] >> 24) & 0xff;   Keyboard.release(keycode);
        keycode = (key_code[i] >> 16) & 0xff;   Keyboard.release(keycode);
        keycode = (key_code[i] >> 8) & 0xff;    Keyboard.release(keycode);
        keycode = (key_code[i] >> 0) & 0xff;    Keyboard.release(keycode);
      }
    }
  }

  // Update status
  for( int i=0; i<KEY_NUM_MAX; i++) {
    key_status_before[i] = key_status_current[i];
  }

  // キーコードの動的変更処理
  int rcvcmd;
  if ( Serial.available() > 0 ) {
    rcvcmd = Serial.read();
    if ( rcvcmd == '0' ) {
      Keyboard.releaseAll();
      SetKeyTable(0);
      SetLed(0,0,true);
      SetLed(1,0,false);
      SetLed(2,0,false);
      SetLed(3,0,false);
    } else if ( rcvcmd == '1' ) {
      Keyboard.releaseAll();
      SetKeyTable(1);
      SetLed(0,0,false);
      SetLed(1,0,true);
      SetLed(2,0,false);
      SetLed(3,0,false);
    } else if ( rcvcmd == '2' ) {
      SetKeyTable(2);
      SetLed(0,0,false);
      SetLed(1,0,false);
      SetLed(2,0,true);
      SetLed(3,0,false);
    }else if ( rcvcmd == '3' ) {
      SetKeyTable(3);
      SetLed(0,0,false);
      SetLed(1,0,false);
      SetLed(2,0,false);
      SetLed(3,0,true);
    }
  } 
}
