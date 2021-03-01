#pragma once

#include "doctest/doctest.h"
#include "src/PP/pp_constexpr.hpp"

// 網羅するのキビシイので問題になったら適宜追加していく方針で・・・
namespace kusabira_test::constexpr_test::integral_constant_test {

  TEST_CASE("decode_integral_ppnumber test") {
    using kusabira::PP::free_func::decode_integral_ppnumber;
    using kusabira::overloaded;

    // 10進リテラル
    {
      auto result = decode_integral_ppnumber(u8"12345", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 12345); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12345", -1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == -12345); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      // 64bit符号なし整数最大値
      auto result = decode_integral_ppnumber(u8"18'446'744'073'709'551'615", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 18'446'744'073'709'551'615); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      // 64bit符号付整数最大値
      auto result = decode_integral_ppnumber(u8"9'223'372'036'854'775'807", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 9'223'372'036'854'775'807); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      // 64bit符号付整数最大値+1、結果は符号なし整数型で得られる
      auto result = decode_integral_ppnumber(u8"9'223'372'036'854'775'808", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 9'223'372'036'854'775'808); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      // 64bit符号付整数最小値
      constexpr auto min64 = std::numeric_limits<std::int64_t>::min();

      auto result = decode_integral_ppnumber(u8"9'223'372'036'854'775'808", -1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == min64); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 16進リテラル
    {
      auto result = decode_integral_ppnumber(u8"0xffFFabc", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 268434108); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0XffFFabc", -1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == -268434108); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 8進リテラル
    {
      auto result = decode_integral_ppnumber(u8"01234567", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 342391); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"01234567", -1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == -342391); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 2進リテラル
    {
      auto result = decode_integral_ppnumber(u8"0b1100110111011", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 6587); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B1100110111011", -1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == -6587); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

  }

  TEST_CASE("decode_integral_ppnumber suffix test") {
    using kusabira::PP::free_func::decode_integral_ppnumber;
    using kusabira::overloaded;

    // リテラルサフィックスに対して型が合うかのテスト
    {
      auto result = decode_integral_ppnumber(u8"1234567890u", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890l", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890z", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"123456789'0ll", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890ul", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890ull", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12'34567890uz", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890lu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"123456'7890llu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890zu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 16進
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0Xabcdefl", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdeFz", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcDefll", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefuL", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefull", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xaBcdefUz", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdeflu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefLLu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xAbcdefZu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 2進
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101U", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101L", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101Z", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101LL", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101Ul", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101ull", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101uz", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101lu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101LLu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101Zu", 1);
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
  }

  TEST_CASE("decode_integral_ppnumber error test") {
    using kusabira::PP::free_func::decode_integral_ppnumber;
    using kusabira::overloaded;
    using kusabira::PP::pp_parse_context;
    //using enum kusabira::PP::pp_parse_context;

    // 浮動小数点数リテラル
    {
      auto result = decode_integral_ppnumber(u8"1.0", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1.0fl", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0.00001010101", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xC.68p+2", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1E-3", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"234e3", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"3E+4", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }

    // lL/Llみたいなリテラルサフィックス
    {
      auto result = decode_integral_ppnumber(u8"10lL", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"10Ll", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabf759Ll", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabf759lL", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0710lL", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0710Ll", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b0111Ll", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b0111lL", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }

    // オーバーフロー
    {
      // 64bit符号なし整数最大値+1
      auto result = decode_integral_ppnumber(u8"18'446'744'073'709'551'616", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_OutOfRange); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      // 64bit符号なし整数最大値+1
      auto result = decode_integral_ppnumber(u8"18'446'744'073'709'551'616ull", 1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_OutOfRange); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      // 64bit符号付整数最小値-1
      auto result = decode_integral_ppnumber(u8"9'223'372'036'854'775'809", -1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_OutOfRange); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
  }

}