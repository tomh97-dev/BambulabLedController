// #include <Arduino.h>
// #include "ws2812_utils.h"
// #include <FastLED.h>

// void updateLed() {
//   static unsigned long lastUpdate = 0;
//   const long interval = 1000; // Example interval

//   if (millis() - lastUpdate >= interval) {
//     lastUpdate = millis();

//     switch (currentLedState) {
//       case NORMAL_OPERATION:
//         // Set LEDs for normal operation
//         break;
//       case ERROR_STATE:
//         // Blink LEDs or set to red for error
//         break;
//       case WARNING_STATE:
//         // Different LED pattern for warning
//         break;
//       // Add other cases as needed
//     }
//   }
// }