#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN D2 // Hier wird angegeben, an welchem digitalen Pin die WS2812 LEDs bzw. NeoPixel angeschlossen sind
#define NUMPIXELS 7 // Hier wird die Anzahl der angeschlossenen WS2812 LEDs bzw. NeoPixel angegeben
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint32_t white = pixels.Color(255,255,255);
uint32_t green = pixels.Color(255,0,0);
uint32_t red = pixels.Color(0,255,0);
uint32_t off = pixels.Color(0,0,0);

//setup OTA
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include <Servo.h>
#include <Bounce2.h>
Bounce b = Bounce(); // Instantiate a Bounce object

#define BUTTON_FEET D6

Servo servo_right;
Servo servo_left;

#define LEFT_OPEN 55
#define LEFT_CLOSED 135

#define RIGHT_OPEN 145
#define RIGHT_CLOSED 42 


int isOpen = LOW;
int potiVal = 0; // 0-1024

//setup leds states and timer 
#define LED_WARMUP 350
#define LED_TRAPPED 350
#define LED_TRAPFLASH 35

unsigned long timeTrappedLed;
unsigned long timeTrapOpenLed;

bool trapped = false;
bool wasOpen = false;
bool trapLed = false;
bool flashLeds = false;
bool wasOpenLed = false;

void setup() { 
  Serial.begin(115200);
  Serial.println();
  //WIFI connect && setup ota
  WiFi.begin("", "");
  WiFi.hostname("ESP-Ghost-Trap");
  Serial.print("Connecting");
  
  pixels.begin(); // Initialisierung der NeoPixel
  pixels.setPixelColor(6, green); // Pixel4 leuchtet
  pixels.show();
  //setup servos
  servo_right.attach(D8);
  servo_left.attach(D7);
  
  servo_right.write(RIGHT_CLOSED);
  servo_left.write(LEFT_CLOSED);

  //setup led
  pinMode(A0, INPUT); // Setup the LED
  pinMode(LED_BUILTIN, OUTPUT); // Setup the LED
  digitalWrite(LED_BUILTIN, !isOpen); // Turn off the LED

  //setup buttons
  b.attach (BUTTON_FEET, INPUT_PULLUP);
  b.interval(25);


  //startup phase
  for (int x=0; x<10; x++) {
    pixels.setPixelColor(6, green); // Pixel4 leuchtet
    pixels.show();
    delay (LED_WARMUP);
    pixels.setPixelColor(6, off); // Pixel4 leuchtet
    pixels.show();
    delay (LED_WARMUP);
  }


  //Wait for Wifi and start OTA

  /*while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }*/
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  
//  ArduinoOTA.setPassword((const char *)"Ghosty");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
  });

  ArduinoOTA.begin();

  //Done
  
  pixels.setPixelColor(6, red); // Pixel4 leuchtet
  pixels.show();
}

void loop() {
  ArduinoOTA.handle();
  
  b.update(); // Update the Bounce instance
   
  if ( b.fell() ) {  // Call code if button transitions from HIGH to LOW
    
    //wenn sie das erste Mal geöffnet wird
    if (!wasOpen) {
      wasOpen = true;
    } else {
      trapped = true;
    }
    
    isOpen = !isOpen; // Toggle LED state
    digitalWrite(LED_BUILTIN, !isOpen); // Apply new LED state

    if (isOpen) {
      Serial.println("OPEN");
      servo_left.write(LEFT_OPEN);
      servo_right.write(RIGHT_OPEN);
    } else {
      Serial.println("CLOSED");
      servo_left.write(LEFT_CLOSED);
      servo_right.write(RIGHT_CLOSED);
    }
    
  }
 
  potiVal = analogRead(A0); // Pin einlesen

  //Blitzen wenn offen
  if (isOpen && ((millis() - timeTrapOpenLed) > LED_TRAPFLASH)) {
    //merken,dass die LEDs an waren
    wasOpenLed = true;
    if (flashLeds) {
      setTrapLeds(white);
    } else {
      setTrapLeds(off);
    }
    flashLeds = !flashLeds;
    pixels.show();
    timeTrapOpenLed = millis();
  }

  //sicherstellen, dass die Leds aus sind wenn die Kiste zu geht
  if (!isOpen && wasOpenLed) {
    wasOpenLed = false;
    setTrapLeds(off);
    pixels.show();
  }

  //Blinken wenn Geist im Kasten
  if (trapped && ((millis() - timeTrappedLed) > LED_TRAPPED)) {
    if (trapLed) {
      pixels.setPixelColor(6, red); // Pixel4 leuchtet
    } else {
      pixels.setPixelColor(6, off); // Pixel4 leuchtet
    }
    trapLed = !trapLed;
    pixels.show();
    timeTrappedLed = millis();
  }
}

void setTrapLeds(uint32_t color) {
  for (int x=0; x<6; x++) {
    pixels.setPixelColor(x, color);
  }
}
