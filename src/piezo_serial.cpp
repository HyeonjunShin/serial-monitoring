#include "piezo_serial.hpp"

class SerialThread {
public:
  SerialThread(ThreadSafeQueue<uint8_t> &q)
      : queue(q), running(false), serial(io_) {

    // boost::asio::io_context io_;
    // boost::asio::serial_port serial(io_);

    try {
      serial.open(SERIAL_PORT.data());
      serial.set_option(boost::asio::serial_port_base::baud_rate(BAUD_RATE));
      serial.set_option(boost::asio::serial_port_base::character_size(8));
      serial.set_option(boost::asio::serial_port_base::stop_bits(
          boost::asio::serial_port_base::stop_bits::one));
      serial.set_option(boost::asio::serial_port_base::parity(
          boost::asio::serial_port_base::parity::none));
    } catch (const std::exception &e) {
      std::cerr << "Serial open error: " << e.what() << std::endl;
      // return 1;
      stop();
      exit(1);
    }
  }

  ~SerialThread() { stop(); }

  void start() {
    running = true;
    worker = std::thread(&SerialThread::loop_, this);
  }

  void stop() {
    running = false;
    serial.close();
    if (worker.joinable())
      worker.join();
  }

private:
  void loop_() {
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
  }

  ThreadSafeQueue<uint8_t> &queue;
  std::thread worker;
  std::atomic<bool> running;
  boost::asio::io_context io_;
  boost::asio::serial_port serial;
};
