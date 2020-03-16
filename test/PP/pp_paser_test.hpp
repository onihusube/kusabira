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


  TEST_CASE("build raw string test") {

    using ll_paser = kusabira::PP::ll_paser<kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>>;
    using kusabira::PP::lex_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_tokenize_result;
    using kusabira::PP::pp_tokenize_status;
    using namespace std::literals;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //トークン列
    std::vector<lex_token> tokens{};
    tokens.reserve(10);

    auto pos = ll.before_begin();

    //論理行オブジェクト1
    pos = ll.emplace_after(pos, 0);
    (*pos).line = u8"R\"(testrawstringriteral";
    (*pos).line_offset.emplace_back(7);
    (*pos).line_offset.emplace_back(7 + 3);
    (*pos).line_offset.emplace_back(7 + 3 + 6);
    {
      auto &r = tokens.emplace_back(pp_tokenize_result{.status = pp_tokenize_status::DuringRawStr}, (*pos).line, pos);
      CHECK_UNARY(r.is_multiple_phlines());
    }

    //論理行オブジェクト2
    pos = ll.emplace_after(pos, 4);
    (*pos).line = u8"testline1";
    (*pos).line_offset.emplace_back(4);
    {
      auto &r = tokens.emplace_back(pp_tokenize_result{.status = pp_tokenize_status::DuringRawStr}, (*pos).line, pos);
      CHECK_UNARY(r.is_multiple_phlines());
    }

    //論理行オブジェクト３
    pos = ll.emplace_after(pos, 6);
    (*pos).line = u8"testlogicalline2)\"";
    (*pos).line_offset.emplace_back(4);
    (*pos).line_offset.emplace_back(4 + 7);
    {
      auto &r = tokens.emplace_back(pp_tokenize_result{.status = pp_tokenize_status::RawStrLiteral}, (*pos).line, pos);
      CHECK_UNARY(r.is_multiple_phlines());
    }

    auto it = std::begin(tokens);
    auto end = std::end(tokens);

    auto pptoken = ll_paser::read_rawstring_tokens(it, end);

    //生文字列リテラル中の行継続は無視してそのままにしなければならない
    auto expect = u8R"**(R"(test\
raw\
string\
riteral
test\
line1
test\
logical\
line2)")**"sv;

    CHECK_UNARY(pptoken.token == expect);
    CHECK_EQ(pptoken.category, pp_token_category::raw_string_literal);
    CHECK_EQ(std::distance(pptoken.lextokens.begin(), pptoken.lextokens.end()), 3u);

    //途中で切れてるやつ
    tokens.clear();
    pos = ll.emplace_after(pos, 9);
    (*pos).line = u8"R\"(testrawstri";
    tokens.emplace_back(pp_tokenize_result{.status = pp_tokenize_status::DuringRawStr}, (*pos).line, pos);

    it = std::begin(tokens);
    end = std::end(tokens);

    pptoken = ll_paser::read_rawstring_tokens(it, end);
    CHECK_UNARY(it == end);
    CHECK_UNARY(pptoken.token.to_view().empty());
    CHECK_UNARY_FALSE(pptoken.lextokens.empty());

    //途中でトークナイズエラーが出る
    tokens.clear();
    pos = ll.emplace_after(pos, 10);
    (*pos).line = u8"R\"(rwastring read error";
    tokens.emplace_back(pp_tokenize_result{.status = pp_tokenize_status::DuringRawStr}, (*pos).line, pos);
  
    pos = ll.emplace_after(pos, 11);
    (*pos).line = u8"";
    tokens.emplace_back(pp_tokenize_result{.status = pp_tokenize_status::FailedRawStrLiteralRead}, (*pos).line, pos);

    it = std::begin(tokens);
    end = std::end(tokens);

    pptoken = ll_paser::read_rawstring_tokens(it, end);
    CHECK_UNARY_FALSE(it == end);
    CHECK_UNARY(pptoken.token.to_view().empty());
    CHECK_UNARY_FALSE(pptoken.lextokens.empty());
  }

}

#include "pp_tokenizer_test.hpp"

namespace pp_parsing_test
{

  TEST_CASE("text-line test") {
    using kusabira::PP::pp_token_category;
    using namespace std::literals;
    using pp_tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;
    using ll_paser = kusabira::PP::ll_paser<pp_tokenizer>;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    pp_tokenizer tokenizer{testdir / "parse_text-line.cpp"};
    ll_paser parser{};

    auto status = parser.start(tokenizer);

    CHECK_UNARY(bool(status));
  }

} // namespace pp_parsing_test
