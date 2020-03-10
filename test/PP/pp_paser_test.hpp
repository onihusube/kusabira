#pragma once

#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"
#include "PP/pp_parser.hpp"

namespace pp_paser_test {

  TEST_CASE("make error test") {

    using namespace std::literals;
    using kusabira::PP::lex_token;
    using kusabira::PP::pp_parse_context;
    using kusabira::PP::pp_tokenize_status;

    std::vector<lex_token> test_tokens{};
    test_tokens.emplace_back(lex_token{{pp_tokenize_status::Identifier}, u8"test"sv, {}});

    using kusabira::PP::make_error;

    auto it = std::begin(test_tokens);
    auto err = make_error(it, pp_parse_context::GroupPart);

    //イテレータを進めない
    CHECK_NE(it, std::end(test_tokens));

    CHECK_UNARY_FALSE(bool(err));
    CHECK_EQ(err.error().context, pp_parse_context::GroupPart);
    CHECK_UNARY(err.error().token.token == u8"test"sv);
  }

  TEST_CASE("skip whitespace token test") {

    using kusabira::PP::pp_tokenize_status;

    //テストのためのトークン型
    struct test_token {
      kusabira::PP::pp_tokenize_status kind;
    };

    //トークン列を準備
    std::vector<test_token> test_tokens{};
    test_tokens.reserve(23);

    for (auto i = 0u; i < 5u; ++ i) {
      test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::Whitespaces});
    }
    test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::Identifier});

    for (auto i = 0u; i < 7u; ++ i) {
      test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::BlockComment});
    }
    test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::NumberLiteral});

    for (auto i = 0u; i < 11u; ++ i) {
      test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::Whitespaces});
      test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::BlockComment});
    }
    test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::StringLiteral});
    test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::NewLine});

    //テスト開始
    auto it = std::begin(test_tokens);
    auto end = std::end(test_tokens);

    using kusabira::PP::skip_whitespaces;

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).kind, pp_tokenize_status::Identifier);

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).kind, pp_tokenize_status::NumberLiteral);

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).kind, pp_tokenize_status::StringLiteral);

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).kind, pp_tokenize_status::NewLine);

    CHECK_UNARY_FALSE(skip_whitespaces(it, end));
  }

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