#include "serial_reader.hpp"
// #include "DataParser.hpp"
// #include "CsvWriter.hpp"
#include "config.hpp"
#include <boost/asio.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <thread>

std::ostream &operator<<(std::ostream &os, const DataPacket &packet) {
  os << "--- DataPacket Info ---" << std::endl;

  os << "Header: [";
  for (int i = 0; i < 4; ++i) {
    os << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(packet.header[i]) << (i == 3 ? "" : " ");
  }
  os << std::dec << "]" << std::endl; // 10진수로 복귀

  os << "Data Size: " << packet.data_size << " bytes" << std::endl;
  os << "Tick: " << packet.tick << std::endl;
  os << "Flag: " << (packet.flag ? "True" : "False") << std::endl;

  os << "Acceleration (X, Y, Z): (" << packet.acc[0] << ", " << packet.acc[1]
     << ", " << packet.acc[2] << ")" << std::endl;

  os << "Gyroscope (X, Y, Z): (" << packet.gyro[0] << ", " << packet.gyro[1]
     << ", " << packet.gyro[2] << ")" << std::endl;

  os << "Temperature: " << packet.temperature << " C" << std::endl;
  os << "Loadcell Value: " << packet.loadcell << std::endl;

  os << "Mic Data (first 5): [";
  for (int i = 0; i < 32; ++i) {
    // os << packet.mic[i] << (i == 4 ? "" : ", ");
    os << packet.mic[i] << ", ";
  }
  os << ", ...]" << std::endl;

  os << "CRC: 0x" << std::hex << std::setw(4) << std::setfill('0') << packet.crc
     << std::dec << std::endl;
  os << "-----------------------";
  return os;
}

uint16_t calcCRC(const uint8_t *data, size_t length,
                 const std::vector<uint16_t> &crc_table) {
  uint16_t crc = 0;
  for (size_t i = 0; i < length; ++i) {
    uint8_t idx = ((crc >> 8) ^ data[i]) & 0xFF;
    crc = ((crc << 8) ^ crc_table[idx]) & 0xFFFF;
  }
  return crc;
}

int main() {
  Config config;
  try {
    config = load_config("/workspaces/serial-monitoring/config.json");
  } catch (const std::exception &e) {
    std::cerr << "Error loading config: " << e.what() << std::endl;
    return 1;
  }

  const std::string PORT = config.serial_config.port;
  const int BAUD_RATE = config.serial_config.baud_rate;
  const std::vector<uint8_t> HEADER = config.packet_format.header;
  const std::vector<uint16_t> CRC_TABLE = config.packet_format.crc_table;
  const std::vector<PacketField> PACKET_STRUCTURE = config.packet_structure;

  boost::asio::io_context io_;
  boost::asio::serial_port serial(io_);

  try {
    serial.open(PORT);
    serial.set_option(boost::asio::serial_port_base::baud_rate(BAUD_RATE));
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
      if (memcmp(&buffer[i], HEADER.data(), 4) == 0) {
        DataPacket packet_vaild;
        memcpy(&packet_vaild, &buffer[i], PACKET_SIZE);
        if (calcCRC(reinterpret_cast<uint8_t *>(&packet_vaild), PACKET_SIZE - 2,
                    CRC_TABLE) == packet_vaild.crc) {
          buffer.erase(buffer.begin(), buffer.begin() + i + PACKET_SIZE);
          // return packet_vaild;
        }
        std::cout << packet_vaild << std::endl;
      }
    }
    // std::cout << buffer.size() << std::endl;
  }

  if (serial.is_open()) {
    serial.close();
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

  std::cout << "Done.\n";
  return 0;
}
