#pragma once

#include <filesystem>
#include <memory_resource>
#include <optional>
#include <system_error>

#define fn  [[nodiscard]] auto
#define fni [[nodiscard]] inline auto
#define fnc [[nodiscard]] constexpr auto

namespace kusabira {
  namespace fs = std::filesystem;

  using u8_pmralloc = std::pmr::polymorphic_allocator<char8_t>;

  using maybe_u8str = std::optional<std::pmr::u8string>;

  inline std::pmr::monotonic_buffer_resource def_mr{1024ul};
}

namespace kusabira::PP
{
  enum class pp_tokenize_status : int {
    Complete = 0,
    MissingRawStringDelimiter
  };

  class pp_tokenize_status_category : public std::error_category {
  
    fn name() const noexcept override -> const char* {
      return "tokenizer process status";
    }

    fn message(int ev) const override -> std::string {
      switch (pp_tokenize_status(ev))
      {
      case pp_tokenize_status::MissingRawStringDelimiter :
        return "";
      case pp_tokenize_status::Complete:
        [[fallthrough]];
      default:
        return "no error";
      }
    }

  };
} // namespace kusabira::PP

namespace std {
  struct is_error_code_enum<kusabira::PP::pp_tokenize_status> : std::true_type {};
}


