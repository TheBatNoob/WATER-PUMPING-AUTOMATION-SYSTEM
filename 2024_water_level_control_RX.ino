#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>
#include <EEPROM.h>
#include <Keypad.h>
#include "TANK1.h"
#include "temp.h"
#include "hum.h"
#include "Alarm.h"
#include "Well.h"
#include "network.h"

unsigned long previousMillis = 1500;
const long interval = 500;
unsigned long previousMillis_tank = 1000;
const long interval_tank = 1500;

#define pushbutton 34

const int MANUAL_START_STOP = 32;  // the number of the pushbutton pin
int RELAYState = HIGH;             // the current state of the output pin
int buttonState;                   // the current reading from the input pin
int lastButtonState = HIGH;        // the previous reading from the input pin
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggRELAY
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


RF24 radio(21, 22);
const byte address[][6] = { "00001", "00002" };

//U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/5, /* data=*/17, /* CS=*/4, /* reset=*/16);
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/4, /* data=*/5, /* CS=*/16, /* reset=*/17);

const byte ROWS = 4;  //four rows
const byte COLS = 3;  //three columns

char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte rowPins[ROWS] = { 33, 25, 26, 27 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 14, 12, 13 };      //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

long b = 0;
int c = 0;
float i = 0;  // reading from the sensor
int H2O_percent_1 = 0;
unsigned long Vcm;
unsigned short int Water_Vol;
//int tank_diameter = 0;

long int Num1, Num2, Number;

unsigned int addr_1 = 0;
unsigned int addr_2 = 50;
unsigned int addr_3 = 100;
unsigned int addr_4 = 150;

unsigned int height;  //double
unsigned int diameter;
unsigned int fillable;
unsigned int autostart;

double tank_height;    //  Thank Height
double tank_diameter;  // Tank Diameter
double tank_fillable;
double pump_autostart;

long int Tank_volume;

int WH_1;

//unsigned int radious = tank_diameter;
float fill;
float empty;
//const int // buzzer = 32;
// // buzzer

const int BatteryPin = 36;
float battery_voltage = 0;

int set = 0;
int flag1 = 0, flag2 = 0;

struct Data_Package {
  byte hum;
  byte temp;
  byte distance;
  byte high_level;
  byte mid_level;
  byte low_level;
  byte RX_STARTSTOP_STATE;
  byte water_state;
  byte STOP;
  byte START;
};

Data_Package data;

int hum = 0;
int temp = 0;
int distance = 0;
int high_level = 0;
int mid_level = 0;
int low_level = 0;
int RX_STARTSTOP_STATE;
int water_state;
int START = 0;
int STOP = 0;

void setup() {
  EEPROM.begin(500);
  Serial.begin(115200);
  u8g2.begin();
  radio.begin();
  radio.openWritingPipe(address[0]);     //Setting the address at which we will send the data
  radio.openReadingPipe(1, address[1]);  //Setting the address at which we will receive the data
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);

  //pinMode( buzzer, OUTPUT);
  pinMode(pushbutton, INPUT);
  pinMode(MANUAL_START_STOP, INPUT_PULLUP);

  data.START;
  data.STOP;
  data.water_state;
  //data.TX_STARTSTOP_STATE;
  data.RX_STARTSTOP_STATE;

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.setCursor(15, 9);
  u8g2.print("WATER  PUMPING");  //18

  u8g2.setCursor(3, 24);
  u8g2.print("AUTOMATION SYSTEM");

  u8g2.setCursor(60, 37);
  u8g2.print("BY");

  u8g2.setCursor(0, 50);
  u8g2.print("Agboola Adebayo ");


  u8g2.setCursor(0, 63);
  u8g2.print("Amlabu Samuel ");

  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.setCursor(89, 50);
  u8g2.print("18/30GR015");
  u8g2.setCursor(89, 63);
  u8g2.print("18/30GR025");
  u8g2.sendBuffer();

  delay(3000);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB08_tf);

  u8g2.setCursor(35, 24);
  u8g2.print("SUPERVISED");

  u8g2.setCursor(60, 37);

  u8g2.print("BY");

  u8g2.setCursor(10, 50);
  u8g2.print("Engineer J.A. Adesina");
  u8g2.sendBuffer();

  delay(3000);

}

void display() {
  // delay(5);
  u8g2.clearBuffer();
  radio.startListening();  //This sets the module as receiver
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package));  // Read the whole data and store it into the 'data' structure

    //NETWORK logo//
    u8g2.drawXBMP(57, 32, u8g2_network_width, u8g2_network_height, u8g2_hum_network);

  }

  // Parse the data from the Joystic 1 to the throttle and steering variables
  hum = data.hum;
  temp = data.temp;
  distance = data.distance;
  high_level = data.high_level;
  mid_level = data.mid_level;
  low_level = data.low_level;
  //TX_STARTSTOP_STATE = data.TX_STARTSTOP_STATE;
  RX_STARTSTOP_STATE = data.RX_STARTSTOP_STATE;

  char key = keypad.getKey();
  if ((key) && (c < 5)) {
    if (key == '1')  //If Button 1 is pressed
    {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 1;
      else
        Number = (Number * 10) + 1;  //Pressed twice
    }
    if (key == '2')  //Button 2 is Pressed
    {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 2;
      else
        Number = (Number * 10) + 2;  //Pressed twice
    }

    if (key == '3') {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 3;
      else
        Number = (Number * 10) + 3;  //Pressed twice
    }

    if (key == '4')  //If Button 4 is pressed
    {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 4;
      else
        Number = (Number * 10) + 4;  //Pressed twice
    }

    if (key == '5') {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 5;
      else
        Number = (Number * 10) + 5;  //Pressed twice
    }

    if (key == '6') {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 6;
      else
        Number = (Number * 10) + 6;  //Pressed twice
    }

    if (key == '7')  //If Button 7 is pressed
    {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 7;
      else
        Number = (Number * 10) + 7;  //Pressed twice
    }

    if (key == '8') {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 8;
      else
        Number = (Number * 10) + 8;  //Pressed twice
    }

    if (key == '9') {
      //tone( buzzer, 50, 20);
      if (Number == 0)
        Number = 9;
      else
        Number = (Number * 10) + 9;  //Pressed twice
    }

    if (key == '0') {
      //tone( buzzer, 50, 20);
      if (Number == 0) {
        Number = 0;
        c = c - 1;
      } else
        Number = (Number * 10) + 0;  //Pressed twice
    }
    c = c + 1;
  }

  if (key == '*')  //If cancel Button is pressed
  {
    //tone( buzzer, 50, 20);
    Number = 0;
    c = 0;
  }

  // well();
  /*===============SCROLL TO SET THE THANK HEIGHT AND TANK DIAMETER AND FILLABLE SPACE=================*/
  // Scroll to the left
  if (digitalRead(pushbutton) == 0) {
    if (flag1 == 0 && flag2 == 0) {
      flag1 = 1;
      set = set + 1;
      if (set > 4) {
        set = 0;
      }
      delay(50);
    }
  }

  else {
    flag1 = 0;
  }

  /*========= RETREAVING SETTING SAVED TO EEPROM  =====*/
  tank_height = EEPROM.get(addr_1, height);
  tank_diameter = EEPROM.get(addr_2, diameter);
  pump_autostart = EEPROM.get(addr_3, autostart);
  tank_fillable = EEPROM.get(addr_4, fillable);
  i = distance;  // tank_height FROM SENSOR TRANSMITTER

  u8g2.setFontMode(1);  /* activate transparent font mode */
  u8g2.setDrawColor(1); /* color 1 for the box */
  u8g2.drawBox(78, 0, 128, 32);
  u8g2.setDrawColor(2);
  u8g2.setFont(u8g2_font_courR08_tf);
  u8g2.setFontDirection(0);

  /*AUTOSTART*/
  u8g2.setCursor(79, 31);
  int AS = (autostart);  // my tank_height cylinder weight dat was enter on keypad  in cm
  u8g2.print("AS=");
  u8g2.print(AS);
  u8g2.print("%");

  /*TANK fillable space*/
  u8g2.setCursor(79, 23);
  int F = (fillable);  // my tank_height cylinder weight dat was enter on keypad  in cm
  u8g2.print("FS=");
  u8g2.print(F);
  u8g2.print("%");

  /*THANKS HEIGHT*/
  u8g2.setCursor(79, 7);
  int v = (tank_height);  // my tank_height cylinder weight dat was enter on keypad  in cm
  u8g2.print("TH=");
  u8g2.print(v);
  u8g2.print("cm");


  /*THANKS DIAMETER*/
  u8g2.setCursor(79, 15);
  int D = (tank_diameter);  // my tank_height cylinder weight dat was enter on keypad  in cm
  u8g2.print("TR=");
  u8g2.print(D);
  u8g2.print("cm");

  /*SENSOR READING */
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.setCursor(0, 62);
  u8g2.print("SH=");
  u8g2.print(distance);
  u8g2.print("cm");

  /*========= FIRST PAGE SETTING  =====*/

  if (set == 1) {
    u8g2.setFontMode(1);  /* activate transparent font mode */
    u8g2.setDrawColor(1); /* color 1 for the box */
    u8g2.drawBox(78, 0, 128, 32);
    u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_courR08_tf);
    u8g2.setFontDirection(0);
    u8g2.setCursor(94, 24);
    u8g2.print(Number);

    /*THANKS HEIGHT*/
    u8g2.setCursor(79, 7);
    int v = (height);  // my tank_height cylinder weight dat was enter on keypad  in cm
    u8g2.print("TH=");
    u8g2.print(v);
    u8g2.print("cm");

    height = Number;
    if ((key == '#') && height < 301) {
      //tone( buzzer, 120, 20);
      EEPROM.put(addr_1, height);
      EEPROM.commit();
    }

    if ((key == '#') && height > 301)  //||
    {
      height = 0;
      //tone( buzzer, 2000, 3000);
    }
  }


  /*========= SECOUND PAGE SETTING  =====*/
  if (set == 2) {
    u8g2.setFontMode(1);  /* activate transparent font mode */
    u8g2.setDrawColor(1); /* color 1 for the box */
    u8g2.drawBox(78, 0, 128, 32);
    u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_courR08_tf);
    u8g2.setFontDirection(0);
    u8g2.setCursor(94, 24);
    u8g2.print(Number);

    /*THANKS DIAMETER*/
    u8g2.setCursor(79, 15);
    int D = (diameter);  // my tank_height cylinder weight dat was enter on keypad  in cm
    u8g2.print("TR=");
    u8g2.print(D);
    u8g2.print("cm");

    diameter = Number;

    if ((key == '#') && diameter < 301) {
      //tone( buzzer, 120, 20);
      EEPROM.put(addr_2, diameter);
      EEPROM.commit();
    }

    if ((key == '#') && diameter > 301)  //||
    {
      diameter = 0;
      //tone( buzzer, 2000, 3000);
    }
  }


  /*========= THIRD PAGE SETTING  =====*/
  if (set == 3) {
    u8g2.setFontMode(1);  /* activate transparent font mode */
    u8g2.setDrawColor(1); /* color 1 for the box */
    u8g2.drawBox(78, 0, 128, 32);
    u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_courR08_tf);
    u8g2.setFontDirection(0);
    u8g2.setCursor(94, 24);
    u8g2.print(Number);

    /*THANKS fillable space*/
    u8g2.setCursor(79, 15);
    int F = (fillable);  // my tank_height cylinder weight dat was enter on keypad  in cm
    u8g2.print("FS=");
    u8g2.print(F);
    u8g2.print("%");

    fillable = Number;

    if ((key == '#') && fillable < 98) {
      //tone( buzzer, 120, 20);
      EEPROM.put(addr_4, fillable);
      EEPROM.commit();
    }

    if ((key == '#') && fillable > 98)  //||
    {
      fillable = 0;
      //tone( buzzer, 2000, 3000);
    }
  }

  /*========= FORTH PAGE SETTING  =====*/
  if (set == 4) {
    u8g2.setFontMode(1);  /* activate transparent font mode */
    u8g2.setDrawColor(1); /* color 1 for the box */
    u8g2.drawBox(78, 0, 128, 32);
    u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_courR08_tf);
    u8g2.setFontDirection(0);
    u8g2.setCursor(94, 24);
    u8g2.print(Number);

    /*THANKS fillable space*/
    u8g2.setCursor(79, 15);
    int AS = (autostart);  // my tank_height cylinder weight dat was enter on keypad  in cm
    u8g2.print("AS=");
    u8g2.print(AS);
    u8g2.print("%");

    autostart = Number;

    if ((key == '#') && autostart < 70) {
      //tone( buzzer, 120, 20);
      EEPROM.put(addr_3, autostart);
      EEPROM.commit();
    }

    if ((key == '#') && autostart > 70)  //||
    {
      fillable = 0;
      //tone( buzzer, 2000, 3000);
    }
  }



  //========= BATTERY VOLTAGE CALCULATION =====//

  float voltage = (float)analogRead(BatteryPin) / 4096 * 4.2 * 10000 / 10000;
  float BV = battery_voltage * 2;
  float BAT_PERCENT = map(BV, 2.7, 4.2, 0, 100);

  /*
  //========= BATTERY VOLTAGE MONITORING =====//
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.setCursor(100, 32);
  u8g2.print("BV=");
  u8g2.print(voltage, 1);
  u8g2.print("V");


  //========= BATTERY PERCENTAGE MONITORING =====//
  u8g2.setCursor(60, 32);
  u8g2.print("BL=");
  u8g2.print(BAT_PERCENT, 1);
  u8g2.print("%");
*/

  fill = tank_fillable / 100;
  empty = pump_autostart / 100;
  //===============Water HEIGHT=======================//
  int Tank_1_Fillable_space = tank_height * fill;
  int Tank_1_empty_space = tank_height - Tank_1_Fillable_space;
  WH_1 = tank_height - distance;

  //=============Water PERCENTAGE=================//
  H2O_percent_1 = map(distance, Tank_1_empty_space, tank_height, 100, 0);

  if (H2O_percent_1 >= 100) {
    H2O_percent_1 = 100;
  }

  if (H2O_percent_1 <= 0) {
    H2O_percent_1 = 0;
  }

  if (height <= 0) {
    height = 0;
  }

  //==========Water VOLUME===============//
  Vcm = PI * tank_diameter * tank_diameter * WH_1;
  Water_Vol = Vcm / 1000;

  //=====================Tanke_volume====================//
  long int Tank_size = PI * tank_diameter * tank_diameter * tank_height;
  Tank_volume = Tank_size / 1000;

  if (WH_1 < 0) {

    WH_1 = 0;
  }


  if (Water_Vol <= 2) {

    Water_Vol = 0;
  }

  if (Water_Vol > Tank_volume) {

    Water_Vol = Tank_volume;
  }

  u8g2.drawBox(71, 48, 128, 64);

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.setCursor(31, 55);
  u8g2.print("TV=");
  u8g2.print(Tank_volume);
  u8g2.print("L");

  u8g2.setCursor(31, 63);
  u8g2.print("WV=");
  u8g2.print(Water_Vol);  //k, 1
  u8g2.print("L");

  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.setCursor(72, 54);
  u8g2.print("Height   Level");

  u8g2.setFont(u8g2_font_courR08_tf);
  u8g2.setCursor(71, 63);
  u8g2.print(WH_1);  //l / 1000, 1
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.print("cm");

  u8g2.setFont(u8g2_font_courR08_tf);
  u8g2.setCursor(103, 63);
  u8g2.print(H2O_percent_1);
  u8g2.print("%");

  if (H2O_percent_1 < 20) {

    //digitalWrite(buzzer, HIGH);
  } else if (H2O_percent_1 > 20) {
    // digitalWrite(buzzer, LOW);
  }


  //EMPTY
  if (H2O_percent_1 <= 0) {
    H2O_percent_1 = 0;
    unsigned long currentMillis_tank = millis();
    if (currentMillis_tank - previousMillis_tank >= interval_tank) {
      previousMillis_tank = currentMillis_tank;
      u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_0_25_bits);
    }
  }

  else if ((H2O_percent_1 > 0) && (H2O_percent_1 <= 2)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_2_25_bits);
  }

  else if ((H2O_percent_1 > 2) && (H2O_percent_1 <= 4)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_4_25_bits);
  }

  else if ((H2O_percent_1 > 4) && (H2O_percent_1 <= 6)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_6_25_bits);
  }

  else if ((H2O_percent_1 > 6) && (H2O_percent_1 <= 8)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_8_25_bits);
  }

  else if ((H2O_percent_1 > 8) && (H2O_percent_1 <= 10)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_10_25_bits);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 10) && (H2O_percent_1 <= 12)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_12_25_bits);
  }

  else if ((H2O_percent_1 > 12) && (H2O_percent_1 <= 14)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_14_25_bits);
  }

  else if ((H2O_percent_1 > 14) && (H2O_percent_1 <= 16)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_16_25_bits);
  } else if ((H2O_percent_1 > 16) && (H2O_percent_1 <= 18)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_18_25_bits);
  }

  else if ((H2O_percent_1 > 18) && (H2O_percent_1 <= 20)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_20_25_bits);
  }


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 20) && (H2O_percent_1 <= 22)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_22_25_bits);
  }

  else if ((H2O_percent_1 > 22) && (H2O_percent_1 <= 24)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_24_25_bits);
  }

  else if ((H2O_percent_1 > 24) && (H2O_percent_1 <= 26)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_26_25_bits);
  }

  else if ((H2O_percent_1 > 26) && (H2O_percent_1 <= 28)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_28_25_bits);
  }

  else if ((H2O_percent_1 > 28) && (H2O_percent_1 <= 30)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_30_25_bits);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  else if ((H2O_percent_1 > 30) && (H2O_percent_1 <= 32)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_32_25_bits);
  }

  else if ((H2O_percent_1 > 32) && (H2O_percent_1 <= 34)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_34_25_bits);
  }

  else if ((H2O_percent_1 > 34) && (H2O_percent_1 <= 36)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_36_25_bits);
  }

  else if ((H2O_percent_1 > 36) && (H2O_percent_1 <= 38)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_38_25_bits);
  }

  else if ((H2O_percent_1 > 38) && (H2O_percent_1 <= 40)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_40_25_bits);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 40) && (H2O_percent_1 <= 42)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_42_25_bits);
  }

  else if ((H2O_percent_1 > 42) && (H2O_percent_1 <= 44)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_44_25_bits);
  }

  else if ((H2O_percent_1 > 44) && (H2O_percent_1 <= 46)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_46_25_bits);
  } else if ((H2O_percent_1 > 46) && (H2O_percent_1 <= 48)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_48_25_bits);
  }

  else if ((H2O_percent_1 > 48) && (H2O_percent_1 <= 50)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_50_25_bits);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 50) && (H2O_percent_1 <= 52)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_52_25_bits);
  }

  else if ((H2O_percent_1 > 52) && (H2O_percent_1 <= 54)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_54_25_bits);
  }

  else if ((H2O_percent_1 > 54) && (H2O_percent_1 <= 56)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_56_25_bits);
  }

  else if ((H2O_percent_1 > 56) && (H2O_percent_1 <= 58)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_58_25_bits);
  }

  else if ((H2O_percent_1 > 58) && (H2O_percent_1 <= 60)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_60_25_bits);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 60) && (H2O_percent_1 <= 62)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_62_25_bits);
  }

  else if ((H2O_percent_1 > 62) && (H2O_percent_1 <= 64)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_64_25_bits);
  }

  else if ((H2O_percent_1 > 64) && (H2O_percent_1 <= 66)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_66_25_bits);
  }

  else if ((H2O_percent_1 > 66) && (H2O_percent_1 <= 68)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_68_25_bits);
  }

  else if ((H2O_percent_1 > 68) && (H2O_percent_1 <= 70)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_70_25_bits);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 70) && (H2O_percent_1 <= 72)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_72_25_bits);
  }

  else if ((H2O_percent_1 > 72) && (H2O_percent_1 <= 74)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_74_25_bits);
  }

  else if ((H2O_percent_1 > 74) && (H2O_percent_1 <= 76)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_76_25_bits);
  } else if ((H2O_percent_1 > 76) && (H2O_percent_1 <= 78)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_78_25_bits);
  }

  else if ((H2O_percent_1 > 78) && (H2O_percent_1 <= 80)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_80_25_bits);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 80) && (H2O_percent_1 <= 82)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_82_25_bits);
  }

  else if ((H2O_percent_1 > 82) && (H2O_percent_1 <= 84)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_84_25_bits);
  }

  else if ((H2O_percent_1 > 84) && (H2O_percent_1 <= 86)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_86_25_bits);
  }

  else if ((H2O_percent_1 > 86) && (H2O_percent_1 <= 88)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_88_25_bits);
  }

  else if ((H2O_percent_1 > 88) && (H2O_percent_1 <= 90)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_90_25_bits);

  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  else if ((H2O_percent_1 > 90) && (H2O_percent_1 <= 92)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_92_25_bits);
  }

  else if ((H2O_percent_1 > 92) && (H2O_percent_1 <= 94)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_94_25_bits);
  }

  else if ((H2O_percent_1 > 94) && (H2O_percent_1 <= 96)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_96_25_bits);
  }

  else if ((H2O_percent_1 > 96) && (H2O_percent_1 <= 98)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_98_25_bits);
  }

  else if ((H2O_percent_1 > 98) && (H2O_percent_1 <= 100)) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_100_25_bits);
  }

  else if (H2O_percent_1 >= 100) {

    u8g2.drawXBMP(2, 0, u8g2_tank1_0_25_width, u8g2_tank1_0_25_height, u8g2_tank1_100_25_bits);
  }

  //Tempreture logo//
  u8g2.drawXBMP(30, 1, u8g2_temp_width, u8g2_temp_height, u8g2_temp_bits);

  //Humidity logo//
  u8g2.drawXBMP(30, 27, u8g2_hum_width, u8g2_hum_height, u8g2_hum_bits);

  //Well logo//

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    //EMPTY
    if (high_level == 0 && mid_level == 0 && low_level == 0) {

      u8g2.drawXBMP(60, 0, u8g2_empty_width, u8g2_empty_height, u8g2_empty_bits);
    }
  }

  if (high_level == 0 && mid_level == 0 && low_level == 0 && buttonState == LOW) {

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_courR08_tf);
    u8g2.setCursor(3, 10);
    u8g2.print("UNDERGROUND Water Is");
    u8g2.setCursor(3, 20);
    u8g2.print("Low, Kindly Wait For");
    u8g2.setCursor(13, 30);
    u8g2.print("The Water Level to");
    u8g2.setCursor(35, 40);
    u8g2.print("Increase");
    u8g2.setCursor(3, 50);
    u8g2.print("We Are Sorry For Any");
    u8g2.setCursor(25, 60);
    u8g2.print("INCONVENIENCE");

    u8g2.sendBuffer();
    delay(5000);
  }

  if (high_level == 0 && mid_level == 0 && low_level == 0) {
    RELAYState = 0;
  }


  //LOW   RELAYState
  if (high_level == 0 && mid_level == 0 && low_level == 10) {

    u8g2.drawXBMP(60, 0, u8g2_low_width, u8g2_low_height, u8g2_low_bits);
  }


  //MID
  if (high_level == 0 && mid_level == 10 && low_level == 10) {

    u8g2.drawXBMP(60, 0, u8g2_mid_width, u8g2_mid_height, u8g2_mid_bits);
  }

  //HIGH
  if (high_level == 10 && mid_level == 10 && low_level == 10) {

    u8g2.drawXBMP(60, 0, u8g2_high_width, u8g2_high_height, u8g2_high_bits);
  }


  u8g2.setCursor(43, 14);
  u8g2.print(temp);

  u8g2.setCursor(43, 43);
  u8g2.print(hum);


  if (RELAYState == 1) {

    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(83, 40);
    u8g2.print("PUMP ON");

  }

  else if (RELAYState == 0) {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(83, 40);
    u8g2.print("PUMP OFF");
  }

  //=============PUMP CONTROL STATUS=======================//
  if (RELAYState == 1 && H2O_percent_1 == 100) {
    RELAYState = 0;
  }

  else if (RELAYState == 1 && H2O_percent_1 < 100) {
    RELAYState = 1;
  }

  else if (RELAYState == 0 && H2O_percent_1 < pump_autostart) {
    RELAYState = 1;
  }

  u8g2.sendBuffer();

  Serial.print("dis ");
  Serial.print(distance);

  Serial.print("    TEMP ");
  Serial.print(temp);

  Serial.print("    HUM ");
  Serial.print(hum);

  Serial.print("    high_level ");
  Serial.print(high_level);

  Serial.print("    mid_level ");
  Serial.print(mid_level);

  Serial.print("    low_level ");
  Serial.print(low_level);

  Serial.print("    FILL ");
  Serial.print(fill);

  Serial.print("    AS ");
  Serial.print(voltage);

}

void loop() {
  display();
  TX_MODE();
}

void TX_MODE() {
  radio.stopListening();
  // read the state of the switch into a local variable:
  int reading = digitalRead(MANUAL_START_STOP);

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the RELAY if the new button state is HIGH
      if (buttonState == HIGH) {
        RELAYState = !RELAYState;
      }
    }
  }
  lastButtonState = reading;
  data.RX_STARTSTOP_STATE = RELAYState;
  Serial.print("  TX_state= ");
  Serial.println(RELAYState);
  
  radio.write(&data, sizeof(Data_Package));

}
