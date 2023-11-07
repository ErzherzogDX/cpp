#pragma once

#include <algorithm>
#include <iosfwd>
#include <string>
#include <vector>

using limb_t = std::uint32_t;
using dlimb_t = std::uint64_t;

struct big_integer {
  big_integer();
  big_integer(const big_integer& other);

  big_integer(long long a);
  big_integer(unsigned long long a);

  big_integer(long a);
  big_integer(unsigned long a);

  big_integer(int a);
  big_integer(unsigned int a);

  explicit big_integer(const std::string& str);
  ~big_integer() = default;

  big_integer& operator=(const big_integer& other);

  big_integer& operator+=(const big_integer& rhs);
  big_integer& operator-=(const big_integer& rhs);
  big_integer& operator*=(const big_integer& rhs);
  big_integer& operator/=(const big_integer& rhs);
  big_integer& operator%=(const big_integer& rhs);

  big_integer& operator&=(const big_integer& rhs);
  big_integer& operator|=(const big_integer& rhs);
  big_integer& operator^=(const big_integer& rhs);

  big_integer& operator<<=(int rhs);
  big_integer& operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer& operator++();
  big_integer operator++(int);

  big_integer& operator--();
  big_integer operator--(int);

  friend bool operator==(const big_integer& a, const big_integer& b);
  friend bool operator!=(const big_integer& a, const big_integer& b);
  friend bool operator<(const big_integer& a, const big_integer& b);
  friend bool operator>(const big_integer& a, const big_integer& b);
  friend bool operator<=(const big_integer& a, const big_integer& b);
  friend bool operator>=(const big_integer& a, const big_integer& b);

  friend std::string to_string(const big_integer& a);

  void get_absolute(bool normalize);
  void get_negate(bool normalize);

private:
  bool negate;
  std::vector<limb_t> limbs;

  limb_t get(size_t i) const;
  size_t size() const;

  void set_number(dlimb_t a);

  void normalization();
  void nullify();

  static void add_sub(big_integer& a, const big_integer& b, bool normalize, bool sub);
  static big_integer long_divide(big_integer& a, big_integer& b, bool getRem);

  limb_t get_filler() const;

  static void add_sub_small(big_integer& a, const limb_t v, bool bnegate, bool normalize);
  static void mul_small(big_integer& bi, const limb_t v);
  static limb_t div_small(big_integer& bi, const limb_t v);
  static big_integer get_reminder(big_integer& buf, limb_t f);

  void mul(big_integer& a, const big_integer& b);

  static big_integer div(big_integer& a, big_integer b, bool getRem);
  static void div_difference(big_integer& a, const big_integer& b);
  static bool div_compare(big_integer& a, big_integer& b);
  static void shift(big_integer& a);
  enum class Booleanic {
    AND,
    OR,
    XOR
  };
  void binary_thing(big_integer& a, const big_integer& b, Booleanic op);
};

big_integer operator+(big_integer a, const big_integer& b);
big_integer operator-(big_integer a, const big_integer& b);
big_integer operator*(big_integer a, const big_integer& b);
big_integer operator/(big_integer a, const big_integer& b);
big_integer operator%(big_integer a, const big_integer& b);

big_integer operator&(big_integer a, const big_integer& b);
big_integer operator|(big_integer a, const big_integer& b);
big_integer operator^(big_integer a, const big_integer& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(const big_integer& a, const big_integer& b);
bool operator!=(const big_integer& a, const big_integer& b);
bool operator<(const big_integer& a, const big_integer& b);
bool operator>(const big_integer& a, const big_integer& b);
bool operator<=(const big_integer& a, const big_integer& b);
bool operator>=(const big_integer& a, const big_integer& b);

std::string to_string(const big_integer& a);
std::ostream& operator<<(std::ostream& s, const big_integer& a);
