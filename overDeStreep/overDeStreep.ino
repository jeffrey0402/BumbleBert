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
int R1 = 17;
int R2 = 16;
// Motor B/links connections
int L1 = 5;
int L2 = 18;

//IR pin
int IRR = 34;
int IRL = 39;

//IR waarde van de tape. Staat op 1k om safe te zijn, maar werkt mogelijk slecht met andere vloeren.
const int tapeWaarde = 2000;

// Setting PWM properties
const int freq = 300;
const int pwmR1 = 0;
const int pwmR2 = 1;
const int pwmL1 = 2;
const int pwmL2 = 3;
const int resolution = 8;


//heleboel confusing waarden. Speed is de snelheid van de motor (255 max, ~180 min.)
//de onderstaande waarden slaan de snelheid van de motor, en het sturen aan.
//positief is vooruit, negatief is achteruit. 
int maxSpeed = 220;
int maxSpeedAchteruit = 0; //lagere waarden = slechter sturen, meer vastlopers.

int speedL1 = 0;
int speedL2 = 0;
int speedR1 = 0;
int speedR2 = 0;
int speedL = 0;
int speedR = 0;

void setup() 
{
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  Serial.println("Booting...");
  resetDisplay();
  display.println("SETUP START");
  display.println("1: Pinouts & set vars");
  display.display();

  // Set all the motor control pins to outputs
  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(L1, OUTPUT);
  pinMode(L2, OUTPUT);

  // configure LED PWM functionalitites
  ledcSetup(pwmR1, freq, resolution);
  ledcSetup(pwmR2, freq, resolution);
  ledcSetup(pwmL1, freq, resolution);
  ledcSetup(pwmL2, freq, resolution);

  ledcAttachPin(R1, pwmR1);
  ledcAttachPin(R2, pwmR2);
  ledcAttachPin(L1, pwmL1);
  ledcAttachPin(L2, pwmL2);

  // Turn off motors - Initial state
  digitalWrite(R1, LOW);
  digitalWrite(R2, LOW);
  digitalWrite(L1, LOW);
  digitalWrite(L2, LOW);

  pinMode(IRL, INPUT);
  pinMode(IRR, INPUT);
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
    display.println("Verbinding mislukt! Druk RST om opnieuw te proberen");
    display.display();
    delay(3000);
    //ESP.restart();
    break;
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
    delay(3000);
  }

  //TODO: MPU6050 DATA
 
  resetDisplay();
  display.println("Welkom!");
  display.print("IP:"); display.println(WiFi.localIP());
  display.println("Druk BOOT");
  display.println("<---");
  display.display();
  ArduinoOTA.handle();

}

void loop() 
{
  bool startScript = false;
  int bootButton = digitalRead(0); // lees BOOT button
  if(bootButton == 0) 
  {
    resetDisplay();
    display.println("Karren maar!");
    display.display();
    startScript = true;
  }

  //Karren maar! Motor driver code:
  while (startScript == true)
  {
    int statusSensorL = analogRead(IRL);
    int statusSensorR = analogRead(IRR);

    //Check sensor data, vergelijk het met de zwartheid van de tape.
    //de robot blijft over de tape lopen
    //while loop, om de inhoud van de loop zo snel mogelijk te laten lopen.

    while (statusSensorL <= tapeWaarde)
    {
      //stuur Links
      speedL--;
      speedR++;
      statusSensorL = analogRead(IRL);
      statusSensorR = analogRead(IRR);
      checkOverFlow();
      setEngineVars();
      writeEngine();
    }
    while (statusSensorR <= tapeWaarde)
    {
      //stuur Rechts
      speedR--;
      speedL++;
      statusSensorR = analogRead(IRR);
      statusSensorL = analogRead(IRL);
      checkOverFlow();
      setEngineVars();
      writeEngine();
    }
    if (statusSensorR > tapeWaarde && statusSensorL > tapeWaarde)
    {
      // Je vind het misschien een beetje raar, maar we zitten nog steeds op de tape, dus karren maar!
      speedR++;
      speedL++;
    }

    checkOverFlow();
    setEngineVars();
    writeEngine();
  }
}

//functies
void checkOverFlow()
{
    //voorkom dat de getallen hoger dan toegestaan worden. 255 is max!
    //vooruit
    if (speedR > maxSpeed)
    {
      speedR = maxSpeed;
    }
    if (speedL > maxSpeed)
    {
      speedL = maxSpeed;
    }
    // achteruit
    if (speedR < maxSpeedAchteruit)
    {
      speedR = maxSpeedAchteruit;
    }
    if (speedL < maxSpeedAchteruit)
    {
      speedL = maxSpeedAchteruit;
    }
}

void writeEngine()
{
    ledcWrite(pwmR1, speedR1);
    ledcWrite(pwmR2, speedR2);
    
    ledcWrite(pwmL1, speedL1);
    ledcWrite(pwmL2, speedL2);
}

void setEngineVars()
{
    // speedR en speedL bepalen de snelheid van de linker en rechter motoren. Positief = vooruit, negatief = achteruit.
    //speedR1/2 + speedR1/2 worden gebruikt om de snelheid te "writen".
    if (speedR > 0)
    {
      speedR1 = 0;
      speedR2 = speedR;
    }
    else if (speedR < 0)
    {
      speedR1 = (speedR) * -1;
      speedR2 = 0;
    }
    else
    {
      speedR1 = 0;
      speedR2 = 0;
    }

    if (speedL > 0)
    {
      speedL1 = 0;
      speedL2 = speedL;
    }
    else if (speedL < 0)
    {
      speedL1 = (speedL) * -1;
      speedL2 = 0;
    }
    else
    {
      speedL1 = 0;
      speedL2 = 0;
    }
}

void resetDisplay()
{
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
}

void stop()
{
  digitalWrite(R1, LOW);
  digitalWrite(R2, LOW);
  digitalWrite(L1, LOW);
  digitalWrite(L2, LOW);
}

void vooruit()
{
  display.println("Vooruit");
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, maxSpeed);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, maxSpeed);
}

void cirkelLinks()
{
  display.println("Cirkel links");
  ledcWrite(pwmR1, maxSpeed);
  ledcWrite(pwmR2, 0);
  ledcWrite(pwmL1, maxSpeed);
  ledcWrite(pwmL2, 0);
}

void cirkelRechts()
{
  display.println("Cirkel rechts");
  ledcWrite(pwmR1, maxSpeed);
  ledcWrite(pwmR2, 0);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, maxSpeed);
}

void bochtLinks()
{
  display.println("Bocht links");
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, 0);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, maxSpeed);
}
void bochtRechts()
{
  display.println("Cirkel rechts");
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, maxSpeed);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, 0);
}
