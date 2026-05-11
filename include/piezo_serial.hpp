
#include "device.hpp"
#include "queue.hpp"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <cstddef>
#include <cstdint>
#include <thread>

using Callback = std::function<void(PacketStruct packet)>;

class PiezoSerial {
private:
  std::thread io_thread;

  boost::asio::io_context io_context;
  boost::asio::serial_port serial;
  // Buffers
  uint8_t buffer_temp[1024];
  std::vector<uint8_t> buffer_ring;
  size_t head;
  size_t tail;

  // std::vector<uint8_t> buffer_reader;

  // std::thread workerReader;
  // std::thread workerProcessor;
  std::atomic<bool> running;
  // ThreadSafeQueue<uint8_t> queue;
  std::vector<Callback> callbacks;

  void initSerial();
  void pushBytes(const uint8_t *target, size_t length);
  size_t available_bytes();
  uint8_t peek(size_t offset);
  bool find_header();
  void copy2packet(PacketStruct &packet);
  bool verifyPacketInBuffer();

  // void readLoop();
  // void processLoop();
  void asyncRead();
  void processPackets();

public:
  PiezoSerial();
  PiezoSerial(std::string serialPort);
  ~PiezoSerial();

  void start();
  void stop();

  void openPort(std::string port);
  void addCallback(Callback callback);
};