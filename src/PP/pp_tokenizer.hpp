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

    logical_line(u8_pmralloc& alloc, std::size_t line_num)
      : line{alloc}
      , phisic_line{line_num}
      , line_offset{alloc}
    {}

    //論理1行の文字列
    std::pmr::u8string line;
    //元の開始行
    std::size_t phisic_line;
    //1行毎の文字列長、このvectorの長さ=継続行数
    std::pmr::vector<std::size_t> line_offset;
  };

  using maybe_line = std::optional<logical_line>;

  class filereader {

    std::ifstream m_srcstream;
    u8_pmralloc m_alloc;
    std::pmr::string m_buffer;
    std::size_t m_line_num = 1;

    /**
    * @brief ファイルから1行を読み込み、引数末尾に追記する
    * @param str 結果を追記する文字列
    * @return 読み込み成否を示すbool値
    */
    fn readline_impl(std::pmr::u8string& str) -> bool {
      if (std::getline(m_srcstream, m_buffer)) {
        const auto first = reinterpret_cast<char8_t*>(m_buffer.data());
        auto last = first + m_buffer.size();

#ifndef _MSC_VER
        //CRLF読み取り時にCRが残っていたら飛ばす、非Windowsの時のみ
        if (m_buffer.back() == '\r') {
          --last;
        }
#endif
        str.append(first, last);

        //物理行数をカウント
        ++m_line_num;

        return true;
      } else {
        return false;
      }
    }

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

    /**
    * @brief ファイルからソースコードの論理1行を取得する
    * @detail BOMはスキップされ、末尾に改行コードは現れず、バックスラッシュによる行継続が処理済
    * @detail すなわち、翻訳フェーズ2までを完了する（1行分）
    * @return 論理行型のoptional
    */
    fn readline() -> maybe_line {
      logical_line ll{m_alloc, m_line_num};

      if (this->readline_impl(ll.line)) {

        //バックスラッシュによる行継続と論理行生成
        while (ll.line.back() == u8'\x5c') {
          auto last = ll.line.end() - 1;
          //バックスラッシュ2つならびは行継続しない
          if (*(last - 1) == u8'\x5c') break;

          //行末尾バックスラッシュを削除
          ll.line.erase(last);
          //現在までの行の文字列長を記録
          ll.line_offset.emplace_back(ll.line.length());

          //次の行を読み込み、追記
          if (this->readline_impl(ll.line) == false) break;
          //読み込みに成功したら再び行継続チェック
        }

        return maybe_line{std::in_place, std::move(ll)};
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