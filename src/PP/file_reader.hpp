#pragma once

#include <fstream>
#include <filesystem>
#include <memory_resource>
#include <optional>

#include "common.hpp"

namespace kusabira::PP {

  /**
  * @brief ソースファイルから文字列を読み出す処理を担う
  * @detail 入力はUTF-8固定、BOM有無、改行コードはどちらでもいい
  */
  class filereader {

    using maybe_line = std::optional<logical_line>;

    //入力ファイルストリーム
    std::ifstream m_srcstream;
    //1行分を一時的に持っておくバッファ
    std::pmr::string m_buffer;
    //現在の物理行数
    std::size_t m_pline_num = 1;
    //現在の論理行数
    std::size_t m_lline_num = 1;

    /**
    * @brief ファイルから1行を読み込み、引数末尾に追記する
    * @param str 結果を追記する文字列
    * @return 読み込み成否を示すbool値
    */
    fn readline_impl(std::pmr::u8string &str) -> bool {
      if (std::getline(m_srcstream, m_buffer)) {
        const auto first = reinterpret_cast<char8_t *>(m_buffer.data());
        auto last = first + m_buffer.size();

  #ifndef _MSC_VER
        //CRLF読み取り時にCRが残っていたら飛ばす、非Windowsの時のみ
        //バグの元になる可能性はある？要検証
        if (not m_buffer.empty() and m_buffer.back() == '\r') {
          --last;
        }
  #endif
        str.append(first, last);

        //物理行数をカウント
        ++m_pline_num;

        return true;
      } else {
        return false;
      }
    }

  public:

    filereader(const fs::path &filepath, std::pmr::memory_resource *mr = &kusabira::def_mr)
        : m_srcstream{filepath}
        , m_buffer{mr}
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
      logical_line ll{m_pline_num, m_lline_num};

      //論理行数カウント
      ++m_lline_num;

      if (this->readline_impl(ll.line)) {
        //空行のチェック
        if (ll.line.empty() == false) {
          //バックスラッシュによる行継続と論理行生成
          while (ll.line.back() == u8'\x5c') {
            auto last = ll.line.end() - 1;
            //バックスラッシュ2つならびは行継続しない
            if (*(std::prev(last)) == u8'\x5c') break;

            //行末尾バックスラッシュを削除
            ll.line.erase(last);
            //現在までの行の文字列長を記録
            ll.line_offset.emplace_back(ll.line.length());

            //次の行を読み込み、追記
            if (this->readline_impl(ll.line) == false) break;
            //読み込みに成功したら再び行継続チェック
          }
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

} // namespace kusabira::PP
