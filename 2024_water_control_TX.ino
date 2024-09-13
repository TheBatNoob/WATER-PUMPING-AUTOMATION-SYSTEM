#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <NewPing.h>
#include <MedianFilter.h>
#include <Wire.h>
#include "DHT.h";
#include <Adafruit_NeoPixel.h>

#define TRIGGER_PIN 7
#define ECHO_PIN 8
#define DHTPIN A2  // DHT-22 Output Pin connection
#define DHTTYPE DHT22

#define NEOPIXEL_PIN A5
#define NUMPIXELS 4
#define BRIGHTNESS 255
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int blind_spot = 20;

// constants won't change. They're used here to set pin numbers:
const int relay = A4;  // the number of the RELAY pin

// well water sensor
const int high_level_pin = 6;
const int mid_level_pin = 7;
const int low_level_pin = 8;

int high_level = 10;
int mid_level = 10;
int low_level = 10;
int RX_STARTSTOP_STATE;

NewPing sonar(TRIGGER_PIN, ECHO_PIN);

float hum;       // Stores humidity value in percent
float temp;      // Stores temperature value in Celcius
float distance;  // Stores calculated distance in cm
float soundsp;   // Stores calculated speed of sound in M/S
float soundcm;   // Stores calculated speed of sound in cm/ms

DHT dht(DHTPIN, DHTTYPE);

MedianFilter filter(15, 0);
RF24 radio(10, 9);                               // nRF24L01 (CE, CSN)
const byte address[][6] = { "00001", "00002" };  // Address  const byte addresses [][6] = {"00001", "00002"};

// Max size of this struct is 32 bytes - NRF24L01 buffer limit
struct Data_Package {
  byte hum;
  byte temp;
  byte distance;
  byte high_level;
  byte mid_level;
  byte low_level;
  byte RX_STARTSTOP_STATE;
  byte water_state;
};

Data_Package data;  //Create a variable with the above structure

void setup() {
  Serial.begin(9600);

  // Define the radio communication
  dht.begin();
  radio.begin();
  radio.openWritingPipe(address[1]);  //Setting the address at which we will send the data
  radio.openReadingPipe(1, address[0]);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);

  pinMode(high_level_pin, INPUT);
  pinMode(mid_level_pin, INPUT);
  pinMode(low_level_pin, INPUT);

  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);

  // // Set initial default values
  data.hum = 0;
  data.temp = 0;
  data.distance = 0;
  data.high_level;
  data.mid_level;
  data.low_level;
  data.water_state;
  data.RX_STARTSTOP_STATE;

  pixels.begin();
  pixels.setBrightness(BRIGHTNESS);
}

void loop() {

  TX_MODE();
  RX_MODE();
}

void TX_MODE() {

  radio.stopListening();
  high_level = digitalRead(high_level_pin);  // read input value
  if (high_level == HIGH) {
    data.high_level = 10;

    for (int i = 2; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(2, pixels.Color(0, 0, 225));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  } else {
    data.high_level = 0;

    for (int i = 2; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(2, pixels.Color(0, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  }


  mid_level = digitalRead(mid_level_pin);  // read input value
  if (mid_level == HIGH) {
    data.mid_level = 10;

    for (int i = 1; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(1, pixels.Color(0, 225, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  } else {
    data.mid_level = 0;

    for (int i = 1; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(1, pixels.Color(0, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  }


  low_level = digitalRead(low_level_pin);  // read input value
  if (low_level == LOW) {
    data.low_level = 10;
    for (int i = 0; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }

  } else {
    data.low_level = 0;
    for (int i = 0; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  }

  data.hum = dht.readHumidity();      // Get Humidity value
  data.temp = dht.readTemperature();  // Get Temperature value
  data.high_level;
  data.mid_level;
  data.low_level;
  //data.TX_STARTSTOP_STATE;

  // Calculate the Speed of Sound in M/S
  soundsp = 331.4 + (0.606 * temp) + (0.0124 * hum);
  soundcm = soundsp / 10000;

  unsigned int o, uS = sonar.ping();  // Send ping, get ping time in microseconds (uS).
  filter.in(uS);
  o = filter.out();

  // Read all analog inputs and map them to one Byte value
  int actual_distance = o / US_ROUNDTRIP_CM;
  data.distance = actual_distance - blind_spot;

  Serial.print("sound speed= ");
  Serial.print(soundsp);

  Serial.print("  o= ");
  Serial.print(o);

  Serial.print("  US= ");
  Serial.print(uS);

  Serial.print("  US R T= ");
  Serial.print(US_ROUNDTRIP_CM);

  Serial.print("  actual_distance= ");
  Serial.print(actual_distance);

  Serial.print("  distance= ");
  Serial.print(data.distance);

  Serial.print("  temp= ");
  Serial.print(data.temp);

  Serial.print("  hum= ");
  Serial.print(data.hum);

  Serial.print("  high_level= ");
  Serial.print(data.high_level);

  Serial.print("  mid_level= ");
  Serial.print(data.mid_level);

  Serial.print("  low_level= ");
  Serial.print(data.low_level);

  radio.write(&data, sizeof(Data_Package));
}


void RX_MODE() {

  radio.startListening();
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package));  // Read the whole data and store it into the 'data' structure
  }

  RX_STARTSTOP_STATE = data.RX_STARTSTOP_STATE;

  if (RX_STARTSTOP_STATE == 1 && low_level == 0) {
    digitalWrite(relay, LOW);

    for (int i = 3; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(3, pixels.Color(0, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }

  }

  else if (RX_STARTSTOP_STATE == 1 && low_level == HIGH) {
    digitalWrite(relay, HIGH);

    for (int i = 3; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(3, pixels.Color(255, 0, 55));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  }

  else if (RX_STARTSTOP_STATE == 0) {
    digitalWrite(relay, LOW);

    for (int i = 3; i < NUMPIXELS; i++) {  // For each pixel...
      pixels.setPixelColor(3, pixels.Color(0, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      //delay(500);
    }
  }

  Serial.print("  RX_START= ");
  Serial.println(RX_STARTSTOP_STATE);
}
