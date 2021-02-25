#pragma once

#include <ranges>
#include <optional>
#include <functional>
#include <utility>
#include <variant>

#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira::PP::inline free_func {

  fn decode_integral_ppnumber(std::u8string_view number_str) -> std::pair<std::variant<std::intmax_t, std::uintmax_t, bool>, std::uint8_t> {
    constexpr auto npos = std::u8string_view::npos;
    constexpr std::uint8_t signed_integral = 0;
    constexpr std::uint8_t unsigned_integral = 1;
    constexpr std::uint8_t floating_point = 2;

    // 浮動小数点数判定
    // 16進浮動小数点数には絶対にpが表れる
    if (auto p = std::ranges::find_if(number_str, [](char8_t c) { return c == u8'.' or c == u8'p' or c == u8'P';}); p != number_str.end()) {
      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
    }
    // 小数点の現れないタイプの10進浮動小数点数
    // 1E-1とか2e+2とか3E-4flみたいな、絶対にeが表れる
    if (auto e = std::ranges::find_if(number_str, [](char8_t c) { return c == u8'e' or c == u8'E';}); e != number_str.end()) {
      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
    }

    // 抜けたら整数値のはず・・・

    // 2 or 8 or 10 or 16進数判定

    // リテラル判定、ユーザー定義リテラルはエラー

    // 'を取り除く

    // from_charsに叩き込む

    // おわり
  }
}

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

    void err_report(const PP::pp_token& token, pp_parse_context context) const {
      this->reporter.pp_err_report(this->filename, token, context);
    }

    template<std::ranges::forward_range Tokens>
      requires std::same_as<std::iter_value_t<std::ranges::iterator_t<Tokens>>, pp_token>
    fn operator()(const Tokens& token_list) const -> std::optional<bool> {
      // 事前条件、空でないこと
      assert(empty(token_list));

      auto it = std::ranges::begin(token_list);
      auto last = std::ranges::end(token_list);

      auto succeed = this->conditional_expression(it, last);

      // エラー報告は済んでいるものとする
      if (not succeed) return std::nullopt;

      // 全部消費してるよね？
      ++it;
      assert(it == last);

      auto num = *std::move(succeed);

      // 0 == false, それ以外 == true
      return num != 0;
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    fn conditional_expression(I& it, S last) const -> pp_constexpr_result {
      return this->primary_expressions(it, last);
    }

    
    
    template<std::input_iterator I, std::sentinel_for<I> S>
    fn primary_expressions(I& it, S last) const -> pp_constexpr_result {
      // 考慮すべきはliteralと(conditional-expression)の2種類

      const auto token_cat = deref(it).category;

      // 整数型と文字型を考慮
      if (token_cat == pp_token_category::pp_number or token_cat == pp_token_category::charcter_literal) {
        // pp_numberの読み取り
        // 整数値のみがwell-formed、浮動小数点数値はエラー

        return kusabira::ok(0);
      }

      // ()に囲まれた式の考慮
      else if (token_cat == pp_token_category::op_or_punc and deref(it).token == u8"(") {
        auto succeed = this->conditional_expression(it, last);

        if (not succeed) return kusabira::error(false);

        const auto before_it = it;
        ++it;

        // 対応する閉じ括弧が出現していない
        if (it == last) {
          this->err_report(*before_it, pp_parse_context::PPConstexpr_MissingCloseParent);
          return kusabira::error(false);
        } else if (not deref(it).token == u8")") {
          this->err_report(*it, pp_parse_context::PPConstexpr_MissingCloseParent);
          return kusabira::error(false);
        }

        auto num = *std::move(succeed);

        return kusabira::ok(num);
      }

      // それ以外の出現はエラー
      this->err_report(*it, pp_parse_context::PPConstexpr_Invalid);
      return kusabira::error(false);
    }
  };
}