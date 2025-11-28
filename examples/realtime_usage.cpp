#include "device.hpp"
#include "logger.hpp"
#include "piezo_serial.hpp"
#include <array>
#include <boost/circular_buffer.hpp>
#include <cstdint>
#include <iostream>
#include <queue>
#include <unistd.h>

int main(int argc, char *argv[]) {
  PiezoSerial piezoSerial;
  if (argc < 2) {
    std::string port = "/dev/ttyUSB0";
    piezoSerial.setPort(port);
  } else {
    piezoSerial.setPort(argv[1]);
  }

  boost::circular_buffer<int> cb(3);
  cb.push_back(1);
  cb.push_back(2);
  cb.push_back(3);
  cb.push_back(4); // 1 을 덮어씀

  // std::queue<std::array<uint16_t, 64>> queue(100);

  // piezoSerial.addCallback(
  // [](PacketStruct packet) -> void { std::cout << packet << std::endl; });
  // piezoSerial.addCallback(
  // [&logger](PacketStruct packet) -> void { logger.appendData(packet); });

  // piezoSerial.start();
  // sleep(10);
  // piezoSerial.stop();

  return 0;
}
