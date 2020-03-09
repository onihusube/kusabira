#pragma once

#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"
#include "PP/pp_parser.hpp"

namespace pp_paser_test {

  TEST_CASE("status to category test"){
    
    using ll_paser = kusabira::PP::ll_paser<kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>>;
    using kusabira::PP::pp_tokenize_status;
    using kusabira::PP::pp_token_category;

    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::Identifier), pp_token_category::identifier);
    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::NumberLiteral), pp_token_category::pp_number);
    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::OPorPunc), pp_token_category::op_or_punc);
    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::OtherChar), pp_token_category::other_character);
  }
}