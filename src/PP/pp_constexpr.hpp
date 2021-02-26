#pragma once

#include <ranges>
#include <optional>
#include <functional>
#include <utility>
#include <variant>
#include <cctype>

#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira::PP::inline free_func {

  fn decode_integral_ppnumber(std::u8string_view number_strview) -> std::pair<std::variant<std::intmax_t, std::uintmax_t, bool>, std::uint8_t> {
    constexpr auto npos = std::u8string_view::npos;
    constexpr std::uint8_t signed_integral = 0;
    constexpr std::uint8_t unsigned_integral = 1;
    constexpr std::uint8_t floating_point = 2;

    std::pmr::u8string numstr{ &kusabira::def_mr };
    numstr.reserve(number_strview.length());

    // 桁区切り'を取り除くとともに、大文字を小文字化しておく
    for (auto inner_range : number_strview | std::views::split(u8'\'')) {
      for (char8_t c : inner_range | std::views::transform([](unsigned char ch) -> char8_t { return std::tolower(ch); })) {
        numstr.push_back(c);
      }
    }

    // 浮動小数点数判定
    // 16進浮動小数点数には絶対にpが表れる
    if (auto p = std::ranges::find_if(numstr, [](char8_t c) { return c == u8'.' or c == u8'p';}); p != numstr.end()) {
      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
    }
    // 小数点の現れないタイプの10進浮動小数点数
    // 1E-1とか2e+2とか3E-4flみたいな、絶対にeが表れる
    if (auto e = std::ranges::find_if(numstr, [](char8_t c) { return c == u8'e';}); e != numstr.end()) {
      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
    }

    // 抜けたら整数値のはず・・・

    // 2 or 8 or 10 or 16進数判定
    std::uint8_t base = 10;
    const char8_t *start_pos = std::ranges::data(numstr);

    if (numstr.starts_with(u8"0b")) {
      base = 2;
      start_pos += 2;
    } else if (numstr.starts_with(u8"0x")) {
      base = 16;
      start_pos += 2;
    } else if (numstr.starts_with(u8"0")) {
      base = 8;
      start_pos += 1;
    }

    // リテラル部を除いた数値の終端を求める
    char8_t const * const true_end = std::addressof(numstr.back()) + 1;
    const char8_t *end_pos = true_end;

    if (base == 16) {
      end_pos = std::ranges::find_if(start_pos, true_end, [](unsigned char ch) -> bool { return std::isxdigit(ch); });
    } else {
      end_pos = std::ranges::find_if(start_pos, true_end, [](unsigned char ch) -> bool { return std::isdigit(ch); });
    }

    // リテラル判定、ユーザー定義リテラルはエラー
    // u U
    // l L
    // ll LL
    // z Z
    // 大文字は考慮しなくてよく、u以外は他とくっつかない
    // 候補は  u l ll z ul ull uz lu llu zu
    // uが入ってたら符号なし、それ以外は符号付
    bool is_signed = true;
    std::u8string_view suffix{end_pos, true_end};

    // 空だったら符号付整数
    if (not empty(suffix)) {

    }

    // from_charsに叩き込む

    if (is_signed) {
      std::intmax_t num;
    } else {
      std::uintmax_t num;
    }

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
        } else if (deref(it).token != u8")") {
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