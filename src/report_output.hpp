#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <bit>

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
    UnexpectedEOF,                     //予期しないファイル終端への到達

    GroupPart = 0,   // #の後で有効なトークンが現れなかった
    IfSection,       // #ifセクションの途中で読み取り終了してしまった？
    IfGroup_Mistake, // #ifから始まるifdef,ifndefではない間違ったトークン
    IfGroup_Invalid, // 1つの定数式・識別子の前後、改行までの間に不正なトークンが現れている
    PPConstexpr_Invalid, // #ifの条件式に不正なトークンがある
    PPConstexpr_MissingCloseParent, // #ifの条件式に不正なトークンがある
    PPConstexpr_FloatingPointNumber, // #ifの条件式で浮動小数点数がある
    PPConstexpr_UDL,                  // #ifの条件式でユーザー定義リテラルがある
    PPConstexpr_OutOfRange,           // #ifの条件式の整数定数が表現しきれないほど大きい
    PPConstexpr_InvalidArgument,      // #ifの条件式の整数リテラルの書式がおかしい（8進リテラルに89があるとか）
    PPConstexpr_EmptyCharacter,      // #ifの条件式の文字リテラルが空
    PPConstexpr_MultiCharacter,      // #ifの条件式のマルチキャラクタリテラルを警告

    ControlLine,
    Define_No_Identifier,       // #defineの後に識別子が現れなかった
    Define_ParamlistBreak,      // 関数マクロの仮引数リストパース中に改行した
    Define_Duplicate,           // 異なるマクロ再定義
    Define_Redfined,            // 異なる形式のマクロの再定義
    Define_Sharp2BothEnd,       // ##トークンが置換リストの両端に現われている
    Define_InvalidSharp,        // #トークンが仮引数の前に現れなかった
    Define_InvalidVAARGS,       // 可変長マクロではないのに__VA_ARGS__が参照された
    Define_VAOPTRecursive,      // __VA_OPT__が再帰している
    Define_InvalidTokenConcat,  // 不正なプリプロセッシングトークンの連結が行われた
    Define_InvalidDirective,    // #defineディレクティブが正しくない
    Define_MissingComma,        // #defineディレクティブ（関数マクロ）の仮引数列中のカンマの位置に不正なトークンがいる
    Funcmacro_NotInvoke,        // 関数マクロ名が参照されているが、呼び出しではなかった（エラーじゃない）
    Funcmacro_InsufficientArgs, // 関数マクロ呼び出しの際、引数が足りなかった
    Funcmacro_ReplacementFail,  // 関数マクロの呼び出し中、引数に対するマクロ置換が失敗した
    ControlLine_Undef,          // #undefにマクロ名が指定されていない
    ControlLine_Line_Num,       // #lineディレクティブの行数指定が符号なし整数値として読み取れない
    ControlLine_Line_ManyToken, // #lineディレクティブの後ろに不要なトークンが付いてる（警告）
    ControlLine_Error,          // #errorディレクティブによる終了
    ControlLine_Pragma,         // #pragmaディレクティブ中のエラー

    EndifLine_Mistake,  // #endifがくるべき所に別のものが来ている
    EndifLine_Invalid,  // #endif ~ 改行までの間に不正なトークンが現れている
    TextLine,           // 改行が現れる前にファイル終端に達した？バグっぽい
    Newline_NotAppear   // 改行の前に不正なトークンが現れている
  };
}

namespace kusabira::report {

  namespace detail {

    /**
    * @brief 標準出力への出力
    */
    struct stdoutput {

      static void output_u8string(const std::u8string_view str) {

        for (auto u8char : str) {
          #ifdef __cpp_lib_bit_cast
          auto c = std::bit_cast<char>(u8char);
          #else
          auto c = *reinterpret_cast<char *>(&u8char);
          #endif
          std::cout << c;
        }
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
            {PP::pp_parse_context::UnexpectedEOF, u8"Unexpected EOF."},
            {PP::pp_parse_context::Define_No_Identifier, u8"Can't find the macro name in the #define directive."},
            {PP::pp_parse_context::Define_ParamlistBreak, u8"Line breaks are not allowed in the parameter list of function macro definition."},
            {PP::pp_parse_context::Define_Duplicate, u8"A macro with the same name has been redefined with a different definition."},
            {PP::pp_parse_context::Define_Redfined, u8"A different type of macro has been redefined."},
            {PP::pp_parse_context::Define_Sharp2BothEnd, u8"The ## token must appear inside the replacement list."},
            {PP::pp_parse_context::Define_InvalidSharp, u8"The # token must appear only before the name of parameter."},
            {PP::pp_parse_context::Define_InvalidVAARGS, u8"__VA_ARGS__ is referenced even though it is not a variadic arguments macro."},
            {PP::pp_parse_context::Define_VAOPTRecursive, u8"__VA_OPT__ must not be recursive."},
            {PP::pp_parse_context::Define_InvalidTokenConcat, u8"Concatenation result by ## is not a valid preprocessing token."},
            {PP::pp_parse_context::Define_InvalidDirective, u8"Unexpected characters appear in #define directive."},
            {PP::pp_parse_context::Define_MissingComma, u8"In the parameter sequence of the function macro, other tokens are appearing where commas should appear."},
            {PP::pp_parse_context::Funcmacro_InsufficientArgs, u8"The number of arguments of the function macro call does not match."},
            {PP::pp_parse_context::Funcmacro_ReplacementFail, u8"Macro expansion failed to replace the macro contained in the argument."},
            {PP::pp_parse_context::ControlLine_Undef, u8"Specify the macro name."},
            {PP::pp_parse_context::ControlLine_Line_Num, u8"The number specified for the #LINE directive is incorrect. Please specify a number in the range of std::size_t."},
            {PP::pp_parse_context::ControlLine_Line_ManyToken, u8"There is an unnecessary token after the #line directive."},
            {PP::pp_parse_context::Newline_NotAppear, u8"An unexpected token appears before a line break."},
            {PP::pp_parse_context::PPConstexpr_MissingCloseParent, u8"Could not find the corresponding closing parenthesis ')'."},
            {PP::pp_parse_context::PPConstexpr_Invalid, u8"This token cannot be processed by a constant expression during preprocessing."},
            {PP::pp_parse_context::PPConstexpr_EmptyCharacter, u8"It is an empty character constant."},
            {PP::pp_parse_context::PPConstexpr_MultiCharacter, u8"Multicharacter literal use only the last character."}};

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
    * @brief 指定文字列を周辺情報と一緒に出力する
    * @param message 本文
    * @param filename ソースファイル名
    * @param context 出力のきっかけとなった場所を示す字句トークン
    */
    void print_report(std::u8string_view message, const fs::path& filename, const PP::pp_token& context, report_category cat = report_category::error) const {
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
    void pp_err_report(const fs::path &filename, const PP::pp_token& token, PP::pp_parse_context context, report_category cat = report_category::error) const {

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
      {PP::pp_parse_context::UnexpectedEOF, u8"予期しないEOFです。"},
      {PP::pp_parse_context::Define_No_Identifier, u8"#defineディレクティブ中にマクロ名が見つかりません。"},
      {PP::pp_parse_context::Define_ParamlistBreak, u8"関数マクロ定義の仮引数リストの中では改行できません.。"},
      {PP::pp_parse_context::Define_Duplicate, u8"同じ名前のマクロが異なる定義で再定義されました。"},
      {PP::pp_parse_context::Define_Redfined, u8"同じ名前で異なる形式のマクロが再定義されました。"},
      {PP::pp_parse_context::Define_Sharp2BothEnd, u8"##トークンは置換リストの内部に現れなければなりません。"},
      {PP::pp_parse_context::Define_InvalidSharp, u8"#トークンは仮引数名の前だけに現れなければなりません。"},
      {PP::pp_parse_context::Define_InvalidVAARGS, u8"可変引数マクロではないのに __VA_ARGS__ が参照されています。"},
      {PP::pp_parse_context::Define_VAOPTRecursive, u8"__VA_OPT__は再帰してはいけません。"},
      {PP::pp_parse_context::Define_InvalidTokenConcat, u8"##による連結結果は有効なプリプロセッシングトークンではありません。"},
      {PP::pp_parse_context::Define_InvalidDirective, u8"#defineディレクティブに予期しない文字が現れています。"},
      {PP::pp_parse_context::Define_MissingComma, u8"関数マクロの仮引数列において、カンマが現れるべき位置に他のトークンが現れています。"},
      {PP::pp_parse_context::ControlLine_Undef, u8"マクロ名を指定してください。"},
      {PP::pp_parse_context::Funcmacro_InsufficientArgs, u8"関数マクロ呼び出しの引数の数が合いません。"},
      {PP::pp_parse_context::Funcmacro_ReplacementFail, u8"マクロ展開時、実引数に含まれているマクロの置換に失敗しました。"},
      {PP::pp_parse_context::ControlLine_Line_Num , u8"#lineディレクティブに指定された数値が不正です。std::size_tの範囲内の数値を指定してください。"},
      {PP::pp_parse_context::ControlLine_Line_ManyToken , u8"#lineディレクティブの後に不要なトークンがあります。"},
      {PP::pp_parse_context::Newline_NotAppear, u8"改行の前に予期しないトークンが現れています。"},
      {PP::pp_parse_context::PPConstexpr_MissingCloseParent, u8"対応する閉じ括弧')'が見つかりませんでした。"},
      {PP::pp_parse_context::PPConstexpr_Invalid, u8"プリプロセス時の定数式ではこのトークンは処理できません。"},
      {PP::pp_parse_context::PPConstexpr_EmptyCharacter, u8"文字リテラルが空です。"},
      {PP::pp_parse_context::PPConstexpr_MultiCharacter, u8"マルチキャラクタリテラルは、最後の文字だけが使用されます。"}
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