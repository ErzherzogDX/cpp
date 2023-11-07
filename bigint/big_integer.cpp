#include "big_integer.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <ostream>
#include <stdexcept>

big_integer::big_integer() : negate(false), limbs(0, 0) {}

big_integer::big_integer(const big_integer& other) = default;

big_integer::big_integer(unsigned long long a) : negate(false) {
  set_number(a);
}

big_integer::big_integer(long long a) : negate(a < 0) {
  set_number(static_cast<dlimb_t>(a));
}

big_integer::big_integer(int a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned int a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}

const size_t LIMB_BITS = 32;
const dlimb_t RADIX = 1LL << LIMB_BITS;
const limb_t MAX = RADIX - 1;

// Устанавливает число в массив лимбов.
void big_integer::set_number(dlimb_t a) {
  limbs.reserve(size() + 2);
  limbs.push_back(a % RADIX);
  a /= RADIX;
  limbs.push_back(a % RADIX);
  (*this).normalization();
}

// Аналог из cpp-ref.
static bool my_isdigit(char ch) {
  return std::isdigit(static_cast<unsigned char>(ch));
}

big_integer::big_integer(const std::string& str) {
  for (size_t i = 0; i < str.size(); i++) {
    if (!i) {
      if (str[i] != '-' && (!my_isdigit(str[i]))) {
        throw std::invalid_argument("Invalid number - unexpected char at start position");
      }
    } else {
      if (!my_isdigit(str[i])) {
        throw std::invalid_argument("Invalid number - unexpected chat at position " + std::to_string(i));
      }
    }
  }
  if (str.empty()) {
    throw std::invalid_argument("Invalid input - given the empty string.");
  }
  if (str.size() == 1 && (str[0] == '-' || str[0] == '+')) {
    throw std::invalid_argument("Invalid input - no value after the sign.");
  }

  if (str == "0" || str == "-0") {
    negate = false;
  } else {
    size_t size = str.length();
    size_t it = (str[0] == '-') ? 1 : 0;
    negate = str[0] == '-';

    size_t first = (size - it) % 9 + it;
    uint32_t number = 0;
    std::from_chars(str.data() + it, str.data() + first, number);

    *this = number;
    for (it = first; it < size; it += 9) {
      mul_small(*this, 1000000000);
      std::from_chars(str.data() + it, str.data() + it + 9, number);
      *this += number;
    }
    if (str[0] == '-') {
      get_negate(true);
    }
  }
}

limb_t big_integer::get(std::size_t i) const {
  return (i < limbs.size()) ? limbs[i] : get_filler();
}

std::size_t big_integer::size() const {
  return limbs.size();
}

// Взять модуль от бигинта.
void big_integer::get_absolute(bool normalize) {
  if (negate) {
    get_negate(normalize);
  }
}

// Взять противоположное.
void big_integer::get_negate(bool normalize) {
  for (limb_t& i : limbs) {
    i ^= MAX;
  }
  negate = !negate;
  add_sub_small(*this, 1, false, normalize);
}

// Нормализация (удаление лишних нулей).
void big_integer::normalization() {
  while (!limbs.empty() && limbs.back() == get_filler()) {
    limbs.pop_back();
  }
}

// Для заполнения числа нулями/MAX в зависимости от знака.
limb_t big_integer::get_filler() const {
  return (negate) ? MAX : 0;
}

// Длинное сложение и вычитание.
void big_integer::add_sub(big_integer& a, const big_integer& b, bool normalize, bool sub) {
  std::size_t new_size = std::max(a.size(), b.size()) + 1;
  a.limbs.resize(new_size, a.get_filler());
  dlimb_t carry = 0;
  if (sub) {
    carry++;
  }

  for (std::size_t i = 0; i < new_size; i++) {
    dlimb_t tmp = (sub) ? carry + a.get(i) + ~(b.get(i)) : carry + a.get(i) + (b.get(i));
    a.limbs[i] = static_cast<limb_t>(tmp);
    carry = tmp >> LIMB_BITS;
  }
  a.negate = a.limbs.back() & LIMB_BITS;
  if (normalize) {
    a.normalization();
  }
}

// Короткое.
void big_integer::add_sub_small(big_integer& a, const limb_t v, bool bnegate, bool normalize) {
  std::size_t new_size = a.size() + 1;
  a.limbs.resize(new_size, a.get_filler());
  dlimb_t res = 0;

  for (std::size_t i = 0; i < new_size; i++) {
    res += static_cast<dlimb_t>(a.get(i)) + ((i < 1) ? v : (bnegate ? MAX : 0));
    a.limbs[i] = static_cast<limb_t>(res % RADIX);
    res >>= LIMB_BITS;
  }
  a.negate ^= (res) ^ bnegate;

  if (normalize) {
    a.normalization();
  }
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  add_sub(*this, rhs, true, false);
  return *this;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  add_sub(*this, rhs, true, true);
  return *this;
}

// Умножение.

void big_integer::mul_small(big_integer& bi, const limb_t v) {
  bi.limbs.resize(bi.limbs.size() + 1, bi.get_filler());
  bool neg = bi.negate;
  bi.get_absolute(true);
  limb_t cnst = v;

  dlimb_t s = 0;
  for (std::size_t i = 0; i < bi.size(); i++) {
    s += static_cast<dlimb_t>(bi.limbs[i]) * cnst;
    bi.limbs[i] = static_cast<limb_t>(s);
    s >>= LIMB_BITS;
  }
  if (neg) {
    bi.get_negate(true);
  }
  if (s > 0) {
    bi.limbs[bi.size() - 1] = s;
  }
}

void big_integer::mul(big_integer& a, const big_integer& b) {
  bool neg = a.negate ^ b.negate;
  std::size_t asize = a.size();
  a.limbs.resize(a.size() + b.size() + 1, a.get_filler());

  a.get_absolute(false);
  big_integer b2 = b;
  b2.get_absolute(false);

  for (size_t i = asize; i > 0; i--) {
    std::size_t cnst = a.limbs[i - 1];
    a.limbs[i - 1] = 0;
    std::size_t j = 0;
    dlimb_t s_mul = 0;
    dlimb_t carry = 0;
    while (j < b.size() || (carry > 0)) {
      s_mul = static_cast<dlimb_t>(b2.get(j)) * cnst + carry;
      carry = s_mul / RADIX;
      dlimb_t to_add = s_mul % RADIX;
      if (to_add + a.limbs[i - 1 + j] > MAX) {
        carry++;
      }
      a.limbs[i - 1 + j] = static_cast<limb_t>(to_add + a.limbs[i - 1 + j]);
      j++;
    }
  }

  if (neg) {
    a.get_negate(true);
  }
  a.normalization();
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  mul((*this), rhs);
  return *this;
}

// Деление.

// Аналог "difference" из статьи.
void big_integer::div_difference(big_integer& a, const big_integer& b) {
  dlimb_t res = static_cast<dlimb_t>(~b.get(0)) + 1 + a.get(0);
  dlimb_t carry = res >> LIMB_BITS;
  a.limbs[0] = static_cast<limb_t>(res);
  for (std::size_t i = 1; i < b.size(); i++) {
    res = static_cast<dlimb_t>(a.get(i)) + static_cast<dlimb_t>(~b.get(i)) + carry;
    a.limbs[i] = static_cast<limb_t>(res % RADIX);
    carry = res >> LIMB_BITS;
  }
}

// Аналог "shift" из статьи.
void big_integer::shift(big_integer& a) {
  for (std::size_t j = a.limbs.size() - 1; j > 0; j--) {
    a.limbs[j] = a.limbs[j - 1];
  }
}

// Аналог "smaller" из статьи.
bool big_integer::div_compare(big_integer& a, big_integer& b) {
  for (size_t i = a.limbs.size(); i > 0; i--) {
    if (a.get(i - 1) != b.get(i - 1)) {
      return a.get(i - 1) < b.get(i - 1);
    }
  }
  return false;
}

// Короткое деление.
limb_t big_integer::div_small(big_integer& bi, const limb_t v) {
  if (v == 0) {
    throw std::invalid_argument("haha no zero division");
  }
  limb_t s = 0;
  dlimb_t cur = 0;
  size_t i = bi.size();
  while (i > 0) {
    cur = (static_cast<dlimb_t>(s) << LIMB_BITS) + bi.limbs[i - 1];
    bi.limbs[i - 1] = (cur / v);
    s = (cur % v);
    i--;
  }
  bi.normalization();
  return s;
}

// Получение остатка.
big_integer big_integer::get_reminder(big_integer& buf, limb_t f) {
  assert(f != 0);

  big_integer rem = 0;
  for (size_t i = 1; i < buf.size(); i++) {
    buf.limbs[i - 1] = buf.limbs[i];
  }
  buf.limbs.pop_back();
  rem = buf;
  rem.normalization();

  div_small(rem, f);
  return rem;
}

big_integer big_integer::long_divide(big_integer& a, big_integer& b, bool getRem) {
  auto back = static_cast<dlimb_t>(b.limbs.back());
  auto f = static_cast<limb_t>((RADIX / (back + 1)));

  size_t asize = a.limbs.size();
  size_t bsize = b.limbs.size();

  big_integer dq;
  dq.limbs.resize(bsize + 1);
  big_integer buf;
  buf.limbs.resize(bsize + 1);
  big_integer ans;
  ans.limbs.resize(asize - bsize + 1);

  mul_small(a, f);
  mul_small(b, f);
  a.normalization();
  b.normalization();

  for (size_t i = 0; i <= bsize; i++) {
    buf.limbs[i] = a.get(asize + i - bsize);
  }
  for (size_t i = asize - bsize + 1; i > 0; i--) {
    buf.limbs[0] = a.get(i - 1);

    limb_t qt =
        std::min(static_cast<dlimb_t>(MAX),
                 ((static_cast<dlimb_t>(buf.limbs.back()) << LIMB_BITS) + buf.limbs[buf.size() - 2]) / b.limbs.back());

    dq = b * qt;
    while (div_compare(buf, dq)) {
      qt--;
      dq = b * qt;
    }
    div_difference(buf, dq);
    shift(buf);
    ans.limbs[i - 1] = qt;
  }

  a = ans;
  a.normalization();

  if (getRem) {
    return get_reminder(buf, f);
  } else {
    return 0;
  }
}

// Обнуление массива чисел.
void big_integer::nullify() {
  while (!limbs.empty()) {
    limbs.pop_back();
  }
}

big_integer big_integer::div(big_integer& a, big_integer b, bool getRem) {
  a.get_absolute(true);
  b.get_absolute(true);

  if (a < b) {
    big_integer rem = a;
    a.nullify();
    return rem;
  }
  if (b.limbs.size() == 1) {
    limb_t res = (div_small(a, b.get(0)));
    return static_cast<big_integer>(res);
  }
  return long_divide(a, b, (getRem));
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  bool neg = (*this).negate ^ rhs.negate;
  div(*this, rhs, false);
  if (neg) {
    get_negate(true);
  }
  return *this;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  bool neg = (*this).negate;
  *this = div(*this, rhs, true);
  if (neg) {
    get_negate(true);
  }
  return *this;
}

// Выбор булевой функции.
void big_integer::binary_thing(big_integer& a, const big_integer& b, Booleanic op) {
  std::size_t n = std::max(a.limbs.size(), b.limbs.size());
  a.limbs.resize(n, get_filler());
  for (std::size_t i = 0; i < n; i++) {
    switch (op) {
    case Booleanic::AND:
      a.limbs[i] = a.get(i) & b.get(i);
      break;
    case Booleanic::OR:
      a.limbs[i] = a.get(i) | b.get(i);
      break;
    case Booleanic::XOR:
      a.limbs[i] = a.get(i) ^ b.get(i);
      break;
    }
  }

  switch (op) {
  case Booleanic::AND:
    a.negate &= b.negate;
    break;
  case Booleanic::OR:
    a.negate |= b.negate;
    break;
  case Booleanic::XOR:
    a.negate ^= b.negate;
    break;
  }
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  binary_thing((*this), rhs, Booleanic::AND);
  (*this).normalization();
  return *this;
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  binary_thing((*this), rhs, Booleanic::OR);
  (*this).normalization();
  return *this;
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  binary_thing((*this), rhs, Booleanic::XOR);
  (*this).normalization();
  return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
  limb_t sdv = rhs % LIMB_BITS;
  limb_t s = 0;
  limbs.reserve(size() + 1);

  if (sdv != 0) {
    for (limb_t& limb : limbs) {
      dlimb_t temp = (static_cast<dlimb_t>(limb) << sdv) + s;
      s = static_cast<limb_t>(temp >> LIMB_BITS);
      limb = static_cast<limb_t>(temp);
    }
    limbs.push_back(((negate ? MAX : 0) << sdv) + s);
  }
  limbs.insert(limbs.begin(), rhs / LIMB_BITS, 0);
  normalization();
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  limbs.reserve(size() + 1);
  limbs.erase(limbs.begin(),
              limbs.begin() + std::min(static_cast<int>(limbs.size()), rhs / static_cast<int>(LIMB_BITS)));
  if (limbs.empty()) {
    limbs.push_back(0);
    (*this).normalization();
    return *this;
  }
  if (negate) {
    limbs.push_back(MAX);
  }
  rhs %= LIMB_BITS;
  dlimb_t s = 0;
  if (rhs != 0) {
    for (std::size_t i = limbs.size(); i > 0; i--) {
      dlimb_t tmp = (static_cast<dlimb_t>(limbs[i - 1]) << (LIMB_BITS - rhs)) + s;
      s = tmp << LIMB_BITS;
      limbs[i - 1] = static_cast<limb_t>(tmp >> LIMB_BITS);
    }
  }
  if (negate) {
    limbs.pop_back();
  }
  (*this).normalization();
  return *this;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  return ~(*this) + 1;
}

big_integer big_integer::operator~() const {
  big_integer res(*this);
  for (limb_t& i : res.limbs) {
    i ^= MAX;
  }
  res.negate = !negate;
  return res;
}

big_integer& big_integer::operator=(const big_integer& other) {
  big_integer copy = other;
  std::swap((*this).limbs, copy.limbs);
  std::swap((*this).negate, copy.negate);
  return *this;
}

big_integer& big_integer::operator++() {
  add_sub_small(*this, 1, false, true);
  return *this;
}

big_integer big_integer::operator++(int) {
  big_integer tmp(*this);
  add_sub_small(*this, 1, false, true);
  return tmp;
}

big_integer& big_integer::operator--() {
  add_sub_small(*this, -1, true, true);
  return *this;
}

big_integer big_integer::operator--(int) {
  big_integer tmp(*this);
  add_sub_small(*this, -1, true, true);
  return tmp;
}

big_integer operator+(big_integer a, const big_integer& b) {
  return a += b;
}

big_integer operator-(big_integer a, const big_integer& b) {
  return a -= b;
}

big_integer operator*(big_integer a, const big_integer& b) {
  return a *= b;
}

big_integer operator/(big_integer a, const big_integer& b) {
  return a /= b;
}

big_integer operator%(big_integer a, const big_integer& b) {
  return a %= b;
}

big_integer operator&(big_integer a, const big_integer& b) {
  return a &= b;
}

big_integer operator|(big_integer a, const big_integer& b) {
  return a |= b;
}

big_integer operator^(big_integer a, const big_integer& b) {
  return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
  return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
  return a >>= b;
}

bool operator==(const big_integer& a, const big_integer& b) {
  return a.negate == b.negate && a.limbs == b.limbs;
}

bool operator!=(const big_integer& a, const big_integer& b) {
  return !(a == b);
}

bool operator<(const big_integer& a, const big_integer& b) {
  if (a.negate != b.negate) {
    return a.negate;
  }
  if (a.limbs.size() != b.limbs.size()) {
    return a.negate ^ (a.limbs.size() < b.limbs.size());
  } else {
    return std::lexicographical_compare(a.limbs.rbegin(), a.limbs.rend(), b.limbs.rbegin(), b.limbs.rend());
  }
}

bool operator>(const big_integer& a, const big_integer& b) {
  return b < a;
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return !(a < b);
}

std::string to_string(const big_integer& a) {
  if (a == 0) {
    return "0";
  }
  big_integer temp = a;
  temp.get_absolute(true);
  std::string prm;

  while (temp.size() > 0) {
    limb_t gt = big_integer::div_small(temp, 1000000000);
    for (size_t i = 0; i < 9; i++) {
      prm.append(std::to_string(gt % 10));
      gt /= 10;
    }
  }
  while (prm.length() > 0 && prm[prm.length() - 1] == '0') {
    prm.pop_back();
  }
  if (a.negate) {
    prm.append("-");
  }
  reverse(prm.begin(), prm.end());
  return prm;
}

std::ostream& operator<<(std::ostream& s, const big_integer& a) {
  return s << to_string(a);
}
