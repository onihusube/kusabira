#pragma once

#include <fstream>
#include <filesystem>
#include <memory_resource>
#include <optional>
#include <forward_list>
#include <cctype>
#include <string>

#include "common.hpp"

namespace kusabira::PP {

  /**
  * @brief トークナイザのイテレータの番兵型
  */
  struct pp_token_iterator_end {
    using difference_type = std::size_t;
    using value_type = lex_token;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::input_iterator_tag;
  };


  /**
  * @brief トークナイザのイテレータ型
  * @tparam Tokenizer 対象となるトークナイザクラス
  */
  template<typename Tokenizer>
  class pp_token_iterator {
    Tokenizer* m_tokenizer = nullptr;
    std::optional<lex_token> m_token_opt = std::nullopt;

  public:
    using difference_type = std::size_t;
    using value_type = lex_token;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::input_iterator_tag;

    pp_token_iterator() = default;

    pp_token_iterator(Tokenizer& ref_tokenizer)
      : m_tokenizer{&ref_tokenizer}
    {
      m_token_opt = m_tokenizer->tokenize();
    }

    pp_token_iterator(const pp_token_iterator &) = default;
    pp_token_iterator &operator=(const pp_token_iterator &) = default;

    pp_token_iterator(pp_token_iterator&&) = default;
    pp_token_iterator &operator=(pp_token_iterator&&) = default;

    auto operator++() -> pp_token_iterator& {
      m_token_opt = m_tokenizer->tokenize();
      return *this;
    }

    fn operator*() -> reference {
      return *m_token_opt;
    }

    fn operator*() const -> const value_type& {
      return *m_token_opt;
    }

    fn operator==(const pp_token_iterator&) const noexcept -> bool {
      return false;
    }

    fn operator==(pp_token_iterator_end) const noexcept -> bool {
      //トークンを受けているoptionalが空ならファイル末尾に到達している
      return !m_token_opt.has_value();
    }

#ifndef __cpp_impl_three_way_comparison
    fn operator!=(pp_token_iterator_end end) const noexcept -> bool {
      return !(*this == end);
    }
#endif
  };


  /**
  * @brief ソースファイルからプリプロセッシングトークンを抽出する
  * @tparam FileReader ファイル読み込みを実装した型
  * @tparam Automaton 入力トークンを識別するオートマトンの型
  */
  template<typename FileReader, typename Automaton>
  class tokenizer {
    using line_iterator = std::pmr::forward_list<logical_line>::const_iterator;
    using char_iterator = std::pmr::u8string::const_iterator;

    //ファイルリーダー
    FileReader m_fr;
    //トークナイズのためのオートマトン
    Automaton m_accepter{};
    //行バッファ
    std::pmr::forward_list<logical_line> m_lines;
    //行位置
    line_iterator m_line_pos;
    //文字位置
    char_iterator m_pos{};
    //文字の終端
    char_iterator m_end{};
    //終了判定、ファイルを読み切ったらtrue
    bool m_is_terminate = false;
    //行末に到達しているかどうか
    bool m_is_endline = false;

    /**
    * @brief 現在の読み取り行を進める
    * @return ファイル終端に到達したか否か
    */
    fn readline() -> bool {
      if (auto line_opt = m_fr.readline(); line_opt) {
        //次の行の読み出しに成功したら、行バッファに入れておく（optionalを剥がす）
        m_line_pos = m_lines.emplace_after(m_line_pos, *std::move(line_opt));

        //現在の文字参照位置を更新
        auto& str = (*m_line_pos).line;
        m_pos = str.begin();
        m_end = str.end();

        //まだ行末かどうかはわからない
        m_is_endline = false;

        return false;
      } else {
        return true;
      }
    }

  public:
    using iterator = pp_token_iterator<tokenizer>;

    tokenizer(const fs::path& srcpath, std::pmr::memory_resource* mr = &kusabira::def_mr) 
      : m_fr{srcpath, mr}
      , m_lines{ std::pmr::polymorphic_allocator<logical_line>{mr} }
      , m_line_pos{m_lines.before_begin()}
    {
      //とりあえず1行読んでおく
      m_is_terminate = this->readline();
    }

    /**
    * @brief トークンを一つ切り出す
    * @return 切り出したトークンのoptional
    */
    fn tokenize() -> std::optional<lex_token> {
      using kusabira::PP::pp_tokenize_status;
      using kusabira::PP::pp_tokenize_result;

      //読み込み終了のお知らせ
      if (m_is_terminate == true) return std::nullopt;

      //行末に到達した
      if (m_is_endline == true) {
        //先にreadline()してしまうと更新されてしまうためバックアップ
        auto linepos = m_line_pos;

        //次の行を読み込む
        m_is_terminate = this->readline();

        return std::optional<lex_token>{ std::in_place, pp_tokenize_result{ pp_tokenize_status::NewLine }, std::u8string_view{}, std::move(linepos) };
      }

      //現在の先頭文字位置を記録
      const auto first = m_pos;

      //行末に到達するまで、1文字づつ読んでいく
      for (; m_pos != m_end; ++m_pos) {
        if (auto is_accept = m_accepter.input_char(*m_pos); is_accept) {
          //受理、エラーとごっちゃ
          return std::optional<lex_token>{std::in_place, std::move(is_accept), std::u8string_view{&*first, std::size_t(std::distance(first, m_pos))}, m_line_pos};
        } else {
          //非受理
          continue;
        }
      }
      //ここに出てきた場合、その行の文字を全て見終わったということ
      m_is_endline = true;

      //空行の時、不正なイテレータのデリファレンスをしないように
      auto token_str = (first == m_pos) ? std::u8string_view{} : std::u8string_view{&*first, std::size_t(std::distance(first, m_pos))};
      return std::optional<lex_token>{std::in_place, m_accepter.input_newline(), token_str, m_line_pos};;
    }

    ffn begin(tokenizer& attached_tokenizer) -> pp_token_iterator<tokenizer> {
      return pp_token_iterator<tokenizer>{attached_tokenizer};
    }

    ffn end(const tokenizer &) -> pp_token_iterator_end {
      return {};
    }
  };

} // namespace kusabira::PP