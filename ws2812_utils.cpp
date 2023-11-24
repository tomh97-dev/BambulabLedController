#include <Arduino.h>
#include "ws2812_utils.h"
// #include <FastLED.h>

// #define MAX_LEDS 16
// #define DATA_PIN D4
// CRGB leds[MAX_LEDS];

// void initialiseWS2812(int ledcount) {
//     Serial.println("Initialising WS2812 Driver...");
//     FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, ledcount);
//     for(int i = 0; i < ledcount; i++) {
//         leds[i] = CRGB::Red;
//     }
//     FastLED.clear();
//     FastLED.show();
// }

void setws2812Color(int red, int green, int blueValue, int blue, int brightness) { // Fixed color
  
}

void pulsews2812Color(int redValue, int greenValue, int blueValue, int coldValue, int warmValue) { //Function to change ledstrip color
    // if (redValue == startR && greenValue == startG && blueValue == startB && coldValue == startC && warmValue == startW) {
    //     return;
    // }
    // int oldR = startR;
    // int oldG = startG;
    // int oldB = startB;
    // int oldC = startC;
    // int oldW = startW;
    // transitionws2812Color(redValue,greenValue,blueValue,coldValue,warmValue,50);
    // transitionws2812Color(oldR,oldG,oldB,oldC,oldW,50);
}

void setws2812Pins(int redValue, int greenValue, int blueValue, int coldValue, int warmValue) { //Function to change lestrip pins
    // pinMode(LED_PIN_R, OUTPUT);
    // pinMode(LED_PIN_G, OUTPUT);
    // pinMode(LED_PIN_B, OUTPUT);
    // pinMode(LED_PIN_W, OUTPUT);
    // pinMode(LED_PIN_WW, OUTPUT);

    // analogWrite(LED_PIN_R, redValue);
    // analogWrite(LED_PIN_G, greenValue);
    // analogWrite(LED_PIN_B, blueValue);
    // analogWrite(LED_PIN_W, coldValue);
    // analogWrite(LED_PIN_WW, warmValue);
}

void transitionws2812Color(int endR, int endG, int endB, int endC, int endW, int duration) { //Function to tween color change
    // float stepTime = (float)duration / 255.0;
    // int rStep = (endR - startR) / 255;
    // int gStep = (endG - startG) / 255;
    // int bStep = (endB - startB) / 255;
    // int cStep = (endC - startC) / 255;
    // int wStep = (endW - startW) / 255;
    
    // for (int i = 0; i < 256; i++) {
    //     int r = startR + i * rStep;
    //     int g = startG + i * gStep;
    //     int b = startB + i * bStep;
    //     int c = startC + i * cStep;
    //     int w = startW + i * wStep;
        
    //     setws2812Pins(r, g, b, c, w);
    //     delay(stepTime);
    // }
    // startR = endR;
    // startG = endG;
    // startB = endB;
    // startC = endC;
    // startW = endW;
}