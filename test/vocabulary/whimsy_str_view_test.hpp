#pragma once

#include "doctest/doctest.h"
#include "vocabulary/whimsy_str_view.hpp"

namespace kusabira::whimsy_str_view_test {

  using u8whimsy_str_view = kusabira::vocabulary::whimsy_str_view<>;
  using namespace std::literals;

  TEST_CASE("construct test") {
    {
      std::pmr::u8string str = u8"test string copy";
      u8whimsy_str_view string(str);

      CHECK_UNARY(string.has_own_string() == true);
      CHECK_UNARY(bool(string) == true);

      CHECK_UNARY(string == u8"test string copy"sv);
    }

    {
      std::pmr::u8string str = u8"test string move";
      u8whimsy_str_view string(std::move(str));

      CHECK_UNARY(string.has_own_string() == true);
      CHECK_UNARY(bool(string) == true);

      CHECK_UNARY(string == u8"test string move"sv);
    }

    {
      //デフォルト構築はviewとして初期化
      u8whimsy_str_view string{};

      CHECK_UNARY(string.has_own_string() == false);
      CHECK_UNARY(bool(string) == false);

      CHECK_UNARY(string == u8""sv);
    }

    {
      u8whimsy_str_view string(u8"test string view copy"sv);

      CHECK_UNARY(string.has_own_string() == false);
      CHECK_UNARY(bool(string) == false);

      CHECK_UNARY(string == u8"test string view copy"sv);
    }

    {
      u8whimsy_str_view string2(u8"test copy view to view"sv);
      u8whimsy_str_view string(string2);

      CHECK_UNARY(string2.has_own_string() == false);
      CHECK_UNARY(string.has_own_string() == false);
      CHECK_UNARY(bool(string) == false);

      CHECK_UNARY(string == u8"test copy view to view"sv);
      CHECK_UNARY(string.to_view() == string2);
    }

    {
      u8whimsy_str_view string2(std::pmr::u8string{u8"test copy string to string"});
      u8whimsy_str_view string(string2);

      CHECK_UNARY(string2.has_own_string() == true);
      CHECK_UNARY(string.has_own_string() == true);
      CHECK_UNARY(bool(string) == true);

      CHECK_UNARY(string == u8"test copy string to string"sv);
      CHECK_UNARY(string.to_view() == string2);
    }
  }

}
