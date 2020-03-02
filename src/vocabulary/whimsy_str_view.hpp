#include <string>
#include <string_view>
#include <memory_resource>
#include <memory>

namespace kusabira::vocabulary {
  
  template<typename CharT = char8_t, typename Allocator = std::pmr::polymorphic_allocator<CharT>>
  class whimsy_str_view {
    using string_t  = std::basic_string<CharT, Allocator>;
    using strview_t = std::basic_string_view<CharT>;

    union {
      string_t  m_string;
      strview_t m_strview;
    };
    bool m_is_view = true;

    template<typename Self>
    void adaptive_construct(Self&& other) {
      if (m_is_view) {
        if (other.m_is_view) {
          //両方ともview
          m_strview = other.m_strview;
          return;
        } else {
          //相手はviewじゃない
          std::destroy_at(m_strview);
          new (&m_string) string_t(other.m_string);
        }
      } else {
        if (other.m_is_view) {
          //相手はview
          std::destroy_at(m_string);
          new (&m_strview) string_t(other.m_strview);
        } else {
          //両方ともstring
          if constexpr (std::is_rvalue_reference_v<decltype(other)>) {
            m_string = std::move(other.m_string); //違うのここだけ・・・・
          } else {
            m_string = other.m_string;
          }
          return;
        }
      }
      m_is_view = other.m_is_view;
    }

  public:

    constexpr whimsy_str_view() noexcept : m_strview() {}

    constexpr whimsy_str_view(std::basic_string_view<CharT> view) noexcept : m_strview(view) {}

    whimsy_str_view(const string_t& str) : m_string(str), m_is_view(false) {}

    whimsy_str_view(string_t&& str) : m_string(std::move(str)), m_is_view(false) {}

    whimsy_str_view(const whimsy_str_view& other) {
      this->adaptive_construct(other);
    }

    whimsy_str_view(whimsy_str_view&& other) {
      this->adaptive_construct(std::move(other));
    }

    operator strview_t() const & {
      if (m_is_view) {
        return m_strview;
      } else {
        return strview_t{m_string};
      }
    }

  };
}