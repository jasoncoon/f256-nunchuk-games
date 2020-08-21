/*
   Fibonacci256 Nunchuk Games: https://github.com/jasoncoon/f256-nunchuk-games
   Copyright (C) 2020 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <FastLED.h>
FASTLED_USING_NAMESPACE

#include <NintendoExtensionCtrl.h> // https://github.com/dmadison/NintendoExtensionCtrl

#define DATA_PIN      D6
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      256

#define MILLI_AMPS         2000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  120  // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.

CRGB leds[NUM_LEDS];

uint8_t brightness = 32;

Nunchuk nunchuk;

uint8_t deadMin = 120;
uint8_t deadMax = 140;

#include "Map.h"

CHSV playerColor = CHSV(255, 255, 255);
uint8_t playerAngle = 0;
uint8_t playerWidth = 8;
uint8_t playerRadiusStart = 220;
uint8_t playerRadiusEnd = 255;

typedef struct {
  uint8_t angle;
  uint8_t radius;
  uint8_t width = 8;
  uint8_t height = 16;
  CRGB color;
  boolean active = false;
} Particle;
const uint8_t particleCount = 10;
Particle particles[particleCount];

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  nunchuk.begin();
  while (!nunchuk.connect()) {
    Serial.println("Nunchuk not detected!");
    delay(100);
  }

  Particle particle0 = particles[0];
  particle0.active = true;
  particle0.color = CRGB::Black;
  particle0.radius = 0;
  particles[0] = particle0;
}

void loop() {
  boolean movingLeft = false;
  boolean movingRight = false;
  boolean movingUp = false;
  boolean movingDown = false;

  boolean zPrevState = false;
  boolean firing = false;

  boolean success = nunchuk.update();  // Get new data from the controller

  if (!success) {  // Ruh roh
    Serial.println("Controller disconnected!");
  }
  else {
    boolean zButton = nunchuk.buttonZ();
    boolean cButton = nunchuk.buttonC();

    // Read the joystick axis (0-255)
    // Note: I havent seen it go near 0 or 255
    // I've seen ranges closer to 30-210
    uint8_t joyY = nunchuk.joyY();
    uint8_t joyX = nunchuk.joyX();

    // Read the accelerometer (0-1023)
    uint8_t accelX = nunchuk.accelX() / 4;
    uint8_t accelY = nunchuk.accelY() / 4;
    uint8_t accelZ = nunchuk.accelZ() / 4;

    //    Serial.print(joyX);
    //    Serial.print(", ");
    //    Serial.print(joyY);
    //    Serial.print(", ");
    //    Serial.print(accelX);
    //    Serial.print(", ");
    //    Serial.print(accelY);
    //    Serial.print(", ");
    //    Serial.print(accelZ);
    //    Serial.print(", ");
    //    Serial.print(zButton ? 255 : 0);
    //    Serial.print(", ");
    //    Serial.print(cButton ? 255 : 0);
    //    Serial.println();

    if (joyX > deadMax) {
      movingRight = true;
    }
    else if (joyX < deadMin) {
      movingLeft = true;
    }

    if (joyY > deadMax) {
      movingUp = true;
    }
    else if (joyY < deadMin) {
      movingDown = true;
    }

    if (zButton) {
      firing = true;
      zPrevState = true;
    } else if (!zButton) {
      zPrevState = false;
    }
  }

  if (movingLeft) {
    playerAngle--;
    //    Serial.println("left");
  }
  else if (movingRight) {
    playerAngle++;
    //    Serial.println("right");
  }

  if (firing) {
    Serial.println("firing");
    // find an inactive particle
    int particleIndex = -1;
    for (uint8_t i = 0; i < particleCount; i++) {
      if (particles[i].active) continue;
      particleIndex = i;
      break;
    }
    Serial.print("particleIndex: ");
    Serial.println(particleIndex);
    if (particleIndex > -1) {
      Particle particle = particles[particleIndex];
      particle.active = true;
      particle.radius = playerRadiusStart;
      particle.angle = playerAngle;
      particle.color = CRGB::Yellow;
      particles[particleIndex] = particle;
    }
  }

  fadeToBlackBy(leds, NUM_LEDS, 127);

  antialiasPixelAR(playerAngle, playerWidth, playerRadiusStart, playerRadiusEnd, playerColor);

  for (uint8_t i = 0; i < particleCount; i++) {
    Particle particle = particles[i];
    if (!particle.active) continue;

    //    Serial.print("particle index: ");
    //    Serial.print(i);
    //    Serial.print(", angle: ");
    //    Serial.print(particle.angle);
    //    Serial.print(", radius: ");
    //    Serial.println(particle.radius);

    antialiasPixelAR(particle.angle, particle.width, particle.radius, particle.radius + particle.height, particle.color);

    if (particle.radius > 0)
      particle.radius -= 4;
    else
      particle.active = false;

    particles[i] = particle;
  }

  FastLED.show();
  delay(1000 / FRAMES_PER_SECOND);
}
