#ifndef __HMI_H
#define __HMI_H

#include <Stream.h>

class HMI {
public:
  static const uint8_t REG_WRITE = 0x80;
  static const uint8_t REG_READ = 0x81;
  static const uint8_t MEM_WRITE = 0x82;
  static const uint8_t MEM_READ = 0x83;

  struct __attribute__((__packed__)) hmiframe_t {
    uint8_t len;
    uint8_t cmd;
    union {
      struct __attribute__((__packed__)) {
        uint8_t addr;
        uint8_t count;
        uint8_t data[];
      } regread;
      struct __attribute__((__packed__)) {
        uint16_t addr;
        uint8_t count;
        uint16_t data[];
      } memread;
      uint8_t raw[];
    };
  };

  HMI(Stream &uart, uint8_t header1 = 0xA5, uint8_t header2 = 0x5A) : _uart(&uart), _frame(NULL), _header1(header1), _header2(header2) {}
  ~HMI() {
    freeFrame();
  }

  bool writeReg(uint8_t start, uint8_t count, const uint8_t *data);
  bool readReg(uint8_t start, uint8_t count, uint8_t *data);
  bool writeMem(uint16_t start, uint8_t count, const uint16_t *data);
  bool readMem(uint16_t start, uint8_t count, uint16_t *data);
  bool readFrame();
  hmiframe_t *getFrame() const {
    return _frame;
  }

protected:
  static const uint32_t FRAME_READ_TIMEOUT = 100; // 100 ms

  void freeFrame();
  bool writeFrameHeader(uint8_t len, uint8_t cmd);

  Stream *_uart;
  hmiframe_t *_frame;
  uint8_t _header1, _header2;
};

#endif
