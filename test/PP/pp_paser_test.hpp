#pragma once

#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"
#include "PP/pp_parser.hpp"

namespace pp_paser_test {

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
      test_tokens.emplace_back(test_token{.category = pp_token_category::whitespace});
    }
    test_tokens.emplace_back(test_token{.category = pp_token_category::identifier});

    for (auto i = 0u; i < 7u; ++ i) {
      test_tokens.emplace_back(test_token{.category = pp_token_category::block_comment});
    }
    test_tokens.emplace_back(test_token{.category = pp_token_category::pp_number});

    for (auto i = 0u; i < 11u; ++ i) {
      test_tokens.emplace_back(test_token{.category = pp_token_category::whitespace});
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

  // TEST_CASE("status to category test") {
    
  //   using ll_paser = kusabira::PP::ll_paser<kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>>;
  //   using kusabira::PP::pp_token_category;
  //   using kusabira::PP::pp_token_category;

  //   CHECK_EQ(ll_paser::tokenize_status_to_category(pp_token_category::identifier), pp_token_category::identifier);
  //   CHECK_EQ(ll_paser::tokenize_status_to_category(pp_token_category::pp_number), pp_token_category::pp_number);
  //   CHECK_EQ(ll_paser::tokenize_status_to_category(pp_token_category::op_or_punc), pp_token_category::op_or_punc);
  //   CHECK_EQ(ll_paser::tokenize_status_to_category(pp_token_category::other_character), pp_token_category::other_character);
  // }

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
    test_tokens.emplace_back(test_token{ .category = pp_token_category::whitespace });
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
    tokens.emplace_back(pp_token_category::whitespace, u8" "sv, 5, pos);

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
    tokens.emplace_back(pp_token_category::whitespace, u8" "sv, 4, pos);

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
    using ll_paser = kusabira::PP::ll_paser<pp_tokenizer>;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    //pp_tokenizer tokenizer{testdir / "parse_text-line.cpp"};
    ll_paser parser{testdir / "parse_text-line.cpp"};

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

  TEST_CASE("macro test") {
    using kusabira::PP::pp_parse_status;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;
    using pp_tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;
    using ll_paser = kusabira::PP::ll_paser<pp_tokenizer>;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    //pp_tokenizer tokenizer{testdir / "parse_macro.cpp"};
    ll_paser parser{testdir / "parse_macro.cpp"};

    auto status = parser.start();

    REQUIRE_UNARY(bool(status));
    CHECK_EQ(status.value(), pp_parse_status::Complete);

    //for (auto& token : parser.pptoken_list) {
    //  std::cout << reinterpret_cast<const char *>(data(token.token)) << std::endl;
    //}

    // 残ったトークン+改行
    constexpr auto token_num = 85 + 34;
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
        u8"f", u8"(", u8"0", u8")", u8""
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
        pp_token_category::identifier, pp_token_category::op_or_punc, pp_token_category::pp_number, pp_token_category::op_or_punc, pp_token_category::newline
    };
    static_assert(std::size(expect_category) == token_num, "The number of pp-token-categorys between expect_category and token_num does not match.");

    auto it = std::begin(parser.get_phase4_result());

    for (auto i = 0u; i < token_num; ++i) {
      auto &pptoken = *it;
      CHECK_EQ(pptoken, pp_token{expect_category[i], expect_token[i]});
      ++it;
    }
  }

} // namespace pp_parsing_test
