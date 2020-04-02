#include <Arduino.h>
#include "HMI.h"

/*
static uint16_t crc16(const uint8_t *data, uint8_t len, uint16_t crc = 0xFFFF) {
  while (len--) {
    crc ^= (uint16_t)*data++;
    for (uint8_t i = 0; i < 8; ++i)
      crc = crc & 0x0001 ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}
*/

bool HMI::writeReg(uint8_t start, uint8_t count, const uint8_t *data) {
  if ((! writeFrameHeader(count + 2, REG_WRITE)) || (! _uart->write(start)) || (_uart->write(data, count) != count))
    return false;
  _uart->flush();
  return true;
}

bool HMI::readReg(uint8_t start, uint8_t count, uint8_t *data) {
  while (_uart->available())
    (void)_uart->read();
  if ((! writeFrameHeader(3, REG_READ)) || (! _uart->write(start)) || (! _uart->write(count)))
    return false;
  _uart->flush();

  uint32_t begin = millis();

  while ((! readFrame()) || (_frame->cmd != REG_READ) || (_frame->regread.addr != start) || (_frame->regread.count != count)) {
    if (millis() - begin > FRAME_READ_TIMEOUT * 2)
      return false;
    delay(1);
  }
  memcpy(data, _frame->regread.data, count);
  return true;
}

bool HMI::writeMem(uint16_t start, uint8_t count, const uint16_t *data) {
  if ((! writeFrameHeader(count * 2 + 3, MEM_WRITE)) || (! _uart->write(start >> 8)) || (! _uart->write(start & 0xFF)))
    return false;
  for (uint8_t i = 0; i < count; ++i) {
    if ((! _uart->write(data[i] >> 8)) || (! _uart->write(data[i] & 0xFF)))
      return false;
  }
  _uart->flush();
  return true;
}

bool HMI::readMem(uint16_t start, uint8_t count, uint16_t *data) {
  while (_uart->available())
    (void)_uart->read();
  if ((! writeFrameHeader(4, MEM_READ)) || (! _uart->write(start >> 8)) || (! _uart->write(start & 0xFF)) || (! _uart->write(count)))
    return false;

  uint32_t begin = millis();

  while ((! readFrame()) || (_frame->cmd != MEM_READ) || (_frame->memread.addr != start) || (_frame->memread.count != count)) {
    if (millis() - begin > FRAME_READ_TIMEOUT * 2)
      return false;
    delay(1);
  }
  memcpy(data, _frame->memread.data, count * sizeof(uint16_t));
  return true;
}

bool HMI::readFrame() {
  freeFrame();
  if ((_uart->available() >= (int16_t)(sizeof(_header1) + sizeof(_header2) + sizeof(hmiframe_t::len))) &&
    (_uart->read() == _header1) && (_uart->read() == _header2)) {
    uint8_t len = _uart->read();
    uint32_t begin = millis();

    while ((_uart->available() < len) && (millis() - begin < FRAME_READ_TIMEOUT)) {
      delay(1);
    }
    if (_uart->available() >= len) {
      if ((_frame = (hmiframe_t*)malloc(sizeof(hmiframe_t::len) + len)) != NULL) {
        _frame->len = len;
        _frame->cmd = _uart->read();
        if (_frame->cmd == REG_READ) {
          _frame->regread.addr = _uart->read();
          _frame->regread.count = _uart->read();
          if ((_frame->regread.count != len - 3) || (_uart->readBytes(_frame->regread.data, _frame->regread.count) != _frame->regread.count)) {
            freeFrame();
          }
        } else if (_frame->cmd == MEM_READ) {
          _frame->memread.addr = ((uint16_t)_uart->read() << 8) | _uart->read();
          _frame->memread.count = _uart->read();
          if (_frame->memread.count * 2 == len - 4) {
            for (uint8_t i = 0; i < _frame->memread.count; ++i) {
              _frame->memread.data[i] = ((uint16_t)_uart->read() << 8) | _uart->read();
            }
          } else {
            freeFrame();
          }
        } else {
          if (_uart->readBytes(_frame->raw, _frame->len - sizeof(hmiframe_t::cmd)) != _frame->len - sizeof(hmiframe_t::cmd)) {
            freeFrame();
          }
        }
      }
    }
  }
  return (_frame != NULL);
}

void HMI::freeFrame() {
  if (_frame) {
    free(_frame);
    _frame = NULL;
  }
}

bool HMI::writeFrameHeader(uint8_t len, uint8_t cmd) {
  return (_uart->write(_header1) && _uart->write(_header2) && _uart->write(len) && _uart->write(cmd));
}
