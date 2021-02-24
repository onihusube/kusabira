#pragma once

#include <ranges>
#include <optional>
#include <functional>

#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira::PP {

  using pp_constexpr_result = kusabira::expected<std::int64_t, bool>;

  /**
  * @brief プリプロセッシングディレクティブの実行に必要な程度の定数式を処理する
  * @details C++の意味論で解釈される構文は考慮しなくて良い
  */
  template<typename Reporter>
  struct pp_constexpr {

    const Reporter& reporter;
    const fs::path& filename;

    template<std::ranges::range Tokens>
      requires std::same_as<std::iter_value_t<std::ranges::iterator_t<Tokens>>, pp_token>
    fn operator()(const Tokens& token_list) const -> std::optional<bool> {
      if (empty(token_list)) {
        return true;
      } else {
        return false;
      }
    }


    template<std::input_iterator I, std::sentinel_for<I> S>
    fn conditional_expression(I& it, S last) const -> pp_constexpr_result {
      return kusabira::ok(1);
    }

    
    
    template<std::input_iterator I, std::sentinel_for<I> S>
    fn primary_expressions(I& it, S last) const -> pp_constexpr_result {
      // 考慮すべきはliteralと(conditional-expression)の2種類

      const auto token_cat = deref(it).category;

      // 整数型と文字型を考慮
      if (token_cat == pp_token_category::pp_number or token_cat == pp_token_category::charcter_literal) {

        return kusabira::ok(0ull);
      }

      // ()に囲まれた式の考慮
      else if (token_cat == pp_token_category::op_or_punc and deref(it).token == u8"(") {
        auto [num, success] = this->conditional_expression(it, last);

        if (not success) return kusabira::error(false);

        ++it;

        if (not deref(it).token == u8")") {
          // 対応する閉じ括弧が出現していない
          return kusabira::error(false);
        }

        return kusabira::ok(num);
      }
    }
  };
}