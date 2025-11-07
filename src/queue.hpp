#pragma once
#include <condition_variable>
#include <cstddef>
// #include <iostream>
#include <mutex>
#include <queue>

template <typename T> class ThreadSafeQueue {
public:
  ThreadSafeQueue(size_t packet_size) : packet_size(packet_size) {}

  void push(T value) {
    std::lock_guard<std::mutex> lock(mtx);
    q.push(std::move(value));

    if (!(q.size() < packet_size))
      cv.notify_one();

    // std::cout << q.size() << std::endl;
  }

  bool try_pop(T &value) {
    std::lock_guard<std::mutex> lock(mtx);
    if (q.empty())
      return false;
    value = std::move(q.front());
    q.pop();
    return true;
  }

  void wait_and_pop(T &value) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !(q.size() < packet_size); });
    // n개 이상이면 동작
    // cv.wait(lock, condition)에서 condition이 false면 thread를 중지(sleep)
    value = std::move(q.front());
    q.pop();

    // std::cout << q.size() << std::endl;
  }

  int size() {
    std::lock_guard<std::mutex> lock(mtx);
    return q.size();
  }

private:
  std::queue<T> q;
  std::mutex mtx;
  std::condition_variable cv;
  size_t packet_size;
};
