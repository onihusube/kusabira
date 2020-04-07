#pragma once

#include "doctest/doctest.h"
#include "PP/pp_directive_manager.hpp"

namespace kusabira_test::preprocessor
{

  TEST_CASE("rawstrliteral extract test") {
    using namespace std::string_view_literals;

    {
      auto rstr = u8R"++++(u8R"(normal raw string literal)")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"normal raw string literal");
    }

    {
      auto rstr = u8R"++++(LR"()")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY(result.empty());
    }

    {
      auto rstr = u8R"++++(UR"abcdefghijklmnop(max delimiter)abcdefghijklmnop")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"max delimiter");
    }

    {
      auto rstr = u8R"++++(UR"abcdefghijklmnop()abcdefghijklmnop")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY(result.empty());
    }
  }

  TEST_CASE("strliteral extract test") {

    {
      auto str = u8R"++++("normal string literal")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(str, false);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"normal string literal");
    }

    {
      auto str = u8R"++++(U"")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(str, false);

      CHECK_UNARY(result.empty());
    }

    {
      auto str = u8R"++++(u8"string " literal")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(str, false);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"string \" literal");
    }
  }

} // namespace kusabira_test::preprocessor
