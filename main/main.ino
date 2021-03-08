#include "Adafruit_VL53L0X.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//WIFI settings
const char* ssid = "Roossien";
const char* password = "RoossienWiFi1";

//i2c: 0x29 (Afstandmeter), 0x3c (oled), 0x68 (gyro)

//OLED init
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Afstandsensor init
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

//gyro init

// Motor A/rechts connections
// R1 op low = Vooruit
int R1 = 17;
int R2 = 16;
// Motor B/links connections
// L1 op low = vooruit
int L1 = 5;
int L2 = 18;

//IR pin
int IRR = 34;
int IRL = 39;

void setup() 
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  Serial.begin(115200);
  Serial.println("Booting...");
  resetDisplay();
  display.println("SETUP START");
  display.println("1: Set motor en IR pins.");
  display.display();
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

  pinMode(IRL, INPUT);
  pinMode(IRR, INPUT);
  delay(50);
  resetDisplay();
  display.println("SETUP");
  display.println("2: WiFi");
  display.display();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println("Verbinding mislukt! Opnieuw opstarten...");
    resetDisplay();
    display.println("SETUP");
    display.println("2: WiFi");
    display.println("Verbinding mislukt! Opnieuw opstarten...");
    display.display();
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("BumbleBert");
  while (! Serial) 
  {
    delay(1);
  }
  //OTA update code
  ArduinoOTA
    .onStart([]() 
    {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      resetDisplay();
      display.println("Start update " + type);
      display.display();
    })
    .onEnd([]() 
    {
      Serial.println("\nEnd");
      resetDisplay();
      display.println("Klaar!");
      display.display();
    })
    .onProgress([](unsigned int progress, unsigned int total) 
    {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      resetDisplay();
      display.printf("Voortgang: %u%%\r", (progress / (total / 100)));
      display.display();
    })
    .onError([](ota_error_t error) 
    {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

resetDisplay();
display.println("SETUP");
display.println("3: Check I2C");
display.display();
//check of VL53L0X werkt
  if (!lox.begin()) 
  {
    Serial.println(F("Failed to boot VL53L0X"));
    resetDisplay();
    display.println("SETUP");
    display.println("3: Check I2C");
    display.println("Afstandsmeter werkt niet.");
    display.display();
    delay(1000);
  }

//TODO: MPU6050 DATA
 
  resetDisplay();
  display.println("Welkom!");
  display.print("IP:"); display.println(WiFi.localIP());
  display.println("Druk BOOT");
  display.println("<---");
  display.display();
}


void loop() 
{
  ArduinoOTA.handle();
  bool startScript = false;
  int bootButton = digitalRead(0); // lees BOOT button
  if(bootButton == 0) 
  {
    startScript = true;
  }
  while (startScript == true)
  {
    resetDisplay();
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
    int statusSensor1 = analogRead(IRL);
    int statusSensor2 = analogRead(IRR);
    display.print("IRL: "); display.print(statusSensor1);
    display.print(" IRR: "); display.println(statusSensor2);
    if (measure.RangeStatus != 4)
      { // phase failures have incorrect data
        display.print("Afstand: "); display.println(measure.RangeMilliMeter);
      }
    display.display();
    delay(100);
  }
}
void resetDisplay()
{
    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
}


