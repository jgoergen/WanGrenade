#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <SoftwareSerial.h>

#define speakerPin  4
#define pixelPin    3
#define txPin       0
#define rxPin       1

const char COMMAND_BEGIN_CHAR = '<';
const char COMMAND_END_CHAR = '>';
const byte COMMAND_LENGTH = 32;

char receivedChars[COMMAND_LENGTH];
boolean newData = false;
char receivedNewData[COMMAND_LENGTH];
int ledChangeSpeed = 1;

// target rgb values
int r1 = 0;
int g1 = 0;
int b1 = 0;
int r2 = 0;
int g2 = 0;
int b2 = 0;

// actual rgb values
int ar1 = 0;
int ag1 = 0;
int ab1 = 0;
int ar2 = 0;
int ag2 = 0;
int ab2 = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, pixelPin, NEO_GRB + NEO_KHZ800);
SoftwareSerial Serial(rxPin, txPin);

void setup() {
  
  Serial.begin(9600);
  Serial.println("Init");
  strip.begin();
  pinMode(speakerPin, OUTPUT);
  playNote('c', 40);
  playNote('e', 80);
  playNote('g', 120);
}

void loop() {
  
  readSerial();
  
  if (receivedNewData[0] != 0 && receivedNewData[1] != 0 && receivedNewData[2] != 0) {
  
    // seperate the command from the parameter 
    
    char* command = strtok(receivedNewData, ":");
    char* param = strtok(NULL, ":");
    processCommand(command, param);
  }

  updateLeds();
  delay(10);
}

void processCommand(char* command, char* param) {

  if (strcmp(command, "r1") == 0)             r1 = atoi(param);
  else if (strcmp(command, "g1") == 0)        g1 = atoi(param);
  else if (strcmp(command, "b1") == 0)        b1 = atoi(param);
  else if (strcmp(command, "r2") == 0)        r2 = atoi(param);
  else if (strcmp(command, "g2") == 0)        g2 = atoi(param);
  else if (strcmp(command, "b2") == 0)        b2 = atoi(param);
  else if (strcmp(command, "playc") == 0)     playNote('c', atoi(param));
  else if (strcmp(command, "playd") == 0)     playNote('d', atoi(param));
  else if (strcmp(command, "playe") == 0)     playNote('e', atoi(param));
  else if (strcmp(command, "playf") == 0)     playNote('f', atoi(param));
  else if (strcmp(command, "playg") == 0)     playNote('g', atoi(param));
  else if (strcmp(command, "playa") == 0)     playNote('a', atoi(param));
  else if (strcmp(command, "playb") == 0)     playNote('b', atoi(param));
  else if (strcmp(command, "playc") == 0)     playNote('c', atoi(param));
  else if (strcmp(command, "ledspeed") == 0)  ledChangeSpeed = atof(param);  
}

// commands should be in the format "<COMMAND:PARAMER>"

void readSerial() {
  
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char rc;
  
  while (Serial.available() > 0 && newData == false) {
    
    rc = Serial.read();

    if (recvInProgress == true) {
      
        if (rc != COMMAND_END_CHAR) {
          
            receivedChars[ndx] = rc;
            ndx++;
            
            if (ndx >= COMMAND_LENGTH)              
                ndx = COMMAND_LENGTH - 1;
        } else {
          
            receivedChars[ndx] = '\0'; // terminate the string
            recvInProgress = false;
            ndx = 0;
            newData = true;
        }
    }

    else if (rc == COMMAND_BEGIN_CHAR)      
        recvInProgress = true;
  }

  if (newData == true) {

    newData = false;
    memcpy(
      receivedNewData, 
      receivedChars, 
      COMMAND_LENGTH * sizeof(char));
      
  } else {

    receivedNewData[0] = 0;
    receivedNewData[1] = 0;
    receivedNewData[2] = 0;
  }
}

void updateLeds() {

  // slide values towards targets, unless the distance is less then the 'slide distance' in which case, just set them to the value to prevent banding.

  if (abs(r1 - ar1) < ledChangeSpeed) {

    ar1 = r1;
    
  } else {

    if (r1 < ar1) 
      ar1 -= ledChangeSpeed; 
    else if (r1 > ar1) 
      ar1 += ledChangeSpeed;
  }

  if (abs(g1 - ag1) < ledChangeSpeed) {

    ag1 = g1;
    
  } else {

    if (g1 < ag1) 
      ag1 -= ledChangeSpeed; 
    else if (g1 > ag1) 
      ag1 += ledChangeSpeed;
  }

  if (abs(b1 - ab1) < ledChangeSpeed) {

    ab1 = b1;
    
  } else {

    if (b1 < ab1) 
      ab1 -= ledChangeSpeed; 
    else if (b1 > ab1) 
      ab1 += ledChangeSpeed;
  }
  
  if (abs(r2 - ar2) < ledChangeSpeed) {

    ar2 = r2;
    
  } else {
    
    if (r2 < ar2) ar2 -= ledChangeSpeed; 
    else if (r2 > ar2) ar2 += ledChangeSpeed;
  }

  if (abs(g2 - ag2) < ledChangeSpeed) {

    ag2 = g2;
    
  } else {
    
    if (g2 < ag2) 
      ag2 -= ledChangeSpeed; 
    else if (g2 > ag2) 
      ag2 += ledChangeSpeed;
  }

  if (abs(b2 - ab2) < ledChangeSpeed) {

    ab2 = b2;
    
  } else {

    if (b2 < ab2) 
      ab2 -= ledChangeSpeed; 
    else if (b2 > ab2) 
      ab2 += ledChangeSpeed;
  }
  
  // apply actual rgb values. 
  // ( NOTE: leds are reversed, hence the led indexs being reversed )
  
  strip.setPixelColor(1, strip.Color(ag1, ar1, ab1));
  strip.setPixelColor(0, strip.Color(ag2, ar2, ab2));
  strip.show();  
}

void playTone(int tone, int duration) {
  
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  // play the tone corresponding to the note name
  
  for (int i = 0; i < 8; i++)
    if (names[i] == note)
      playTone(tones[i], duration);
}


