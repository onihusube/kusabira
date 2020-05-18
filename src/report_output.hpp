#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

#include "common.hpp"

//雑なWindows判定
#ifdef _MSC_VER
#define KUSABIRA_TARGET_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#include <fcntl.h>

#endif // _MSC_VER


namespace kusabira::PP {

  enum class pp_parse_context : std::int8_t
  {
    UnknownError = std::numeric_limits<std::int8_t>::min(),
    FailedRawStrLiteralRead,           //生文字列リテラルの読み取りに失敗した、バグの可能性が高い
    RawStrLiteralDelimiterOver16Chars, //生文字列リテラルデリミタの長さが16文字を超えた
    RawStrLiteralDelimiterInvalid,     //生文字列リテラルデリミタに現れてはいけない文字が現れた
    UnexpectedNewLine,                 //予期しない改行入力があった
    UnexpectedEOF,                     //予期しない改行入力があった
  
    GroupPart = 0,   // #の後で有効なトークンが現れなかった
    IfSection,       // #ifセクションの途中で読み取り終了してしまった？
    IfGroup_Mistake, // #ifから始まるifdef,ifndefではない間違ったトークン
    IfGroup_Invalid, // 1つの定数式・識別子の前後、改行までの間に不正なトークンが現れている
    ControlLine,
    Define_No_Identifier,       // #defineの後に識別子が現れなかった
    Define_Duplicate,           // 異なるマクロ再定義
    Define_Redfined,            // 異なる形式のマクロの再定義
    Define_Func_Disappointing_Token,
    ControlLine_Line_Num,       // #lineディレクティブの行数指定が符号なし整数値として読み取れない
    ControlLine_Line_ManyToken, // #lineディレクティブの後ろに不要なトークンが付いてる（警告）
  
    ElseGroup,         // 改行の前に不正なトークンが現れている
    EndifLine_Mistake, // #endifがくるべき所に別のものが来ている
    EndifLine_Invalid, // #endif ~ 改行までの間に不正なトークンが現れている
    TextLine           // 改行が現れる前にファイル終端に達した？バグっぽい
  };
}

namespace kusabira::report {

  namespace detail {

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

  } // namespace detail

  /**
  * @brief メッセージ出力時のカテゴリ指定
  */
  enum class report_category : std::uint8_t {
    error,
    warning
  };

  /**
  * @brief メッセージカテゴリを文字列へ変換する
  * @param cat メッセージ種別を示す列挙値
  */
  ifn report_category_to_u8string(report_category cat) -> std::string_view {
    using namespace std::string_view_literals;
    
    std::string_view str{};

    switch (cat)
    {
      case report_category::error :
        str = "error"sv;
        break;
      case report_category::warning :
        str = "warning"sv;
        break;
    }

    return str;
  }

  /**
  * @brief 翻訳フェーズ3~4くらいでのエラーをメッセージに変換するためのmap
  */
  using pp_message_map = std::unordered_map<PP::pp_parse_context, std::u8string_view>;


  /**
  * @brief エラー出力のデフォルト実装兼インターフェース
  * @tparam Destination 出力先
  */
  template<typename Destination = detail::stdoutput>
  struct ireporter {

    inline static const pp_message_map pp_err_message_en =
    {
      {PP::pp_parse_context::Define_No_Identifier, u8"#define directive is followed by no identifier to replace."},
      {PP::pp_parse_context::Define_Duplicate, u8"A macro with the same name has been redefined with a different definition."},
      {PP::pp_parse_context::Define_Redfined, u8"A different type of macro has been redefined."},
      {PP::pp_parse_context::ControlLine_Line_Num , u8"The number specified for the #LINE directive is incorrect. Please specify a number in the range of std::size_t."},
      {PP::pp_parse_context::ControlLine_Line_ManyToken, u8"There is an unnecessary token after the #line directive."}
    };

//Windowsのみ、コンソール出力のために少し調整を行う
//コンソールのコードページを変更し、UTF-8を直接出力する
#ifdef KUSABIRA_TARGET_WIN
  private:
    const std::uint32_t m_cp{};

  public:

    ireporter()
      //コードページを保存
      : m_cp(::GetConsoleCP())
    {
      //コードページをUTF-8に変更
      ::SetConsoleOutputCP(65001u);
      // 標準出力をバイナリモードにする
      ::_setmode(_fileno(stdout), _O_BINARY);
    }
    
    virtual ~ireporter() {
      //コードページを元に戻しておく
      ::SetConsoleOutputCP(m_cp);
    }

#else

    virtual ~ireporter() = default;

#endif // KUSABIRA_TARGET_WIN

  protected:

    /**
    * @brief プリプロセス時エラーのコンテキストからエラーメッセージへ変換する
    * @param context エラー発生地点を示す列挙値
    * @details これをオーバライドすることで、多言語対応する（やるかはともかく・・・
    */
    [[nodiscard]]
    virtual auto pp_context_to_message_impl(PP::pp_parse_context context) const -> std::u8string_view {
      using std::end;

      auto it = pp_err_message_en.find(context);

      //少なくとも英語メッセージは全て用意しておくこと！
      assert(it != end(pp_err_message_en));

      return (*it).second;
    }

    [[nodiscard]]
    auto pp_context_to_message(PP::pp_parse_context context) const -> std::u8string_view {
      auto message = this->pp_context_to_message_impl(context);
      if (message.empty()) {
        //英語メッセージにフォールバック
        message = ireporter::pp_err_message_en.at(context);
      }
      return message;
    }

  public:

    /**
    * @brief 指定文字列を直接出力する
    * @param message 本文
    * @param filename ソースファイル名
    * @param context 出力のきっかけとなった場所を示す字句トークン
    */
    void print_report(std::u8string_view message, const fs::path& filename, const PP::lex_token& context, report_category cat = report_category::error) const {
      //<file名>:<行>:<列>: <メッセージのカテゴリ>: <本文>
      //の形式で出力

      //ファイル名を出力
      Destination::output_u8string(filename.filename().u8string());

      //物理行番号、列番号
      const auto [row, col] = context.get_phline_pos();
      //カテゴリ文字列
      const auto category = report_category_to_u8string(cat);

      //ソースコード上位置、カテゴリを出力
      Destination::output(":", row, ":", col, ": ", category, ": ");

      //本文出力
      Destination::output_u8string(message);
      Destination::endl();
    }

    /**
    * @brief プリプロセス時エラーメッセージを出力する
    * @param filename ソースファイル名
    * @param token エラー発生時の字句トークン
    * @param context エラー発生箇所の文脈
    */
    void pp_err_report(const fs::path &filename, PP::lex_token token, PP::pp_parse_context context, report_category cat = report_category::error) const {

      //エラーメッセージ本文出力
      this->print_report(this->pp_context_to_message(context), filename, token, cat);

      //ソースライン出力（物理行を出力するのが多分正しい？
      Destination::output_u8string(token.get_line_string());
      Destination::endl();

      //位置のマーキングとかできるといいなあ・・・
    }
  };

  /**
  * @brief 日本語エラーメッセージ出力器
  * @tparam Destination 出力先
  */
  template<typename Destination = detail::stdoutput>
  struct reporter_ja final : public ireporter<Destination> {

    inline static const pp_message_map pp_err_message_ja =
    {
      {PP::pp_parse_context::Define_No_Identifier, u8"#defineディレクティブの後に置換対象の識別子がありません。"},
      {PP::pp_parse_context::Define_Duplicate, u8"同じ名前のマクロが異なる定義で再定義されました。"},
      {PP::pp_parse_context::Define_Redfined, u8"同じ名前で異なる形式のマクロが再定義されました。"},
      {PP::pp_parse_context::ControlLine_Line_Num , u8"#lineディレクティブに指定された数値が不正です。std::size_tの範囲内の数値を指定してください。"},
      {PP::pp_parse_context::ControlLine_Line_ManyToken , u8"#lineディレクティブの後に不要なトークンがあります。"}
    };

    fn pp_context_to_message_impl(PP::pp_parse_context context) const -> std::u8string_view override {
      using std::end;

      if (auto it = pp_err_message_ja.find(context); it != end(pp_err_message_ja)) {
        return (*it).second;
      } else {
        return {};
      }
    }
  };


  /**
  * @brief 出力言語を指定する列挙型
  * @details RFC4646、ISO 639-1に従った命名をする
  */
  enum class report_lang : std::uint8_t {
    en_us,
    ja
  };

  /**
  * @brief メッセージ出力型の実装を取得するためのFactory
  * @tparam Destination 出力先を指定するTraits型
  */
  template<typename Destination = detail::stdoutput>
  struct reporter_factory {

    /**
    * @brief このファクトリによって取得できるメッセージ出力器の型
    */
    using reporter_t = std::unique_ptr<ireporter<Destination>>;

    /**
    * @brief メッセージ出力型の実装オブジェクトを取得する
    * @param lang 出力言語指定
    * @return ireporterインターフェースのポインタ (unique_ptr)
    */
    sfn create(report_lang lang = report_lang::ja) -> reporter_t {
      switch (lang)
      {
      case kusabira::report::report_lang::ja:
        return std::make_unique<reporter_ja<Destination>>();
      case kusabira::report::report_lang::en_us: [[fallthrough]];
      default:
        return std::make_unique<ireporter<Destination>>();
      } 
    }
  };

} // namespace kusabira::report

#undef KUSABIRA_TARGET_WIN