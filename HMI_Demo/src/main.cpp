//#define DEBUG

#include <Arduino.h>
#include <Stream.h>
#ifdef DEBUG
#include <SoftwareSerial.h>
#endif
#include "HMI.h"

#ifdef DEBUG
SoftwareSerial uart(2, 3);
HMI hmi(uart);
#else
HMI hmi(Serial);
#endif

#ifdef DEBUG
static void printHex(uint8_t b) {
  uint8_t d;

  d = b >> 4;
  if (d < 10)
    Serial.write('0' + d);
  else
    Serial.write('A' + d - 10);
  d = b & 0x0F;
  if (d < 10)
    Serial.write('0' + d);
  else
    Serial.write('A' + d - 10);
}
#endif

static void setBrightness(uint8_t bright) {
  bright = constrain(bright, 1, 64);
  hmi.writeReg(0x01, 1, &bright);
}

void setup() {
  Serial.begin(115200);
#ifdef DEBUG
  uart.begin(115200);
#endif

#ifdef DEBUG
  for (uint8_t i = 4; i <= 13; ++i) {
#else
  for (uint8_t i = 2; i <= 13; ++i) {
#endif
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  for (uint8_t i = A0; i <= A5; ++i) {
    pinMode(i, INPUT);
  }
  {
    uint16_t data;

    delay(1000);
    data = 0xA55A;
    hmi.writeReg(0xEE, 2, (uint8_t*)&data); // Reset
    delay(2000);
    data = 32;
    hmi.writeMem(0x0200, 1, &data);
    setBrightness(data);
  }
  {
    uint8_t ver;

    if (hmi.readReg(0x00, 1, &ver)) {
      uint16_t verw = (ver >> 4) * 10 + (ver & 0x0F);

      hmi.writeMem(0x0300, 1, &verw);
    }
  }
}

void loop() {
  const uint32_t PERIOD = 500; // 500 ms.

  static uint32_t lastRead = 0;

  while (hmi.readFrame()) {
    HMI::hmiframe_t *frame = hmi.getFrame();

    if (frame->cmd == HMI::REG_READ) {
#ifdef DEBUG
      Serial.print(F("REG_READ from address "));
      printHex(frame->regread.addr);
      Serial.print(F(" count "));
      Serial.print(frame->regread.count);
      Serial.print(':');
      for (uint8_t i = 0; i < frame->regread.count; ++i) {
        Serial.print(' ');
        printHex(frame->regread.data[i]);
      }
      Serial.println();
#endif
    } else if (frame->cmd == HMI::MEM_READ) {
#ifdef DEBUG
      Serial.print(F("MEM_READ from address "));
      printHex(frame->memread.addr >> 8);
      printHex(frame->memread.addr & 0xFF);
      Serial.print(F(" count "));
      Serial.print(frame->memread.count);
      Serial.print(':');
      for (uint8_t i = 0; i < frame->memread.count; ++i) {
        Serial.print(' ');
        printHex(frame->memread.data[i] >> 8);
        printHex(frame->memread.data[i] & 0xFF);
      }
      Serial.println();
#endif
      if (frame->memread.addr == 0) {
        uint16_t value = frame->memread.data[0];

#ifdef DEBUG
        for (uint8_t i = 4; i <= 13; ++i) {
#else
        for (uint8_t i = 2; i <= 13; ++i) {
#endif
          digitalWrite(i, (value >> i) & 0x01);
        }
        {
          uint8_t duration = 2;

          hmi.writeReg(0x02, 1, &duration);
        }
      } else if (frame->memread.addr == 0x0200) {
        setBrightness(frame->memread.data[0]);
        {
          uint8_t duration = 1;

          hmi.writeReg(0x02, 1, &duration);
        }
      }
    }
  }

  if ((! lastRead) || (millis() - lastRead >= PERIOD)) {
    int16_t inputs[6];

    for (uint8_t i = 0; i < 6; ++i) {
      inputs[i] = analogRead(A0 + i);
    }
    hmi.writeMem(0x0100, 6, (uint16_t*)inputs);
    lastRead = millis();
  }
}
