// set mode to station and save AT+CWMODE_DEF=1
// join wifi AT+CWJAP="titania","puckpuck"
// set static ip and save AT+CIPSTA_DEF="192.168.0.2x"
// 
// check ip address AT+CIFSR
// start udp connection AT+CIPSTART="UDP","<remote ip>",<remote port>,<local port>,0
// AT+CIPSTART="UDP","192.168.0.100",55056,55057,0


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


bool titanState = false;           // Be controlled by titania
unsigned int waitTime = 60;        // seconds to wait until change state

String recBuffer;
unsigned long recTime;  //time last message received
bool wifiConnected;

void setup() {

  Serial.begin(115200);
  Serial.println("Serial");

  //wifi chip uses serial1
  Serial1.begin(115200);
  wifiConnected = false;

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

  // wait for 60 seconds before opening udp connection (needs time to connect)
  if(millis() > 60000 && wifiConnected == false){
    Serial1.write("AT+CIPSTART=\"UDP\",\"192.168.0.50\",55056,55057,0\r\n");
    wifiConnected = true;
  }

  stateTimer(); // Change states on timer
  //read and send messages if there are any
  readMessage();
  sendMessage();

  // Titania state
  if (titanState) {
    hue = fade(hue, targetHue, 0.003);
    saturation = fade(saturation, targetSat, 0.003);
    brightness = fade(brightness, targetBright, 0.003);         
    

    H2R_HSBtoRGBfloat(hue, saturation, brightness, colour);

  } //--end of titania state

  else if (!titanState){
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
    
    breathe();
    
    
  //  Serial.print("holdVal:");
  //  Serial.print(holdVal());
  //  Serial.print(" blinkSpeed:");
  //  Serial.print(blinkSpeed);
  //  Serial.print(" newVol:");
  //  Serial.print(newVol);
  //  for(int i=0; i<newVol*20; i++){
  //    Serial.print("-");
  //  }
  //  Serial.println("|");
  
    
    
    prevVol = newVol;
    
    saturation = 1.0;
    H2R_HSBtoRGBfloat(hue, saturation, brightness, colour);
  }
  //--end of peak state

  

  // apply colour to the pixels
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(colour[0], colour[1], colour[2]));
  }
  pixels.show();
}


void breathe(){
  
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

//fade function for titania state
float fade(float current, float target, float amount){
  float result;
  if(current < target){
    result = current + amount;
    if(result > target){
      result = target;
    }
  }
  else if(current > target){
    result = current - amount;
    if(result < target){
      result = target;
    }
  }
  else if(current == target){
    result = target;
  }
  return result;
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

void readMessage(){
  // Send bytes from ESP8266 -> Teensy to Computer
  while ( Serial1.available() ) {
    char received = Serial1.read();
    recBuffer += received;  // add char from serial to buffer

    if (received == '\n'){  // do stuff with the message when line ends
      Serial.print(recBuffer);

//      recBuffer = "";
//    }
    // message from titania
//    else if(received == '\0'){
//      Serial.print(recBuffer);
      // when a message is received 
      if (recBuffer.startsWith("+IPD,5:")){  //udp messages start with this string
        recTime = millis();
        String message = recBuffer;
        message.remove(0,7);    // take out the "+IPD,5:" part of the message
        
        Serial.print(message);
        
        if(message.charAt(3)=='\n'){
          if(!titanState){
            titanState = true;
            Serial.print("state: Titania");
          }
          
          if(message.charAt(0)=='w'){
            targetSat = 0;
            targetBright = 0.8;
          }

          else if(message.charAt(0)=='b'){
            targetBright = 0;
          }
          
          else{
            String hueString = message;
            hueString.remove(3);
            int huein = hueString.toInt();
            targetHue = mapFloat(huein,0,359,0,1);
            targetSat = 1;
            targetBright = 1;
          }
        }
      }
      recBuffer = "";
    }
  }
}

void sendMessage(){
  if ( Serial.available() ) {
    Serial1.write( Serial.read() );
  }
}

//state change timer
void stateTimer(){
  if(millis() > recTime + waitTime*1000 && titanState){
    titanState = false;
    Serial.print("State: Peak");
  }
}

