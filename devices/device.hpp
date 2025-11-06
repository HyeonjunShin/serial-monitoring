#include <cstddef>
#include <cstdint>
#include <string_view>


class Device {
public:
  virtual ~Device() = default;
  virtual const uint16_t calcCRC(const uint8_t *data, size_t length) = 0;
  virtual const uint8_t *getHeader() const = 0;
  virtual const int getPacketSize() const = 0;
  virtual const uint16_t *getCRCTable() const = 0;
  virtual const std::string_view getSerialPort() const = 0;
  virtual const int getBaudRate() const = 0;

};