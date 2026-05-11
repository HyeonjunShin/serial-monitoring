#include "piezo_serial.hpp"
#include "device.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <thread>

PiezoSerial::PiezoSerial()
    : serial(io_context), buffer_ring(2048), head(0), tail(0) {}

PiezoSerial::PiezoSerial(std::string serialPort) : PiezoSerial() {
  openPort(serialPort);
}

PiezoSerial::~PiezoSerial() { stop(); }

void PiezoSerial::openPort(std::string serialPort) {
  std::cout << serialPort << std::endl;
  try {
    serial.open(serialPort);
    serial.set_option(boost::asio::serial_port_base::baud_rate(BAUD_RATE));
    serial.set_option(boost::asio::serial_port_base::character_size(8));
    serial.set_option(boost::asio::serial_port_base::stop_bits(
        boost::asio::serial_port_base::stop_bits::one));
    serial.set_option(boost::asio::serial_port_base::parity(
        boost::asio::serial_port_base::parity::none));
  } catch (const std::exception &e) {
    std::cerr << "Serial open error: " << e.what() << std::endl;
    exit(1);
  }
}

void PiezoSerial::pushBytes(const uint8_t *target, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    buffer_ring[head] = target[i];
    head = (head + 1) % buffer_ring.size();

    if (head == tail) {
      tail = (tail + 1) % buffer_ring.size(); // overwrite oldest data
    }
  }
  // std::cout << available_bytes() << " bytes available. " << head << ", " <<
  // tail
  // << std::endl;
}
size_t PiezoSerial::available_bytes() {
  // 0 1 2 3 4 5 6 7 8 9
  //   ↑-------↑
  //  tail    head
  if (head >= tail)
    return head - tail;
  // 0 1 2 3 4 5 6 7 8 9
  // ↑-------↑         ↑
  // head            tail
  return buffer_ring.size() - tail + head;
}

uint8_t PiezoSerial::peek(size_t offset) {
  size_t index = (tail + offset) % buffer_ring.size();
  return buffer_ring[index];
}

bool PiezoSerial::find_header() {
  for (size_t i = 0; i < HEADER_SIZE; ++i) {
    if (peek(i) != HEADER[i]) {
      return false;
    }
  }
  return true;
}

void PiezoSerial::asyncRead() {
  serial.async_read_some(
      boost::asio::buffer(buffer_temp),
      [this](const boost::system::error_code &ec, std::size_t bytes) {
        if (!running || ec) {
          std::cerr << "Serial read stopped or error: " << ec.message()
                    << std::endl;
          return;
        }
        // std::cout << "Read: " << bytes << std::endl;
        // std::cout << buffer_temp << std::endl;
        pushBytes(buffer_temp, bytes);

        processPackets();
        asyncRead();
      });
}

void PiezoSerial::copy2packet(PacketStruct &packet) {
  uint8_t temp[PACKET_SIZE];
  for (size_t i = 0; i < PACKET_SIZE; ++i) {
    temp[i] = peek(i);
  }
  std::memcpy(&packet, temp, PACKET_SIZE);
}

bool PiezoSerial::verifyPacketInBuffer() {
  constexpr size_t DATA_SIZE = PACKET_SIZE - 2;
  uint8_t temp_for_crc[DATA_SIZE];

  for (size_t i = 0; i < DATA_SIZE; ++i) {
    temp_for_crc[i] = peek(i); // tail + i 위치의 데이터를 가져옴
  }

  uint16_t calculated_crc = calcCRC(temp_for_crc, DATA_SIZE);
  uint16_t expected_crc = peek(DATA_SIZE) | (peek(DATA_SIZE + 1) << 8);

  return calculated_crc == expected_crc;
}

void PiezoSerial::processPackets() {
  while (available_bytes() >= PACKET_SIZE) {
    if (!find_header()) {
      tail = (tail + 1) % buffer_ring.size();
      continue;
    }

    if (verifyPacketInBuffer()) {
      PacketStruct packet;
      copy2packet(packet);

      tail = (tail + PACKET_SIZE) % buffer_ring.size();
      for (auto &callback : callbacks) {
        callback(packet);
      }
    } else {
      tail = (tail + 1) % buffer_ring.size();
    }
  }
}

void PiezoSerial::addCallback(Callback callback) {
  callbacks.push_back(callback);
}

void PiezoSerial::start() {
  running = true;

  asyncRead();

  io_thread = std::thread([this]() {
    try {
      io_context.run();
    } catch (const std::exception &e) {
      std::cerr << "io_context error: " << e.what() << std::endl;
    }
  });
  // workerReader = std::thread(&PiezoSerial::readLoop, this);
  // workerProcessor = std::thread(&PiezoSerial::processLoop, this);
}

void PiezoSerial::stop() {
  running = false;

  // if (workerReader.joinable()) {
  //   workerReader.join();
  // }

  // if (workerProcessor.joinable()) {
  //   workerProcessor.join();
  // }

  if (io_thread.joinable()) {
    io_thread.join();
  }
  if (serial.is_open()) {
    serial.close();
  }
}

// void PiezoSerial::readLoop() {
//   std::vector<uint8_t> read_buf(1024);
//   while (running) {
//     boost::system::error_code ec;

//     try {
//       size_t n = serial.read_some(boost::asio::buffer(read_buf), ec);
//       if (ec) {
//         std::cerr << "Serial read error: " << ec.message() << std::endl;
//         break;
//       }

//       for (size_t i = 0; i < n; ++i) {
//         queue.push(read_buf[i]);
//       }

//     } catch (const std::exception &e) {
//       std::cerr << e.what() << std::endl;
//     }
//   }

//   stop();
// }

// void PiezoSerial::processLoop() {
//   uint8_t byte;
//   std::vector<uint8_t> buffer;
//   buffer.reserve(1024);

//   while (running) {
//     queue.wait_and_pop(byte);
//     buffer.push_back(byte);
//     // std::cout << buffer.size() << std::endl;

//     while (buffer.size() >= PACKET_SIZE) {
//       bool packet_found = false;

//       for (size_t i = 0; i + PACKET_SIZE <= buffer.size(); ++i) {
//         if (memcmp(&buffer[i], HEADER, HEADER_SIZE) == 0) {
//           PacketStruct packet_vaild;
//           memcpy(&packet_vaild, &buffer[i], PACKET_SIZE);

//           if (calcCRC(reinterpret_cast<uint8_t *>(&packet_vaild),
//                       PACKET_SIZE - 2) == packet_vaild.crc) {
//             buffer.erase(buffer.begin(), buffer.begin() + i + PACKET_SIZE);
//             // packet_vaild.tick = packet_vaild.tick / MCU_CLOCK
//             for (auto &callback : callbacks) {
//               // auto pkt_clone = packet_vaild.clone();
//               // std::cout << packet_vaild << std::endl;
//               callback(packet_vaild);
//             }
//           }

//           // if (device->calcCRC(reinterpret_cast<uint8_t *>(packet),
//           //                     PACKET_SIZE - 2) == packet_valid.crc) {
//           //   for (auto &callback : callbacks) {
//           //     auto pkt_clone = packet_valid.clone(); // 또는
//           //     std::make_shared callback(pkt_clone);
//           //   }

//           //   // 사용한 데이터 삭제
//           //   buffer.erase(buffer.begin(), buffer.begin() + i +
//           PACKET_SIZE);
//           //   packet_found = true;
//           //   break;
//           // } else {
//           //   // 헤더는 맞지만 CRC 실패 → 다음 위치 탐색
//           //   continue;
//           // }
//         }
//       }

//       // 유효한 패킷이 하나도 없으면 루프 탈출 (데이터 더 필요)
//       if (!packet_found)
//         break;
//     }
//   }
// }