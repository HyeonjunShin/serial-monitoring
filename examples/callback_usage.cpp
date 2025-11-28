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

  piezoSerial.addCallback(
      [](PacketStruct packet) -> void { std::cout << packet << std::endl; });
  
  piezoSerial.start();
  sleep(100);
  piezoSerial.stop();

  return 0;
}
