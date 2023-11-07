#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>

inline constexpr size_t dynamic_extent = -1;

namespace sizeholder_space {

template <size_t Extent_>
struct size_holder {
public:
  constexpr size_t get_size() const noexcept {
    return Extent_;
  }

  void set_size(size_t) noexcept {}
};

template <>
struct size_holder<dynamic_extent> {
public:
  [[no_unique_address]] size_t exsize = 0;

  size_t get_size() const noexcept {
    return exsize;
  }

  void set_size(size_t new_size) noexcept {
    exsize = new_size;
  }
};
} // namespace sizeholder_space

template <typename T, size_t Extent = dynamic_extent>
class contiguous_view {
public:
  using value_type = std::remove_const_t<T>;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  using iterator = pointer;
  using const_iterator = const_pointer;

private:
  iterator start = nullptr;
  [[no_unique_address]] sizeholder_space::size_holder<Extent> size_stuff;

private:
  inline static /*irony*/ constexpr bool is_static() {
    return Extent != dynamic_extent;
  }

  template <typename R, size_t N>
  struct build_const_ptr {
    using byte_p = std::byte*;
    using get_conview = contiguous_view<std::byte, N * sizeof(R)>;
  };

  template <typename R, size_t N>
  struct build_const_ptr<const R, N> {
    using byte_p = const std::byte*;
    using get_conview = contiguous_view<const std::byte, N * sizeof(R)>;
  };

  template <typename R>
  struct build_const_ptr<R, dynamic_extent> {
    using byte_p = std::byte*;
    using get_conview = contiguous_view<std::byte, dynamic_extent>;
  };

  template <typename R>
  struct build_const_ptr<const R, dynamic_extent> {
    using byte_p = const std::byte*;
    using get_conview = contiguous_view<const std::byte, dynamic_extent>;
  };

public:
  contiguous_view() noexcept : start(nullptr) {}

  template <typename It>
  explicit(is_static()) contiguous_view(It first, size_t count) : start(std::to_address(first)) {
    size_stuff.set_size(count);
  }

  template <typename It>
  explicit(is_static()) contiguous_view(It first, It last) : start(std::to_address(first)) {
    size_stuff.set_size(std::distance(first, last));
  }

  contiguous_view(const contiguous_view& other) noexcept = default;
  contiguous_view& operator=(const contiguous_view& other) noexcept = default;

  template <typename U, size_t N>
  explicit(N == dynamic_extent && Extent != dynamic_extent)
      contiguous_view(const contiguous_view<U, N>& other,
                      typename std::enable_if<N == dynamic_extent || !is_static() || N == Extent, void**>::type = nullptr) noexcept
      : start(other.begin()) {
      size_stuff.set_size(other.size());
    assert(size() == other.size());
  }

  pointer data() const noexcept {
    return start;
  }

  size_t size() const noexcept {
    return size_stuff.get_size();
  }

  size_t size_bytes() const noexcept {
    return size() * sizeof(T);
  }

  bool empty() const noexcept {
    return (size_stuff.get_size() == 0);
  }

  iterator begin() const noexcept {
    return start;
  }

  const_iterator cbegin() const noexcept {
    return start;
  }

  iterator end() const noexcept {
    return start + size_stuff.get_size();
  }

  const_iterator cend() const noexcept {
    return start + size_stuff.get_size();
  }

  reference operator[](size_t idx) const {
    return start[idx];
  }

  reference front() const {
    return *start;
  }

  reference back() const {
    return *(start + size() - 1);
  }

  auto subview(size_t offset, size_t count = dynamic_extent) const {
    assert(offset <= size());
    contiguous_view<T> dd(data() + offset, count == dynamic_extent ? size() - offset : count);
    return dd;
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  auto subview() const {
    assert(Offset <= size());
    if constexpr (Count == dynamic_extent && is_static()) {
      return contiguous_view<T, Extent - Offset>(data() + Offset, size() - Offset);
    } else if constexpr (Count == dynamic_extent && !is_static()) {
      return contiguous_view<T, Count>(data() + Offset, size() - Offset);
    } else if constexpr (Count != dynamic_extent) {
      assert(Count + Offset <= size());
      return contiguous_view<T, Count>(data() + Offset, Count);
    }
  }

  template <size_t Count>
  contiguous_view<T, Count> first() const {
    assert(Count > 0);
    return contiguous_view<T, Count>(start, Count);
  }

  contiguous_view<T, dynamic_extent> first(size_t count) const {
    assert(count > 0);
    return contiguous_view<T>(start, count);
  }

  template <size_t Count>
  contiguous_view<T, Count> last() const {
    assert(Count > 0);
    return contiguous_view<T, Count>(start + size() - Count, Count);
  }

  contiguous_view<T, dynamic_extent> last(size_t count) const {
    assert(count > 0);
    return contiguous_view<T>(start + size() - count, count);
  }

  auto as_bytes() const {
    return typename build_const_ptr<T, Extent>::get_conview(
        reinterpret_cast<typename build_const_ptr<T, Extent>::byte_p>(begin()), size() * sizeof(T));
  }
};
