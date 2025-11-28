#include "device.hpp"
#include "logger.hpp"
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

  Logger logger("./logs/");

  piezoSerial.addCallback(
      [](PacketStruct packet) -> void { std::cout << packet << std::endl; });
  piezoSerial.addCallback(
      [&logger](PacketStruct packet) -> void { logger.appendData(packet); });

  logger.start();
  piezoSerial.start();
  sleep(10);
  piezoSerial.stop();
  logger.stop();

  return 0;
}
