#pragma once

#include <list>

#include "doctest/doctest.h"

#include "PP/macro_manager.hpp"
#include "../report_output_test.hpp"

namespace unified_macro_test {
  using kusabira::PP::logical_line;
  using kusabira::PP::pp_token;
  using kusabira::PP::pp_token_category;
  using namespace std::string_view_literals;
  using kusabira::PP::unified_macro;

  TEST_CASE("validate_argnum test") {
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<kusabira_test::report::test_out>::create();

    // 普通の関数マクロのテスト
    {
      //(*pos).line = u8"#define test(a, b)";

      // 置換リスト
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      // 仮引数
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      params.emplace_back(u8"a"sv);
      params.emplace_back(u8"b"sv);

      // 関数マクロ登録
      unified_macro macro{u8"test"sv, params, rep_list, false};

      CHECK_UNARY(macro.is_function());
      REQUIRE_UNARY(macro.is_ready() == std::nullopt);

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};

      // 0
      CHECK_UNARY(macro.validate_argnum(args) == false);

      token_list.emplace_back(pp_token_category::identifier, u8"i");
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

      // 1
      CHECK_UNARY(macro.validate_argnum(args) == false);

      token_list.emplace_back(pp_token_category::identifier, u8"j");
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

      // 2
      CHECK_UNARY(macro.validate_argnum(args));

      token_list.emplace_back(pp_token_category::identifier, u8"k");
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

      // 3
      CHECK_UNARY(macro.validate_argnum(args) == false);

      // 2+空のトークン
      args.pop_back();
      args.emplace_back(token_list);

      CHECK_UNARY(macro.validate_argnum(args) == false);
    }


    // 引き数なしマクロ
    {
      // #define test() ...;

      // 置換リスト
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      // 仮引数
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };

      // 関数マクロ登録
      unified_macro macro{ u8"test"sv, params, rep_list, false };

      CHECK_UNARY(macro.is_function());
      REQUIRE_UNARY(macro.is_ready() == std::nullopt);

      std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };

      // 0
      CHECK_UNARY(macro.validate_argnum(args));


      args.emplace_back(token_list);

      // 1、空のトークン
      CHECK_UNARY(macro.validate_argnum(args));

      args.pop_back();
      token_list.emplace_back(pp_token_category::identifier, u8"i");
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

      // 1
      CHECK_UNARY(macro.validate_argnum(args) == false);
    }

    // 可変長マクロ1
    {
      // #define test(...)

      // 置換リスト
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      // 仮引数
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };
      params.emplace_back(u8"..."sv);

      // 関数マクロ登録
      unified_macro macro{ u8"test"sv, params, rep_list, true };

      CHECK_UNARY(macro.is_function());
      REQUIRE_UNARY(macro.is_ready() == std::nullopt);

      std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };

      // 0
      CHECK_UNARY(macro.validate_argnum(args));

      args.emplace_back(token_list);

      // 1、空のトークン
      CHECK_UNARY(macro.validate_argnum(args));

      args.pop_back();
      token_list.emplace_back(pp_token_category::identifier, u8"i");
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

      // 1
      CHECK_UNARY(macro.validate_argnum(args));

      args.emplace_back(token_list);

      // 1 + 空のトークン
      CHECK_UNARY(macro.validate_argnum(args));
    }

    {
      // #define test(a, ...)

      // 置換リスト
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      // 仮引数
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };
      params.emplace_back(u8"a"sv);
      params.emplace_back(u8"..."sv);

      // 関数マクロ登録
      unified_macro macro{ u8"test"sv, params, rep_list, true };

      CHECK_UNARY(macro.is_function());
      REQUIRE_UNARY(macro.is_ready() == std::nullopt);

      std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };

      // 0
      CHECK_UNARY(macro.validate_argnum(args) == false);

      token_list.emplace_back(pp_token_category::identifier, u8"i");
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

      // 1
      CHECK_UNARY(macro.validate_argnum(args));

    }
  }

}