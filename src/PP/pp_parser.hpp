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

    static fn start(Tokenizer &pp_tokenizer) -> void {
      auto it = begin(pp_tokenizer);
      auto se = end(pp_tokenizer);

      if (it == end) {
        //空のファイルだった
        return;
      }
      
      group(it, end);
    }

    static fn group(iterator& iter, sentinel end) -> void {
      group_part(iter, end);
    }

    static fn group_part(iterator& iter, sentinel end) -> void {

    }

    static fn control_line(Tokenizer &pp_tokenizer) -> void {
    }
  
    static fn include_directive(Tokenizer &pp_tokenizer) -> void {
    }
  };

} // namespace kusabira::PP