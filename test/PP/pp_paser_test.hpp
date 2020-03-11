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

  //テストのためのトークン型
  struct test_token {
    kusabira::PP::pp_tokenize_status kind;
  };

  TEST_CASE("skip whitespace token test") {

    using kusabira::PP::pp_tokenize_status;

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

  TEST_CASE("status to category test") {
    
    using ll_paser = kusabira::PP::ll_paser<kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>>;
    using kusabira::PP::pp_tokenize_status;
    using kusabira::PP::pp_token_category;

    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::Identifier), pp_token_category::identifier);
    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::NumberLiteral), pp_token_category::pp_number);
    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::OPorPunc), pp_token_category::op_or_punc);
    CHECK_EQ(ll_paser::tokenize_status_to_category(pp_tokenize_status::OtherChar), pp_token_category::other_character);
  }

  TEST_CASE("string literal calssify test") {
    
    using ll_paser = kusabira::PP::ll_paser<kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>>;
    using kusabira::PP::pp_tokenize_status;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;

    //トークン列を準備
    std::vector<test_token> test_tokens{};
    test_tokens.reserve(12);
    test_tokens.emplace_back(test_token{.kind = pp_tokenize_status::Identifier});

    //一つ前に出現したトークン
    kusabira::PP::pp_token pptoken{pp_token_category::string_literal};

    auto it = std::begin(test_tokens);

    //文字リテラル -> ユーザー定義文字リテラル
    CHECK_UNARY(ll_paser::strliteral_classify(it, u8R"**('c')**"sv, pptoken));
    CHECK_EQ(pptoken.category, pp_token_category::user_defined_charcter_literal);

    pptoken.category = pp_token_category::string_literal;

    //文字列リテラル -> ユーザー定義文字列リテラル
    CHECK_UNARY(ll_paser::strliteral_classify(it, u8R"**("string")**"sv, pptoken));
    CHECK_EQ(pptoken.category, pp_token_category::user_defined_string_literal);

    pptoken.category = pp_token_category::raw_string_literal;

    //生文字列リテラル -> ユーザー定義生文字列リテラル
    CHECK_UNARY(ll_paser::strliteral_classify(it, u8R"**(R"raw string")**"sv, pptoken));
    CHECK_EQ(pptoken.category, pp_token_category::user_defined_raw_string_literal);

    //何もしないはずのトークン種別の入力
    test_tokens.clear();
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::Whitespaces });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::LineComment });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::BlockComment });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::NumberLiteral });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::StringLiteral });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::RawStrLiteral });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::DuringRawStr });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::OPorPunc });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::OtherChar });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::NewLine });
    test_tokens.emplace_back(test_token{ .kind = pp_tokenize_status::Empty });

    it = std::begin(test_tokens);
    auto end = std::end(test_tokens);

    pptoken.category = pp_token_category::op_or_punc;

    for (; it != end; ++it) {
      CHECK_UNARY_FALSE(ll_paser::strliteral_classify(it, u8R"**(')**"sv, pptoken));
      //CHECK_EQ(pptoken.category, pp_token_category::op_or_punc);
    }

  }
}