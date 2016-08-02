#ifndef __DATA_H__
#define __DATA_H__
#include "mbed.h"

class Data {
  int32_t _x;
  int32_t _y;
  int32_t _z;

public:
  Data() : _x(0), _y(0), _z(0) {};
  Data(int32_t x, int32_t y, int32_t z) : _x(x), _y(y), _z(z) {};

  Data operator+ (const Data& rhs) {
    _x += rhs.x();
    _y += rhs.y();
    _z += rhs.z();

    return *this;
  }

  Data operator/ (int32_t divisor) {
    _x /= divisor;
    _y /= divisor;
    _z /= divisor;

    return *this;
  }

  int32_t x() const {
    return _x;
  }

  int32_t  y() const {
    return _y;
  }

  int32_t z() const {
    return _z;
  }
};

#endif //__DATA_H__
