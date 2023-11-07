#pragma once

#include <cassert>
#include <cstddef>
#include <memory>

template <typename T, typename Deleter = std::default_delete<T>>
class shared_ptr {
public:
  shared_ptr() noexcept = default;

  ~shared_ptr() {
    release_ref();
  }

  shared_ptr(std::nullptr_t) noexcept {}

  explicit shared_ptr(T* ptr) {
    try {
      p_data = new buffer(ptr);
    } catch (...) {
      Deleter d;
      d(ptr);
      throw;
    }
    add_ref();
  }

  shared_ptr(T* ptr, Deleter deleter) {
    try {
      p_data = new buffer(ptr, deleter);
    } catch (...) {
      deleter(ptr);
      throw;
    }
    add_ref();
  }

  shared_ptr(const shared_ptr& other) noexcept {
    *this = other;
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    if (this != &other) {
      release_ref();
      p_data = other.p_data;
      add_ref();
    }
    return *this;
  }

  T* get() const noexcept {
    if (p_data != nullptr) {
      return p_data->data;
    }
    return nullptr;
  }

  explicit operator bool() const noexcept {
    if (p_data == nullptr) {
      return false;
    }
    return (p_data->data != nullptr);
  }

  T& operator*() const noexcept {
    // if (p_data != nullptr) {
    return *p_data->data;
    //}
  }

  T* operator->() const noexcept {
    if (p_data != nullptr) {
      return p_data->data;
    }
    return nullptr;
  }

  std::size_t use_count() const noexcept {
    if (p_data == nullptr) {
      return 0;
    }
    return p_data->ref_count;
  }

  void reset() noexcept {
    release_ref();
  }

  void reset(T* new_ptr) {
    buffer* p_data_new;
    try {
      p_data_new = new buffer(new_ptr);
    } catch (...) {
      Deleter d;
      d(new_ptr);
      throw;
    }
    reset();
    p_data = p_data_new;
    add_ref();
  }

  void reset(T* new_ptr, Deleter deleter) {
    buffer* p_data_new;
    try {
      p_data_new = new buffer(new_ptr);
    } catch (...) {
      deleter(new_ptr);
      throw;
    }
    reset();
    p_data = p_data_new;
    p_data->my_deleter.~Deleter();
    new (&p_data->my_deleter) Deleter(deleter);
    add_ref();
  }

  friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return (lhs.p_data == rhs.p_data);
  }

  friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return !(lhs == rhs);
  }

private:
  // bool shared = false;

  struct buffer {
    T* data;
    size_t ref_count = 0;
    Deleter my_deleter;

    buffer(T* data) : data(data) {}

    buffer(T* data, Deleter my_deleter) : data(data), my_deleter(my_deleter) {}

    ~buffer() {
      assert(ref_count == 0);
      my_deleter(data);
    }
  };

  void add_ref() {
    if (p_data == nullptr) {
      return;
    }
    p_data->ref_count++;
  }

  void release_ref() {
    if (p_data == nullptr) {
      return;
    }
    if (--p_data->ref_count == 0) {
      delete p_data;
      p_data = nullptr;
    }
  }

  buffer* p_data = nullptr;
};
