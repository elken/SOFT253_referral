#include <atomic>
#include <cstddef>

template <class T, int32_t m_capacity>
class Buffer {
  int32_t m_size;
  std::atomic<int32_t> m_head;
  std::atomic<int32_t> m_tail;

  T* m_data;

public:
  Buffer() : m_size(0), m_head(0), m_tail(0) {
    m_data = new T[m_capacity];
  }
  ~Buffer();

  int32_t size() {
    return m_size;
  }

  int32_t capacity() {
    return m_capacity;
  }

  bool push(T& item) {
    int32_t head = m_head.load(std::memory_order_relaxed);
    int32_t nextHead = (head + 1) % m_capacity;

    if (nextHead == m_tail.load(std::memory_order_acquire)) {
      return false;
    }

    m_data[head] = item;
    m_head.store(nextHead, std::memory_order_release);

    m_size++;
    return true;

    // if ((m_head == 0 && m_tail == m_capacity) || m_head == m_tail + 1) {
    //   return false;
    // } else if (m_head == -1 && m_tail == -1) {
    //   m_head = 0;
    //   m_tail = 0;
    //   m_data[m_head] = item;
    //   m_size++;
    // } else if (m_tail == m_size) {
    //   m_tail = 0;
    //   m_data[m_tail] = item;
    //   m_size++;
    // }
  }

  bool pop(T& item) {
    int32_t tail = m_tail.load(std::memory_order_relaxed);
    if (tail == m_head.load(std::memory_order_acquire)) {
      return false;
    }

    item = m_data[tail];
    m_tail.store((tail + 1) % m_capacity, std::memory_order_release);

    m_size--;
    return true;
    // T result = nullptr;
    // if (m_head == -1 && m_tail == -1) {
    //   return result;
    // } else {
    //   if (m_head == m_tail) {
    //     m_data[m_head] = 0;
    //     m_head = -1;
    //     m_tail = -1;
    //   }
    // }
  }
};
