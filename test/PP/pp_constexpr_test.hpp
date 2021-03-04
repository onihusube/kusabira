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
      auto result = decode_integral_ppnumber(u8"12345");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 12345); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      // 64bit符号なし整数最大値
      auto result = decode_integral_ppnumber(u8"18'446'744'073'709'551'615");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 18'446'744'073'709'551'615ull); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      // 64bit符号付整数最大値
      auto result = decode_integral_ppnumber(u8"9'223'372'036'854'775'807");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 9'223'372'036'854'775'807); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      // 64bit符号付整数最大値+1、結果は符号なし整数型で得られる
      auto result = decode_integral_ppnumber(u8"9'223'372'036'854'775'808");
      std::visit(overloaded{
                     [](std::uintmax_t num) { CHECK_UNARY(num == 9'223'372'036'854'775'808ull); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 1); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 16進リテラル
    {
      auto result = decode_integral_ppnumber(u8"0xffFFabc");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 268434108); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0x0");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 8進リテラル
    {
      auto result = decode_integral_ppnumber(u8"01234567");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 342391); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 2進リテラル
    {
      auto result = decode_integral_ppnumber(u8"0b1100110111011");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 6587); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

  }

  TEST_CASE("decode_integral_ppnumber suffix test") {
    using kusabira::PP::free_func::decode_integral_ppnumber;
    using kusabira::overloaded;

    // リテラルサフィックスに対して型が合うかのテスト
    {
      auto result = decode_integral_ppnumber(u8"1234567890u");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890l");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890z");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"123456789'0ll");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890ul");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890ull");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12'34567890uz");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890lu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"123456'7890llu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1234567890zu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 1234567890); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 16進
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0Xabcdefl");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdeFz");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcDefll");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefuL");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefull");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xaBcdefUz");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdeflu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabcdefLLu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xAbcdefZu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0xabcdef); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

    // 2進
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101U");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101L");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101Z");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101LL");
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101Ul");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101ull");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101uz");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101lu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b101101110101LLu");
      std::visit(overloaded{
        [](std::uintmax_t num) { CHECK_UNARY(num == 0b101101110101); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0B101101110101Zu");
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
      auto result = decode_integral_ppnumber(u8"1.0");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1.0fl");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0.00001010101");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xC.68p+2");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"1E-3");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"234e3");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"3E+4");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }

    // lL/Llみたいなリテラルサフィックス
    {
      auto result = decode_integral_ppnumber(u8"10lL");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"10Ll");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabf759Ll");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xabf759lL");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0710lL");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0710Ll");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b0111Ll");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0b0111lL");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }

    // 10進数のabcdef
    {
      auto result = decode_integral_ppnumber(u8"12a");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12b34");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12c732398");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12d");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }
    // eが入るとエラーが違う・・・
    {
      auto result = decode_integral_ppnumber(u8"12e");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12defab");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_FloatingPointNumber); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      auto result = decode_integral_ppnumber(u8"12f");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_UDL); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); } },
                     result);
    }

    // オーバーフロー
    {
      // 64bit符号なし整数最大値+1
      auto result = decode_integral_ppnumber(u8"18'446'744'073'709'551'616");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_OutOfRange); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    {
      // 64bit符号なし整数最大値+1
      auto result = decode_integral_ppnumber(u8"18'446'744'073'709'551'616ull");
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_OutOfRange); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }
    /*{
      // 64bit符号付整数最小値-1
      auto result = decode_integral_ppnumber(u8"9'223'372'036'854'775'809", -1);
      std::visit(overloaded{
                     [](pp_parse_context err) { CHECK_EQ(err, pp_parse_context::PPConstexpr_OutOfRange); },
                     []([[maybe_unused]] auto context) { CHECK_UNARY(false); }},
                 result);
    }*/
  }

}