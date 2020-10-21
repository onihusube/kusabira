#pragma once

#include <fstream>
#include <filesystem>
#include <memory_resource>
#include <optional>
#include <forward_list>
#include <cctype>
#include <string>

#include "common.hpp"

namespace kusabira::PP::inline tokenizer_v2 {

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

    tokenizer(const fs::path& srcpath) 
      : m_fr{srcpath, &kusabira::def_mr}
      , m_lines{ &kusabira::def_mr }
      , m_line_pos{m_lines.before_begin()}
    {
      //とりあえず1行読んでおく
      m_is_terminate = this->readline();
    }

    tokenizer(const tokenizer &) = delete;
    tokenizer &operator=(const tokenizer &) = delete;

    tokenizer(tokenizer &&) = default;
    tokenizer &operator=(tokenizer &&) = default;

    /**
    * @brief トークンを一つ切り出す
    * @return 切り出したトークンのoptional
    */
    fn tokenize() -> std::optional<pp_token> {
      using kusabira::PP::pp_token_category;

      //読み込み終了のお知らせ
      if (m_is_terminate == true) return std::nullopt;

      //行末に到達した
      if (m_is_endline == true) {
        //先にreadline()してしまうと更新されてしまうためバックアップ
        auto linepos = m_line_pos;
        auto length = (*linepos).line.length();

        //次の行を読み込む
        m_is_terminate = this->readline();

        return std::optional<pp_token>{std::in_place, pp_token_category::newline, std::u8string_view{}, length, std::move(linepos)};
      }

      //現在の先頭文字位置を記録
      const auto first = m_pos;

      //行末に到達するまで、1文字づつ読んでいく
      for (; m_pos != m_end; ++m_pos) {
        if (auto is_accept = m_accepter.input_char(*m_pos); is_accept != pp_token_category::Unaccepted) {
          //受理、エラーとごっちゃ
          std::size_t length = std::distance((*m_line_pos).line.cbegin(), first);
          return std::optional<pp_token>{std::in_place, is_accept, std::u8string_view{&*first, std::size_t(std::distance(first, m_pos))}, length, m_line_pos};
        } else {
          //非受理
          continue;
        }
      }
      //ここに出てきた場合、その行の文字を全て見終わったということ
      //改行の一つ前までのトークンが来ているはず
      m_is_endline = true;

      //空行の時、不正なイテレータのデリファレンスをしないようにする
      std::u8string_view token_str{};
      std::size_t length{};
      if (first != m_pos) {
        //空行ではなく何らかのトークンを識別しているとき
        token_str = std::u8string_view{ &*first, std::size_t(std::distance(first, m_pos)) };
        length = std::distance((*m_line_pos).line.cbegin(), first);
      }

      return std::optional<pp_token>{std::in_place, m_accepter.input_newline(), token_str, length, m_line_pos};;
    }

  private:

    // トークナイズ結果一つ分を一時保存しておく、イテレータを可搬かつ軽量にするため
    std::optional<pp_token> m_elem = std::nullopt;

    // イテレータを進める処理
    void iter_increment() {
      this->m_elem = this->tokenize();
    }

    class tokenizer_iterator {
      tokenizer* m_parent = nullptr;

    public:
      using iterator_concept = std::input_iterator_tag;
      //using iterator_category = std::input_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = pp_token;

      tokenizer_iterator() = default;

      explicit tokenizer_iterator(tokenizer& parent) : m_parent{std::addressof(parent)}
      {}

      tokenizer_iterator(const tokenizer_iterator&) = delete;
      tokenizer_iterator &operator=(const tokenizer_iterator &) = delete;

      tokenizer_iterator(tokenizer_iterator&&) = default;
      tokenizer_iterator &operator=(tokenizer_iterator&&) = default;

      fn operator*() const -> value_type& {
        return *(m_parent->m_elem);
      }

      auto operator++() -> tokenizer_iterator& {
        m_parent->iter_increment();
        return *this;
      }

      void operator++(int) {
        ++*this;
      }

      fn operator==(std::default_sentinel_t) const noexcept -> bool {
        return m_parent == nullptr or m_parent->m_elem == std::nullopt;
      }
    };

  public:

    using iterator = tokenizer_iterator;

    ffn begin(tokenizer& self) -> tokenizer_iterator {
      self.iter_increment();
      return tokenizer_iterator{self};
    }

    ffn end(tokenizer&) -> std::default_sentinel_t {
      return {};
    }
  };
}