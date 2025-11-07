#include "devices/device.hpp"
#include "process_thread.hpp"
#include "queue.hpp"
#include "serial_thread.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main() {

  ThreadSafeQueue<uint8_t> queue(PACKET_SIZE);

  SerialThread serial_thread(queue);
  ProcessThread process_thread(queue);
  process_thread.addCallback(
      [](PacketStruct packet) -> void { std::cout << packet << std::endl; });

  serial_thread.start();
  process_thread.start();

  sleep(10000);

  serial_thread.stop();
  process_thread.stop();

  std::cout << "Done.\n";
  return 0;
}
