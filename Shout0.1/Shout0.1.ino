// Makes use of the Adafruit NeoPixel library and the Teensy Audio Library

#include <Adafruit_NeoPixel.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <HSBColor.h>


// Define pin and pixel number for LEDs
#define PIN            3
#define NUMPIXELS      150

#define LIMIT 0.5       //Sound levels over this will change the colour
#define HOLDTIME 500    //How long to hold the sound level
#define SPEEDLIMIT 3  //Max blinking speed
#define MICSENS 1.0     //Mic sensitivity


//setup the NeoPixel library, tell it how many pixels, and which pin to use.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Audio input
const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
AudioInputI2S          audioInput;
AudioAnalyzeFFT1024    myFFT;
AudioAnalyzePeak       peak;


// Connect audio connections
AudioConnection patchCord1(audioInput, 0, myFFT, 0);
AudioConnection patchCord2(audioInput, 0, peak, 0);


// Create an object to control the audio shield.
AudioControlSGTL5000 audioShield;

float red, green, blue;   // holds the brightness for each colour


float peakVol;
float newVol, prevVol;
const float Pi = 3.14159;     // Steak and cheese
bool up;

float hue, saturation, brightness;
float targetHue, targetSat, targetBright;
int colour[3];

unsigned long holdStart;
float blinkSpeed;
float blinkVal;

void setup() {

  Serial.begin(115200);
  Serial.println("Serial");


  pixels.begin(); // This initializes the NeoPixel library.
  Serial.println("pixels");

  AudioMemory(20);
  Serial.println("Audio Memory");

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  Serial.println("Shield");

  // Configure the window algorithm to use
  myFFT.windowFunction(AudioWindowHanning1024);
  Serial.println("FFT");

  saturation = 1.0;
  brightness = 1.0;
  
  for(int i = 0; i<3; i++){
    H2R_HSBtoRGBfloat(i*0.3, saturation, brightness, colour);
    for (int j = 0; j < NUMPIXELS; j++) {
      pixels.setPixelColor(j, pixels.Color(colour[0], colour[1], colour[2]));
      pixels.show();
      delay(5);
    }
  }
  

  Serial.println("setup done");


}
//-------------------------- end of setup  ----------------------------

//-------------------------- start of loop -----------------------------
void loop() {

  if (peak.available()) {
    
    newVol = peak.read()*MICSENS;
    
    
    
    if(newVol > LIMIT){   // if volume is over the limit
      blinkSpeed += 0.01;
      if(newVol<prevVol){   // if volume is decreasing
        if(holdVal()){          //if volume is being held
          newVol = prevVol;       //set the new volume to the previous volume
        }
        else{                 //if volume is not being held
          newVol = lerp(prevVol, newVol, 0.04); // fade down
        }
      }
      else{               //if the volume is increasing
        holdStart = millis();  // set new holdStart time
      }
      hue = mapFloat(newVol,LIMIT,1,1,0);
    }
    else{    // if volume is under the limit
      blinkSpeed -= 0.001;
      
    }
  }

//  blinking();
  fade();
  
  Serial.print("holdVal:");
  Serial.print(holdVal());
  Serial.print(" blinkSpeed:");
  Serial.print(blinkSpeed);
  Serial.print(" newVol:");
  Serial.print(newVol);
  for(int i=0; i<newVol*20; i++){
    Serial.print("-");
  }
  Serial.println("|");

  
  
  prevVol = newVol;
  
  saturation = 1.0;
  H2R_HSBtoRGBfloat(hue, saturation, brightness, colour);
//
//      Serial.print(" - red: ");
//      Serial.print(red);
//      Serial.print(" - green: ");
//      Serial.print(green);
//      Serial.print(" - blue: ");
//      Serial.println(blue);
  // apply colour to the pixels
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(colour[0], colour[1], colour[2]));
  }
  pixels.show();
}

// blinking function
void blinking(){
  if(blinkSpeed<0.001) blinkSpeed = 0.001;
  else if (blinkSpeed>SPEEDLIMIT) blinkSpeed = SPEEDLIMIT;
  blinkVal += blinkSpeed;

  if(blinkVal > 1) blinkVal = 0;
  if(blinkVal < 0.7) brightness = 1;
  else brightness = 0;
}

void fade(){
  
  if(blinkSpeed<0.1) blinkSpeed = 0.1;
  else if (blinkSpeed>SPEEDLIMIT) blinkSpeed = SPEEDLIMIT;

  
  if(brightness < 0){  //When dim, change  fade direction to up
    brightness = 0;
    up = true;
  }
  else if (brightness > 1){ //When bright, change fade direction to down
    brightness = 1;
    up = false;
  }

  if(up){  //When fading up, add fade amount
    brightness += blinkSpeed*0.1;
  }
  else{
    brightness -=blinkSpeed*0.01;  //When fading down, minus fade amount
  }
  
}

//hold function
bool holdVal(){
  bool result;
  if(millis() < holdStart + HOLDTIME) result = true;
  else result = false;
  return result;
}

//map function
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//lerp function
float lerp(float v0, float v1, float t) {
  return (1-t)*v0 + t*v1;
}



