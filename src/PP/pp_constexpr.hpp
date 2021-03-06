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

  fn decode_integral_ppnumber(std::u8string_view input_str) -> integer_result {
    // 事前条件
    //assert(sign == -1 or sign == 1);

    //using enum pp_parse_context;


    constexpr std::uint8_t signed_integral = 0;
    constexpr std::uint8_t unsigned_integral = 1;
    constexpr std::uint8_t parse_err = 2;
    // 想定される最大の桁数（2進リテラルの桁数）+ マイナス符号変換用の先頭部分 + 想定されるリテラルサフィックスの最大長（ullの3文字）
    constexpr auto max_digit = std::numeric_limits<std::uintmax_t>::digits + 3;

    // 前処理用バッファ
    char numstr[max_digit]{};
    // 数字文字列の長さ
    std::uint16_t num_length = 0;

    constexpr auto npos = std::u8string_view::npos;
    // lLとかLlみたいなサフィックスをはじく（アルファベット小文字化の前にチェックする必要がある）
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
    std::string_view numstr_view{ numstr,  numstr_end };

    // 浮動小数点数判定
    // 16進浮動小数点数には絶対にpが表れる
    if (auto p = std::ranges::find_if(numstr_view, [](char c) { return c == '.' or c == 'p'; }); p != numstr_view.end()) {
      return integer_result{ std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_FloatingPointNumber };
    }

    // まだ1E-1とか2e2みたいな指数表記浮動小数点数が含まれてる可能性がある

    // 基数
    std::uint8_t base = 10;
    // 数字本体の開始位置
    const char* start_pos = numstr;

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

    // リテラル判定、ユーザー定義リテラルはエラー
    // u U
    // l L
    // ll LL
    // z Z
    // 大文字は考慮しなくてよく、u以外は他とくっつかない
    // 候補は  u l z ul uz lu zu ll ull llu
    // uが入ってれば符号なし、z, l, llは符号付
    bool is_signed = true;
    bool has_signed_suffix = false;

    // 数値頭の+-記号は分けて読まれている、その適用は後から
    // -付きのリテラルは想定しなくていい（数値リテラルの値は正であると仮定する）

    std::string_view suffix{ end_pos, numstr_end };

    switch (size(suffix)) {
    case 0: goto convert_part;
    case 1:
      if (suffix == "u") {
        is_signed = false;
        goto convert_part;
      } else if (suffix == "l" or suffix == "z") {
        has_signed_suffix = true;
        goto convert_part;
      }
      // エラー
      break;
    case 2:
      if (suffix == "ul" or suffix == "uz" or suffix == "lu" or suffix == "zu") {
        is_signed = false;
        goto convert_part;
      } else if (suffix == "ll") {
        has_signed_suffix = true;
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

  convert_part:

    // from_charsに叩き込む
    if (is_signed) {
      std::intmax_t num;
      const auto [_, ec] = std::from_chars(start_pos, end_pos, num, base);

      if (ec == std::errc::result_out_of_range) {
        // 指定された値が大きすぎる

        if (has_signed_suffix) {
          // 拡張整数型ですら表現できない場合コンパイルエラーとなる
          return integer_result{std::in_place_index<parse_err>, pp_parse_context::PPConstexpr_OutOfRange};
        }

        // 符号付きサフィックスがなく数値が正であれば、符号なし整数型として変換を試みる
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


  ifn character_literal_to_integral(std::u8string_view input_str) -> std::pair<int, pp_parse_context> {
    // 事前条件
    assert(not empty(input_str));
    assert(input_str.back() == u8'\'');

    // 'の次の位置
    const auto start_pos = input_str.find_first_of(u8'\'') + 1;
    // 閉じの'は含めない
    auto char_str = input_str.substr(start_pos, input_str.length() - start_pos - 1);
    // encoding-prefixはソース文字集合から指定した実行時文字集合へ変換する指定なので無視でいい（？
    //auto prefix_str = input_str.substr(0, start_pos - 1);

    if (empty(char_str)) {
      // 空の文字リテラル、エラー
      return { -1, pp_parse_context::PPConstexpr_EmptyCharacter };
    }
    
    if (size(char_str) == 1) {
      // 単純な文字リテラル
      // UTF-8エンコーディングであり、その値を整数値として読み取り
      return { char_str.front(), {} };
    }

    if (not char_str.find('\\')) {
      // エスケープシーケンスのないマルチキャラクタリテラル
      return { char_str.back(), pp_parse_context::PPConstexpr_MultiCharacter };
    }

    // ここでは、エスケープシーケンスを含んだマルチキャラクタリテラル、か1つのエスケープシーケンス、かのどちらか
    // 一番最後に現れるバックスラッシュを探索し、その位置からエスケープシーケンスの読み取りを試みる
    // 1. 読み終わり、文字の残りもなければそのエスケープシーケンスをデコードして終わり
    // 2. 途中でエスケープシーケンスが終わりなお文字が残っていたら（それは1文字のはず）、その1文字をデコードする
    // 1の場合にエスケープシーケンスの位置が先頭でないならマルチキャラクタリテラル
    // 2の場合は常にマルチキャラクタリテラル

  }
}

namespace kusabira::PP {

  using pp_constexpr_result = kusabira::expected<std::variant<std::intmax_t, std::uintmax_t>, bool>;
  //using pp_constexpr_result = kusabira::expected<std::intmax_t, bool>;

  /**
  * @brief プリプロセッシングディレクティブの実行に必要な程度の定数式を処理する
  * @details C++の意味論で解釈される構文は考慮しなくて良い
  * @details トークンの消費責任は常に呼び出し側にある
  * @details 内部で再帰的にパース処理を呼び出すときは消費してから呼び出す
  * @details 一方、処理を終えて戻るときは消費しきらずに戻る
  */
  template<typename Reporter>
  struct pp_constexpr {

    const Reporter& reporter;
    const fs::path& filename;

    void err_report(const PP::pp_token& token, pp_parse_context context) const {
      this->reporter.pp_err_report(this->filename, token, context);
    }

    template<std::bidirectional_iterator I>
    auto reach_end(I it) const {
      --it;
      this->err_report(*it, pp_parse_context::PPConstexpr_Invalid);
      return kusabira::error(false);
    }

    template<std::ranges::bidirectional_range Tokens>
      requires std::same_as<std::iter_value_t<std::ranges::iterator_t<Tokens>>, pp_token>
    fn operator()(const Tokens& token_list) const -> std::optional<bool> {
      // 事前条件、空でないこと
      assert(not empty(token_list));

      auto it = std::ranges::begin(token_list);
      auto last = std::ranges::end(token_list);

      auto succeed = this->conditional_expression(it, last);

      // エラー報告は済んでいるものとする
      if (not succeed) return std::nullopt;

      // 全部消費してるよね？
      ++it;
      assert(it == last);

      // 0 == false, それ以外 == true
      return std::visit([](std::integral auto n) -> bool { return n != 0; }, *std::move(succeed));
    }

    template<std::bidirectional_iterator I, std::sentinel_for<I> S>
    fn conditional_expression(I& it, S last) const -> pp_constexpr_result {
      return this->primary_expressions(it, last);
    }

    
    
    template<std::bidirectional_iterator I, std::sentinel_for<I> S>
    fn primary_expressions(I& it, S last) const -> pp_constexpr_result {
      // 考慮すべきはliteralと(conditional-expression)の2種類

      if (it == last) {
        return this->reach_end(it);
      }

      const auto token_cat = deref(it).category;

      // 整数型と文字型を考慮
      if (token_cat == pp_token_category::pp_number) {
        // pp_numberの読み取り
        // 整数値のみがwell-formed、浮動小数点数値はエラー
        auto val = decode_integral_ppnumber(deref(it).token);

        return std::visit<pp_constexpr_result>(overloaded{
                                                   []<std::integral Int>(Int n) {
                                                     return kusabira::ok(std::variant<std::intmax_t, std::uintmax_t>{std::in_place_type<Int>, n});
                                                   },
                                                   [this, &it](pp_parse_context err_context) {
                                                     this->err_report(*it, err_context);
                                                     return kusabira::error(false);
                                                   }},
                                               std::move(val));
      } else if (token_cat == pp_token_category::charcter_literal) {
        auto token_str = deref(it).token.to_view();

        if (auto len = size(token_str); len < 3) {
          // 空の文字リテラル、エラー
          this->err_report(*it, pp_parse_context::PPConstexpr_EmptyCharacter);
          return kusabira::error(false);
        } else if (3 < len) {
          // マルチキャラクタリテラルに関する警告を表示、エラーにはしない?
          this->reporter.pp_err_report(this->filename, *it, pp_parse_context::PPConstexpr_MultiCharacter, report::report_category::warning);
        }

        // マルチキャラクタリテラル対応のために、最後の文字を取り出す
        // ToDo:エスケープシーケンス考慮が必要
        char8_t c = *std::ranges::prev(token_str.end(), 2);

        return kusabira::ok(std::variant<std::intmax_t, std::uintmax_t>{std::in_place_index<0>, static_cast<int>(c)});
      }

      // ()に囲まれた式
      else if (token_cat == pp_token_category::op_or_punc and deref(it).token == u8"(") {
        
        // "("を消費してから、中身を読みに行く
        ++it;
        auto succeed = this->conditional_expression(it, last);

        if (not succeed) return kusabira::error(false);

        ++it;

        // 対応する閉じ括弧が出現していない
        if (it == last or deref(it).token != u8")") {
          if (it == last) --it;

          this->err_report(*it, pp_parse_context::PPConstexpr_MissingCloseParent);
          return kusabira::error(false);
        }

        return kusabira::ok(std::move(succeed));
      }

      // それ以外の出現はエラー
      this->err_report(*it, pp_parse_context::PPConstexpr_Invalid);
      return kusabira::error(false);
    }
  };

  template<typename R, typename P>
  pp_constexpr(R&, P&&) -> pp_constexpr<R>;
}