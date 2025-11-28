
#include "device.hpp"
#include "queue.hpp"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <cstdint>
#include <thread>

using Callback = std::function<void(PacketStruct packet)>;

class PiezoSerial {
private:
  std::thread workerReader;
  std::thread workerProcessor;
  std::atomic<bool> running;
  boost::asio::io_context io;
  boost::asio::serial_port serial;
  ThreadSafeQueue<uint8_t> queue;
  std::vector<Callback> callbacks;

  void initSerial();
  void readLoop();
  void processLoop();

public:
  PiezoSerial();
  PiezoSerial(std::string serialPort);
  ~PiezoSerial();

  void start();
  void stop();

  void setPort(std::string serialPort);
  void addCallback(Callback callback);
};