#pragma once

#include "doctest/doctest.h"
#include "vocabulary/whimsy_str_view.hpp"

namespace kusabira::whimsy_str_view_test {

  using u8whimsy_str_view = kusabira::vocabulary::whimsy_str_view<>;
  using namespace std::string_view_literals;

  TEST_CASE("construct test") {
    {
      std::pmr::u8string str = u8"test string copy";
      u8whimsy_str_view string(str);

      CHECK_UNARY(string.has_own_string());
      CHECK_UNARY(bool(string));

      CHECK_UNARY(string == u8"test string copy"sv);
    }

    {
      std::pmr::u8string str = u8"test string move";
      u8whimsy_str_view string(std::move(str));

      CHECK_UNARY(string.has_own_string());
      CHECK_UNARY(bool(string));

      CHECK_UNARY(string == u8"test string move"sv);
    }

    {
      //デフォルト構築はviewとして初期化
      u8whimsy_str_view string{};

      CHECK_UNARY_FALSE(string.has_own_string());
      CHECK_UNARY_FALSE(bool(string));

      CHECK_UNARY(string == u8""sv);
    }

    {
      u8whimsy_str_view string(u8"test string view copy"sv);

      CHECK_UNARY_FALSE(string.has_own_string());
      CHECK_UNARY_FALSE(bool(string));

      CHECK_UNARY(string == u8"test string view copy"sv);
    }

    {
      u8whimsy_str_view string2(u8"test copy view to view"sv);
      u8whimsy_str_view string(string2);

      CHECK_UNARY_FALSE(string2.has_own_string());
      CHECK_UNARY_FALSE(string.has_own_string());
      CHECK_UNARY_FALSE(bool(string));

      CHECK_UNARY(string == u8"test copy view to view"sv);
      CHECK_UNARY(string.to_view() == string2);
    }

    {
      u8whimsy_str_view string2(std::pmr::u8string{u8"test copy string to string"});
      u8whimsy_str_view string(string2);

      CHECK_UNARY(string2.has_own_string());
      CHECK_UNARY(string.has_own_string());
      CHECK_UNARY(bool(string));

      CHECK_UNARY(string == u8"test copy string to string"sv);
      CHECK_UNARY(string.to_view() == string2);
    }

    {
      u8whimsy_str_view string2(u8"test move view to view"sv);
      u8whimsy_str_view string(std::move(string2));

      CHECK_UNARY_FALSE(string.has_own_string());
      CHECK_UNARY_FALSE(bool(string));

      CHECK_UNARY(string == u8"test move view to view"sv);
    }

    {
      u8whimsy_str_view string2(std::pmr::u8string{ u8"test move string to string" });
      u8whimsy_str_view string(std::move(string2));

      CHECK_UNARY(string.has_own_string());
      CHECK_UNARY(bool(string));

      CHECK_UNARY(string == u8"test move string to string"sv);
    }
  }

  TEST_CASE("assignment test") {

    const std::pmr::u8string str = u8"assignment test";
    std::u8string_view view{ str };

    //コピー代入
    {
      u8whimsy_str_view tmp(str);
      u8whimsy_str_view copy{};

      copy = tmp;

      CHECK_UNARY(copy.has_own_string());
      CHECK_UNARY(bool(copy));

      CHECK_UNARY(copy == str);

      tmp = view;
      copy = tmp;

      CHECK_UNARY_FALSE(copy.has_own_string());
      CHECK_UNARY_FALSE(bool(copy));

      CHECK_UNARY(copy == view);
    }

    //ムーブ代入
    {
      u8whimsy_str_view move{};

      move = u8whimsy_str_view(str);

      CHECK_UNARY(move.has_own_string());
      CHECK_UNARY(bool(move));

      CHECK_UNARY(move == str);

      move = u8whimsy_str_view(view);

      CHECK_UNARY_FALSE(move.has_own_string());
      CHECK_UNARY_FALSE(bool(move));

      CHECK_UNARY(move == view);
    }

    //std::stringのコピー代入
    {
      u8whimsy_str_view copy{};

      copy = str;

      CHECK_UNARY(copy.has_own_string());
      CHECK_UNARY(bool(copy));

      CHECK_UNARY(copy == str);
    }

    //std::stringのムーブ代入
    {
      u8whimsy_str_view move{};

      move = std::pmr::u8string{ str };

      CHECK_UNARY(move.has_own_string());
      CHECK_UNARY(bool(move));

      CHECK_UNARY(move == str);
    }

    //std::string_viewの代入
    {
      u8whimsy_str_view copy{};

      copy = view;

      CHECK_UNARY_FALSE(copy.has_own_string());
      CHECK_UNARY_FALSE(bool(copy));

      CHECK_UNARY(copy == str);
    }
  }

  TEST_CASE("convertion test") {

    std::pmr::u8string str = u8"test string";
    u8whimsy_str_view string(str);
    u8whimsy_str_view view(u8"test view"sv);

    //string保持状態からstringへの変換
    auto string_copy = string.to_string();
    CHECK_UNARY(string.has_own_string());
    CHECK_UNARY(string_copy == string.to_view());

    //view状態からstringへの変換
    string_copy = view.to_string();
    CHECK_UNARY_FALSE(view.has_own_string());
    CHECK_UNARY(string_copy == view.to_view());

    //右辺値での変換
    string_copy = u8whimsy_str_view{ u8"test rvalue"sv }.to_string();
    CHECK_UNARY(string_copy == u8"test rvalue"sv);
  }

  TEST_CASE("swap test") {
    {
      //viewとstring保持状態のswap

      std::pmr::u8string str = u8"test swap string";
      u8whimsy_str_view string(str);
      u8whimsy_str_view view(u8"test swap view"sv);

      CHECK_UNARY_FALSE(view.has_own_string());
      CHECK_UNARY(string.has_own_string());

      swap(string, view);

      CHECK_UNARY_FALSE(string.has_own_string());
      CHECK_UNARY(view.has_own_string());

      CHECK_UNARY(view == u8"test swap string"sv);
      CHECK_UNARY(string == u8"test swap view"sv);

      //戻す
      swap(string, view);

      CHECK_UNARY_FALSE(view.has_own_string());
      CHECK_UNARY(string.has_own_string());

      CHECK_UNARY(string == u8"test swap string"sv);
      CHECK_UNARY(view == u8"test swap view"sv);
    }

    {
      //viewとview

      u8whimsy_str_view view1(u8"test swap view and view 1"sv);
      u8whimsy_str_view view2(u8"test swap view and view 2"sv);

      CHECK_UNARY_FALSE(view1.has_own_string());
      CHECK_UNARY_FALSE(view2.has_own_string());

      swap(view1, view2);

      CHECK_UNARY_FALSE(view1.has_own_string());
      CHECK_UNARY_FALSE(view2.has_own_string());

      CHECK_UNARY(view1 == u8"test swap view and view 2"sv);
      CHECK_UNARY(view2 == u8"test swap view and view 1"sv);
    }

    {
      //stringとstring

      u8whimsy_str_view string1{ std::pmr::u8string{ u8"test swap string and string 1" } };
      u8whimsy_str_view string2{ std::pmr::u8string{ u8"test swap string and string 2" } };

      CHECK_UNARY(string1.has_own_string());
      CHECK_UNARY(string1.has_own_string());

      swap(string1, string2);

      CHECK_UNARY(string1.has_own_string());
      CHECK_UNARY(string1.has_own_string());

      CHECK_UNARY(string1 == u8"test swap string and string 2"sv);
      CHECK_UNARY(string2 == u8"test swap string and string 1"sv);
    }
  }

  TEST_CASE("iterator interface test") {
    {
      std::pmr::u8string expect = u8"test iterator interface";
      u8whimsy_str_view string{expect};

      CHECK_UNARY(string.has_own_string());

      int i = 0;
      for (auto c : string) {
        CHECK_UNARY(c == expect[i]);
        ++i;
      }
    }

    {
      auto expect = u8"test iterator interface"sv;
      u8whimsy_str_view string{expect};

      CHECK_UNARY_FALSE(string.has_own_string());

      int i = 0;
      for (auto c : string) {
        CHECK_UNARY(c == expect[i]);
        ++i;
      }
    }
  }

  TEST_CASE("span interface test") {
    {
      std::pmr::u8string expect = u8"test iterator interface";
      u8whimsy_str_view string{expect};

      CHECK_UNARY(string.has_own_string());

      auto ptr = data(string);
      auto len = size(string);

      for (auto i = 0u; i < len; ++i) {
        CHECK_UNARY(ptr[i] == expect[i]);
      }
    }

    {
      auto expect = u8"test iterator interface"sv;
      u8whimsy_str_view string{expect};

      CHECK_UNARY_FALSE(string.has_own_string());

      auto ptr = data(string);
      auto len = size(string);

      for (auto i = 0u; i < len; ++i) {
        CHECK_UNARY(ptr[i] == expect[i]);
      }

    }
  }

  TEST_CASE("comparison test") {
    const std::pmr::u8string str = u8"comparison test";
    const std::u8string_view view{ str };
    const u8whimsy_str_view wview{ str };

    CHECK_UNARY(wview == str);
    CHECK_UNARY(wview == view);
    CHECK_UNARY(wview == wview);

    const u8whimsy_str_view neq{u8"comparison test not equal"sv};

    CHECK_UNARY_FALSE(neq == str);
    CHECK_UNARY_FALSE(neq == view);
    CHECK_UNARY_FALSE(neq == wview);
  }

}
