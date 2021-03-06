#pragma once

#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"
#include "PP/pp_parser.hpp"
#include "../report_output_test.hpp"

namespace kusabira_test::pp_paser_test {

  TEST_CASE("make error test") {

    using namespace std::literals;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_parse_context;
    using kusabira::PP::pp_token_category;

    using lint_it = pp_token::line_iterator;

    std::vector<pp_token> test_tokens{};
    test_tokens.emplace_back(pp_token_category::identifier, u8"test"sv, 0, lint_it{});

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
    kusabira::PP::pp_token_category category;
    std::u8string_view token{};
  };

  TEST_CASE("skip whitespace token test") {

    using kusabira::PP::pp_token_category;

    //トークン列を準備
    std::vector<test_token> test_tokens{};
    test_tokens.reserve(23);

    for (auto i = 0u; i < 5u; ++ i) {
      test_tokens.emplace_back(test_token{.category = pp_token_category::whitespaces});
    }
    test_tokens.emplace_back(test_token{.category = pp_token_category::identifier});

    for (auto i = 0u; i < 7u; ++ i) {
      test_tokens.emplace_back(test_token{.category = pp_token_category::block_comment});
    }
    test_tokens.emplace_back(test_token{.category = pp_token_category::pp_number});

    for (auto i = 0u; i < 11u; ++ i) {
      test_tokens.emplace_back(test_token{.category = pp_token_category::whitespaces});
      test_tokens.emplace_back(test_token{.category = pp_token_category::block_comment});
    }
    test_tokens.emplace_back(test_token{.category = pp_token_category::string_literal});
    test_tokens.emplace_back(test_token{.category = pp_token_category::newline});

    //テスト開始
    auto it = std::begin(test_tokens);
    auto end = std::end(test_tokens);

    using kusabira::PP::skip_whitespaces;

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).category, pp_token_category::identifier);

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).category, pp_token_category::pp_number);

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).category, pp_token_category::string_literal);

    CHECK_UNARY(skip_whitespaces(it, end));
    CHECK_EQ((*it).category, pp_token_category::newline);

    CHECK_UNARY_FALSE(skip_whitespaces(it, end));
  }

  TEST_CASE("string literal calssify test") {
    
    using ll_paser = kusabira::PP::ll_paser<kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>>;
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;
    using my_strview = kusabira::vocabulary::whimsy_str_view<>;

    //トークン列を準備
    std::vector<test_token> test_tokens{};
    test_tokens.reserve(12);
    test_tokens.emplace_back(test_token{.category = pp_token_category::identifier, .token = u8"sv"sv});

    //一つ前に出現したトークン
    kusabira::PP::pp_token pptoken{pp_token_category::string_literal};
    pptoken.token = my_strview{u8R"**('c')**"sv};

    auto it = std::begin(test_tokens);

    //文字リテラル -> ユーザー定義文字リテラル
    CHECK_UNARY(ll_paser::strliteral_classify(it, pptoken.token, pptoken));
    CHECK_EQ(pptoken.category, pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY(pptoken.token == u8"'c'sv"sv);

    pptoken.category = pp_token_category::string_literal;
    pptoken.token = my_strview{u8R"**("string")**"sv};

    //文字列リテラル -> ユーザー定義文字列リテラル
    CHECK_UNARY(ll_paser::strliteral_classify(it, pptoken.token, pptoken));
    CHECK_EQ(pptoken.category, pp_token_category::user_defined_string_literal);
    CHECK_UNARY(pptoken.token == u8R"**("string"sv)**"sv);

    pptoken.category = pp_token_category::raw_string_literal;
    pptoken.token = my_strview{u8R"**(R"raw string")**"sv};

    //生文字列リテラル -> ユーザー定義生文字列リテラル
    CHECK_UNARY(ll_paser::strliteral_classify(it, pptoken.token, pptoken));
    CHECK_EQ(pptoken.category, pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY(pptoken.token == u8R"**(R"raw string"sv)**"sv);

    //何もしないはずのトークン種別の入力
    test_tokens.clear();
    test_tokens.emplace_back(test_token{ .category = pp_token_category::whitespaces });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::line_comment });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::block_comment });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::pp_number });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::string_literal });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::raw_string_literal });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::during_raw_string_literal });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::op_or_punc });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::other_character });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::newline });
    test_tokens.emplace_back(test_token{ .category = pp_token_category::empty });

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
    using kusabira::PP::pp_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //トークン列
    std::vector<pp_token> tokens{};
    tokens.reserve(10);

    auto pos = ll.before_begin();

    //論理行オブジェクト1
    pos = ll.emplace_after(pos, 0, 0);
    (*pos).line = u8"R\"(testrawstringriteral";
    (*pos).line_offset.emplace_back(7);
    (*pos).line_offset.emplace_back(7 + 3);
    (*pos).line_offset.emplace_back(7 + 3 + 6);
    {
      auto &r = tokens.emplace_back(pp_token_category::during_raw_string_literal, (*pos).line, 0, pos);
      CHECK_UNARY(r.is_multiple_phlines());
    }
    tokens.emplace_back(pp_token_category::newline, u8""sv, (*pos).line.length(), pos);

    //論理行オブジェクト2
    pos = ll.emplace_after(pos, 4, 4);
    (*pos).line = u8"testline1";
    (*pos).line_offset.emplace_back(4);
    {
      auto &r = tokens.emplace_back(pp_token_category::during_raw_string_literal, (*pos).line, 0, pos);
      CHECK_UNARY(r.is_multiple_phlines());
    }
    tokens.emplace_back(pp_token_category::newline, u8""sv, (*pos).line.length(), pos);

    //論理行オブジェクト３
    pos = ll.emplace_after(pos, 6, 6);
    (*pos).line = u8"testlogicalline2)\"";
    (*pos).line_offset.emplace_back(4);
    (*pos).line_offset.emplace_back(4 + 7);
    {
      auto &r = tokens.emplace_back(pp_token_category::raw_string_literal, (*pos).line, 0, pos);
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
    CHECK_EQ(std::distance(pptoken.composed_tokens.begin(), pptoken.composed_tokens.end()), 3u);

    //途中で切れてるやつ
    tokens.clear();
    pos = ll.emplace_after(pos, 9, 9);
    (*pos).line = u8"R\"(testrawstri";
    tokens.emplace_back(pp_token_category::during_raw_string_literal, (*pos).line, 0, pos);
    tokens.emplace_back(pp_token_category::newline, u8""sv, (*pos).line.length(), pos);

    it = std::begin(tokens);
    end = std::end(tokens);

    pptoken = ll_paser::read_rawstring_tokens(it, end);
    CHECK_UNARY(it == end);
    CHECK_UNARY(pptoken.token.to_view().empty());
    CHECK_UNARY_FALSE(pptoken.composed_tokens.empty());

    //途中でトークナイズエラーが出る
    tokens.clear();
    pos = ll.emplace_after(pos, 10, 10);
    (*pos).line = u8"R\"(rwastring read error";
    tokens.emplace_back(pp_token_category::during_raw_string_literal, (*pos).line, 0, pos);
  
    pos = ll.emplace_after(pos, 11, 11);
    (*pos).line = u8"";
    tokens.emplace_back(pp_token_category::FailedRawStrLiteralRead, (*pos).line, 0, pos);

    it = std::begin(tokens);
    end = std::end(tokens);

    pptoken = ll_paser::read_rawstring_tokens(it, end);
    CHECK_UNARY_FALSE(it == end);
    CHECK_UNARY(pptoken.token.to_view().empty());
    CHECK_UNARY_FALSE(pptoken.composed_tokens.empty());

    //単に改行されてる生文字列リテラル
    tokens.clear();
    pos = ll.emplace_after(pos, 11, 11);
    (*pos).line = u8"R\"(rawstring test";
    tokens.emplace_back(pp_token_category::during_raw_string_literal, (*pos).line, 0, pos);
    tokens.emplace_back(pp_token_category::newline, u8""sv, (*pos).line.length(), pos);
    pos = ll.emplace_after(pos, 12, 12);
    (*pos).line = u8R"**(newline)")**";
    tokens.emplace_back(pp_token_category::raw_string_literal, (*pos).line, 0, pos);

    it = std::begin(tokens);
    end = std::end(tokens);

    pptoken = ll_paser::read_rawstring_tokens(it, end);

    //生文字列リテラル中の改行はそのままに
    expect = u8R"**(R"(rawstring test
newline)")**"sv;

    CHECK_UNARY(pptoken.token == expect);
    CHECK_EQ(pptoken.category, pp_token_category::raw_string_literal);
    CHECK_EQ(std::distance(pptoken.composed_tokens.begin(), pptoken.composed_tokens.end()), 2u);
  }

  TEST_CASE("longest match exception test") {

    using ll_paser = kusabira::PP::ll_paser<kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>>;
    using kusabira::PP::pp_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //トークン列
    std::vector<pp_token> tokens{};
    tokens.reserve(10);

    auto pos = ll.before_begin();

    //論理行オブジェクト1
    pos = ll.emplace_after(pos, 0, 0);
    (*pos).line = u8"<::Foo>;";

    tokens.emplace_back(pp_token_category::op_or_punc, u8"<:"sv, 0, pos);
    tokens.emplace_back(pp_token_category::op_or_punc, u8":"sv, 2, pos);
    tokens.emplace_back(pp_token_category::identifier, u8"Foo"sv, 3, pos);

    {
      auto it = std::begin(tokens);
      auto end = std::end(tokens);

      auto list = ll_paser::longest_match_exception_handling(it, end);

      CHECK_EQ(list.size(), 2u);
      CHECK_UNARY((*it).token == u8"Foo"sv);

      auto lit = std::begin(list);
      CHECK_EQ((*lit).token, u8"<"sv);
      CHECK_EQ((*lit).category, pp_token_category::op_or_punc);
      ++lit;
      CHECK_EQ((*lit).token, u8"::"sv);
      CHECK_EQ((*lit).category, pp_token_category::op_or_punc);
    }


    //論理行オブジェクト2
    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8"<:::Foo::value:>;";

    tokens.clear();
    tokens.emplace_back(pp_token_category::op_or_punc, u8"<:"sv, 0, pos);
    tokens.emplace_back(pp_token_category::op_or_punc, u8"::"sv, 2, pos);
    tokens.emplace_back(pp_token_category::identifier, u8"Foo"sv, 4, pos);

    {
      auto it = std::begin(tokens);
      auto end = std::end(tokens);

      auto list = ll_paser::longest_match_exception_handling(it, end);

      CHECK_EQ(list.size(), 2u);
      CHECK_UNARY((*it).token == u8"Foo"sv);

      auto lit = std::begin(list);
      CHECK_UNARY((*lit).token == u8"<:"sv);
      CHECK_UNARY((*lit).category == pp_token_category::op_or_punc);
      ++lit;
      CHECK_UNARY((*lit).token == u8"::"sv);
      CHECK_UNARY((*lit).category == pp_token_category::op_or_punc);
    }

    //論理行オブジェクト3
    pos = ll.emplace_after(pos, 2, 2);
    (*pos).line = u8"<:::> "; //こんなトークンエラーでは・・・

    tokens.clear();
    tokens.emplace_back(pp_token_category::op_or_punc, u8"<:"sv, 0, pos);
    tokens.emplace_back(pp_token_category::op_or_punc, u8"::"sv, 2, pos);
    tokens.emplace_back(pp_token_category::op_or_punc, u8">"sv,4, pos);
    tokens.emplace_back(pp_token_category::whitespaces, u8" "sv, 5, pos);

    {
      auto it = std::begin(tokens);
      auto end = std::end(tokens);

      auto list = ll_paser::longest_match_exception_handling(it, end);

      CHECK_EQ(list.size(), 2u);
      CHECK_UNARY((*it).token == u8">"sv);

      auto lit = std::begin(list);
      CHECK_UNARY((*lit).token == u8"<:"sv);
      CHECK_UNARY((*lit).category == pp_token_category::op_or_punc);
      ++lit;
      CHECK_UNARY((*lit).token == u8"::"sv);
      CHECK_UNARY((*lit).category == pp_token_category::op_or_punc);
    }

    //論理行オブジェクト4
    pos = ll.emplace_after(pos, 3, 3);
    (*pos).line = u8"<::> = {}";

    tokens.clear();
    tokens.emplace_back(pp_token_category::op_or_punc, u8"<:"sv, 0, pos);
    tokens.emplace_back(pp_token_category::op_or_punc, u8":>"sv, 2, pos);
    tokens.emplace_back(pp_token_category::whitespaces, u8" "sv, 4, pos);

    {
      auto it = std::begin(tokens);
      auto end = std::end(tokens);

      auto list = ll_paser::longest_match_exception_handling(it, end);

      CHECK_EQ(list.size(), 2u);
      CHECK_UNARY((*it).token == u8" "sv);

      auto lit = std::begin(list);
      CHECK_UNARY((*lit).token == u8"<:"sv);
      CHECK_UNARY((*lit).category == pp_token_category::op_or_punc);
      ++lit;
      CHECK_UNARY((*lit).token == u8":>"sv);
      CHECK_UNARY((*lit).category == pp_token_category::op_or_punc);
    }
  }
}

#include "pp_tokenizer_test.hpp"

namespace pp_parsing_test
{

  TEST_CASE("text-line test") {
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_parse_status;
    using kusabira::PP::pp_token;
    using namespace std::literals;
    using pp_tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    auto testfile_path = testdir / "parse_text-line.cpp";
    kusabira::PP::ll_paser parser{pp_tokenizer{testfile_path}, testfile_path};

    auto status = parser.start();

    REQUIRE_UNARY(bool(status));
    CHECK_EQ(status.value(), pp_parse_status::Complete);
    constexpr auto token_num = 160u + 42u;
    REQUIRE_EQ(parser.get_phase4_result().size(), token_num);

    constexpr std::u8string_view expect_token[] = {
      u8"",
      u8"int", u8"main", u8"(", u8")", u8"{", u8"",
      u8"int", u8"n", u8"=", u8"10", u8";", u8"",
      u8"double", u8"d", u8"=", u8"1.0", u8";", u8"",
      u8"",
      u8"int", u8"add", u8"=", u8"n", u8"+", u8"10", u8";", u8"",
      u8"",
      u8"",
      u8"",
      u8"",
      u8"char", u8"c", u8"=", u8"'a'", u8";", u8"",
      u8"",
      u8"char8_t", u8"u8str", u8"[", u8"]", u8"=", u8R"(u8"test string")", u8";", u8"",
      u8"",
      u8"char", u8"rawstr", u8"[", u8"]", u8"=", u8R"+++(R"+*(test raw string)+*")+++", u8";", u8"",
      u8"",
      u8"auto", u8"udl1", u8"=", u8R"+++("test udl"sv)+++", u8";", u8"",
      u8"auto", u8"udl2", u8"=", u8R"+++(U"test udl"s)+++", u8";", u8"",
      u8"auto", u8"udl3", u8"=", u8R"+++(L"test udl"sv)+++", u8";", u8"",
      u8"auto", u8"udl4", u8"=", u8R"+++(u8"test udl"sv)+++", u8";", u8"",
      u8"auto", u8"udl5", u8"=", u8R"+++(u"test udl"sv)+++", u8";", u8"",
      u8"auto", u8"udl6", u8"=", u8"u8R\"(test udl\nnewline)\"_udl", u8";", u8"",
      u8"",
      u8"constexpr", u8"int", u8"num", u8"=", u8"123'456'789", u8";", u8"",
      u8"auto", u8"un", u8"=", u8"111u", u8";", u8"",
      u8"",
      u8"float", u8"f1", u8"=", u8"1.0E-8f", u8";", u8"",
      u8"volatile", u8"float", u8"f2", u8"=", u8".1F", u8";", u8"",
      u8"double", u8"d1", u8"=", u8"0x1.2p3", u8";", u8"",
      u8"double", u8"d2", u8"=", u8"1.", u8";", u8"",
      u8"long", u8"double", u8"d3", u8"=", u8"0.1e-1L", u8";", u8"",
      u8"long", u8"double", u8"d4", u8"=", u8".1e-1l", u8";", u8"",
      u8"const", u8"double", u8"d5", u8"=", u8"3.1415", u8"_udl", u8";", u8"",
      u8"",
      u8"int", u8"y", u8"<:", u8":>", u8"{", u8"}", u8";", u8"",
      u8"std", u8"::", u8"vector", u8"<", u8"::", u8"Foo", u8">", u8"x", u8";", u8"",
      u8"int", u8"z", u8"<:", u8"::", u8"Foo", u8"::", u8"value", u8":>", u8";", u8"",
      u8"",
      u8"auto", u8"a", u8"=", u8"true", u8"?", u8"1", u8":", u8"0", u8";", u8"",
      u8"",
      u8"return", u8"n", u8";", u8"",
      u8"}", u8""};
    static_assert(std::size(expect_token) == token_num, "The number of pp-tokens between expect_token and token_num does not match.");

    constexpr pp_token_category expect_category[] = {
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::charcter_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::raw_string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::user_defined_string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::user_defined_string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::user_defined_string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::user_defined_string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::user_defined_string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::user_defined_raw_string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::op_or_punc, pp_token_category::newline};
    static_assert(std::size(expect_category) == token_num, "The number of pp-token-categorys between expect_category and token_num does not match.");

    auto it = std::begin(parser.get_phase4_result());

    for (auto i = 0u; i < token_num; ++i) {
      CHECK_EQ(*it, pp_token{ expect_category[i], expect_token[i] });
      ++it;
    }
  }

  TEST_CASE("macro test" * doctest::skip(false)) {
    using kusabira::PP::pp_parse_status;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;
    using pp_tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    auto testfile_path = testdir / "parse_macro.cpp";
    kusabira::PP::ll_paser parser{pp_tokenizer{testfile_path}, testfile_path};

    auto status = parser.start();

    REQUIRE_UNARY(bool(status));
    CHECK_EQ(status.value(), pp_parse_status::Complete);

    //for (auto& token : parser.pptoken_list) {
    //  std::cout << reinterpret_cast<const char *>(data(token.token)) << std::endl;
    //}

    // 残ったトークン+改行
    constexpr auto token_num = 300 + 110;
    //constexpr auto token_num = 222 + 107;
    REQUIRE_EQ(parser.get_phase4_result().size(), token_num);

    constexpr std::u8string_view expect_token[] = {
        u8"",
        u8"",
        u8"int", u8"vm", u8"=", u8"1", u8";", u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"int", u8"x", u8"=", u8"42", u8";", u8"",
        u8"int", u8"x", u8"=", u8")", u8";", u8"",
        u8"int", u8"x", u8"=", u8"42", u8";", u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"fprintf", u8"(", u8"stderr", u8",", u8R"("Flag")", u8")", u8";", u8"",
        u8"fprintf", u8"(", u8"stderr", u8",", u8R"("X = %d\n")", u8",", u8"x", u8")", u8";", u8"",
        u8"puts", u8"(", u8R"("The first, second, and third items.")", u8")", u8";", u8"",
        u8"(", u8"(", u8"x", u8">", u8"y", u8")",  u8"?", u8"puts", u8"(", u8R"("x>y")", u8")", u8":", u8"printf", u8"(", u8R"("x is %d but y is %d")", u8",", u8"x", u8",", u8"y", u8")", u8")", u8";", u8"",
        u8"",
        u8"", // #undef F
        u8"", // #undef G
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"f", u8"(", u8"0", u8",", u8"a", u8",", u8"b", u8",", u8"c", u8")", u8"",
        u8"f", u8"(", u8"0", u8")", u8"",
        u8"f", u8"(", u8"0", u8")", u8"",
        u8"f", u8"(", u8"0", u8")", u8"",
        u8"",
        u8"f", u8"(", u8"0", u8",", u8"a", u8",", u8"b", u8",", u8"c", u8")", u8"",
        u8"f", u8"(", u8"0", u8",", u8"a", u8")", u8"",
        u8"f", u8"(", u8"0", u8",", u8"a", u8")", u8"",
        u8"",
        u8"S", u8"foo", u8";", u8"",
        u8"S", u8"bar", u8"=", u8"{", u8"1", u8",", u8"2", u8"}", u8";", u8"",
        u8"",
        u8"",  // #define H2(X, Y, ...) __VA_OPT__(X ## Y,) __VA_ARGS__
        u8"",
        u8"ab", u8",", u8"c", u8",", u8"d", u8"",
        u8"",
        u8"",
        u8"",
        u8R"("")", u8"",  // H3(, 0)
        u8"",
        u8"",
        u8"",
        u8"a", u8"b", u8"",
        u8"",
        u8"",  // #define H5A(...) __VA_OPT__()/**/__VA_OPT__()
        u8"",  // #define H5B(X) a ## X ## b
        u8"",  // #define H5C(X) H5B(X)
        u8"",
        u8"ab", u8"",
        u8"",
        u8"",
        u8"",
        u8"", // #define str(s)      # s
        u8"",
        u8"",
        u8"",
        u8"", // #define glue(a, b)  a ## b
        u8"",
        u8"",
        u8"",
        u8"",
        u8"printf", u8"(", u8R"("x")", u8R"("1")", u8R"("= %d, x")", u8R"("2")", u8R"("= %s")", u8",", u8"x1", u8",", u8"x2", u8")", u8";", u8"",
        u8"fputs", u8"(", u8R"++("strncmp(\"abc\\0d\", \"abc\", '\\4') == 0")++", u8R"(": @\n")", u8",", u8"s", u8")", u8";", u8"",
        u8R"++("vers2.h")++", u8"",
        u8R"++("hello")++", u8";", u8"",
        u8R"++("hello")++", u8R"++(", world")++", u8"",
        u8"", // #define hash_hash # ## #
        u8"",
        u8"",
        u8"",
        u8"",
        u8"", // #define join(c, d) in_between(c hash_hash d)
        u8R"++("x ## y")++", u8";", u8"",
        u8"",
        u8"", // #define t(x,y,z) x ## y ## z
        u8"",
        u8"int", u8"j", u8"[", u8"]", u8"=", u8"{", u8"123", u8",", u8"45", u8",", u8"67", u8",", u8"89", u8",", u8"",
        u8"10", u8",", u8"11", u8",", u8"12", u8",", u8"}", u8";", u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"",
        u8"f", u8"(", u8"2", u8"*", u8"(", u8"y", u8"+", u8"1", u8")", u8")", u8"+", u8"f", u8"(", u8"2", u8"*", u8"(", u8"f", u8"(", u8"2", u8"*", u8"(", u8"z", u8"[", u8"0", u8"]", u8")", u8")", u8")", u8")", u8"%", u8"f", u8"(", u8"2", u8"*", u8"(", u8"0", u8")", u8")", u8"+", u8"t", u8"(", u8"1" , u8")", u8";", u8"",
        u8"f", u8"(", u8"2", u8"*", u8"(", u8"2", u8"+", u8"(", u8"3", u8",", u8"4", u8")", u8"-", u8"0", u8",", u8"1", u8")", u8")", u8"|", u8"f", u8"(", u8"2", u8"*", u8"(", u8"~", u8"5", u8")", u8")", u8"&", u8"f", u8"(", u8"2", u8"*", u8"(", u8"0", u8",", u8"1", u8")", u8")", u8"^", u8"m", u8"(", u8"0", u8",", u8"1", u8")", u8";", u8"",
        u8"int", u8"i", u8"[", u8"]", u8"=", u8"{", u8"1", u8",", u8"23", u8",", u8"4", u8",", u8"5", u8",", u8"}", u8";", u8"",
        u8"char", u8"c", u8"[", u8"2", u8"]", u8"[", u8"6", u8"]", u8"=", u8"{", u8R"*("hello")*", u8",", u8R"*("")*", u8"}", u8";", u8""
    };
    static_assert(std::size(expect_token) == token_num, "The number of pp-tokens between expect_token and token_num does not match.");

    constexpr pp_token_category expect_category[] = {
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::string_literal, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::string_literal, pp_token_category::string_literal, pp_token_category::string_literal, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::string_literal, pp_token_category::newline,
        pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::string_literal, pp_token_category::string_literal, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline,
        pp_token_category::identifier, pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::string_literal, pp_token_category::op_or_punc, pp_token_category::op_or_punc, pp_token_category::newline
    };
    static_assert(std::size(expect_category) == token_num, "The number of pp-token-categorys between expect_category and token_num does not match.");

    auto it = std::begin(parser.get_phase4_result());

    for (auto i = 0u; i < token_num; ++i) {
      auto &pptoken = *it;
      CAPTURE(i);
      CHECK_EQ(pptoken, pp_token{expect_category[i], expect_token[i]});
      ++it;
    }
  }

  // 文字列から読み込むソースファイルリーダ
  class string_reader {

    std::vector<std::u8string_view> src_lines{};
    std::size_t index = 0;

  public:

    string_reader(const std::filesystem::path &)
    {}

    template<std::convertible_to<std::u8string_view>... Ts>
    void setlines(Ts&&... args) {
      auto inserter = [this](std::u8string_view str) {
        src_lines.emplace_back(str);
      };

      (void)std::initializer_list<int>{(inserter(args), 0)...};
    }

    auto readline() -> std::optional<kusabira::PP::logical_line> {
      if (size(src_lines) <= index) return std::nullopt;
    
      auto linenum = index + 1;
      kusabira::PP::logical_line line{ linenum, linenum };
      line.line = src_lines[index];
      ++index;

      return line;
    }
  };

  static_assert(kusabira::PP::concepts::src_reader<string_reader>);

  // テスト用トークナイザ、ファイル読み込み部分だけ入れ替え
  using test_tokenizer = kusabira::PP::tokenizer<string_reader, kusabira::PP::pp_tokenizer_sm>;
  // トークナイザだけ入れ替えたCPPパーサ
  using test_paser = kusabira::PP::ll_paser<test_tokenizer, kusabira::report::reporter_factory<kusabira_test::report::test_out>>;


  TEST_CASE("error test") {
    using kusabira::PP::pp_parse_context;
    using kusabira::PP::pp_parse_status;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;

    // 溜まっているログを消す
    [[maybe_unused]] auto trash = kusabira_test::report::test_out::extract_string();

    // 単純な例
    {
      // 仮想ソースファイル生成
      string_reader str_reader{"test/error_directive.cpp"};
      str_reader.setlines(u8"# error simple error"sv);

      test_paser parser{test_tokenizer{std::move(str_reader)}, "test/error_directive.cpp"};

      auto status = parser.start();

      // エラーで終わる
      REQUIRE_UNARY(bool(status) == false);
      auto &err_info = status.error();
      CHECK_EQ(err_info.context, pp_parse_context::ControlLine_Error);
      CHECK_EQ(err_info.token, pp_token{pp_token_category::identifier, u8"error"sv});

      auto expect_text = u8"error_directive.cpp:1:2: error: simple error\n"sv;
      auto str = kusabira_test::report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }

    // 本文がない
    {
      // 仮想ソースファイル生成
      string_reader str_reader{"test/error_directive.cpp"};
      str_reader.setlines(u8"#  /* comment block */  error"sv);

      test_paser parser{test_tokenizer{std::move(str_reader)}, "test/error_directive.cpp"};

      auto status = parser.start();

      // エラーで終わる
      REQUIRE_UNARY(bool(status) == false);
      auto &err_info = status.error();
      CHECK_EQ(err_info.context, pp_parse_context::ControlLine_Error);
      CHECK_EQ(err_info.token, pp_token{pp_token_category::identifier, u8"error"sv});

      auto expect_text = u8"error_directive.cpp:1:24: error: \n"sv;
      auto str = kusabira_test::report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }

    // マクロ展開例1
    {
      // 仮想ソースファイル生成
      string_reader str_reader{"test/error_directive.cpp"};
      str_reader.setlines(
          u8"#define A macro"sv,
          u8"#define B expand"sv,
          u8"#define C test"sv,
          u8"#error A B C "sv);

      test_paser parser{test_tokenizer{std::move(str_reader)}, "test/error_directive.cpp"};

      auto status = parser.start();

      // エラーで終わる
      REQUIRE_UNARY(bool(status) == false);
      auto &err_info = status.error();
      CHECK_EQ(err_info.context, pp_parse_context::ControlLine_Error);
      CHECK_EQ(err_info.token, pp_token{pp_token_category::identifier, u8"error"sv});

      auto expect_text = u8"error_directive.cpp:4:1: error: A B C \n"sv;
      auto str = kusabira_test::report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }

    // コメントがある例
    {
      // 仮想ソースファイル生成
      string_reader str_reader{"test/error_directive.cpp"};
      str_reader.setlines(
          u8"#define add(a, b, c) a + b + c;"sv,
          u8"#error /*block comment*/ add(1, 2, 3)"sv);

      test_paser parser{test_tokenizer{std::move(str_reader)}, "test/error_directive.cpp"};

      auto status = parser.start();

      // エラーで終わる
      REQUIRE_UNARY(bool(status) == false);
      auto &err_info = status.error();
      CHECK_EQ(err_info.context, pp_parse_context::ControlLine_Error);
      CHECK_EQ(err_info.token, pp_token{pp_token_category::identifier, u8"error"sv});

      auto expect_text = u8"error_directive.cpp:2:1: error: /*block comment*/ add(1, 2, 3)\n"sv;
      auto str = kusabira_test::report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }
  }


} // namespace pp_parsing_test
