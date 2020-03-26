#pragma once

#include "common.hpp"

namespace kusabira::PP {

  enum class pp_parse_context : std::int8_t {
    UnknownError = std::numeric_limits<std::int8_t>::min(),
    FailedRawStrLiteralRead,            //生文字列リテラルの読み取りに失敗した、バグの可能性が高い
    RawStrLiteralDelimiterOver16Chars,  //生文字列リテラルデリミタの長さが16文字を超えた
    RawStrLiteralDelimiterInvalid,      //生文字列リテラルデリミタに現れてはいけない文字が現れた
    UnexpectedNewLine,                  //予期しない改行入力があった

    GroupPart = 0,      // #の後で有効なトークンが現れなかった
    IfSection,          // #ifセクションの途中で読み取り終了してしまった？
    IfGroup_Mistake,    // #ifから始まるifdef,ifndefではない間違ったトークン
    IfGroup_Invalid,    // 1つの定数式・識別子の前後、改行までの間に不正なトークンが現れている
    ControlLine,

    ElseGroup,          // 改行の前に不正なトークンが現れている
    EndifLine_Mistake,  // #endifがくるべき所に別のものが来ている
    EndifLine_Invalid,  // #endif ~ 改行までの間に不正なトークンが現れている
    TextLine            // 改行が現れる前にファイル終端に達した？バグっぽい
  };
}

namespace kusabira::report
{

  enum class report_type : std::uint8_t {
    error,
    warning
  };

  namespace detail {

    struct stdout {

      static auto output(std::u8string_view str) -> std::basic_ostream<char>& {
        auto punned_str = reinterpret_cast<const char*>(str.data());
        return (std::cout << punned_str);
      }

      static auto output_err(std::u8string_view str) -> std::basic_ostream<char>& {
        auto punned_str = reinterpret_cast<const char*>(str.data());
        return (std::cerr << punned_str);
      }
    };

  }

  template<typename Destination = detail::stdout>
  struct ireporter {

    void print_report(std::u8string_view message, const fs::path& filename, const PP::lex_token& context) {
      //<file名>:<行>:<列>: <メッセージのカテゴリ>: <本文>
      //の形式で出力

      //ファイル名を出力
      //detail::print_u8string(filename.filename().u8string()) << ":";
      Destination::output_err(filename.filename().u8string()) << ":";

      //物理行番号、列番号を出力


    }

    virtual void pp_err_report(const fs::path &filename, PP::lex_token token, PP::pp_parse_context context) {
    }
  };

  template<typename Destination = detail::stdout>
  struct reporter_ja : public ireporter<Destination> {

    void pp_err_report(const fs::path &filename, PP::lex_token token, PP::pp_parse_context context) override {
    }
  };

} // namespace kusabira::report
