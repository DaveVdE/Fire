#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Protomatter.h> // For RGB matrix

#define WIDTH   64
#define HEIGHT  32
#define FIRE_WIDTH  32
#define FIRE_HEIGHT 65
#define MAX_FPS 25

// Remaining pins are the same for all matrix sizes. These values
// are for MatrixPortal M4. See "simple" example for other boards.
uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;

Adafruit_Protomatter matrix(
  WIDTH, 6, 1, rgbPins, sizeof(addrPins), addrPins,
  clockPin, latchPin, oePin, true);

uint16_t palette[256];

uint8_t fire[FIRE_HEIGHT][FIRE_WIDTH];

uint16_t HSLto565(float H, float S, float V) {
  float s = S/100;
  float v = V/100;
  float C = s*v;
  float X = C*(1-abs(fmod(H/60.0, 2)-1));
  float m = v-C;
  float r,g,b;

  if(H >= 0 && H < 60){
      r = C,g = X,b = 0;
  }
  else if(H >= 60 && H < 120){
      r = X,g = C,b = 0;
  }
  else if(H >= 120 && H < 180){
      r = 0,g = C,b = X;
  }
  else if(H >= 180 && H < 240){
      r = 0,g = X,b = C;
  }
  else if(H >= 240 && H < 300){
      r = X,g = 0,b = C;
  }
  else{
      r = C,g = 0,b = X;
  }
  int R = (r+m)*255;
  int G = (g+m)*255;
  int B = (b+m)*255;

  return matrix.color565(R, G, B);  
}

void setup() {
  // put your setup code here, to run once:
  ProtomatterStatus status = matrix.begin();
  if(status != PROTOMATTER_OK) {
    // DO NOT CONTINUE if matrix setup encountered an error.
    for(;;);
  }

  // Calculate palette
  for (int index = 0; index < 128; ++index) {
    palette[index] = HSLto565(index / 6, 100, (index * 2.0) * 100 / 256);
  }

  for (int index = 128; index < 256; ++index) {
    palette[index] = HSLto565(index / 6, (384 - index) * 100 / 256, 100);
  }

  // Clear buffer
  for (int y = 0; y < FIRE_HEIGHT; ++y) {
    for (int x = 0; x < FIRE_WIDTH; ++x) {
      fire[y][x] = 0;
    }
  }
}

uint32_t prevTime = 0;

void loop() {
  uint32_t t;
  while(((t = micros()) - prevTime) < (1000000L / MAX_FPS));
  prevTime = t;

  // Randomize bottom row
  for (int x = 0; x < FIRE_WIDTH; ++x) {
    int n = random(4) + 1;
    fire[FIRE_HEIGHT - 1][x] = n * n * n * n - 1;
  }

  //do the fire calculations for every pixel, from top to bottom
  for(int y = 0; y < FIRE_HEIGHT - 1; y++)
  for(int x = 0; x < FIRE_WIDTH; x++)
  {
    // Once in a great while, add in a white spot.
    if (random(100000000) < y * y * y * y / 1000) { // Desperate attempt to bias then to the bottom.
      fire[y][x] = 255;
    }
    else {
      fire[y][x] =
        ((fire[(y + 1) % FIRE_HEIGHT][(x - 1 + FIRE_WIDTH) % FIRE_WIDTH]
        + fire[(y + 1) % FIRE_HEIGHT][(x) % FIRE_WIDTH]
        + fire[(y + 1) % FIRE_HEIGHT][(x + 1) % FIRE_WIDTH]
        + fire[(y + 2) % FIRE_HEIGHT][(x) % FIRE_WIDTH])
        * 32) / 132;
    }
  }

  // Draw fire
  for (int y = 0; y < matrix.width(); ++y) {
    for (int x = 0; x < matrix.height(); ++x) {
      // Things are rotated 90 CCW
      matrix.drawPixel(y, x, palette[fire[y][x]]);
    }
  }

  matrix.show();
}