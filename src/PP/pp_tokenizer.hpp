#pragma once

#include <fstream>
#include <filesystem>
#include <memory_resource>
#include <optional>

#include "common.hpp"

namespace kusabira::PP {

  
  /**
  * @brief ソースコードの論理行での1行を表現する型
  * @detail 改行継続（バックスラッシュ+改行）後にソースファイルの物理行との対応を取るためのもの
  */
  struct logical_line {
    //論理1行の文字列
    maybe_u8str line;
    //元の開始行
    std::size_t phisic_line;
    //1行毎の文字列長、このvectorの長さ=継続行数
    std::pmr::vector<std::size_t> line_offset;
  };

  class filereader {

    std::ifstream m_srcstream;
    u8_pmralloc m_alloc;
    std::pmr::string m_buffer;

  public:

    filereader(const fs::path& filepath, std::pmr::memory_resource* mr = &kusabira::def_mr)
      : m_srcstream{filepath}
      , m_alloc{mr}
      , m_buffer{m_alloc}
    {
      m_srcstream.sync_with_stdio(false);
      m_srcstream.tie(nullptr);

      //BOMスキップ
      if (m_srcstream) {
        int first = m_srcstream.peek();
        if (first == 0xef) {
          m_srcstream.seekg(3, std::ios::beg);
        }
      }
    }

    fn readline() -> maybe_u8str {

      if (std::getline(m_srcstream, m_buffer)) {
        const auto first = reinterpret_cast<char8_t*>(m_buffer.data());
        auto last = first + m_buffer.size();

#ifndef _MSC_VER
        //CRLF読み取り時にCRが残っていたら飛ばす、非Windowsの時のみ
        if (m_buffer.back() == '\r') {
          --last;
        }
#endif

        return maybe_u8str{std::in_place, first, last, m_alloc };
      } else {
        return std::nullopt;
      }
    }

    explicit operator bool() const noexcept {
      return bool(m_srcstream);
    }
  };

  struct tokenizer {
    filereader m_fr;
    u8_pmralloc m_alloc;
    std::pmr::vector<std::pmr::u8string> m_lines;

    tokenizer(const fs::path& srcpath, std::pmr::memory_resource* mr = &kusabira::def_mr) 
      : m_fr{srcpath, mr}
      , m_alloc{mr}
      , m_lines{m_alloc}
    {
      m_lines.reserve(50);
    }

    // fn operator()() -> std::u8string_view {
    //   auto& line = m_lines.emplace_back(m_fr.readline());

    // }
  };

} // namespace kusabira::PP