#include "device.hpp"
#include "logger.hpp"
#include "piezo_serial.hpp"
#include "realtime_spectrogram_widget.hpp"
#include <iostream>
#include <qapplication.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  PiezoSerial piezoSerial;
  if (argc < 2) {
    std::string port = "/dev/ttyUSB0";
    piezoSerial.setPort(port);
  } else {
    piezoSerial.setPort(argv[1]);
  }

  QApplication app(argc, argv);

  RealtimeSpectrogramWidget widget;
  widget.resize(2000, 300);
  widget.show();

  Logger logger("./logs/");

  piezoSerial.addCallback(
      [](PacketStruct packet) -> void { std::cout << packet << std::endl; });
  piezoSerial.addCallback(
      [&logger](PacketStruct packet) -> void { logger.appendData(packet); });
  piezoSerial.addCallback(
      [&widget](PacketStruct packet) -> void { widget.appendData(packet); });

  logger.start();
  piezoSerial.start();

  // sleep(10);
  app.exec();

  piezoSerial.stop();
  logger.stop();

  return 0;
}
