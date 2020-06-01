#pragma once

#include "doctest/doctest.h"
#include "vocabulary/scope.hpp"

namespace kusabira::scope_test {

  using namespace kusabira::vocabulary;

  TEST_CASE("common_scope_exit test") {
    auto f = []() {};

    //代入/コピー構築不可
    static_assert(not std::is_copy_constructible<scope_exit<decltype(f)>>::value);
    static_assert(not std::is_copy_assignable<scope_exit<decltype(f)>>::value);
    static_assert(not std::is_move_assignable<scope_exit<decltype(f)>>::value);

    //ムーブ構築可
    static_assert(std::is_move_constructible<scope_exit<decltype(f)>>::value);

    //デストラクタは例外を投げない
    static_assert(std::is_nothrow_destructible<scope_exit<decltype(f)>>::value);
  }

  TEST_CASE("scope_exit test") {
    int n = 10;

    //nを20にする
    auto &&f = [&n]() {
      n += 10;
    };

    //このスコープの終わりで実行
    {
      scope_exit test_scope_exit{std::move(f)};

      //未実行
      CHECK_EQ(10, n);
    }

    //実行済
    CHECK_EQ(20, n);
  }

  TEST_CASE("scope_succes test") {
    int n = 10;

    //nを+10する
    auto &&f = [&n]() {
      n += 10;
    };

    //このスコープの終わりで実行
    {
      scope_success test_scope_exit{f};

      //未実行
      CHECK_EQ(10, n);
    }

    //実行済
    CHECK_EQ(20, n);

    //ここは実行されない
    try
    {
      scope_success test_scope_exit{f};

      throw std::exception{};
    }
    catch (...)
    {
    }

    //未実行
    CHECK_EQ(20, n);

    //ここも実行されない
    try
    {
      try
      {
        scope_success test_scope_exit{std::move(f)};

        throw std::exception{};
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
    }

    //未実行
    CHECK_EQ(20, n);
  }

  TEST_CASE("scope_fail test") {
    int n = 10;

    //nを+10する
    auto &&f = [&n]() {
      n += 10;
    };

    //ここは実行されない
    {
      scope_fail test_scope_exit{f};
    }

    //未実行
    CHECK_EQ(10, n);

    //このスコープの終わりで実行
    try
    {
      scope_fail test_scope_exit{f};

      throw std::exception{};
    }
    catch (...)
    {
    }

    //実行済
    CHECK_EQ(20, n);

    //ここも実行される
    try
    {
      try
      {
        scope_fail test_scope_exit{std::move(f)};

        throw std::exception{};
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
    }

    //実行済
    CHECK_EQ(30, n);
  }

  TEST_CASE("release test") {
    //呼び出されると失敗
    auto f = []() {
      REQUIRE_UNARY(false);
    };

    {
      scope_exit test_scope_exit{f};

      //実行責任を手放す
      test_scope_exit.release();
    }

    {
      scope_success test_scope_exit{f};

      //実行責任を手放す
      test_scope_exit.release();
    }

    try {
      scope_fail test_scope_exit{f};

      //実行責任を手放す
      test_scope_exit.release();

      throw std::exception{};
    }
    catch (...) {}
  }

} // namespace kusabira::scope_test
