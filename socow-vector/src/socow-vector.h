#pragma once

#include <algorithm>
#include <memory>
#include <utility>

template <typename T, size_t SMALL_SIZE>
class socow_vector {
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  socow_vector() : _size(0) {}

  socow_vector(const socow_vector& other) {
    *this = other;
  }

  socow_vector(const socow_vector& other, size_t capacity) : isLarge(true) {
    socow_vector tmp(capacity);
    std::uninitialized_copy_n(other.data(), std::min(capacity, other.size()),
                              (tmp.isLarge ? tmp.d_data->data : tmp.static_buffer));
    tmp._size = other.size();
    swap(tmp);
  }

  socow_vector& operator=(const socow_vector& other) {
    if (this != &other) {
      if (isLarge) {
        if (other.isLarge) {
          release_ref();
          d_data = other.d_data;
          add_ref();
        } else {
          dynamic_buffer* x = d_data;
          try {
            std::uninitialized_copy_n(other.data(), other.size(), static_buffer);
          } catch (...) {
            d_data = x;
            throw;
          }
          if (--x->ref_count == 0) {
            clear_buffer(x->data, size());
            x->~dynamic_buffer();
            operator delete(x);
          }
        }
      } else {
        if (other.isLarge) {
          clear_buffer(static_buffer, size());
          d_data = other.d_data;
          add_ref();
        } else {
          socow_vector reserve;
          for (size_t i = 0; i < std::min(size(), other.size()); i++) {
            reserve.push_back(other[i]);
          }
          if (size() <= other.size()) {
            std::uninitialized_copy_n(other.data() + size(), other.size() - size(), data() + size());
          } else {
            clear_buffer(data(), size() - other.size(), other.size());
          }
          _size = other.size();
          std::swap_ranges(reserve.begin(), reserve.end(), data());
        }
      }
      _size = other.size();
      isLarge = other.isLarge;
    }
    return *this;
  }

  ~socow_vector() {
    if (isLarge) {
      release_ref();
      isLarge = false;
    } else {
      clear_buffer(static_buffer, size());
      _size = 0;
    }
  }

  T& operator[](size_t pos) {
    return data()[pos];
  }

  const T& operator[](size_t pos) const {
    return data()[pos];
  }

  pointer data() {
    if (isLarge) {
      unshare();
      return d_data->data;
    } else {
      return static_buffer;
    }
  }

  const_pointer data() const {
    if (isLarge) {
      return d_data->data;
    } else {
      return static_buffer;
    }
  }

  size_t size() const {
    return _size;
  }

  T& front() {
    return data()[0];
  }

  const T& front() const {
    return data()[0];
  }

  T& back() {
    return data()[size() - 1];
  }

  const T& back() const {
    return data()[size() - 1];
  }

  void push_back(const T& val) {
    size_t pos = size();
    if (shared() || size() == capacity()) {
      socow_vector temp(*this, size() == capacity() ? (size() * 2) : capacity());
      temp.push_back(val);
      *this = temp;
    } else {
      new (data() + pos) T(val);
      _size++;
    }
  }

  void pop_back() {
    if (size() > 0) {
      erase(std::as_const(*this).end() - 1);
    }
  }

  bool empty() const {
    return size() == 0;
  }

  size_t capacity() const {
    if (isLarge) {
      return d_data->_capacity;
    } else {
      return SMALL_SIZE;
    }
  }

  void reserve(std::size_t new_capacity) {
    if (new_capacity <= SMALL_SIZE && shared()) {
      shrink_to_fit();
    } else if ((size() < new_capacity && shared()) || new_capacity > capacity()) {
      *this = socow_vector(*this, new_capacity);
    }
  }

  void shrink_to_fit() {
    if (capacity() != SMALL_SIZE && size() != capacity()) {
      if (size() > SMALL_SIZE) {
        *this = socow_vector(*this, size());
      } else {
        socow_vector temp = *this;
        try {
          std::uninitialized_copy_n(temp.d_data->data, size(), static_buffer);
        } catch (...) {
          d_data = temp.d_data;
          throw;
        }
        isLarge = false;
        temp.release_ref();
      }
    }
  }

  void clear() {
    erase(std::as_const(*this).begin(), std::as_const(*this).end());
  }

  void swap(socow_vector& other) {
    if (this != &other) {
      if (isLarge) {
        if (other.isLarge) {
          std::swap(_size, other._size);
          std::swap(d_data, other.d_data);
        } else {
          socow_vector tmp = *this;
          *this = other;
          other = tmp;
        }
      } else {
        if (other.isLarge) {
          other.swap(*this);
        } else {
          if (size() <= other.size()) {
            size_t gap = other.size() - size();
            std::uninitialized_copy_n(other.static_buffer + size(), gap, static_buffer + size());
            size_t mx = size();
            while (mx > 0) {
              try {
                std::swap(static_buffer[mx - 1], other.static_buffer[mx - 1]);
              } catch (...) {
                clear_buffer(static_buffer, gap, size());
                throw;
              }
              mx--;
            }
            clear_buffer(other.static_buffer, gap, size());
            std::swap(_size, other._size);
          } else {
            other.swap(*this);
          }
        }
      }
    }
  }

  iterator begin(bool to_unshare = true) {
    if (isLarge && to_unshare) {
      unshare();
    }
    return data();
  }

  iterator end() {
    return data() + size();
  }

  const_iterator begin() const {
    return data();
  }

  const_iterator end() const {
    return data() + size();
  }

  iterator insert(const T* pos, const T& val) {
    std::ptrdiff_t ix = pos - std::as_const(*this).begin();
    if (shared() || size() == capacity()) {
      socow_vector temp(capacity() * 2);
      for (size_t i = 0; i < ix; i++) {
        temp.push_back(std::as_const(*this).begin()[i]);
      }
      temp.push_back(val);
      for (size_t i = ix; i < size(); i++) {
        temp.push_back(std::as_const(*this).begin()[i]);
      }
      *this = temp;
    } else {
      std::size_t length = end() - pos;
      push_back(val);
      T* last = end() - 1;
      for (std::size_t i = 0; i < length; i++) {
        std::iter_swap(last - i, last - i - 1);
      }
    }
    return begin() + ix;
  }

  iterator erase(const T* pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    std::ptrdiff_t ix = first - (std::as_const(*this)).begin();
    std::ptrdiff_t gap = last - first;
    if (first != last) {
      if (shared()) {
        socow_vector temp;
        temp.isLarge = isLarge;
        temp.d_data = allocate_buffer(size() - gap);

        std::uninitialized_copy_n(d_data->data, ix, temp.d_data->data);
        temp._size = ix;

        std::uninitialized_copy_n(d_data->data + ix + gap, size() - ix - gap, temp.d_data->data + ix);
        temp._size = size() - gap;
        *this = temp;
      } else {
        T* dataset = data();
        for (size_t i = ix; i + gap < size(); i++) {
          std::swap(dataset[i], dataset[i + gap]);
        }
        clear_buffer(data(), gap, size() - gap);
        _size = size() - gap;
      }
    }
    return begin() + ix;
  }

private:
  size_t _size{0};
  bool isLarge = false;

  struct dynamic_buffer {
    dynamic_buffer(std::size_t capacity) : _capacity(capacity) {}

    size_t _capacity = 0;
    size_t ref_count = 1;
    T data[0];
  };

  union {
    T static_buffer[SMALL_SIZE];
    dynamic_buffer* d_data = nullptr;
  };

  socow_vector(size_t cap) {
    if (cap == 0 || cap <= SMALL_SIZE) {
      return;
    }
    dynamic_buffer* new_data = allocate_buffer(cap);
    d_data = new_data;
    isLarge = true;
  }

  dynamic_buffer* allocate_buffer(size_t cap) {
    dynamic_buffer* new_dyn_buff =
        static_cast<dynamic_buffer*>(operator new(sizeof(dynamic_buffer) + sizeof(value_type) * cap));
    new (new_dyn_buff) dynamic_buffer{cap};
    return new_dyn_buff;
  }

  bool shared() {
    return isLarge && d_data->ref_count > 1;
  }

  void add_ref() {
    if (!d_data) {
      return;
    }
    ++d_data->ref_count;
  }

  void release_ref() {
    if (d_data == nullptr) {
      return;
    }
    if (--d_data->ref_count == 0) {
      d_data->_capacity = 0;
      clear_buffer(d_data->data, size());
      _size = 0;

      operator delete(d_data);
      d_data = nullptr;
    }
  }

  void unshare() {
    if (d_data->ref_count == 1) {
      return;
    }
    socow_vector temp(*this, d_data->_capacity);
    *this = temp;
  }

  void clear_buffer(T* dataset, size_t val, size_t add = 0) {
    while (val > 0) {
      dataset[add + val - 1].~T();
      val--;
    }
  }
};
