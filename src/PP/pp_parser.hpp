#pragma once

#include "../common.hpp"
#include "file_reader.hpp"
#include "pp_automaton.hpp"
#include "pp_tokenizer.hpp"

namespace kusabira::PP
{

  template<typename T = void>
  struct ll_paser {
    using Tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;
    using iterator = decltype(begin(std::declval<Tokenizer &>()));
    using sentinel = decltype(end(std::declval<Tokenizer &>()));

    static fn start(Tokenizer &pp_tokenizer) -> void {
      auto it = begin(pp_tokenizer);
      auto se = end(pp_tokenizer);

      if (it == end) {
        //‹ó‚Ìƒtƒ@ƒCƒ‹‚¾‚Á‚½
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