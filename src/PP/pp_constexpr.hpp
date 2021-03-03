#pragma once

#include <ranges>
#include <optional>
#include <functional>
#include <utility>
#include <variant>
#include <cctype>

#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira::PP::inline free_func{

  using integer_result = std::variant<std::intmax_t, std::uintmax_t, pp_parse_context>;

  fn decode_integral_ppnumber(std::u8string_view input_str, int sign) -> integer_result {
    // 事前条件
    assert(sign == -1 or sign == 1);

    //using enum pp_parse_context;


    constexpr std::uint8_t signed_integral = 0;
    constexpr std::uint8_t unsigned_integral = 1;
    constexpr std::uint8_t parse_err = 2;
    // 想定される最大の桁数（2進リテラルの桁数）+ マイナス符号変換用の先頭部分 + 想定されるリテラルサフィックスの最大長（ullの3文字）
    constexpr auto max_digit = std::numeric_limits<std::uintmax_t>::digits + 1 + 3;

    // 前処理用バッファ
    char numstr[max_digit]{};
    // 数字文字列の長さ（あとから必要に応じて頭に-を入れるために先頭を予約しておく）
    std::uint16_t num_length = 1;

    constexpr auto npos = std::u8string_view::npos;
    // lLとかLlみたいなサフィックスをはじく
    if (input_str.rfind(u8"lL") != npos or input_str.rfind(u8"Ll") != npos) {
      return integer_result{std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_UDL};
    }

    // 桁区切り'を取り除くとともに、大文字を小文字化しておく
    // char8_t(UTF-8)をchar(Ascii)として処理する
    for (char c : input_str | std::views::filter([](char8_t ch) { return ch != u8'\''; })
                            | std::views::transform([](unsigned char ch) { return char(std::tolower(ch)); }))
    {
      numstr[num_length] = c;
      ++num_length;
    }

    // これが起きたらUDLが長いとしてエラーにする（ToDo
    assert(num_length <= max_digit);

    // 前処理後数字列の終端
    char const* const numstr_end = numstr + num_length;
    // 参照しておく
    std::string_view numstr_view{ numstr + 1,  numstr_end };

    // 浮動小数点数判定
    // 16進浮動小数点数には絶対にpが表れる
    if (auto p = std::ranges::find_if(numstr_view, [](char c) { return c == '.' or c == 'p'; }); p != numstr_view.end()) {
      return integer_result{ std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_FloatingPointNumber };
    }

    // まだ1E-1とか2e2みたいな指数表記浮動小数点数が含まれてる可能性がある

    // 基数
    std::uint8_t base = 10;
    // 数字本体の開始位置
    char* start_pos = numstr + 1;

    // 2 or 8 or 10 or 16進数判定
    if (numstr_view.starts_with("0b")) {
      base = 2;
      start_pos += 2;
    } else if (numstr_view.starts_with("0x")) {
      base = 16;
      start_pos += 2;
    } else if (numstr_view.starts_with("0")) {
      base = 8;
      //start_pos += 1;
    } else {
      // 10進リテラルの時

      // 小数点の現れないタイプの10進浮動小数点数
      // 1E-1とか2e+2とか3E-4flみたいな、絶対にeが表れる
      // 16進浮動小数点数リテラルではこれはpになる、それ以外の基数の浮動小数点数リテラルはない
      // 整数リテラルとしても浮動小数点数リテラルとしてもill-formedなものはトークナイザではじかれるか、ユーザー定義リテラルとして扱われエラーになる（はず
      if (auto e = std::ranges::find_if(numstr_view, [](char c) { return c == 'e'; }); e != numstr_view.end()) {
        return integer_result{std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_FloatingPointNumber};
      }
    }

    // 以降、入力は整数値を表しているはず！

    // リテラル部を除いた数値の終端を求める
    const char* end_pos = numstr_end;

    if (base == 16) {
      end_pos = std::ranges::find_if_not(start_pos, numstr_end, [](unsigned char ch) -> bool { return std::isxdigit(ch); });
    } else {
      end_pos = std::ranges::find_if_not(start_pos, numstr_end, [](unsigned char ch) -> bool { return std::isdigit(ch); });
    }

    // 入力が負なら-符号を入れておく
    if (sign < 0) {
      --start_pos;
      *start_pos = '-';
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
    
    // 数値頭の+-記号は分けて読まれている
    // 頭に-が付いてたら常に符号付整数
    // -符号がついてるのに符号なしが指定されたらエラー？
    if (0 <= sign) {
      // -付きのリテラルは想定しなくていい（数値リテラルの値は正であると仮定する）
      
      std::string_view suffix{ end_pos, numstr_end };

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
      return integer_result{std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_UDL};
    }

  convert_part:

    // from_charsに叩き込む
    if (is_signed) {
      std::intmax_t num;
      const auto [_, ec] = std::from_chars(start_pos, end_pos, num, base);

      if (ec == std::errc::result_out_of_range) {
        // 指定された値が大きすぎる

        if (sign < 0) {
          // 拡張整数型ですら表現できない場合コンパイルエラーとなる
          return integer_result{std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_OutOfRange};
        }

        // 数値が正であれば、符号なし整数型として変換を試みる
        is_signed = false;
        goto convert_part;
      }
      if (ec == std::errc::invalid_argument) {
        // 入力がおかしい
        // 8進リテラルに8,9が出て来たときとか
        return integer_result{std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_InvalidArgument};
      }

      // 他のエラーは想定されないはず・・・
      assert(ec == std::errc{});

      // 符号適用
      //num *= sign;

      return integer_result{ std::in_place_index<signed_integral>, num };
    } else {
      std::uintmax_t num;
      const auto [_, ec] = std::from_chars(start_pos, end_pos, num, base);

      if (ec == std::errc::result_out_of_range) {
        // 指定された値が大きすぎる
        // 拡張整数型ですら表現できない場合コンパイルエラーとなる
        return integer_result{ std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_OutOfRange };
      }
      if (ec == std::errc::invalid_argument) {
        // 入力がおかしい
        // 8進リテラルに8,9が出て来たときとか
        return integer_result{std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_InvalidArgument};
      }

      // 他のエラーは想定されないはず・・・
      assert(ec == std::errc{});

      return integer_result{ std::in_place_index<unsigned_integral>, num };
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