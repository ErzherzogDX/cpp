#pragma once

#include <cstddef>
#include <memory>

struct node {
  node* prev;
  node* next;

  node() : prev(this), next(this) {}

  node(node* prev, node* next) : prev(prev), next(next) {}
};

template <typename T, typename Deleter = std::default_delete<T>>
class linked_ptr {
  template <class Y, class D>
  friend class linked_ptr;

public:
  linked_ptr() noexcept : ptr(nullptr) {}

  ~linked_ptr() {
    release();
  }

  linked_ptr(std::nullptr_t) noexcept : my_node(nullptr, nullptr), ptr(nullptr) {}

  explicit linked_ptr(T* ptr) : my_deleter(), ptr(ptr) {}

  linked_ptr(T* ptr, Deleter deleter) : my_deleter(deleter), ptr(ptr) {}

  linked_ptr(const linked_ptr& other) : my_deleter(other.my_deleter), ptr(other.ptr) {
    my_node.prev = other.my_node.prev;
    my_node.next = &other.my_node;
    other.my_node.prev->next = &my_node;
    other.my_node.prev = &my_node;
  }

  template <typename Y, typename D = std::default_delete<Y>,
            typename = std::enable_if_t<std::is_constructible_v<Deleter, D> && std::is_convertible_v<Y*, T*>>>
  linked_ptr(const linked_ptr<Y, D>& other) noexcept : my_deleter(other.my_deleter),
                                                       ptr(other.ptr) {
    my_node.prev = other.my_node.prev;
    my_node.next = &other.my_node;
    other.my_node.prev->next = &my_node;
    other.my_node.prev = &my_node;
  }

  linked_ptr& operator=(const linked_ptr& other) noexcept {
    if (this != &other) {
      release();
      ptr = other.ptr;

      my_node.prev = other.my_node.prev;
      my_node.next = &other.my_node;
      other.my_node.prev->next = &my_node;
      other.my_node.prev = &my_node;
    }
    return *this;
  }

  template <typename Y, typename D = std::default_delete<Y>,
            typename = std::enable_if_t<std::is_constructible_v<Deleter, D> && std::is_convertible_v<Y*, T*>>>
  linked_ptr& operator=(const linked_ptr<Y, D>& other) noexcept {
    release();
    ptr = other.ptr;

    my_node.prev = other.my_node.prev;
    my_node.next = &other.my_node;
    other.my_node.prev->next = &my_node;
    other.my_node.prev = &my_node;
    return *this;
  }

  T* get() const noexcept {
    return ptr;
  }

  explicit operator bool() const noexcept {
    return (ptr != nullptr);
  }

  T& operator*() const noexcept {
    return *ptr;
  }

  T* operator->() const noexcept {
    return ptr;
  }

  std::size_t use_count() const noexcept {
    if (my_node.next == nullptr) {
      return 0;
    }

    node* current = &my_node;
    size_t sz = 1;
    while (current->next != &my_node) {
      current = current->next;
      sz++;
    }
    return sz;
  }

  void reset() noexcept {
    release();
  }

  void release() {
    if (my_node.next == nullptr) {
      return;
    }
    if (my_node.prev == &my_node) {
      my_deleter(ptr);
    }
    my_node.prev->next = my_node.next;
    my_node.next->prev = my_node.prev;
    my_node.next = &my_node;
    my_node.prev = &my_node;
    ptr = nullptr;
  }

  void reset(T* new_ptr) {
    reset();
    ptr = new_ptr;
  }

  void reset(T* new_ptr, Deleter deleter) {
    reset(new_ptr);
    my_deleter.~Deleter();
    new (&my_deleter) Deleter(deleter);
  }

  friend bool operator==(const linked_ptr& lhs, const linked_ptr& rhs) noexcept {
    return (lhs.ptr == rhs.ptr);
  }

  friend bool operator!=(const linked_ptr& lhs, const linked_ptr& rhs) noexcept {
    return !(lhs == rhs);
  }

private:
  mutable node my_node;
  // bool created_from_nullptr = false;
  Deleter my_deleter;
  T* ptr;
};
