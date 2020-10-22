#pragma once

#include <iterator>
#include <type_traits>
#include <ranges>

namespace kusabira::vocabulary {

  /**
  * @brief 2つのイテレータ範囲を連結するview
  * @tparam Iterator1 1つ目の範囲の開始イテレータ
  * @tparam Sentinel1 1つ目の範囲の終端
  * @tparam Iterator2 2つ目の範囲の開始イテレータ
  * @tparam Sentinel2 2つ目の範囲の終端
  * @details 二つのイテレータの要素型と参照型が一致している必要がある
  */
  //template<std::input_iterator Iterator1, std::sentinel_for<Iterator1> Sentinel1, std::input_iterator Iterator2, std::sentinel_for<Iterator2> Sentinel2>
  template<typename Iterator1, typename Sentinel1, typename Iterator2, typename Sentinel2>
    requires (std::same_as<std::iter_value_t<Iterator1>, std::iter_value_t<Iterator2>> and
              std::same_as<std::iter_reference_t<Iterator1>, std::iter_reference_t<Iterator2>> and
              std::is_reference_v<std::iter_reference_t<Iterator1>>)
  class concat {

    //static_assert(std::same_as<std::iter_value_t<Iterator1>, std::iter_value_t<Iterator2>>);
    //static_assert(std::same_as<std::iter_reference_t<Iterator1>, std::iter_reference_t<Iterator2>>);

    Iterator1 m_it1;
    Iterator2 m_it2;
    [[no_unique_address]] Sentinel1 m_end1;
    [[no_unique_address]] Sentinel2 m_end2;

    // 現在の位置の要素へのポインタ
    typename std::iter_value_t<Iterator1> *m_val = nullptr;
    // 前半の範囲内であるか？
    bool m_is_first_half = true;

  public:

    concat() = default;

    template<typename It1, typename It2>
    constexpr concat(It1&& it1, Sentinel1 end1, It2&& it2, Sentinel2 end2)
      : m_it1{ std::forward<It1>(it1) }
      , m_it2{ std::forward<It2>(it2) }
      , m_end1{ end1 }
      , m_end2{ end2 }
      , m_is_first_half{ m_it1 != m_end1 }
    {}

  private:

    class concat_iterator {
      concat* m_parent;

    public:
      using iterator_concept = std::input_iterator_tag;
      using iterator_category = std::input_iterator_tag;
      using value_type = typename std::iter_value_t<Iterator1>;
      //using pointer = value_type*;
      using reference = typename std::iter_reference_t<Iterator1>;
      using difference_type = std::ptrdiff_t;

      concat_iterator() = default;
      explicit constexpr concat_iterator(concat& parent) : m_parent{std::addressof(parent)} {}

      concat_iterator(const concat_iterator&) = delete;
      concat_iterator(concat_iterator&&) = default;

      concat_iterator& operator=(const concat_iterator&) = delete;
      concat_iterator& operator=(concat_iterator&&) = default;

      constexpr auto operator++() -> concat_iterator& {
        if (m_parent->m_is_first_half) {
          // 前半の範囲内の時
          ++m_parent->m_it1;
          // 進めた後、終端チェック
          if (m_parent->m_it1 != m_parent->m_end1) {
            // 要素へのポインタを取得しておく（operator*での条件分岐をなくすため）
            m_parent->m_val = std::addressof(*m_parent->m_it1);
          } else {
            // 後半部分へ進める
            m_parent->m_is_first_half = false;
            // 後半部分が空でない事をチェックし要素のポインタ更新
            if (m_parent->m_it2 != m_parent->m_end2) {
              m_parent->m_val = std::addressof(*m_parent->m_it2);
            }
          }
        } else {
          // 後半の範囲内の時
          ++m_parent->m_it2;
          // 終端チェック
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

      constexpr bool operator==(std::default_sentinel_t) const {
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
      return concat_iterator{ self };
    }

    [[nodiscard]]
    friend constexpr auto end(concat&) -> std::default_sentinel_t {
      return {};
    }

  };

  template<typename Iterator1, typename Sentinel1, typename Iterator2, typename Sentinel2>	
  concat(Iterator1, Sentinel1, Iterator2, Sentinel2) -> concat<Iterator1, Sentinel1, Iterator2, Sentinel2>;
    
} // namespace kusabira::vocabulary
