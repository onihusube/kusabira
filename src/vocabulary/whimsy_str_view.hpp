#pragma once

#include <string>
#include <string_view>
#include <memory_resource>
#include <memory>

namespace kusabira::vocabulary {

  /**
  * @brief 基本は文字列を参照し、必要なら所有権を保有する、そんな文字列型
  * @detail 基本的に初期化後に書き換えることは考えないので、メンバ関数は最小限
  */
  template<typename CharT = char8_t, typename Allocator = std::pmr::polymorphic_allocator<CharT>>
  class whimsy_str_view {
    using string_t  = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;
    using strview_t = std::basic_string_view<CharT>;

    union {
      string_t  m_string;
      strview_t m_strview;
    };
    bool m_is_view = true;

    /**
    * @brief コピー/ムーブコンストラクタ処理の共通化、最初はデフォルト構築されているものとする
    */
    template<typename Self>
    void adaptive_construct(Self&& other) {
      if (other.m_is_view == true) {
        //両方ともview
        m_strview = other.m_strview;
        return;
      } else {
        //相手はviewじゃない
        std::destroy_at(&m_strview);
        if constexpr (std::is_rvalue_reference_v<decltype(other)>) {
          new (&m_string) string_t(std::move(other.m_string)); //ムーブする
        } else {
          new (&m_string) string_t(other.m_string);
        }
      }
      m_is_view = other.m_is_view;
    }

  public:
    using value_type = CharT;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = std::size_t;
    using iterator = strview_t::iterator;
    using const_iterator = strview_t::const_iterator;
    using difference_type = std::ptrdiff_t;

  public:

    /**
    * @brief デフォルトコンストラクタ、空文字列を参照するviewとして初期化
    */
    constexpr whimsy_str_view() noexcept : m_strview() {}

    /**
    * @brief string_viewからの変換コンストラクタ
    */
    constexpr explicit whimsy_str_view(strview_t view) noexcept : m_strview(view) {}

    /**
    * @brief stringからのコピーコンストラクタ
    */
    whimsy_str_view(const string_t& str) noexcept(std::is_nothrow_copy_constructible_v<string_t>) : m_string(str), m_is_view(false) {}

    /**
    * @brief stringからのムーブコンストラクタ
    */
    whimsy_str_view(string_t&& str) noexcept : m_string(std::move(str)), m_is_view(false) {}

    /**
    * @brief コピーコンストラクタ
    */
    whimsy_str_view(const whimsy_str_view& other) noexcept(noexcept(adaptive_construct(other)))
      : whimsy_str_view{}
    {
      this->adaptive_construct(other);
    }

    /**
    * @brief ムーブコンストラクタ
    */
    whimsy_str_view(whimsy_str_view &&other) noexcept(noexcept(adaptive_construct(std::move(other))))
      : whimsy_str_view{}
    {
      this->adaptive_construct(std::move(other));
    }

    /**
    * @brief デストラクタ
    */
    ~whimsy_str_view() noexcept {
      if (m_is_view) {
        std::destroy_at(&m_strview);
      } else {
        std::destroy_at(&m_string);
      }
    }

    /**
    * @brief string_viewへの暗黙変換
    * @detail 基本的にはこれを通して利用する
    * @return 参照/保有する文字列を参照するstring_view
    */
    [[nodiscard]] operator strview_t() const & noexcept {
      if (m_is_view) {
        return m_strview;
      } else {
        return m_string;
      }
    }

    /**
    * @brief 文字列を所有しているか（viewなのかどうか）を取得
    * @return trueなら文字列を保持している、falseならviewである
    */
    [[nodiscard]] auto has_own_string() const noexcept -> bool {
      return !m_is_view;
    }

    /**
    * @brief 文字列を所有しているか（viewなのかどうか）を取得
    * @return trueなら文字列を保持している、falseならviewである
    */
    [[nodiscard]] explicit operator bool() const noexcept {
      return this->has_own_string();
    }

    /**
    * @brief string_viewへの明示的変換
    * @return 参照/保有する文字列を参照するstring_view
    */
    [[nodiscard]] auto to_view() const & noexcept -> strview_t {
      return *this;
    }

    /**
    * @brief stringへの明示的変換
    * @return 参照/保有する文字列をコピーしたstring
    */
    [[nodiscard]] auto to_string() const & noexcept -> string_t {
      if (m_is_view) {
        return string_t{ m_strview };
      } else {
        return m_string;
      }
    }

    /**
    * @brief stringへの明示的変換
    * @return 参照/保有する文字列をムーブしたstring
    */
    [[nodiscard]] auto to_string() && noexcept -> string_t {
      if (m_is_view) {
        return string_t{ m_strview };
      } else {
        return std::move(m_string);
      }
    }

    friend void swap(whimsy_str_view& lhs, whimsy_str_view& rhs) {
      if (lhs.m_is_view) {
        if (rhs.m_is_view) {
          //両方ともview
          std::swap(lhs.m_strview, rhs.other.m_strview);
          return;
        } else {
          //右辺はviewじゃない
          //コピーしとく
          auto copy = lhs.m_strview;
          //左辺(lhs)から処理
          std::destroy_at(&lhs.m_strview);
          new (&lhs.m_string) string_t(std::move(rhs.m_string));
          //右辺(rhs)を処理
          std::destroy_at(&rhs.m_string);
          new (&rhs.m_strview) strview_t(copy);
        }
      } else {
        if (rhs.m_is_view) {
          //右辺はview
          //コピーしとく
          auto copy = rhs.m_strview;
          //右辺(rhs)から処理
          std::destroy_at(&rhs.m_strview);
          new (&rhs.m_string) string_t(std::move(lhs.m_string));
          //左辺(lhs)を処理
          std::destroy_at(&lhs.m_string);
          new (&lhs.m_strview) strview_t(copy);
        } else {
          //両方ともstring
          std::swap(lhs.m_string, rhs.m_string);
          return;
        }
      }
      //最後にフラグをswap
      std::swap(lhs.m_is_view, rhs.m_is_view);
    }

    /**
    * @brief 先頭イテレータを得る
    * @return 参照/保有する文字列の先頭ポインタ
    */
    [[nodiscard]] friend auto begin(const whimsy_str_view& self) noexcept -> iterator {
      if (self.m_is_view) {
        return self.m_strview.begin();
      }
      else {
        return self.m_string.data();
      }
    }

    /**
    * @brief 終端イテレータを得る
    * @return 参照/保有する文字列の終端ポインタ
    */
    [[nodiscard]] friend auto end(const whimsy_str_view& self) noexcept -> iterator {
      if (self.m_is_view) {
        return self.m_strview.end();
      } else {
        return self.m_string.data() + self.m_string.size();
      }
    }

    /**
    * @brief 文字領域へのポインタを得る
    * @detail spanのために用意
    * @return 参照/保有する文字列の先頭ポインタ
    */
    [[nodiscard]] friend auto data(const whimsy_str_view& self) noexcept -> pointer {
      if (self.m_is_view) {
        return self.m_strview.data();
      } else {
        return self.m_string.data();
      }
    }

    /**
    * @brief 文字のサイズを得る
    * @detail spanのために用意
    * @return 参照/保有する文字列のサイズ
    */
    [[nodiscard]] friend auto size(const whimsy_str_view& self) noexcept -> size_type {
      if (self.m_is_view) {
        return self.m_strview.size();
      }
      else {
        return self.m_string.size();
      }
    }
  };
}