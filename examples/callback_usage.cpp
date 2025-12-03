#include "device.hpp"
#include "piezo_serial.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {
  PiezoSerial piezoSerial;
  if (argc < 2) {
    std::string port = "/dev/ttyUSB0";
    piezoSerial.setPort(port);
  } else {
    piezoSerial.setPort(argv[1]);
  }

  piezoSerial.addCallback([](PacketStruct packet) -> void {
    std::cout << packet << std::endl;
    // float cur = static_cast<float>(packet.tick) / MCU_CLOCK;
    // std::cout << cur << std::endl;
    // std::cout << cur << ", " << cur - prev << std::endl;
    // prev = cur;
  });

  piezoSerial.start();
  sleep(10);
  piezoSerial.stop();

  return 0;
}
