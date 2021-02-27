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

  fn decode_integral_ppnumber(std::u8string_view number_strview, int sign) -> std::pair<std::variant<std::intmax_t, std::uintmax_t, bool>, std::uint8_t> {
    // 事前条件
    assert(sign == -1 or sign == 1);

    constexpr auto npos = std::u8string_view::npos;
    constexpr std::uint8_t signed_integral = 0;
    constexpr std::uint8_t unsigned_integral = 1;
    constexpr std::uint8_t floating_point = 2;

    std::pmr::string numstr{ &kusabira::def_mr };
    numstr.reserve(number_strview.length());

    // 桁区切り'を取り除くとともに、大文字を小文字化しておく
    // char8_t(UTF-8)をchar(ASCII)として処理する
    for (char c : number_strview | std::views::filter([](char8_t ch) { return ch != u8'\''; })
                                 | std::views::transform([](unsigned char ch) -> char { return std::tolower(ch); }))
    {
      numstr.push_back(c);
    }

    // 浮動小数点数判定
    // 16進浮動小数点数には絶対にpが表れる
    if (auto p = std::ranges::find_if(numstr, [](char c) { return c == '.' or c == 'p';}); p != numstr.end()) {
      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
    }
    // 小数点の現れないタイプの10進浮動小数点数
    // 1E-1とか2e+2とか3E-4flみたいな、絶対にeが表れる
    if (auto e = std::ranges::find_if(numstr, [](char c) { return c == 'e';}); e != numstr.end()) {
      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
    }

    // 抜けたら整数値のはず・・・

    // 2 or 8 or 10 or 16進数判定
    std::uint8_t base = 10;
    const char *start_pos = std::ranges::data(numstr);

    if (numstr.starts_with("0b")) {
      base = 2;
      start_pos += 2;
    } else if (numstr.starts_with("0x")) {
      base = 16;
      start_pos += 2;
    } else if (numstr.starts_with("0")) {
      base = 8;
      start_pos += 1;
    }

    // リテラル部を除いた数値の終端を求める
    char const * const true_end = std::addressof(numstr.back()) + 1;
    const char* end_pos = true_end;

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
    // 候補は  u l z ul uz lu zu ll ull llu
    // uが入ってれば符号なし、z, l, llは符号付
    bool is_signed = true;
    std::string_view suffix{end_pos, true_end};

    // 数値頭の+-記号は分けて読まれている
    if (0 <= sign) {
      // -付きのリテラルは想定しなくていい（数値リテラルの値は正であると仮定する）
      switch (size(suffix)) {
      case 0: goto convert_part;
      case 1:
        if (suffix == "u") {
          is_signed = false;
          goto convert_part;
        } else if (suffix == "l" or suffix == "z") {
          goto convert_part;
        }
        // エラー
        break;
      case 2:
        if (suffix == "ul" or suffix == "uz" or suffix == "lu" or suffix == "zu") {
          is_signed = false;
          goto convert_part;
        } else if (suffix == "ll") {
          goto convert_part;
        }
        // エラー
        break;
      case 3:
        if (suffix == "ull" or suffix == "llu") {
          is_signed = false;
          goto convert_part;
        }
        // エラー
        break;
      default:
        // エラー
        break;
      }
      
      // エラー報告
      // 10進数字列にabcdefが含まれいた場合もここに来る
      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
    }
    // 頭に-が付いてたら常に符号付整数

  convert_part:

    // from_charsに叩き込む
    if (is_signed) {
      std::intmax_t num;
      auto [ptr, ec] = std::from_chars(start_pos, end_pos, num, base);

      if (ec == std::errc::result_out_of_range) {
        // 指定された値が大きすぎる
        // 拡張整数型ですら表現できない場合コンパイルエラーとなる
        return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
      }
      if (ec == std::errc::invalid_argument) {
        // 入力がおかしい
        // 8進リテラルに8,9が出て来たときとか
        return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
      }

      // 他のエラーは想定されないはず・・・
      assert(ec == std::errc{});

      // 符号適用
      num *= sign;

      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<signed_integral>, num}, signed_integral);
    } else {
      std::uintmax_t num;
      auto [ptr, ec] = std::from_chars(start_pos, end_pos, num, base);

      if (ec == std::errc::result_out_of_range) {
        // 指定された値が大きすぎる
        // 拡張整数型ですら表現できない場合コンパイルエラーとなる
        return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
      }
      if (ec == std::errc::invalid_argument) {
        // 入力がおかしい
        // 8進リテラルに8,9が出て来たときとか
        return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<floating_point>, false}, floating_point);
      }

      // 他のエラーは想定されないはず・・・
      assert(ec == std::errc{});

      return std::make_pair(std::variant<std::intmax_t, std::uintmax_t, bool>{std::in_place_index<unsigned_integral>, num}, unsigned_integral);
    }
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