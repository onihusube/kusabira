#pragma once

#include <vector>
#include <list>
#include <array>

#include "doctest/doctest.h"
#include "vocabulary/concat.hpp"

// concatのテスト
namespace kusabira::concat_test {

  using kusabira::vocabulary::concat;

  TEST_CASE("concat_view test") {

    std::vector<int> v1 = { 1, 2, 3, 4, 5 };
    std::vector<int> v2 = { 6, 7, 8, 9, 10 };

    auto concat_view = concat(v1.begin(), v1.end(), v2.begin(), v2.end());
  
    auto it = begin(concat_view);
    auto se = end(concat_view);

    CHECK_UNARY_FALSE(it == se);

    CHECK_EQ(1, *it);

    ++it;
    it++;

    CHECK_EQ(3, *it);

    std::advance(it, 3);
    CHECK_EQ(6, *it);

    std::advance(it, 5);

    CHECK_UNARY(it == se);
  }

#ifndef __GNUC__

  TEST_CASE("iterator concat test") {

    std::vector<int> v1 = { 1, 2, 3, 4, 5 };
    std::vector<int> v2 = { 6, 7, 8, 9, 10 };

    auto concat_view = concat(v1.begin(), v1.end(), v2.begin(), v2.end());

    int expect = 1;
    for (auto n : concat_view) {
      CHECK_EQ(expect, n);
      ++expect;
    }
  }

  TEST_CASE("iterator concat test2") {

    std::vector<int> ve = { 1, 2, 3, 4, 5 };
    std::list<int> ls = { 6, 7, 8, 9, 10 };

    int expect = 1;
    for (auto n : concat(ve.begin(), ve.end(), ls.begin(), ls.end())) {
      CHECK_EQ(expect, n);
      ++expect;
    }
  }

  template<std::size_t N>
  constexpr int sum(std::array<int, N> ar1) {

    int ar2[] = { 6, 7, 8, 9, 10 };

    auto concat_view = concat(ar1.begin(), ar1.end(), ar2, std::end(ar2));

    int s = 0;
    for (auto n : concat_view) {
      s += n;
    }

    return s;
  }

  TEST_CASE("constexpr test") {
    constexpr int result = sum(std::array<int, 5>{ 1, 2, 3, 4, 5 });

    static_assert(result == 55);
    CHECK_EQ(55, result);
  }

  TEST_CASE("empty concat test1") {

    std::vector<int> ve{};
    std::list<int> ls = { 1, 2, 3, 4, 5 };

    int expect = 1;
    for (auto n : concat(ve.begin(), ve.end(), ls.begin(), ls.end())) {
      CHECK_EQ(expect, n);
      ++expect;
    }
  }

  TEST_CASE("empty concat test2") {

    std::vector<int> ve = { 1, 2, 3, 4, 5 };
    std::list<int> ls{};

    auto concat_view = concat(ve.begin(), ve.end(), ls.begin(), ls.end());

    int expect = 1;
    for (auto n : concat_view) {
      CHECK_EQ(expect, n);
      ++expect;
    }
  }

  TEST_CASE("empty concat test3") {

    std::vector<int> ve = {};
    std::list<int> ls{};

    auto concat_view = concat(ve.begin(), ve.end(), ls.begin(), ls.end());

    for ([[maybe_unused]] auto n : concat_view) {
      CHECK_UNARY(false);
    }
  }

#else

  // g++がイテレータのbegin()とend()の型が違うと怒るので、最小限のテスト
  TEST_CASE("concat_view gcc test") {

    std::vector<int> v1 = { 1, 2, 3, 4, 5 };
    std::list<int>   ls = { 6, 7, 8, 9, 10};

    auto concat_view = concat(v1.begin(), v1.end(), ls.begin(), ls.end());
  
    auto it = begin(concat_view);
    auto se = end(concat_view);

    int expect = 1;
    for (; it != se; ++it) {
      CHECK_EQ(expect, *it);
      ++expect;
    }
  }

#endif // __GNUC__
}