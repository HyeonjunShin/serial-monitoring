#include "piezo_serial.hpp"
#include <iostream>
#include <thread>

PiezoSerial::PiezoSerial() : serial(io), queue(PACKET_SIZE) {}
PiezoSerial::PiezoSerial(std::string serialPort)
    : serial(io), queue(PACKET_SIZE) {
  setPort(serialPort);
}

PiezoSerial::~PiezoSerial() { stop(); }
void PiezoSerial::setPort(std::string serialPort) {
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

void PiezoSerial::addCallback(Callback callback) {
  callbacks.push_back(callback);
}

void PiezoSerial::start() {
  running = true;
  workerReader = std::thread(&PiezoSerial::readLoop, this);
  workerProcessor = std::thread(&PiezoSerial::processLoop, this);
}

void PiezoSerial::stop() {
  running = false;
  if (workerReader.joinable()) {
    workerReader.join();
  }
  if (workerProcessor.joinable()) {
    workerProcessor.join();
  }
  serial.close();
}

void PiezoSerial::readLoop() {
  std::vector<uint8_t> read_buf(1024);
  while (running) {
    boost::system::error_code ec;

    try {
      size_t n = serial.read_some(boost::asio::buffer(read_buf), ec);
      if (ec) {
        std::cerr << "Serial read error: " << ec.message() << std::endl;
        break;
      }

      for (size_t i = 0; i < n; ++i) {
        queue.push(read_buf[i]);
      }

    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }
  stop();
}

void PiezoSerial::processLoop() {
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
  }
}