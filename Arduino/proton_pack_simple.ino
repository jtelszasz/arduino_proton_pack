#include <Adafruit_NeoPixel.h>
#include <SparkFunSX1509.h> // Include SX1509 library for wand expander board
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

SX1509 io; // Create an SX1509 object

#define NEO_POWER 3 // for powercell
Adafruit_NeoPixel powerStick = Adafruit_NeoPixel(8, NEO_POWER, NEO_GRB + NEO_KHZ800);

#define NEO_CYCLO 2 // for cyclotron
Adafruit_NeoPixel cyclotron = Adafruit_NeoPixel(4, NEO_CYCLO, NEO_GRB + NEO_KHZ800);


// Wand PINs
const int wandFlasherPin = 1;
const int wandVentLightPin = 2;
const int wandTipLightPin = 3;
const int wandFireButtonPin = 5;

//int wandFireButtonState = LOW;         // variable for reading the pushbutton status

// Possible Pack states
bool powerBooted = false;   // has the pack booted up

// ##############################
// available options
// ##############################
const bool useGameCyclotronEffect = true;     // set this to true to get the fading previous cyclotron light in the idle sequence
const bool useCyclotronFadeInEffect = true;   // Instead of the yellow alternate flashing on boot this fades the cyclotron in from off to red

// ##############################
// bootup animation speeds
// ##############################
const unsigned long pwr_boot_interval = 40;       // How fast to do the powercell drop animation on bootup 
const unsigned long cyc_boot_interval = 400;      // If useCyclotronFadeInEffect is false this alternates the cycltron lights yellow 
const unsigned long cyc_boot_alt_interval = 150;  // How fast to fade in the cyclotron lights from black to red on bootup

// ##############################
// idle animation speeds
// ##############################
const unsigned long pwr_interval = 55;            // how fast the powercell cycles: One 14 led cycle per 756 ms
const unsigned long cyc_interval = 770;           // how fast the cycltron cycles from one cell to the next: One full rotation every 3024 ms
const unsigned long cyc_fade_interval = 1;        // if useGameCyclotronEffect is true this is how fast to fade the previous cyclotron to light to nothing
unsigned long wand_flasher_interval = 500;   // how fast the flasher on the wand cycles

int wandFlasherState = LOW;
int fireButtonState = 0;         // variable for reading the pushbutton status
int isFiring = 0;

// soundboard pins and setup
#define SFX_RST 9
#define SFX_RX 10
#define SFX_TX 11
const int ACT = 12;    // this allows us to know if the audio is playing

SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard( &ss, NULL, SFX_RST);
  
// ##############################
// audio files`
// ##############################
const char bootupSound[] =    "T00     WAV";
const char fireSound[] =      "T01     WAV";
const char stopFireSound[] =  "T09     WAV";

/* ************* Audio Board Helper Functions ************* */
// helper function to play a track by name on the audio board
void playAudio( char* trackname, int playing ) {
  // stop track if one is going
  if (playing == 0) {
    sfx.stop();
  }
    
  // now go play
  if (sfx.playTrack(trackname)) {
    sfx.unpause();
  }
}

void setup() {
  ss.begin(9600);
  io.begin(0x3E);

  io.pinMode(wandFlasherPin, OUTPUT);
  io.pinMode(wandVentLightPin, OUTPUT);
  io.pinMode(wandTipLightPin, OUTPUT);
  io.pinMode(wandFireButtonPin, INPUT_PULLUP);

  io.digitalWrite(wandTipLightPin, LOW);  
  io.digitalWrite(wandVentLightPin, LOW);  
  io.digitalWrite(wandFlasherPin, LOW); 
  
  // configure powercell/cyclotron
  powerStick.begin();
  powerStick.setBrightness(80);
  powerStick.show(); // Initialize all pixels to 'off'

  cyclotron.begin();
  cyclotron.setBrightness(80);
  cyclotron.show(); // Initialize all pixels to 'off'

  delay(1500);
  playAudio(bootupSound, 1);
  // wand vent light is always on
  io.digitalWrite(wandVentLightPin, HIGH);  
  io.digitalWrite(wandFlasherPin, HIGH); 
  
}

void loop() {
  // get the current time
  unsigned long currentMillis = millis();

  fireButtonState = io.digitalRead(wandFireButtonPin);

  // find out of the audio board is playing audio
  int playing = digitalRead(ACT);

  if(powerBooted == false){
    
    powerSequenceBoot(currentMillis);
    //playAudio(startupTrack, playing);

  } else {
    
    powerSequenceOne(currentMillis, pwr_interval, cyc_interval, cyc_fade_interval);
    
  }

  if(fireButtonState == LOW){
    
    io.digitalWrite(wandTipLightPin, HIGH);
    wand_flasher_interval = 100;
    
    if ( isFiring == false ) {
          isFiring = true;
              playAudio(fireSound, playing);

    }
    
  //  fireStrobe(currentMillis); 
  } else {
    //sfx.stop();
    io.digitalWrite(wandTipLightPin, LOW);
     if ( isFiring == true) {
      playAudio(stopFireSound, playing);
      isFiring = false;
      wand_flasher_interval = 500;
     }
  
  }

  

  delay(1);
}



int cyclotronRunningFadeOut = 255;  // we reset this variable every time we change the cyclotron index so the fade effect works
int cyclotronRunningFadeIn = 0;     // we reset this to 0 to fade the cyclotron in from 



void setCyclotronLightState(int startLed, int endLed, int state ){
  switch ( state ) {
    case 0: // set all leds to red
      for(int i=startLed; i <= endLed; i++) {
        cyclotron.setPixelColor(i, powerStick.Color(255, 0, 0));
      }
      break;
    case 1: // set all leds to orange
      for(int i=startLed; i <= endLed; i++) {
        cyclotron.setPixelColor(i, powerStick.Color(255, 106, 0));
      }
      break;
    case 2: // set all leds off
      for(int i=startLed; i <= endLed; i++) {
        cyclotron.setPixelColor(i, 0);
      }
      break;
    case 3: // fade all leds from red
      for(int i=startLed; i <= endLed; i++) {
        if( cyclotronRunningFadeOut >= 0 ){
          cyclotron.setPixelColor(i, 255 * cyclotronRunningFadeOut/255, 0, 0);
          cyclotronRunningFadeOut--;
        }else{
          cyclotron.setPixelColor(i, 0);
        }
      }
      break;
  case 4: // fade all leds to red
      for(int i=startLed; i <= endLed; i++) {
        if( cyclotronRunningFadeIn < 255 ){
          cyclotron.setPixelColor(i, 255 * cyclotronRunningFadeIn/255, 0, 0);
          cyclotronRunningFadeIn++;
        }else{
          cyclotron.setPixelColor(i, cyclotron.Color(255, 0, 0));
        }
      }
      break;
  }
}

/*************** Powercell/Cyclotron Animations *********************/
/* These allow you to use more than one neopixel per cyclotron if you
 * want it brighter. If using only one set start and end to the same
 * number */
int c1Start = 0;
int c1End = 0;
int c2Start = 1;
int c2End = 1;
int c3Start = 2;
int c3End = 2;
int c4Start = 3;
int c4End = 3;

unsigned long prevPwrBootMillis = 0;    // the last time we changed a powercell light in the boot sequence
unsigned long prevCycBootMillis = 0;    // the last time we changed a cyclotron light in the boot sequence
unsigned long prevPwrMillis = 0;        // last time we changed a powercell light in the idle sequence
unsigned long prevCycMillis = 0;        // last time we changed a cyclotron light in the idle sequence
unsigned long prevFadeCycMillis = 0;    // last time we changed a fading cyclotron light in the idle sequence
unsigned long prevWandFlasherMillis = 0;    // last time we changed the wand flasher

// LED indexes into the neopixel powerstick chain for the cyclotron
const int powercellLedCount = 14;   // total number of led's in the animation
int powerSeqNum = 0;                // current running powercell sequence leds

// animation level trackers for the boot and shutdown
int currentBootLevel = 0;                          // current powercell boot level sequence led
int currentLightLevel = powercellLedCount;         // current powercell boot light sequence led

// boot animation on the powercell/cyclotron
bool reverseBootCyclotron = false;
void powerSequenceBoot(unsigned long currentMillis) {
    
  bool doPowercellUpdate = false;
  if ((unsigned long)(currentMillis - prevPwrBootMillis) >= pwr_boot_interval) {
    // save the last time you blinked the LED
    prevPwrBootMillis = currentMillis;

    // START POWERCELL
    if( currentBootLevel != powercellLedCount ){
      if( currentBootLevel == currentLightLevel){
        if(currentLightLevel+1 <= powercellLedCount){
          powerStick.setPixelColor(currentLightLevel+1, 0);
        }
        powerStick.setPixelColor(currentBootLevel, powerStick.Color(0, 0, 255));
        currentLightLevel = powercellLedCount;
        currentBootLevel++;
      }else{
        if(currentLightLevel+1 <= powercellLedCount){
          powerStick.setPixelColor(currentLightLevel+1, 0);
        }
        powerStick.setPixelColor(currentLightLevel, powerStick.Color(0, 0, 255));
        currentLightLevel--;
      }
      doPowercellUpdate = true;
    }else{
      powerBooted = true;
      currentBootLevel = 0;
      currentLightLevel = powercellLedCount;
    }
    // END POWERCELL
  }

  // if we have changed an led
  if( doPowercellUpdate == true ){
    powerStick.show(); // commit all of the changes
  }
  
  // START CYCLOTRON
  bool doCycUpdate = false;
  if( useCyclotronFadeInEffect == false ){
    if ((unsigned long)(currentMillis - prevCycBootMillis) >= cyc_boot_interval) {
      prevCycBootMillis = currentMillis;

      if( reverseBootCyclotron == false ){
        setCyclotronLightState(c1Start, c1End, 1);
        setCyclotronLightState(c2Start, c2End, 2);
        setCyclotronLightState(c3Start, c3End, 1);
        setCyclotronLightState(c4Start, c4End, 2);
        
        doCycUpdate = true;
        reverseBootCyclotron = true;
      }else{
        setCyclotronLightState(c1Start, c1End, 2);
        setCyclotronLightState(c2Start, c2End, 1);
        setCyclotronLightState(c3Start, c3End, 2);
        setCyclotronLightState(c4Start, c4End, 1);
        
        doCycUpdate = true;
        reverseBootCyclotron = false;
      }
    }
  }else{
    if ((unsigned long)(currentMillis - prevCycBootMillis) >= cyc_boot_alt_interval) {
      prevCycBootMillis = currentMillis;
      setCyclotronLightState(c1Start, c4End, 4);
      doCycUpdate = true;
    }
  }

  if( doCycUpdate == true ){
    cyclotron.show(); // send to the neopixels
  }
  // END CYCLOTRON
}

int cycOrder = 0;
int cycFading = -1;

// normal animation on the bar graph
void powerSequenceOne(unsigned long currentMillis, unsigned long anispeed, unsigned long cycspeed, unsigned long cycfadespeed) {
  // START POWERCELL
  bool doPowercellUpdate = false;
  if ((unsigned long)(currentMillis - prevPwrMillis) >= anispeed) {
    // save the last time you blinked the LED
    prevPwrMillis = currentMillis;

    for ( int i = 0; i <= powercellLedCount; i++) {
      if ( i <= powerSeqNum ) {
        powerStick.setPixelColor(i, powerStick.Color(0, 0, 150));
      } else {
        powerStick.setPixelColor(i, 0);
      }
    }
    
    if ( powerSeqNum <= powercellLedCount) {
      powerSeqNum++;
    } else {
      powerSeqNum = 0;
    }

    // Update the leds
    powerStick.show();
  }
  
  // END POWERCELL
  
  // START CYCLOTRON 
  bool doCycUpdate = false;
  if( useGameCyclotronEffect == true ){
    // figure out main light
    if ((unsigned long)(currentMillis - prevCycMillis) >= cycspeed) {
      prevCycMillis = currentMillis;
      
      switch ( cycOrder ) {
        case 0:
          setCyclotronLightState(c1Start, c1End, 0);
          setCyclotronLightState(c2Start, c2End, 2);
          setCyclotronLightState(c3Start, c3End, 2);
          setCyclotronLightState(c4Start, c4End, 2);
          cycFading = 1;
          cyclotronRunningFadeOut = 255;
          cycOrder = 1;
          break;
        case 1:
          setCyclotronLightState(c1Start, c1End, 2);
          setCyclotronLightState(c2Start, c2End, 2);
          setCyclotronLightState(c3Start, c3End, 2);
          setCyclotronLightState(c4Start, c4End, 0);
          cycFading = 0;
          cyclotronRunningFadeOut = 255;
          cycOrder = 2;
          break;
        case 2:
          setCyclotronLightState(c1Start, c1End, 2);
          setCyclotronLightState(c2Start, c2End, 2);
          setCyclotronLightState(c3Start, c3End, 0);
          setCyclotronLightState(c4Start, c4End, 2);
          cycFading = 3;
          cyclotronRunningFadeOut = 255;
          cycOrder = 3;
          break;
        case 3:
          setCyclotronLightState(c1Start, c1End, 2);
          setCyclotronLightState(c2Start, c2End, 0);
          setCyclotronLightState(c3Start, c3End, 2);
          setCyclotronLightState(c4Start, c4End, 2);
          cycFading = 2;
          cyclotronRunningFadeOut = 255;
          cycOrder = 0;
          break;
      }
  
      doCycUpdate = true;
    }
  
    // now figure out the fading light
    if( (unsigned long)(currentMillis - prevFadeCycMillis) >= cycfadespeed ){
      prevFadeCycMillis = currentMillis;
      if( cycFading != -1 ){
        switch ( cycFading ) {
          case 0:
            setCyclotronLightState(c1Start, c1End, 3);
            break;
          case 1:
            setCyclotronLightState(c2Start, c2End, 3);
            break;
          case 2:
            setCyclotronLightState(c3Start, c3End, 3);
            break;
          case 3:
            setCyclotronLightState(c4Start, c4End, 3);
            break;
        }
        doCycUpdate = true;
      }
    }
  }else{
    // figure out main light
    if ((unsigned long)(currentMillis - prevCycMillis) >= cycspeed) {
      prevCycMillis = currentMillis;
      
      switch ( cycOrder ) {
        case 0:
          setCyclotronLightState(c1Start, c1End, 0);
          setCyclotronLightState(c2Start, c2End, 2);
          setCyclotronLightState(c3Start, c3End, 2);
          setCyclotronLightState(c4Start, c4End, 2);
          cycOrder = 1;
          break;
        case 1:
          setCyclotronLightState(c1Start, c1End, 2);
          setCyclotronLightState(c2Start, c2End, 2);
          setCyclotronLightState(c3Start, c3End, 2);
          setCyclotronLightState(c4Start, c4End, 0);
          cycOrder = 2;
          break;
        case 2:
          setCyclotronLightState(c1Start, c1End, 2);
          setCyclotronLightState(c2Start, c2End, 2);
          setCyclotronLightState(c3Start, c3End, 0);
          setCyclotronLightState(c4Start, c4End, 2);
          cycOrder = 3;
          break;
        case 3:
          setCyclotronLightState(c1Start, c1End, 2);
          setCyclotronLightState(c2Start, c2End, 0);
          setCyclotronLightState(c3Start, c3End, 2);
          setCyclotronLightState(c4Start, c4End, 2);
          cycOrder = 0;
          break;
      }
  
      doCycUpdate = true;
    }
  }
  
  if( doCycUpdate == true ){
    cyclotron.show(); // send to the neopixels
  }
  // END CYCLOTRON


  // START WAND FLASHER
  if (currentMillis -  prevWandFlasherMillis >= wand_flasher_interval) {
    // save the last time you blinked the LED
     prevWandFlasherMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (wandFlasherState == LOW) {
      wandFlasherState = HIGH;
    } else {
      wandFlasherState = LOW;
    }

    // set the LED with the ledState of the variable:
    io.digitalWrite(wandFlasherPin, wandFlasherState);
  }

}
