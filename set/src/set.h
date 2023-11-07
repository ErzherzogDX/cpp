#pragma once

#include <cassert>
#include <iterator>
#include <utility>

template <typename T>

class set {
  class base_node {
  public:
    base_node* parent = nullptr;
    base_node* left = nullptr;
    base_node* right = nullptr;
    base_node() = default;

    base_node(base_node* par) : parent(par) {}
  };

  class real_node : public base_node {
  public:
    T value;

    real_node(const T& val, base_node* par) : base_node(par), value(val) {}
  };

  static const base_node* findBorders(const base_node* x, bool maximum) {
    if (maximum) {
      if (x->right == nullptr) {
        return x;
      } else {
        return findBorders(x->right, true);
      }
    } else {
      if (x->left == nullptr) {
        return x;
      } else {
        return findBorders(x->left, false);
      }
    }
  }

  class dst_iterator {
  public:
    using value_type = T;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using reference = const T&;
    using pointer = const T*;

  private:
    const base_node* node = nullptr;

    dst_iterator(const base_node* nd) : node(nd) {}

    friend set;

    const base_node* next(const base_node* x, bool next) {
      if (x->right != nullptr && next) {
        return findBorders(x->right, false);
      }
      if (x->left != nullptr && !next) {
        return findBorders(x->left, true);
      }

      base_node* y = x->parent;
      while (y != nullptr && x == ((next) ? y->right : y->left)) {
        x = y;
        y = y->parent;
      }
      return y;
    }

  public:
    dst_iterator() = default;
    dst_iterator& operator=(const dst_iterator& other) = default;

    reference operator*() const {
      return static_cast<const real_node*>(node)->value;
    }

    pointer operator->() const {
      return &(static_cast<const real_node*>(node)->value);
    }

    friend bool operator==(const dst_iterator& a, const dst_iterator& b) {
      return a.node == b.node;
    }

    friend bool operator!=(const dst_iterator& a, const dst_iterator& b) {
      return !(a == b);
    }

    dst_iterator& operator++() {
      node = next(node, true);
      return *this;
    }

    dst_iterator operator++(int) {
      dst_iterator res = *this;
      ++*this;
      return res;
    }

    dst_iterator& operator--() {
      node = next(node, false);
      return *this;
    }

    dst_iterator operator--(int) {
      dst_iterator res = *this;
      --*this;
      return res;
    }
  };

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = dst_iterator;
  using const_iterator = dst_iterator;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  size_t treesize = 0;
  base_node base;

public:
  // O(1) nothrow
  set() noexcept = default;

  void copy_tree(const base_node* vx, base_node* parent, base_node*& new_parent) {
    if (vx == nullptr) {
      return;
    }

    auto* new_node = new real_node(static_cast<const real_node*>(vx)->value, parent);
    new_parent = new_node;
    copy_tree(vx->left, new_node, new_node->left);
    copy_tree(vx->right, new_node, new_node->right);
  }

  set(const set& other) : set() {
    if (!other.empty()) {
      copy_tree(other.base.left, &base, base.left);
      treesize = other.treesize;
    }
  }

  set& operator=(const set& other) {
    if (this != &other) {
      set new_set(other);
      swap(new_set, *this);
    }
    return *this;
  }

  ~set() noexcept {
    clear();
  }

  void eliminate_nodes(base_node* c) {
    if (c == nullptr) {
      return;
    }
    if (c->left != nullptr) {
      eliminate_nodes(c->left);
    }
    if (c->right != nullptr) {
      eliminate_nodes(c->right);
    }
    delete static_cast<const real_node*>(c);
  }

  void clear() noexcept {
    treesize = 0;
    eliminate_nodes(base.left);
    base.left = nullptr;
  }

  // O(1) nothrow
  size_t size() const noexcept {
    return treesize;
  }

  // O(1) nothrow
  bool empty() const noexcept {
    return (treesize == 0);
  }

  // nothrow
  const_iterator begin() const noexcept {
    const base_node* res = findBorders(&base, false);
    return const_iterator(res);
  }

  // nothrow
  const_iterator end() const noexcept {
    return const_iterator(&base);
  }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  iterator create_node(base_node*& coord, const T& val, base_node* parent) {
    coord = new real_node(val, parent);
    treesize++;
    return iterator(coord);
  }

  std::pair<iterator, bool> insert(const T& val) {
    if (empty()) {
      return std::make_pair(create_node(base.left, val, &base), true);
    }

    base_node* vx = base.left;
    while (vx != nullptr) {
      if (val > static_cast<real_node*>(vx)->value) {
        if (vx->right != nullptr) {
          vx = vx->right;
        } else {
          return std::make_pair(create_node(vx->right, val, vx), true);
        }
      } else if (val < static_cast<real_node*>(vx)->value) {
        if (vx->left != nullptr) {
          vx = vx->left;
        } else {
          return std::make_pair(create_node(vx->left, val, vx), true);
        }
      } else {
        return std::make_pair(iterator(vx), false);
      }
    }
  }

  // O(h) nothrow
  iterator erase(const_iterator pos) {
    const base_node* v = pos.node;
    base_node* p = v->parent;

    const_iterator to_return = pos;
    ++to_return;

    if (v->left == nullptr && v->right == nullptr) {
      if (p->left == v) {
        p->left = nullptr;
      }
      if (p->right == v) {
        p->right = nullptr;
      }
    } else if (v->left == nullptr || v->right == nullptr) {
      if (v->left == nullptr) {
        if (p->left == v) {
          p->left = v->right;
        } else {
          p->right = v->right;
        }
        v->right->parent = p;
      } else {
        if (p->left == v) {
          p->left = v->left;
        } else {
          p->right = v->left;
        }
        v->left->parent = p;
      }
    } else {
      base_node* left = v->left;
      base_node* right = v->right;

      base_node*& parent_child_ptr = ((v->parent->left == v) ? v->parent->left : v->parent->right);
      parent_child_ptr = right;

      right->parent = v->parent;

      while (right->left != nullptr) {
        right = right->left;
      }
      right->left = left;
      left->parent = right;
    }

    treesize--;
    delete static_cast<const real_node*>(v);
    return to_return;
  }

  // O(h) strong
  size_t erase(const T& element) {
    const_iterator it = find(element);
    if (it != end()) {
      erase(it);
      return 1;
    } else {
      return 0;
    }
  }

  const_iterator lower_bound(const T& val) const {
    if (empty()) {
      return end();
    }

    base_node* vx = base.left;
    while (vx != nullptr) {
      if (val > static_cast<real_node*>(vx)->value) {
        if (vx->right != nullptr) {
          vx = vx->right;
        } else {
          return ++const_iterator(vx);
        }
      } else if (val < static_cast<real_node*>(vx)->value) {
        if (vx->left != nullptr) {
          vx = vx->left;
        } else {
          return const_iterator(vx);
        }
      } else {
        return const_iterator(vx);
      }
    }
  }

  const_iterator upper_bound(const T& item) const {
    const_iterator it = lower_bound(item);
    if (it == end() || *it != item) {
      return it;
    } else {
      ++it;
      return it;
    }
  }

  const_iterator find(const T& item) const {
    const_iterator it = lower_bound(item);
    if (it == end() || *it != item) {
      return end();
    } else {
      return it;
    }
  }

  friend void swap(set& x, set& y) noexcept {
    std::swap(x.base, y.base);
    std::swap(x.treesize, y.treesize);

    if (!x.empty()) {
      x.base.left->parent = &x.base;
    }
    if (!y.empty()) {
      y.base.left->parent = &y.base;
    }
  }
};
