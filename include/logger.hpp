#pragma once
#include "device.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

class Logger {
public:
  explicit Logger(const std::string &directory_path)
      : directory_path_(directory_path), running_(false) {
    if (!std::filesystem::exists(directory_path_)) {
      std::filesystem::create_directories(directory_path_);
    }
  }

  ~Logger() { stop(); }

  // 로그 쓰레드 시작
  void start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_)
      return; // 이미 실행 중이면 무시
    running_ = true;
    openNewFile();
    worker_ = std::thread(&Logger::processQueue, this);
  }

  // 로그 쓰레드 종료
  void stop() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!running_)
        return;
      running_ = false;
    }
    cv_.notify_all();
    if (worker_.joinable())
      worker_.join();
    if (file_.is_open())
      file_.close();
  }

  // 외부 스레드에서 데이터 추가
  void appendData(const PacketStruct &packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_)
      return; // 실행 중 아닐 때 무시
    queue_.push(packet);
    cv_.notify_one();
  }

private:
  std::string directory_path_;
  std::queue<PacketStruct> queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread worker_;
  std::atomic<bool> running_;
  std::ofstream file_;

  static std::string generateFilename() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".csv";
    return oss.str();
  }

  void openNewFile() {
    std::string filename = directory_path_ + "/" + generateFilename();
    file_.open(filename, std::ios::out);
    if (!file_.is_open()) {
      std::cerr << "Failed to open log file: " << filename << std::endl;
      return;
    }

    // CSV 헤더 작성
    file_ << "timestamp";
    for (int i = 0; i < 32; ++i)
      file_ << ",mic1_" << i;
    for (int i = 0; i < 32; ++i)
      file_ << ",mic2_" << i;
    file_ << "\n";
  }

  void processQueue() {
    while (true) {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [&]() { return !queue_.empty() || !running_; });

      if (!running_ && queue_.empty())
        break;

      while (!queue_.empty()) {
        PacketStruct packet = queue_.front();
        queue_.pop();
        lock.unlock();

        writePacket(packet);

        lock.lock();
      }
    }

    // 남은 데이터 모두 기록
    while (true) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (queue_.empty())
        break;
      writePacket(queue_.front());
      queue_.pop();
    }
  }

  void writePacket(const PacketStruct &packet) {
    if (!file_.is_open())
      return;

    // timestamp in milliseconds since epoch
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();

    file_ << ms;
    for (int i = 0; i < 32; ++i)
      file_ << "," << packet.mic1[i];
    for (int i = 0; i < 32; ++i)
      file_ << "," << packet.mic2[i];
    file_ << "\n";
    file_.flush();
  }
};
