#pragma once

#include <ranges>
#include <optional>
#include <functional>

#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira::PP {

  using pp_constexpr_result = kusabira::expected<std::int64_t, bool>;

  // プリプロセッシングディレクティブの実行に必要な程度の定数式を処理する
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
    fn conditional_expression(I& it, S end) const -> pp_constexpr_result {
      return kusabira::ok(1);
    }
    
  };
}