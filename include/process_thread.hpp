#include "device.hpp"
#include "queue.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

class ProcessThread {
public:
  using Callback = std::function<void(PacketStruct packet)>;
  // void foo(const std::string& s); 와 같음

  ProcessThread(ThreadSafeQueue<uint8_t> &q) : queue(q), running(false) {}

  ~ProcessThread() { stop(); }

  void addCallback(Callback callback) {
    callbacks.push_back(std::move(callback));
  }

  void start() {
    running = true;
    worker = std::thread(&ProcessThread::loop_, this);
  }

  void stop() {
    running = false;
    if (worker.joinable())
      worker.join();
  }

private:
  void loop_() {
    uint8_t byte;
    std::vector<uint8_t> buffer;
    buffer.reserve(512);

    while (running) {
      queue.wait_and_pop(byte);
      buffer.push_back(byte);
      // std::cout << buffer.size() << std::endl;

      while (buffer.size() >= PACKET_SIZE) {
        bool packet_found = false;

        for (size_t i = 0; i + PACKET_SIZE <= buffer.size(); ++i) {
          if (memcmp(&buffer[i], HEADER, HEADER_SIZE) == 0) {
            PacketStruct packet_vaild;
            memcpy(&packet_vaild, &buffer[i], PACKET_SIZE);

            if (calcCRC(reinterpret_cast<uint8_t *>(&packet_vaild),
                        PACKET_SIZE - 2) == packet_vaild.crc) {
              buffer.erase(buffer.begin(), buffer.begin() + i + PACKET_SIZE);

              for (auto &callback : callbacks) {
                // auto pkt_clone = packet_vaild.clone();
                // std::cout << packet_vaild << std::endl;
                callback(packet_vaild);
              }
            }

            // if (device->calcCRC(reinterpret_cast<uint8_t *>(packet),
            //                     PACKET_SIZE - 2) == packet_valid.crc) {
            //   for (auto &callback : callbacks) {
            //     auto pkt_clone = packet_valid.clone(); // 또는
            //     std::make_shared callback(pkt_clone);
            //   }

            //   // 사용한 데이터 삭제
            //   buffer.erase(buffer.begin(), buffer.begin() + i + PACKET_SIZE);
            //   packet_found = true;
            //   break;
            // } else {
            //   // 헤더는 맞지만 CRC 실패 → 다음 위치 탐색
            //   continue;
            // }
          }
        }

        // 유효한 패킷이 하나도 없으면 루프 탈출 (데이터 더 필요)
        if (!packet_found)
          break;
      }

      // if (bytes.size() == packet->size()) {
      //   if (packet->deserialize(bytes)) {
      //     for (auto &callback : callbacks)
      //       callback(packet->clone());
      //   }
      //   bytes.clear();
      //   std::cout << bytes.size() << std::endl;
      // }

      // DataPacket packet_vaild;
      // memcpy(&packet_vaild, &buffer[i], PACKET_SIZE);
    }
  }

  ThreadSafeQueue<uint8_t> &queue;
  std::thread worker;
  std::atomic<bool> running;
  std::vector<Callback> callbacks;
};

//   for (size_t i = 0; i + PACKET_SIZE <= buffer.size(); ++i) {
//     // byte stream의 길이가 packet을 decoding 할 수 있는만큼 쌓였는가를 확인
//     if (memcmp(&buffer[i], HEADER, 4) == 0) {
//       DataPacket packet_vaild;
//       memcpy(&packet_vaild, &buffer[i], PACKET_SIZE);
//       if (device->calcCRC(reinterpret_cast<uint8_t *>(&packet_vaild),
//                           PACKET_SIZE - 2) == packet_vaild.crc) {
//         buffer.erase(buffer.begin(), buffer.begin() + i + PACKET_SIZE);
//         // return packet_vaild;
//       }
//       std::cout << packet_vaild << std::endl;
//     }
//   }
//   // std::cout << buffer.size() << std::endl;
// }
