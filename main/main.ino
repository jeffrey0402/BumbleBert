#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebSocketsClient.h>

//WIFI settings
const char* ssid = "BattleBotsRouter";
const char* password = "NetwerkBoys";

const char* BOT_NAME = "BumbleBert";

WebSocketsClient webSocket;

String spel = "geen";

//i2c: 0x29 (Afstandmeter), 0x3c (oled), 0x68 (gyro)

//OLED init
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Motor A/rechts connections
int R1 = 17;
int R2 = 16;
// Motor B/links connections
int L1 = 5;
int L2 = 18;

//IR pin
int IRR = 34;
int IRL = 39;

//IR waarde van de tape.
const int tapeWaarde = 200;

// PWM properties
const int freq = 300;
const int pwmR1 = 0;
const int pwmR2 = 1;
const int pwmL1 = 2;
const int pwmL2 = 3;
const int resolution = 8;

// heleboel confusing waarden. Speed is de snelheid van de motor (255 max)
// de onderstaande waarden slaan de snelheid van de motor, en het sturen aan.
// positief is vooruit, negatief is achteruit. 
int maxSpeed = 200;
int maxSpeedAchteruit = -50;

int speedL1 = 0;
int speedL2 = 0;
int speedR1 = 0;
int speedR2 = 0;
int speedL = 0;
int speedR = 0;
int speedtmp = 0;

//snelheid voor de vooruit(), achteruit() etc. functies.
int speedFnc1 = 150;
int speedFnc2 = 120;

bool setupDoolhof = false;

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

  // PWM
  ledcSetup(pwmR1, freq, resolution);
  ledcSetup(pwmR2, freq, resolution);
  ledcSetup(pwmL1, freq, resolution);
  ledcSetup(pwmL2, freq, resolution);

  ledcAttachPin(R1, pwmR1);
  ledcAttachPin(R2, pwmR2);
  ledcAttachPin(L1, pwmL1);
  ledcAttachPin(L2, pwmL2);

  // alle motoren uitzetten. Soms blijven ze aan na een update.
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
    Serial.println("Verbinding mislukt!");
    resetDisplay();
    display.println("SETUP");
    display.println("2: WiFi");
    display.println("Verbinding mislukt! Druk RST om opnieuw te proberen, of wacht 3s");
    display.display();
    delay(3000);
    break;
    //ESP.restart();
  }
  webSocket.begin("194.171.181.139", 49154, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  resetDisplay();
  display.println("Welkom!");
  display.print("IP:"); display.println(WiFi.localIP());
  display.println("Deze ESP heeft geen rem");
  display.println("wachten op command");
  display.display();
  ArduinoOTA.handle();
}

void loop() 
{
  webSocket.loop();
  ArduinoOTA.handle();
  
  int bootButton = digitalRead(0); // lees BOOT button
  if(bootButton == 0) 
  {
    resetDisplay();
    display.println("Karren maar!");
    display.display();
    spel = "tekenen";
  }

  while (spel == "race")
  {
    int statusSensorL = analogRead(IRL);
    int statusSensorR = analogRead(IRR);
    //Check sensor data, vergelijk het met de zwartheid van de tape.
    //de robot blijft over de tape lopen
    //while loop, om de inhoud van de loop zo snel mogelijk te laten lopen.

    while (statusSensorL <= tapeWaarde && statusSensorR > tapeWaarde)
    {
      speedR = speedR+1;
      speedL = speedL-1;
      statusSensorL = analogRead(IRL);
      statusSensorR = analogRead(IRR);
      checkOverFlow();
      setEngineVars();
      writeEngine();
    }
    while (statusSensorR <= tapeWaarde && statusSensorL > tapeWaarde)
    {
      speedL = speedL+1;
      speedR = speedR-1;
      statusSensorR = analogRead(IRR);
      statusSensorL = analogRead(IRL);
      checkOverFlow();
      setEngineVars();
      writeEngine();
    }
    while (statusSensorR > tapeWaarde && statusSensorL > tapeWaarde)
    {
      // Je vind het misschien een beetje raar, maar we zitten nog steeds op de tape, dus karren maar!
      statusSensorR = analogRead(IRR);
      statusSensorL = analogRead(IRL);
      speedR = maxSpeed;
      speedL = maxSpeed;
      checkOverFlow();
      setEngineVars();
      writeEngine();
      webSocket.loop();
    }
    webSocket.loop();
  }

  while (spel == "doolhof")
  {
    webSocket.loop();
    int statusSensorR = analogRead(IRR);
    int statusSensorL = analogRead(IRL);
    speedFnc1 = 90;
    speedFnc2 = 90;
    
    while (setupDoolhof == false)
    {
      int statusSensorR = analogRead(IRR);
      int statusSensorL = analogRead(IRL);
      vooruit();
      if (statusSensorL > tapeWaarde || statusSensorR > tapeWaarde)
      {
        setupDoolhof = true; //setup complete; tape gevonden
      }
    }
    
    while (setupDoolhof)
    {
      webSocket.loop();
      statusSensorR = analogRead(IRR);
      statusSensorL = analogRead(IRL);
      if (statusSensorR >= tapeWaarde && statusSensorL < tapeWaarde)
      {
        //we gucci
        vooruit();
      }
      if (statusSensorR >= tapeWaarde && statusSensorL >= tapeWaarde)
      {
        cirkelLinks();
      }
      if (statusSensorR < tapeWaarde && statusSensorL < tapeWaarde)
      {
        //beide van de tape, hoekomikhierweerop??
        cirkelRechts();
      }
    }
  }

  if (spel == "sps")
  {
    //Steen papier schaar
     //webSocket.loop();
     int random = rand() % 3;
     if (random == 0)
     {
       //steen
       webSocket.sendTXT("5");
     }
     else if (random == 1)
     {
       //papier
       webSocket.sendTXT("6");
     }
     else if (random == 2)
     {
       //schaar
       webSocket.sendTXT("7");
     }
     spel = "geen";
  }

  while (spel == "tekenen")
  {
    //Tekenen
    delay(1000);
    int i = 0;
    while (i <= 4)
    {
      i++;
      vooruit();
      delay(200);
      stop();
      delay(500);
      cirkelRechts();
      delay(180);
      stop();
      delay(500);
      vooruit();
      delay(200);
      stop();
      delay(500);
      cirkelLinks();
      delay(350);
      stop();
      delay(500);
    }
    spel = "geen";
    stop();
  }

  while (spel == "checkir")
  {
    int statusSensorL = analogRead(IRL);
    int statusSensorR = analogRead(IRR);

    resetDisplay();
    display.println(statusSensorL);
    display.println(statusSensorR);
    display.display();
  }
}

//functies
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) 
{
  switch(type) 
  {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      // send message to server when Connected
      webSocket.sendTXT(BOT_NAME);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      commandReceiver(*payload);
      // send message to server
      break;
  }
}

void commandReceiver(uint8_t command)
{ 
  switch(command) 
  {
    case 48:
      spel = "geen";
      stop();
    break;
    case 49:
      spel = "race";
      resetDisplay();
      display.println("Race");
      display.display();
    break;
    case 50:
      spel = "tekenen";
      resetDisplay();
      display.println("Kunst tijd");
      display.display();
    break;
    case 51:
      spel = "doolhof";
      resetDisplay();
      display.println("HELP WAAR BEN IK????");
      display.display();
    break;
    case 52:
      spel = "sps";
      resetDisplay();
      display.println("Steen papier schaar GOTY 2021");
      display.display();
    break;
    case 56:
      resetDisplay();
      display.println("Steen papier schaar GOTY 2021");
      display.println("Epische overwinning!!!!");
      display.display();
    break;
    case 57:
      resetDisplay();
      display.println("Steen papier schaar GOTY 2021");
      display.println("Volgende keer beter!");
      display.display();
    break;
    case 97:
      spel = "geen";
      stop();
    break;
    case 98:
      vooruit();
    break;
    case 99:
      achteruit();
    break;
    case 100:
      bochtLinks();
    break;
    case 101:
      bochtRechts();
    break;
    case 102:
      cirkelLinks();
    break;
    case 103:
      cirkelRechts();
    break;
    case 104:
      resetDisplay();
      display.println("Steen papier schaar GOTY 2021");
      display.println("Gelijk spel. Klopt niets van.");
      display.display();
    break;
    case 122:
    break;
  }
}

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
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, 0);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, 0);
}

void vooruit()
{
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, speedFnc1);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, speedFnc1);
}
void achteruit()
{
  ledcWrite(pwmR1, speedFnc2);
  ledcWrite(pwmR2, 0);
  ledcWrite(pwmL1, speedFnc2);
  ledcWrite(pwmL2, 0);
}

void cirkelLinks()
{
  ledcWrite(pwmR1, speedFnc2);
  ledcWrite(pwmR2, 0);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, speedFnc1);
}

void cirkelRechts()
{
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, speedFnc1);
  ledcWrite(pwmL1, speedFnc2);
  ledcWrite(pwmL2, 0);
}

void bochtLinks()
{
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, 0);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, speedFnc1);
}
void bochtRechts()
{
  ledcWrite(pwmR1, 0);
  ledcWrite(pwmR2, speedFnc1);
  ledcWrite(pwmL1, 0);
  ledcWrite(pwmL2, 0);
}