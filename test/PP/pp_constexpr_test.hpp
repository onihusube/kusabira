#pragma once

#include "doctest/doctest.h"
#include "src/PP/pp_constexpr.hpp"

// てすと
namespace kusabira_test::constexpr_test {

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

    // 16進リテラル
    {
      auto result = decode_integral_ppnumber(u8"0xffFFabc", 1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == 268434108); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }
    {
      auto result = decode_integral_ppnumber(u8"0xffFFabc", -1);
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
      auto result = decode_integral_ppnumber(u8"0b1100110111011", -1);
      std::visit(overloaded{
        [](std::intmax_t num) { CHECK_UNARY(num == -6587); },
        []([[maybe_unused]] auto context) { CHECK_UNARY(false); } }, result);
    }

  }

}