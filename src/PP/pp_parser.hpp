#pragma once

#include <list>
#include <forward_list>

#include "../common.hpp"
#include "file_reader.hpp"
#include "pp_automaton.hpp"
#include "pp_tokenizer.hpp"

namespace kusabira::PP
{
  enum class pp_token_category : std::uint8_t {
    comment,
    whitespaces,
    pp_directive,

    header_name,
    import_keyword,
    export_keyword,
    module_keyword,
    identifier,
    pp_number,
    charcter_literal,
    user_defined_charcter_literal,
    string_literal,
    user_defined_string_literal,
    op_or_punc,
    other_character
  };

  struct pp_token {
    //プリプロセッシングトークン種別
    pp_token_category category;
    //構成する字句トークン列
    std::forward_list<lex_token> tokens;
  };


  template<typename T = void>
  struct ll_paser {
    using Tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;
    using iterator = decltype(begin(std::declval<Tokenizer &>()));
    using sentinel = decltype(end(std::declval<Tokenizer &>()));

    std::pmr::list<pp_token> m_token_list{};

    fn start(Tokenizer &pp_tokenizer) -> void {
      auto it = begin(pp_tokenizer);
      auto se = end(pp_tokenizer);

      if (it == se) {
        //空のファイルだった
        return;
      }

      //先頭ホワイトスペース列を読み飛ばす（トークナイズがちゃんとしてれば改行文字は入ってないはず）
      while ((*it).kind == pp_tokenize_status::Whitespaces) {
        ++it;
      }

      //モジュールファイルを識別
      if (auto& top_token = *it; top_token.kind == pp_tokenize_status::Identifier) {
        using namespace std::string_view_literals;

        if (auto str = top_token.token; str == u8"module"sv or str == u8"export"sv) {
          this->module_file(it, se);
          //モジュールファイルとして読むため、終了後はそのまま終わり
          return;
        }
      }

      //通常のファイル
      this->group(it, se);
    }

    fn module_file(iterator &iter, sentinel end) -> void {
    }

    fn group(iterator& it, sentinel end) -> void {
      while (it != end) {
        group_part(it, end);
      }
    }

    fn group_part(iterator& it, sentinel end) -> void {
      using namespace std::string_view_literals;

      //ホワイトスペース列を読み飛ばす（トークナイズがちゃんとしてれば改行文字は入ってないはず）
      while (it != end and (*it).kind == pp_tokenize_status::Whitespaces)
      {
        ++it;
      }

      if (auto &token = *it; token.kind == pp_tokenize_status::OPorPunc) {
        if (token.token == u8"#"sv) {
          //多くのプリプロセッシングディレクティブ
        }
      }
    }

    fn control_line(Tokenizer &pp_tokenizer) -> void {
    }
  
    fn include_directive(Tokenizer &pp_tokenizer) -> void {
    }
  };

} // namespace kusabira::PP