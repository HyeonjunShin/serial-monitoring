#include "devices/d1.hpp"
// #include "devices/d2.hpp"
#include <boost/asio.hpp>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <pthread.h>

int main() {
  Device *device = new DeviceImpl();

  boost::asio::io_context io_;
  boost::asio::serial_port serial(io_);

  try {
    serial.open(device->getSerialPort().data());
    serial.set_option(
        boost::asio::serial_port_base::baud_rate(device->getBaudRate()));
    serial.set_option(boost::asio::serial_port_base::character_size(8));
    serial.set_option(boost::asio::serial_port_base::stop_bits(
        boost::asio::serial_port_base::stop_bits::one));
    serial.set_option(boost::asio::serial_port_base::parity(
        boost::asio::serial_port_base::parity::none));
  } catch (const std::exception &e) {
    std::cerr << "Serial open error: " << e.what() << std::endl;
    return 1;
  }

  std::vector<uint8_t> buffer;
  while (true) {

    try {
      uint8_t byte;
      size_t n = serial.read_some(boost::asio::buffer(&byte, 1));
      if (n > 0)
        buffer.push_back(byte);
    } catch (const std::exception &) {
      std::cout << "ajsklajkasldfjklsadjfslkj" << std::endl;
    }

    for (size_t i = 0; i + PACKET_SIZE <= buffer.size(); ++i) {
      // byte stream의 길이가 packet을 decoding 할 수 있는만큼 쌓였는가를 확인
      if (memcmp(&buffer[i], HEADER, 4) == 0) {
        DataPacket packet_vaild;
        memcpy(&packet_vaild, &buffer[i], PACKET_SIZE);
        if (device->calcCRC(reinterpret_cast<uint8_t *>(&packet_vaild),
                            PACKET_SIZE - 2) == packet_vaild.crc) {
          buffer.erase(buffer.begin(), buffer.begin() + i + PACKET_SIZE);
          // return packet_vaild;
        }
        std::cout << packet_vaild << std::endl;
      }
    }
    // std::cout << buffer.size() << std::endl;
  }

  // SerialReader serial("/dev/ttyUSB0", 921600);
  // if (!serial.open()) return 1;

  // CsvWriter csv("output.csv");
  // std::vector<uint8_t> buffer;

  // auto start = std::chrono::steady_clock::now();
  // while (std::chrono::steady_clock::now() - start <
  // std::chrono::seconds(1000)) {
  //     if (!serial.readBytes(buffer)) continue;

  //     auto pkt = DataParser::tryParse(buffer);
  //     if (pkt.has_value()) {
  //         csv.write(pkt.value());
  //         std::cout << "Tick: " << pkt->tick << " CRC OK\n";
  //     }
  // }

  if (serial.is_open()) {
    serial.close();
  }

  std::cout << "Done.\n";
  return 0;
}
