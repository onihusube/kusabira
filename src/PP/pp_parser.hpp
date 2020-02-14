#pragma once

#include "common.hpp"
#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"

namespace kusabira::PP
{

  template<typename T = void>
  struct ll_paser {
    using Tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;
    using iterator = decltype(begin(std::declval<Tokenizer &>()));
    using sentinel = decltype(end(std::declval<Tokenizer &>()));

    static fn start(Tokenizer &pp_tokenizer) ->void
    {
    }

    static fn group(Tokenizer &pp_tokenizer) -> void {
    }

    static fn group_part(Tokenizer &pp_tokenizer) -> void {
    }

    static fn control_line(Tokenizer &pp_tokenizer) -> void {
    }
  
    static fn include_directive(Tokenizer &pp_tokenizer) -> void {
    }
  };

} // namespace kusabira::PP