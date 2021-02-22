#include "Adafruit_VL53L0X.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
// Motor A/rechts connections
// R1 op low = Vooruit
int R1 = 17;
int R2 = 16;
// Motor B/links connections
// L1 op low = vooruit
int L1 = 5;
int L2 = 18;

int IR1 = 34;
int IR2 = 35;
void setup(){
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  Serial.begin(115200);
  while (! Serial) {
    delay(1);
  }
  
  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power 
  Serial.println(F("VL53L0X API Simple Ranging example\n\n"));

  // Set all the motor control pins to outputs
    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);
    pinMode(L1, OUTPUT);
    pinMode(L2, OUTPUT);

    // Turn off motors - Initial state
    digitalWrite(R1, LOW);
    digitalWrite(R2, LOW);
    digitalWrite(L1, LOW);
    digitalWrite(L2, LOW);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  display.clearDisplay();
  display.println("Jezus take the wheel");
  display.display();
  delay(5000);
}

void loop() {
  noMovement();
  int x = 1;
  while(x = 1){
  if(analogRead(IR1) < 50 && analogRead(IR2) < 50){
   VL53L0X_RangingMeasurementData_t measure;
   display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
   
  if (measure.RangeStatus != 4)
  { // phase failures have incorrect data
    display.print("Distance (mm): "); display.println(measure.RangeMilliMeter);
  }
  else
  {
    Serial.println(" out of range ");
  }
  int statusSensor1 = analogRead(IR1);
  int statusSensor2 = analogRead(IR2);
  display.print("IR1: ");
  display.println(statusSensor1);
  display.print("IR2: ");
  display.println(statusSensor2);
    if(measure.RangeMilliMeter > 250 || measure.RangeMilliMeter < 30){
      vooruit(); 
    }
    else if(measure.RangeMilliMeter < 250){
      achteruit();
      cirkelLinks();
    }
    else{
      vooruit();
    }
    display.display();
  }
else{
  noMovement();
}
}
}

void noMovement() {
  display.println("Idle");
    digitalWrite(R1, LOW);
    digitalWrite(R2, LOW);
    digitalWrite(L1, LOW);
    digitalWrite(L2, LOW);
    delay(500);
}
void vooruit() {
  display.println("Going forward");
    digitalWrite(R1, LOW);
    digitalWrite(R2, HIGH);
    digitalWrite(L1, LOW);
    digitalWrite(L2, HIGH);
}
void achteruit() {
  display.println("Going backwards");
    digitalWrite(R1, HIGH);
    digitalWrite(R2, LOW);
    digitalWrite(L1, HIGH);
    digitalWrite(L2, LOW);
}
void cirkelLinks() {
  display.println("Circle left");
    digitalWrite(R1, LOW);
    digitalWrite(R2, HIGH);
    digitalWrite(L1, HIGH);
    digitalWrite(L2, LOW);
}
void cirkelRechts() {
  display.println("Circle right");
    digitalWrite(R1, HIGH);
    digitalWrite(R2, LOW);
    digitalWrite(L1, LOW);
    digitalWrite(L2, HIGH);
}
void bochtLinks(){
    display.println("Going left");
    digitalWrite(R1, LOW);
    digitalWrite(R2, LOW);
    digitalWrite(L1, LOW);
    digitalWrite(L2, HIGH);
}
void bochtRechts(){
    display.println("Going right");
    digitalWrite(R1, LOW);
    digitalWrite(R2, HIGH);
    digitalWrite(L1, LOW);
    digitalWrite(L2, LOW);
}
