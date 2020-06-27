#pragma once

#include <iterator>
#include <type_traits>


namespace kusabira::vocabulary {

  struct concat_sentinel {};

  /**
  * @brief 2つのイテレータ範囲を連結するview
  * @tparam Iterator1 1つ目の範囲の開始イテレータ
  * @tparam Sentinel1 1つ目の範囲の終端
  * @tparam Iterator2 2つ目の範囲の開始イテレータ
  * @tparam Sentinel2 2つ目の範囲の終端
  * @details 二つのイテレータの要素型と参照型が一致している必要がある
  */
  template<typename Iterator1, typename Sentinel1, typename Iterator2, typename Sentinel2>
  class concat {

    static_assert(std::is_same_v<typename std::iterator_traits<Iterator1>::value_type, typename std::iterator_traits<Iterator2>::value_type>);
    static_assert(std::is_same_v<typename std::iterator_traits<Iterator1>::reference, typename std::iterator_traits<Iterator2>::reference>);

    Iterator1 m_it1;
    Sentinel1 m_end1;
    Iterator2 m_it2;
    Sentinel2 m_end2;

    typename std::iterator_traits<Iterator1>::value_type* m_val = nullptr;
    bool m_is_first_half = true;

  public:

    concat() = default;

    constexpr concat(Iterator1 it1, Sentinel1 end1, Iterator2 it2, Sentinel2 end2)
      : m_it1{ it1 }
      , m_end1{ end1 }
      , m_it2{ it2 }
      , m_end2{ end2 }
      , m_is_first_half{ it1 != end1 }
    {}

  private:

    class concat_iterator {
      concat* m_parent;

    public:
      using value_type = typename Iterator1::value_type;
      using pointer = value_type*;
      using reference = typename Iterator1::reference;
      using difference_type = std::ptrdiff_t;
      using iterator_category = std::input_iterator_tag;

      concat_iterator() = default;
      constexpr concat_iterator(concat& parent) : m_parent{std::addressof(parent)} {}

      concat_iterator(const concat_iterator&) = delete;
      concat_iterator(concat_iterator&&) = default;

      concat_iterator& operator=(const concat_iterator&) = delete;
      concat_iterator& operator=(concat_iterator&&) = default;

      constexpr auto operator++() -> concat_iterator& {
        if (m_parent->m_is_first_half) {
          ++m_parent->m_it1;
          if (m_parent->m_it1 != m_parent->m_end1) {
            m_parent->m_val = std::addressof(*m_parent->m_it1);
          } else {
            m_parent->m_is_first_half = false;
            if (m_parent->m_it2 != m_parent->m_end2) m_parent->m_val = std::addressof(*m_parent->m_it2);
          }
        } else {
          ++m_parent->m_it2;
          if (m_parent->m_it2 != m_parent->m_end2) {
            m_parent->m_val = std::addressof(*m_parent->m_it2);
          }
        }
        return *this;
      }

      constexpr void operator++(int) {
        ++*this;
      }

      constexpr auto operator*() const -> reference {
        return *(m_parent->m_val);
      }

      constexpr bool operator==(concat_sentinel) const {
        return m_parent->m_it1 == m_parent->m_end1
            && m_parent->m_it2 == m_parent->m_end2;
      }

    };

  public:

    using iterator = concat_iterator;

    [[nodiscard]]
    friend constexpr auto begin(concat& self) -> concat_iterator {
      if (self.m_is_first_half) self.m_val = std::addressof(*self.m_it1);
      else if (self.m_it2 != self.m_end2) self.m_val = std::addressof(*self.m_it2);
      return { self };
    }

    [[nodiscard]]
    friend constexpr auto end(concat&) -> concat_sentinel {
      return {};
    }

  };
  
  template<typename Iterator1, typename Sentinel1, typename Iterator2, typename Sentinel2>
  concat(Iterator1, Sentinel1, Iterator2, Sentinel2) -> concat<Iterator1, Sentinel1, Iterator2, Sentinel2>;
    
} // namespace kusabira::vocabulary
