#pragma once

#include <iterator>
#include <type_traits>


namespace kusabira::vocabulary {

  struct concat_sentinel {};

  /**
  * @brief 2つのイテレータ範囲を連結するview
  */
  template<typename Iterator1, typename Sentinel1, typename Iterator2, typename Sentinel2>
  class concat {

    static_assert(std::is_same_v<typename Iterator1::value_type, typename Iterator2::value_type>);
    static_assert(std::is_same_v<typename Iterator1::reference, typename Iterator2::reference>);

    Iterator1 m_it1;
    Sentinel1 m_end1;
    Iterator1 m_it2;
    Sentinel2 m_end2;

    typename Iterator1::value_type* m_val = nullptr;

  public:

    concat(Iterator1 it1, Sentinel1 end1, Iterator2 it2, Sentinel2 end2)
      : m_it1{ it1 }
      , m_end1{ end1 }
      , m_it2{ it2 }
      , m_end2{ end2 }
    {
      if (m_it1 != m_end1) m_val = std::addressof(*m_it1);
      else m_val = std::addressof(*m_it2);
    }

  private:

    class concat_iterator {
      concat* m_parent;

    public:
      using value_type = typename Iterator1::value_type;
      using reference = typename Iterator1::reference;
      using diference_type = std::ptrdiff_t;
      using iterator_category = std::input_iterator_tag;

      concat_iterator(concat& parent) : m_parent{std::addressof(parent)} {}

      concat_iterator(const concat_iterator&) = delete;
      concat_iterator(concat_iterator&&) = default;

      concat_iterator& operator=(const concat_iterator&) = delete;
      concat_iterator& operator=(concat_iterator&&) = default;

      friend auto operator++(concat_iterator& self) -> concat_iterator& {
        if (m_parent->m_it1 != m_parent->m_end1) {
          ++m_parent->m_it1;
          m_parent->m_val = std::addressof(*m_parent->m_it1);
        } else {
          ++m_parent->m_it2;
          m_parent->m_val = std::addressof(*m_parent->m_it2);
        }

        return *this;
      }

      friend void operator++(concat_iterator& self, int) {
        ++self;
      }

      friend auto operator*(concat& self) -> reference {
        return self.m_parent->m_val;
      }

      friend bool operator==(concat& lhs, concat_sentinel) {
        return self.m_parent->m_it1 == self.m_parent->m_end1 && self.m_parent->m_it2 == self.m_parent->m_end2;
      }

    };

  public:

    using iterator = concat_iterator;

    [[nodiscard]]
    friend auto begin(concat& self) -> concat_iterator {
      return { self };
    }

    [[nodiscard]]
    friend auto end(concat&) -> concat_sentinel {
      return {};
    }

  };
  
  template<typename Iterator1, typename Sentinel1, typename Iterator2, typename Sentinel2>
  concat(Iterator1, Sentinel1, Iterator2, Sentinel2) -> concat<Iterator1, Sentinel1, Iterator2, Sentinel2>;
    
} // namespace kusabira::vovabulary
