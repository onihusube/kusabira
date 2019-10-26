#pragma once

#include <fstream>
#include <filesystem>
#include <memory_resource>
#include <optional>

#include "common.hpp"

namespace kusabira::PP {

  struct filereader {

    filereader(const fs::path& filepath, std::pmr::memory_resource* mr = &kusabira::def_mr)
      : m_srcstream{filepath}
      , m_alloc{mr}
      , m_buffer{m_alloc}
    {}

    fn readline() -> maybe_u8str {

      if (std::getline(m_srcstream, m_buffer)) {
        auto first = reinterpret_cast<char8_t*>(m_buffer.data());
        auto last = first + m_buffer.size();

        return std::pmr::u8string{first, last, m_alloc};
      } else {
        return std::nullopt;
      }
    }

    explicit operator bool() const noexcept {
      return bool(m_srcstream);
    }

  private:
    std::ifstream m_srcstream;
    u8_pmralloc m_alloc;
    std::pmr::string m_buffer;
  };

  struct tokenizer {

  };

} // namespace kusabira::PP