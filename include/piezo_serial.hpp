
#include "queue.hpp"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <cstdint>
#include <thread>

class PiezoSerial {
private:
  std::thread worker;
  std::atomic<bool> running;
  boost::asio::io_context io;
  boost::asio::serial_port serial;
  ThreadSafeQueue<uint8_t> queue;

  void initSerial();
  void readLoop();
  void processLoop();

public:
  PiezoSerial();
  PiezoSerial(std::string serialPort);
  PiezoSerial(std::string serialPort, ThreadSafeQueue<uint8_t> &queue);
  ~PiezoSerial();

  void start();
  void stop();

  void setPort(std::string serialPort);
  void setQueue(ThreadSafeQueue<uint8_t> &queue);
};