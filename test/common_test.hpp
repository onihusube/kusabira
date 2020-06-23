#pragma once

#include "doctest/doctest.h"

#include "common.hpp"

namespace kusabira_test::common
{

  TEST_CASE("lex token test") {
    using kusabira::PP::pp_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;


    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};

    auto pos = ll.before_begin();

    //論理行オブジェクト1
    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8"int main() {";

    {
      //テスト用トークン1
      pp_token test_token{ pp_token_category::identifier, u8"int", 0u, pos };

      CHECK_UNARY_FALSE(test_token.is_multiple_phlines());
      CHECK_UNARY(test_token.get_line_string() == u8"int main() {");
      auto [row, col] = test_token.get_phline_pos();
      CHECK_EQ(row, 1u);
      CHECK_EQ(col, 0u);
    }

    //論理行オブジェクト2
    pos = ll.emplace_after(pos, 2, 2);
    (*pos).line = u8"  return 0; }";
    //"  "の後、";"の後、で行継続が行われているとする
    //"  \\nreturn 0;\\n }"みたいな感じ
    (*pos).line_offset.push_back(2);
    (*pos).line_offset.push_back(9);

    {
      //テスト用トークン2
      pp_token test_token{ pp_token_category::identifier, u8"return", 2u, pos };

      //行継続が行われている
      CHECK_UNARY(test_token.is_multiple_phlines());
      CHECK_UNARY(test_token.get_line_string() == u8"  return 0; }");
      auto [row, col] = test_token.get_phline_pos();
      CHECK_EQ(row, 3u);
      CHECK_EQ(col, 0u);
    }

    {
      //テスト用トークン3
      pp_token test_token{ pp_token_category::op_or_punc, u8"}", 12u, pos };

      //行継続が行われている
      CHECK_UNARY(test_token.is_multiple_phlines());
      CHECK_UNARY(test_token.get_line_string() == u8"  return 0; }");
      auto [row, col] = test_token.get_phline_pos();
      CHECK_EQ(row, 4u);
      CHECK_EQ(col, 1u);
    }
  }

  TEST_CASE("pp_token_category += test") {
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pptokencat_op;

    auto lhs = pp_token_category::identifier;

    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::identifier, lhs);
    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::pp_number);
    CHECK_EQ(pp_token_category::identifier, lhs);

    lhs = pp_token_category::pp_number;

    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::pp_number, lhs);
    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::pp_number);
    CHECK_EQ(pp_token_category::pp_number, lhs);

    lhs = pp_token_category::op_or_punc;

    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_EQ(pp_token_category::op_or_punc, lhs);

    lhs = pp_token_category::charcter_literal;

    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::user_defined_charcter_literal, lhs);
    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::user_defined_charcter_literal, lhs);

    lhs = pp_token_category::string_literal;

    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::user_defined_string_literal, lhs);
    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::user_defined_string_literal, lhs);

    lhs = pp_token_category::raw_string_literal;

    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::user_defined_raw_string_literal, lhs);
    CHECK_UNARY(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_EQ(pp_token_category::user_defined_raw_string_literal, lhs);

    lhs = pp_token_category::identifier;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::pp_number;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::op_or_punc;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::identifier);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::charcter_literal;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::user_defined_charcter_literal;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::string_literal;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::user_defined_string_literal;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::raw_string_literal;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);

    lhs = pp_token_category::user_defined_raw_string_literal;

    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_charcter_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::user_defined_raw_string_literal);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::op_or_punc);
    CHECK_UNARY_FALSE(pptokencat_op{ lhs } += pp_token_category::other_character);
  }

  TEST_CASE("pp_token += test") {
    using kusabira::PP::pp_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_token_category;
    using namespace std::string_view_literals;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};

    auto pos = ll.before_begin();

    //論理行オブジェクト1
    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8R"(R "string" sv)";

    {
      pp_token token1{pp_token_category::identifier, u8"R", 0u, pos};
      pp_token token2{pp_token_category::string_literal, u8R"("string")", 2u, pos};
      pp_token token3{pp_token_category::identifier, u8"sv", 10u, pos};

      //普通の連結
      bool is_success = token2 += std::move(token3);
      is_success &= (token1 += std::move(token2));

      REQUIRE_UNARY(is_success);
      CHECK_UNARY(token1.token == u8R"(R"string"sv)"sv);
      CHECK_EQ(pp_token_category::user_defined_raw_string_literal, token1.category);
      CHECK_EQ(2, std::distance(token1.lextokens.begin(), token1.lextokens.end()));

      //プレイスメーカートークンの右からの連結
      is_success = token1 += pp_token{pp_token_category::placemarker_token};

      REQUIRE_UNARY(is_success);
      CHECK_UNARY(token1.token == u8R"(R"string"sv)"sv);
      CHECK_EQ(pp_token_category::user_defined_raw_string_literal, token1.category);
      CHECK_EQ(2, std::distance(token1.lextokens.begin(), token1.lextokens.end()));

      //プレイスメーカートークンの左からの連結
      pp_token placemaker{pp_token_category::placemarker_token};
      placemaker += std::move(token1);

      REQUIRE_UNARY(is_success);
      CHECK_UNARY(placemaker.token == u8R"(R"string"sv)"sv);
      CHECK_EQ(pp_token_category::user_defined_raw_string_literal, token1.category);
      CHECK_EQ(2, std::distance(placemaker.lextokens.begin(), placemaker.lextokens.end()));
    }

    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8R"(< < =)";
    //記号の連結1
    {
      pp_token token1{pp_token_category::op_or_punc, u8"<", 0u, pos};
      pp_token token2{pp_token_category::op_or_punc, u8"<", 2u, pos};
      pp_token token3{pp_token_category::op_or_punc, u8"=", 4u, pos};

      bool is_success = token2 += std::move(token3);
      is_success &= (token1 += std::move(token2));

      REQUIRE_UNARY(is_success);
      CHECK_UNARY(token1.token == u8"<<="sv);
      CHECK_EQ(pp_token_category::op_or_punc, token1.category);
      CHECK_EQ(2, std::distance(token1.lextokens.begin(), token1.lextokens.end()));
    }

    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8R"(| =)";
    //記号の連結2
    {
      pp_token token1{pp_token_category::op_or_punc, u8"|", 0u, pos};
      pp_token token2{pp_token_category::op_or_punc, u8"=", 2u, pos};

      bool is_success = token1 += std::move(token2);

      REQUIRE_UNARY(is_success);
      CHECK_UNARY(token1.token == u8"|="sv);
      CHECK_EQ(pp_token_category::op_or_punc, token1.category);
      CHECK_EQ(1, std::distance(token1.lextokens.begin(), token1.lextokens.end()));
    }

    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8R"(-> *)";
    //記号の連結3
    {
      pp_token token1{pp_token_category::op_or_punc, u8"->", 0u, pos};
      pp_token token2{pp_token_category::op_or_punc, u8"*", 3u, pos};

      bool is_success = token1 += std::move(token2);

      REQUIRE_UNARY(is_success);
      CHECK_UNARY(token1.token == u8"->*"sv);
      CHECK_EQ(pp_token_category::op_or_punc, token1.category);
      CHECK_EQ(1, std::distance(token1.lextokens.begin(), token1.lextokens.end()));
    }

    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8R"(< > = ! << * ++ =)";
    //記号の連結、失敗例
    {
      pp_token token1{pp_token_category::op_or_punc, u8"<", 0u, pos};
      pp_token token2{pp_token_category::op_or_punc, u8">", 2u, pos};

      bool is_success = token1 += std::move(token2);

      CHECK_UNARY_FALSE(is_success);

      pp_token token3{pp_token_category::op_or_punc, u8"=", 4u, pos};
      pp_token token4{pp_token_category::op_or_punc, u8"!", 6u, pos};

      is_success = token3 += std::move(token4);

      CHECK_UNARY_FALSE(is_success);

      pp_token token5{pp_token_category::op_or_punc, u8"<<", 8u, pos};
      pp_token token6{pp_token_category::op_or_punc, u8"*", 11u, pos};

      is_success = token5 += std::move(token6);

      CHECK_UNARY_FALSE(is_success);

      pp_token token7{pp_token_category::op_or_punc, u8"++", 13u, pos};
      pp_token token8{pp_token_category::op_or_punc, u8"=", 16u, pos};

      is_success = token7 += std::move(token8);

      CHECK_UNARY_FALSE(is_success);
    }
  }

    
} // namespace kusabira_test::common
