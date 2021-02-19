#pragma once

#include <charconv>
#include <functional>
#include <algorithm>
#include <tuple>
#include <chrono>

#include "../common.hpp"
#include "../report_output.hpp"
#include "macro_manager.hpp"

namespace kusabira::PP::inline free_func{
  
  /**
  * @brief 文字列リテラルトークンから文字列そのものを抽出する
  * @param tokenstr 文字列リテラル全体のトークン（リテラルサフィックス付いててもいい）
  * @param is_rawstr 生文字列リテラルであるか？
  * @return 文字列部分を参照するstring_view
  */
  ifn extract_string_from_strtoken(std::u8string_view tokenstr, bool is_rawstr) -> std::u8string_view {
    //正しい（生）文字列リテラルが入ってくることを仮定する

    if (is_rawstr) {
      //生文字列リテラルのR"delimiter(...)delimiter"の中の文字列を取得

      //R"を探してその次の位置を見つける
      auto pos = tokenstr.find(u8"R\"", 0, 2);
      //絶対見つかる筈
      assert(pos != std::string_view::npos);
      //R"の分すすめる
      pos += 2;

      //デリミタ長
      std::size_t delimiter_length = 1u;
      //デリミタ文字列
      char8_t delimiter[18]{u8')'};

      //デリミタを検出
      while (tokenstr[pos] != u8'(') {
        delimiter[delimiter_length] = tokenstr[pos];
        ++delimiter_length;
        ++pos;
      }

      //デリミタの長さは16文字まで、)があるので+1
      assert(delimiter_length <= 17u);

      // )delimiter" の形にしておく
      delimiter[delimiter_length] = u8'"';
      ++delimiter_length;

      //posは R"delimiter( の次の位置（＝文字列本体の先頭）までの文字数
      ++pos;

      //閉じデリミタの位置を検索
      std::boyer_moore_searcher searcher{ delimiter, delimiter + delimiter_length };
      //生文字列リテラルの先頭ポインタ
      auto* p = tokenstr.data();

      //戻り値は[検索対象の開始位置, 検索対象の終了位置]
      const auto [last_ptr, ignore] = searcher(p + pos, p + tokenstr.length());
      //これが来ることは無い・・・はず
      assert(last_ptr != ignore);

      //生文字列リテラル本文長
      std::size_t str_length = std::distance(p + pos, last_ptr);

      return tokenstr.substr(pos, str_length);
    } else {
      //通常の文字列リテラルの""の中の文字列を取得

      //"の次
      auto first = tokenstr.find(u8'"') + 1;
      //最後の"
      auto last = tokenstr.rfind(u8'"');

      assert((last - first) <= (tokenstr.length() - 2));

      return tokenstr.substr(first, last - first);
    }
  }
}

namespace kusabira::PP {

  /**
  * @brief マクロの管理などプリプロセッシングディレクティブの実行を担う
  */
  struct pp_directive_manager {

    using funcmacro_map = std::pmr::unordered_map<std::u8string_view, unified_macro>;
    using timepoint_t = decltype(std::chrono::system_clock::now());

    fs::path m_filename{};
    macro_manager m_macro_manager{};

    pp_directive_manager() = default;

    pp_directive_manager(const fs::path& filename)
      : m_filename{filename}
      , m_macro_manager{ filename }
    {}

    void newline() {
    }

    template<typename TokensIterator, typename TokensSentinel>
    fn include(TokensIterator& it, TokensSentinel end) -> std::pmr::list<pp_token> {
      //未実装
      assert(false);
      return {};
    }

    /**
    * @brief オブジェクトマクロを登録する
    * @param reporter メッセージ出力器
    * @param macro_name マクロ名
    * @param tokenlist 置換リスト
    * @return 登録が恙なく完了したかどうか
    */
    template <typename Reporter, typename ReplacementList = std::pmr::list<pp_token>>
    fn define(Reporter &reporter, const PP::pp_token& macro_name, ReplacementList &&tokenlist) -> bool {
      return m_macro_manager.register_macro(reporter, macro_name, std::forward<ReplacementList>(tokenlist));
    }

    /**
    * @brief 関数マクロを登録する
    * @param reporter メッセージ出力器
    * @param macro_name マクロ名
    * @param tokenlist 置換リスト
    * @param is_va 可変引数であるか否か
    * @param params 仮引数リスト
    * @return 登録が恙なく完了したかどうか
    */
    template <typename Reporter, typename ReplacementList = std::pmr::list<pp_token>, typename ParamList>
    fn define(Reporter& reporter, const PP::pp_token& macro_name, ReplacementList&& tokenlist, ParamList&& params, bool is_va = false) -> bool {
      return m_macro_manager.register_macro(reporter, macro_name, std::forward<ReplacementList>(tokenlist), std::forward<ParamList>(params), is_va);
    }

    template<typename Reporter>
    fn expand_objmacro(Reporter& reporter, const pp_token& macro_name) const -> std::tuple<bool, bool, std::pmr::list<pp_token>, std::pmr::unordered_set<std::u8string_view>> {
      return m_macro_manager.objmacro<false>(reporter, macro_name);
    }

    template<typename Reporter>
    fn expand_funcmacro(Reporter& reporter, const pp_token& macro_name, const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::tuple<bool, bool, std::pmr::list<pp_token>, std::pmr::unordered_set<std::u8string_view>> {
      return m_macro_manager.funcmacro<false>(reporter, macro_name, args);
    }
    template<typename Reporter>
    fn expand_funcmacro(Reporter& reporter, const pp_token& macro_name, const std::pmr::vector<std::pmr::list<pp_token>>& args, std::pmr::unordered_set<std::u8string_view>& outer_macro) const -> std::tuple<bool, bool, std::pmr::list<pp_token>> {
      return m_macro_manager.funcmacro<false>(reporter, macro_name, args, outer_macro);
    }

    /**
    * @brief 識別子がマクロ名であるか、また関数形式かをチェックする
    * @param identifier 識別子の文字列
    * @return マクロでないなら無効値、関数マクロならtrue、オブジェクトマクロならfalse
    */
    fn is_macro(std::u8string_view id_str) const -> std::optional<bool> {
      return m_macro_manager.is_macro(id_str);
    }

    /**
    * @brief #undefディレクティブを実行する
    */
    void undef(std::u8string_view macro_name) {
      m_macro_manager.unregister_macro(macro_name);
    }

    /**
    * @brief #lineディレクティブを実行する
    * @param reporter レポート出力オブジェクトへの参照
    * @param token_range 行末までのプリプロセッシングトークン列（マクロ置換済）
    */
    template<typename Reporter, typename PPTokenRange = std::pmr::list<pp_token>>
    fn line(Reporter& reporter, const PPTokenRange& token_range) -> bool {
      using std::cbegin;
      using std::cend;

      auto it = cbegin(token_range);
      auto end = cend(token_range);

      //事前条件
      assert(it != end);

      if ((*it).category != pp_token_category::pp_number) {
        //#line の後のトークンが数値では無い、構文エラー
        reporter.pp_err_report(m_filename, deref(it), pp_parse_context::ControlLine_Line_Num);
        return false;
      }

      //現在行番号の変更

      //本当の論理行番号
      auto true_line = deref(it).get_logicalline_num();
      auto tokenstr = (*it).token.to_view();
      //数値文字列の先頭
      auto* first = reinterpret_cast<const char*>(tokenstr.data());

      std::size_t value;
      if (auto [ptr, ec] = std::from_chars(first, first + tokenstr.length(), value); ec == std::errc{}) {
        //論理行数に対して指定された行数の対応を取っておく
        m_macro_manager.change_line(true_line, value);
      } else {
        //浮動小数点数が指定されていた？エラー
        reporter.pp_err_report(m_filename, deref(it), pp_parse_context::ControlLine_Line_Num);
        return false;
      }

      //次のトークンが無ければ終わり
      ++it;
      if (it == end or (*it).category == pp_token_category::newline) return true;

      if (pp_token_category::string_literal <= (*it).category and (*it).category <= pp_token_category::user_defined_raw_string_literal) {
        //文字列リテラルから文字列を取得
        auto str = extract_string_from_strtoken((*it).token, pp_token_category::raw_string_literal <= (*it).category);
        //ファイル名変更
        m_macro_manager.change_filename(str);
        ++it;
      }

      if (it != end or (*it).category != pp_token_category::newline) {
        //ここにきた場合は未定義に当たるはずなので、警告出して継続する
        reporter.pp_err_report(m_filename, deref(it), pp_parse_context::ControlLine_Line_ManyToken, report::report_category::warning);
      }

      return true;
    }

    /**
    * @brief #errorディレクティブを実行する
    * @param reporter レポート出力オブジェクトへの参照
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    /*template<typename Reporter, std::input_iterator TokensIterator, std::sentinel_for<TokensIterator> TokensSentinel>
    void error(Reporter& reporter, TokensIterator& it, TokensSentinel end) const {
      assert(it != end);
      assert((*it).token == u8"error");

      auto err_token = std::move(*it);
      const auto &line_str = err_token.get_line_string();

      //#errorの次のホワイトスペースでないトークン以降を出力
      this->skip_whitespace(it, end);

      if ((*it).category != pp_token_category::newline) {
        //行は1から、列は0から・・・
        const auto [row, col] = (*it).get_phline_pos();

        assert(col < line_str.length());

        //その行の文字列中の#errorディレクティブメッセージの開始位置と長さ
        auto start_pos = line_str.data() + col;
        std::size_t length = (line_str.data() + line_str.length()) - start_pos;

        reporter.print_report({ start_pos, length }, m_filename, err_token);
      } else {
        //出力するものがない・・・
        reporter.print_report({}, m_filename, *it);
      }
    }*/

    template<typename Reporter, std::ranges::sized_range PPTokenList>
    void error(Reporter& reporter, PPTokenList&& pptokens, const PP::pp_token& err_context) const {
      // マクロ置換済みトークンを文字列化して連結する
      std::pmr::u8string err_message{&kusabira::def_mr};
      // 予約、トークン数*5(文字)
      err_message.reserve(size(pptokens) * 5);

      for (auto& pptoken : pptokens) {
        err_message.append(pptoken.token.to_view());
      }

      reporter.print_report(err_message, m_filename, err_context);
    }

    template<typename Reporter>
    void error(Reporter& reporter, const PP::pp_token& err_context, const PP::pp_token& err_message) const {

      const auto &line_str = err_message.get_line_string();

      if (err_message.category != pp_token_category::newline) {
        // 行は1から、列は0から・・・
        const auto [row, col] = err_message.get_phline_pos();

        // 行内での位置よりも文字列が短いということはあり得ない（読み込み部分がおかしい）
        assert(col < line_str.length());

        // その行の文字列中の#errorディレクティブメッセージ部分の開始位置と長さ
        auto* start_pos = line_str.data() + col;
        std::size_t length = (line_str.data() + line_str.length()) - start_pos;

        // 1つ多い分と末尾改行の分を引く
        reporter.print_report({start_pos, length - 2}, m_filename, err_context);
      } else {
        //出力するものがない・・・
        reporter.print_report({}, m_filename, err_context);
      }
    }

    /**
    * @brief #pragmaディレクティブを実行する
    * @param reporter レポート出力オブジェクトへの参照
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template<typename Reporter, typename TokensIterator, typename TokensSentinel>
    void pragma(Reporter&, TokensIterator& it, TokensSentinel end) const {
      assert(it != end);

      //改行まで飛ばす
      //あらゆるpragmaを無視、仮実装
      do {
        ++it;
      } while (it != end and (*it).category != pp_token_category::newline);
    }

    /**
    * @brief ホワイトスペースだけを飛ばす
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template<typename TokensIterator, typename TokensSentinel>
    void skip_whitespace(TokensIterator& it, TokensSentinel end) const {
      do {
        ++it;
      } while (it != end and (*it).category == pp_token_category::whitespaces);
    }

  };  
    
} // namespace kusabira::PP