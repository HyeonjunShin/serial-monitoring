#include "device.hpp"
#include "process_thread.hpp"
#include "queue.hpp"
#include "realtime_spectrogram_widget.hpp"
#include "serial_thread.hpp"
#include <QApplication>
#include <QTimer>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {

  QApplication app(argc, argv);

  RealtimeSpectrogramWidget widget;
  widget.resize(1000, 600);
  widget.show();

  ThreadSafeQueue<uint8_t> queue(PACKET_SIZE);

  SerialThread serial_thread(queue);
  ProcessThread process_thread(queue);
  process_thread.addCallback(
      [](PacketStruct packet) -> void { std::cout << packet << std::endl; });
  process_thread.addCallback([&widget](PacketStruct packet) -> void {
    widget.appendData(packet);
  });

  serial_thread.start();
  process_thread.start();

  // sleep(10000);
  app.exec();

  serial_thread.stop();
  process_thread.stop();

  std::cout << "Done.\n";

  return 1;
}
