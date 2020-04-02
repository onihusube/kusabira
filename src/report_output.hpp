#pragma once

#include <iostream>
#include <unordered_map>

#include "common.hpp"

//雑なWindows判定
#ifdef _MSC_VER
#define KUSABIRA_TARGET_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//テストビルドでない場合に追加ヘッダのインクルード
#ifdef KUSABIRA_CL_BUILD
#include <io.h>
#include <fcntl.h>
#endif // KUSABIRA_CL_BUILD

#endif // _MSC_VER


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
    ControlLine_Line_Num,   // #lineディレクティブの行数指定が符号なし整数値として読み取れない

    ElseGroup,          // 改行の前に不正なトークンが現れている
    EndifLine_Mistake,  // #endifがくるべき所に別のものが来ている
    EndifLine_Invalid,  // #endif ~ 改行までの間に不正なトークンが現れている
    TextLine            // 改行が現れる前にファイル終端に達した？バグっぽい
  };
}

namespace kusabira::report {

  namespace detail {

#ifdef KUSABIRA_TARGET_WIN

    /**
    * @brief 標準出力への出力
    */
    struct stdoutput {

      static void output_u8string(const std::u8string_view str) {
        auto punned_str = reinterpret_cast<const char*>(str.data());

        //変換後の長さを取得
        const std::size_t length = ::MultiByteToWideChar(CP_UTF8, 0, punned_str, static_cast<int>(str.length()), nullptr, 0);
        std::wstring converted(length, L'\0');
        //変換に成功したら出力
        auto res = ::MultiByteToWideChar(CP_UTF8, 0, punned_str, static_cast<int>(str.length()), converted.data(), static_cast<int>(length));
        if (res != 0) {
          std::wcout << converted;

#ifndef KUSABIRA_CL_BUILD
          //UTF-16 -> Ansi(Shift-JIS)にマップできない文字があった時に復帰させる
          if (std::wcout.fail()) std::wcout.clear();
#endif // !KUSABIRA_CL_BUILD

        } else {
          std::wcerr << L"UTF-8文字列の変換に失敗しました。" << std::endl;
          assert(false);
        }
      }

      template<typename... Args>
      static void output(Args&&... args) {
        (std::wcout << ... << std::forward<Args>(args));
      }

      static void endl() {
        std::wcout << std::endl;
      }

    };

#else

    /**
    * @brief 標準出力への出力
    */
    struct stdoutput {

      static void output_u8string(const std::u8string_view str) {
        auto punned_str = reinterpret_cast<const char*>(str.data());
        std::cout << punned_str;
      }

      template<typename... Args>
      static void output(Args&&... args) {
        (std::cout << ... << std::forward<Args>(args));
      }

      static void endl() {
        std::cout << std::endl;
      }

    };

#endif // KUSABIRA_TARGET_WIN

  } // namespace detail

  /**
  * @brief 翻訳フェーズ3~4くらいでのエラーをメッセージに変換するためのmap
  */
  using pp_message_map = std::unordered_map<pp_parse_context, std::u8string_view>;

  /**
  * @brief 上記pp_message_mapの値型
  */
  using pp_message_map_vt = std::unordered_map<pp_parse_context, std::u8string_view>::value_type;


  /**
  * @brief エラー出力のデフォルト実装兼インターフェース
  * @tparam Destination 出力先
  */
  template<typename Destination = detail::stdoutput>
  struct ireporter {

    inline static const pp_message_map pp_err_message_en =
    {
      {pp_parse_context::ControlLine_Line_Num , u8"The number specified for the #LINE directive is incorrect. Please specify a number in the range of std::size_t."}
    };

//Windowsのみ、コンソール出力のために少し調整を行う
#ifdef KUSABIRA_TARGET_WIN
#ifdef KUSABIRA_CL_BUILD
    ireporter() {
      //ストリーム出力をユニコードモードに変更する
      _setmode(_fileno(stdout), _O_U16TEXT);
    }
#else
    ireporter() {
      //ロケール及びコードページをシステムデフォルトに変更
      std::wcout.imbue(std::locale(""));
    }
#endif // KUSABIRA_CL_BUILD
#endif // KUSABIRA_TARGET_WIN

    virtual ~ireporter() = default;

  protected:

    void print_src_pos(const fs::path& filename, const PP::lex_token& context) {
      // "<file名>:<行>:<列>: <メッセージのカテゴリ>: "を出力

      //ファイル名を出力
      Destination::output_u8string(filename.filename().u8string());

      //物理行番号、列番号
      const auto [row, col] = context.get_phline_pos();

      //ソースコード上位置、カテゴリを出力
      Destination::output(":", row, ":", col, ": error: ");
    }


  public:
    /**
    * @brief 指定文字列を直接出力する
    * @param message 本文
    * @param filename ソースファイル名
    * @param context 出力のきっかけとなった場所を示す字句トークン
    */
    void print_report(std::u8string_view message, const fs::path& filename, const PP::lex_token& context) {
      //<file名>:<行>:<列>: <メッセージのカテゴリ>: <本文>
      //の形式で出力

      this->print_src_pos(filename, context);

      //本文出力
      Destination::output_u8string(message);
      Destination::endl();
    }

    virtual void pp_err_report(const fs::path &filename, PP::pp_token token, PP::pp_parse_context context) {
      //事前条件
      assert(token.lextokens.empty() == false);

      auto lextoken = token.lextokens.front();

      //エラー位置等の出力
      this->print_src_pos(filename, lextoken);

      //本文出力
      Destination::output_u8string(pp_err_message_en[context]);
      Destination::endl();

      //ソースライン出力（物理行を出力するのが多分正しい？
      Destination::output_u8string(lextoken.get_line_string());
      Destination::endl();

      //位置のマーキングとかできるといいなあ・・・
    }
  };

  template<typename Destination = detail::stdoutput>
  struct reporter_ja final : public ireporter<Destination> {

    inline static const pp_message_map pp_err_message_ja =
    {
      {pp_parse_context::ControlLine_Line_Num , u8"#lineディレクティブに指定された数値が不正です。std::size_tの範囲内の数値を指定してください。"}
    };

    void pp_err_report([[maybe_unused]] const fs::path &filename, [[maybe_unused]] PP::pp_token token, [[maybe_unused]] PP::pp_parse_context context) override
    {
    }
  };



  /**
  * @brief 出力言語を指定する列挙型
  * @detail RFC4646、ISO 639-1に従った命名をする
  */
  enum class report_lang : std::uint8_t {
    en_us,
    ja
  };

  /**
  * @brief メッセージ出力型のオブジェクトを取得する
  * @tparam Destination 出力先を指定する型
  * @param lang 出力言語指定
  * @return ireporterインターフェースのポインタ (unique_ptr)
  */
  template<typename Destination = detail::stdoutput>
  ifn get_reporter(report_lang lang = report_lang::ja) -> std::unique_ptr<ireporter<Destination>> {

    switch (lang)
    {
    case kusabira::report::report_lang::ja:
      return std::make_unique<reporter_ja<Destination>>();
    case kusabira::report::report_lang::en_us: [[fallthrough]];
    default:
      return std::make_unique<ireporter<Destination>>();
    } 
  }

} // namespace kusabira::report

#undef KUSABIRA_TARGET_WIN