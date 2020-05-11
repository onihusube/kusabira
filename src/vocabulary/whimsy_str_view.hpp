#pragma once

#include <string>
#include <string_view>
#include <memory_resource>
#include <memory>

namespace kusabira::vocabulary {

  /**
  * @brief 基本は文字列を参照し、必要なら所有権を保有する、そんな文字列型
  * @details 基本的に初期化後に書き換えることは考えないので、メンバ関数は最小限
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
    void adaptive_construct(Self&& other) noexcept(std::is_rvalue_reference_v<decltype(other)> || std::is_nothrow_copy_constructible_v<string_t>) {
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
          new (&m_string) string_t(other.m_string, other.m_string.get_allocator()); //例外を投げるとしたらここ
        }
        m_is_view = other.m_is_view;
      }
    }

  public:
    using value_type = CharT;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = std::size_t;
    using iterator = typename strview_t::iterator;
    using const_iterator = typename strview_t::const_iterator;
    using difference_type = std::ptrdiff_t;

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
    explicit whimsy_str_view(const string_t& str) noexcept(std::is_nothrow_copy_constructible_v<string_t>) : m_string(str, str.get_allocator()), m_is_view(false) {}

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
    whimsy_str_view(whimsy_str_view&& other) noexcept(noexcept(adaptive_construct(std::move(other))))
      : whimsy_str_view{}
    {
      this->adaptive_construct(std::move(other));
    }

    /**
    * @brief コピー代入演算子
    */
    whimsy_str_view& operator=(const whimsy_str_view& other) & noexcept(std::is_nothrow_copy_constructible_v<whimsy_str_view<CharT>> && std::is_nothrow_swappable_v<whimsy_str_view<CharT>>) {
      if (this != &other) {
        auto copy = other;
        swap(*this, copy);
      }

      return *this;
    }

    /**
    * @brief ムーブ代入演算子
    */
    whimsy_str_view& operator=(whimsy_str_view&& other) & noexcept {
      if (this != &other) {
        swap(*this, other);
      }

      return *this;
    }

    /**
    * @brief string_viewの代入演算子
    */
    whimsy_str_view& operator=(whimsy_str_view::strview_t view) & {
      whimsy_str_view copy{ view };
      swap(*this, copy);

      return *this;
    }

    /**
    * @brief stringのコピー代入演算子
    */
    whimsy_str_view& operator=(const whimsy_str_view::string_t& str) & {
      whimsy_str_view copy{ str };
      swap(*this, copy);

      return *this;
    }

    /**
    * @brief stringのムーブ代入演算子
    */
    whimsy_str_view& operator=(whimsy_str_view::string_t&& str) & noexcept {
      whimsy_str_view move{ std::move(str) };
      swap(*this, move);

      return *this;
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
    * @brief 右辺値からの変換は危険
    */
    auto to_view() && -> strview_t = delete;

    /**
    * @brief stringへの明示的変換
    * @return 参照/保有する文字列をコピーしたstring
    */
    [[nodiscard]] auto to_string() const & noexcept -> string_t {
      if (m_is_view) {
        return string_t{ m_strview };
      } else {
        return {m_string, m_string.get_allocator()};
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

    /**
    * @brief 2つのwhimsy_str_viewを入れ替える
    * @details std::stringのコピー操作が入らないので例外は投げないはず・・・
    */
    friend void swap(whimsy_str_view& lhs, whimsy_str_view& rhs) noexcept {
      if (lhs.m_is_view) {
        if (rhs.m_is_view) {
          //両方ともview
          std::swap(lhs.m_strview, rhs.m_strview);
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
    [[nodiscard]] friend auto begin(const whimsy_str_view& self) noexcept -> const_pointer {
      if (self.m_is_view) {
        return self.m_strview.data();
      }
      else {
        return self.m_string.data();
      }
    }

    /**
    * @brief 終端イテレータを得る
    * @return 参照/保有する文字列の終端ポインタ
    */
    [[nodiscard]] friend auto end(const whimsy_str_view& self) noexcept -> const_pointer {
      if (self.m_is_view) {
        return self.m_strview.data() + self.m_strview.size();
      } else {
        return self.m_string.data() + self.m_string.size();
      }
    }

    /**
    * @brief 文字領域へのポインタを得る
    * @detail spanのために用意
    * @return 参照/保有する文字列の先頭ポインタ
    */
    [[nodiscard]] friend auto data(const whimsy_str_view& self) noexcept -> const_pointer {
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
      } else {
        return self.m_string.size();
      }
    }

    [[nodiscard]] friend auto operator==(const whimsy_str_view& lhs, const whimsy_str_view& rhs) noexcept -> bool {
      return lhs.to_view() == rhs;
    }

    [[nodiscard]] friend auto operator==(const whimsy_str_view& lhs, whimsy_str_view::strview_t rhs) noexcept -> bool {
      return lhs.to_view() == rhs;
    }


//#ifdef __cpp_impl_three_way_comparison
//
//    [[nodiscard]] friend auto operator<=>(const whimsy_str_view& lhs, const whimsy_str_view& rhs) const noexcept -> std::strong_ordering {
//      return lhs.to_view <=> rhs;
//    }
//#endif
  };
}