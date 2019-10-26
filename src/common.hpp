#pragma once

#include <filesystem>
#include <memory_resource>
#include <optional>

#define fn  [[nodiscard]] auto
#define fni [[nodiscard]] inline auto
#define fnc [[nodiscard]] constexpr auto

namespace kusabira {
  namespace fs = std::filesystem;

  using u8_pmralloc = std::pmr::polymorphic_allocator<char8_t>;

  using maybe_u8str = std::optional<std::pmr::u8string>;

  inline std::pmr::monotonic_buffer_resource def_mr{1024ul};
}