#pragma once

#include <fstream>
#include <filesystem>
#include <memory_resource>
#include <optional>
#include <forward_list>
#include <cctype>
#include <string>

#include "common.hpp"
#include "pp_automaton.hpp"

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

  /**
  * @brief ソースファイルから文字列を読み出す処理を担う
  * @detail 入力はUTF-8固定、BOM有無、改行コードはどちらでもいい
  */
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
        if (not m_buffer.empty() and m_buffer.back() == '\r') {
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
        //空行のチェック
        if (ll.line.empty() == false) {
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

  /**
  * @brief ソースファイルからプリプロセッシングトークンを抽出する
  * @detail ファイルの読み込みはこのクラスでは実装しない
  */
  struct tokenizer {
    using line_iterator = std::pmr::forward_list<logical_line>::iterator;
    using char_iterator = std::pmr::u8string::iterator;

    filereader m_fr;
    std::pmr::polymorphic_allocator<logical_line> m_alloc;

    std::pmr::forward_list<logical_line> m_lines; //行バッファ
    line_iterator m_line_pos; //行位置
    char_iterator m_pos{};  //文字位置
    char_iterator m_end{};  //文字の終端

    tokenizer(const fs::path& srcpath, std::pmr::memory_resource* mr = &kusabira::def_mr) 
      : m_fr{srcpath, mr}
      , m_alloc{mr}
      , m_lines{m_alloc}
      , m_line_pos{m_lines.before_begin()}
    {}

    /**
    * @brief 現在の読み取り行を進める
    * @return 行読み取りに成功したか否か
    */
    fn readline() -> bool {
      if (auto line_opt = m_fr.readline(); line_opt) {
        //次の行の読み出しに成功したら、行バッファに入れておく（optionalを剥がす）
        m_line_pos = m_lines.emplace_after(m_line_pos, *std::move(line_opt));

        //現在の文字参照位置を更新
        auto& str = (*m_line_pos).line;
        m_pos = str.begin();
        m_end = str.end();

        return true;
      }
      else {
        return false;
      }
    }

    /**
    * @brief 文字を1文字読み進める
    * @param mode 読み取りモード（ホワイトスペース/非空白文字）
    * @param pos 現在の読み取り位置
    * @return 読み取り対象以外に達したらfalse
    */
    fn skip_read(bool mode, char_iterator& pos) -> bool {
      ++pos;
      bool is_space = std::isspace(static_cast<unsigned char>(*pos));

      if (mode == true) {
        //ホワイトスペース読み取りモード（ホワイトスペースである間読み取り）
        return is_space;
      } else {
        //非空白文字読み取りモード（ホワイトスペースではない間読み取り）
        return !is_space;
      }
    }

    /**
    * @brief ホワイトスペースとそれ以外を分別する
    * @return ホワイトスペース/それ以外の文字列と付随情報
    */
    fn separate_whitspace() -> std::optional<std::u8string_view> {
      //現在の文字位置が行終端に到達していたら次の行を読む
      if (m_pos != m_end) {
        //次の行の読み込みができなかったら終了
        if (this->readline() == false) return std::nullopt;
      }

      //1文字目はホワイトスペースかそれ以外かで読み取るものが変わる
      bool mode = std::isspace(static_cast<unsigned char>(*m_pos)) ? true : false;
      //先頭位置
      const auto first = m_pos;

      do {
        if (skip_read(mode, m_pos) == false) break;
      } while (m_pos != m_end); //行末に到達しても終了

      //ホワイトスペースかそれ以外の文字、の列（のview）
      return std::optional<std::u8string_view>(std::in_place, &*first, std::distance(first, m_pos));
    }
  };

} // namespace kusabira::PP