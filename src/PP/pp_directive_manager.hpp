#pragma once

#include <charconv>
#include <functional>
#include <algorithm>

#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira {
  inline namespace views {

    template<typename Range>
    auto reverse(Range& rng) noexcept {
      using std::rbegin;
      using std::rend;

      using begin_it = decltype(rbegin(rng));
      using end_it = decltype(rend(rng));

      struct revrse_impl {
        begin_it it;
        end_it sentinel;

        auto begin() const noexcept {
          return it;
        }

        auto end() const noexcept {
          return sentinel;
        }
      };

      return revrse_impl{.it = rbegin(rng), .sentinel = rend(rng)};
    }
  }
}

namespace kusabira::PP {

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


  class func_like_macro {
    //仮引数リスト
    std::pmr::vector<std::u8string_view> m_params;
    //置換トークンのリスト
    std::pmr::list<pp_token> m_tokens;
    //可変長マクロですか？
    bool m_is_va = false;
    //置換リストに現れる仮引数名のインデックスに対応する実引数のインデックス
    std::pmr::vector<std::pair<std::size_t, std::size_t>> m_correspond;

    void make_id_to_param_pair() {
      //仮引数名に対応する実引数リスト上の位置を求めるやつ
      auto find_param_index = [&arglist = m_params](auto token_str) -> std::pair<bool, std::size_t> {
        for (auto i = 0ull; i < arglist.size(); ++i) {
          if (arglist[i] == token_str) {
            return {true, i};
          }
        }
        return {false, 0};
      };

      // 置換対象トークン数
      const auto N = m_tokens.size();

      m_correspond.reserve(N);

      auto it = m_tokens.begin();
      for (auto index = 0ull; index < N; ++index, ++it) {
        //識別子だけを見る
        if ((*it).category != pp_token_category::identifier) continue;

        const auto [ismatch, param_index] = find_param_index((*it).token.to_view());
        //仮引数名ではないから次
        if (not ismatch) continue;

        m_correspond.emplace_back(index, param_index);
      }
    }

  public:
    template <typename T = std::pmr::vector<std::u8string_view>, typename U = std::pmr::list<pp_token>>
    func_like_macro(T &&params, U &&tokens, bool is_va)
        : m_params{ std::forward<T>(params), &kusabira::def_mr }
        , m_tokens{ std::forward<U>(tokens), &kusabira::def_mr }
        , m_is_va{is_va}
        , m_correspond{ &kusabira::def_mr }
    {}

    func_like_macro(func_like_macro&&) = default;
    func_like_macro& operator=(func_like_macro&&) = default;

    fn is_identical(const std::pmr::vector<std::u8string_view>& params, const std::pmr::list<pp_token>& tokens) const -> bool {
      return m_params == params and m_tokens == tokens;
    }

    fn check_args_num(const std::pmr::vector<std::pmr::list<pp_token>>& args) const noexcept -> bool {
      if (m_is_va == true) {
        return (m_params.size() - 1u) <= args.size();
      } else {
        return m_params.size() == args.size();
      }
    }

    template<typename F>
    fn func_macro_impl(const std::pmr::vector<std::pmr::list<pp_token>>& args, F&& find_param_index) const -> std::pmr::list<pp_token> {
      //置換対象のトークンシーケンスをコピー（終了後そのまま置換結果となる）
      std::pmr::list<pp_token> result_list{ m_tokens, &kusabira::def_mr };

      //result_listを先頭から見て、仮引数名が現れたら対応する位置の実引数へと置換する
      const auto last = std::end(result_list);
      for (auto it = std::begin(result_list); it != last; ++it) {
        //識別子だけを見る
        if ((*it).category != pp_token_category::identifier) continue;

        const auto [ismatch, index] = find_param_index((*it).token.to_view());
        //仮引数名ではないから次
        if (not ismatch) continue;

        //対応する実引数のトークン列をコピー
        std::pmr::list<pp_token> arg_list{ args[index], &kusabira::def_mr };

        //結果リストにsplice
        result_list.splice(it, std::move(arg_list));

        auto del = it;
        //消す要素の1つ前、置換したトークン列の最後に移動
        --it;

        //置換済みトークンを消す
        result_list.erase(del);
      }

      return result_list;
    }

    template<typename F>
    fn va_macro_impl(const std::pmr::vector<std::pmr::list<pp_token>>& args, F&& find_param_index) const -> std::pmr::list<pp_token> {
      using namespace std::string_view_literals;

      //置換対象のトークンシーケンスをコピー（終了後そのまま置換結果となる）
      std::pmr::list<pp_token> result_list{ m_tokens, std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr) };
      //実引数リストの先頭
      const auto arg_first = std::begin(args);

      //result_listを先頭から見て、仮引数名が現れたら対応する位置の実引数へと置換する
      const auto last = std::end(result_list);
      for (auto it = std::begin(result_list); it != last; ++it) {
        auto& rep_token = *it;

        //識別子だけを相手にする
        if (rep_token.category != pp_token_category::identifier) continue;

        //可変長マクロだったら・・・？
        if (rep_token.token.to_view() == u8"__VA_ARGS__") {
          auto [ismatch, va_start_index] = find_param_index(u8"...");
          
          //可変長マクロでは無いのに__VA_ARGS__が参照された?
          assert(ismatch);

          //可変長引数部分をコピーしつつカンマを登録
          auto N = args.size();
          for (std::size_t index = va_start_index; index < N; ++index) {
            //対応する実引数のトークン列をコピー
            std::pmr::list<pp_token> arg_list{ args[index], &kusabira::def_mr };

            //最後の引数にはカンマをつけない
            if (index != (N - 1)) {
              //カンマの追加
              auto& comma = arg_list.emplace_back(pp_token_category::op_or_punc);
              comma.token = u8","sv;
            }

            //結果リストにsplice
            result_list.splice(it, std::move(arg_list));
          }

        } else {
          //VA_ARGS以外の普通の置換処理

          const auto [ismatch, index] = find_param_index((*it).token.to_view());
          //仮引数名ではないから次
          if (not ismatch) continue;

          //対応する実引数のトークン列をコピー
          std::pmr::list<pp_token> arg_list{ args[index], &kusabira::def_mr };

          //結果リストにsplice
          result_list.splice(it, std::move(arg_list));
        }

        auto del = it;
        //消す要素の1つ前、置換したトークン列の最後に移動
        --it;

        //置換済みトークンを消す
        result_list.erase(del);
      }

      return result_list;
    }

    fn operator()(const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::pmr::list<pp_token> {
      //仮引数名に対応する実引数リスト上の位置を求めるやつ
      auto find_param_index = [&arglist = m_params](auto token_str) -> std::pair<bool, std::size_t> {
        for (auto i = 0u; i < arglist.size(); ++i) {
          if (arglist[i] == token_str) {
            return {true, i};
          }
        }
        return {false, 0};
      };

      //可変引数マクロと処理を分ける
      if (m_is_va) {
        return this->va_macro_impl(args, find_param_index);
      } else {
        return this->func_macro_impl(args, find_param_index);
      }
    }
  };

  /**
  * @brief マクロの管理などプリプロセッシングディレクティブの実行を担う
  */
  struct pp_directive_manager {

    using macro_map = std::pmr::unordered_map<std::u8string_view, std::pmr::list<pp_token>>;
    using funcmacro_map = std::pmr::unordered_map<std::u8string_view, func_like_macro>;

    std::size_t m_line = 1;
    fs::path m_filename{};
    fs::path m_replace_filename{};
    macro_map m_objmacros{&kusabira::def_mr};
    funcmacro_map m_funcmacros{&kusabira::def_mr};

    pp_directive_manager() = default;

    pp_directive_manager(const fs::path& filename)
      : m_filename{filename}
      , m_replace_filename{filename.filename()}
    {}

    void newline() {
      ++m_line;
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
    template<typename Reporter, typename ReplacementList = std::pmr::list<pp_token>>
    fn define(Reporter& reporter, const PP::lex_token& macro_name, ReplacementList&& tokenlist) -> bool {
      const auto [pos, is_registered] = m_objmacros.try_emplace(macro_name.token, std::forward<ReplacementList>(tokenlist));

      if (not is_registered) {
        //すでに登録されている場合、登録済みの置換リストとの同一性を判定する
        if ((*pos).second == tokenlist) return true;
        //トークンが一致していなければエラー
        reporter.pp_err_report(m_filename, macro_name, pp_parse_context::Define_Duplicate);
        return false;
      }

      //関数マクロが重複していれば消す
      if (auto n = m_funcmacros.erase(macro_name.token); n != 0) {
        //警告表示
        reporter.pp_err_report(m_filename, macro_name, pp_parse_context::Define_Redfined, report::report_category::warning);
      }

      return true;
    }

    /**
    * @brief マクロによる置換リストを取得する
    * @param macro_name 識別子トークン名
    * @return 置換リストのoptional、無効地なら置換対象ではなかった
    */
    fn objmacro(std::u8string_view macro_name) const -> std::optional<std::pmr::list<pp_token>> {
      using std::end;
      const auto pos = m_objmacros.find(macro_name);
      if (pos == end(m_objmacros)) return std::nullopt;
      //コピーして返す
      return std::optional<std::pmr::list<pp_token>>{std::in_place, (*pos).second, &kusabira::def_mr};
    }

    /**
    * @brief 関数マクロを登録する
    * @param reporter メッセージ出力器
    * @param macro_name マクロ名
    * @param params 仮引数リスト
    * @param tokenlist 置換リスト
    * @param is_va 可変引数であるか否か
    * @return 登録が恙なく完了したかどうか
    */
    template<typename Reporter, typename ParamList = std::pmr::vector<std::u8string_view>, typename ReplacementList = std::pmr::list<pp_token>>
    fn define(Reporter& reporter, const PP::lex_token& macro_name, ParamList&& params, ReplacementList&& tokenlist, bool is_va) -> bool {
      //関数マクロを登録する
      const auto [pos, is_registered] = m_funcmacros.try_emplace(macro_name.token, std::forward<ParamList>(params), std::forward<ReplacementList>(tokenlist), is_va);

      if (not is_registered) {
        if ((*pos).second.is_identical(params, tokenlist)) return true;
        //仮引数列と置換リストが一致していなければエラー
        reporter.pp_err_report(m_filename, macro_name, pp_parse_context::Define_Duplicate);
        return false;
      }

      //オブジェクトマクロが重複していれば消す
      if (auto n = m_objmacros.erase(macro_name.token); n != 0) {
        //警告表示
        reporter.pp_err_report(m_filename, macro_name, pp_parse_context::Define_Duplicate, report::report_category::warning);
      }

      return true;
    }

    /**
    * @brief 関数マクロによる置換リストを取得する
    * @param macro_name マクロ名
    * @param args 関数マクロの実引数トークン列
    * @return {仮引数の数と実引数の数があったか否か, 置換リストのoptional}
    */
    fn funcmacro(std::u8string_view macro_name, const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::pair<bool, std::optional<std::pmr::list<pp_token>>> {
      using std::end;

      const auto pos = m_funcmacros.find(macro_name);
      if (pos == end(m_funcmacros)) return {true, std::nullopt};

      const auto& funcmacro = (*pos).second;

      //引数長さのチェック、ここでエラーにしたい
      if (not funcmacro.check_args_num(args)) return {false, std::nullopt};

      //置換結果取得
      auto&& result = funcmacro(args);

      //optionalで包んで返す
      return {true, std::optional<std::pmr::list<pp_token>>{std::in_place, std::move(result), &kusabira::def_mr}};
    }

    /**
    * @brief 識別子がマクロ名であるかをチェックする
    * @param identifier 識別子の文字列
    * @return {オブジェクトマクロであるか？, 関数マクロであるか？}
    */
    fn is_macro(std::u8string_view identifier) const -> std::pair<bool, bool> {
      return {m_objmacros.contains(identifier), m_funcmacros.contains(identifier)};
    }

    /**
    * @brief #undefディレクティブを実行する
    */
    void undef(std::u8string_view macro_name) {
      //消す、登録してあったかは関係ない
      m_objmacros.erase(macro_name);
      m_funcmacros.erase(macro_name);
    }

    /**
    * @brief #lineディレクティブを実行する
    * @param reporter レポート出力オブジェクトへの参照
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template <typename Reporter, typename PPTokenRange>
    fn line(Reporter& reporter, const PPTokenRange& token_range) -> bool {
      using std::cbegin;
      using std::cend;

      auto it = cbegin(token_range);
      auto end = cend(token_range);

      //事前条件
      assert(it != end);

      if ((*it).category == pp_token_category::pp_number) {
        //現在行番号の変更

        std::size_t value;
        auto tokenstr = (*it).token.to_view();
        auto* first = reinterpret_cast<const char*>(tokenstr.data());

        if (auto [ptr, ec] = std::from_chars(first, first + tokenstr.length(), value); ec == std::errc{}) {
          //カウントしてる行番号を変更
          m_line = value;
        } else {
          assert((*it).lextokens.empty() == false);
          //エラーです
          reporter.pp_err_report(m_filename, (*it).lextokens.front(), pp_parse_context::ControlLine_Line_Num);
          return false;
        }

        //次のトークンが無ければ終わり
        ++it;
        if (it == end or (*it).category == pp_token_category::newline) return true;

        if (pp_token_category::string_literal <= (*it).category and (*it).category <= pp_token_category::user_defined_raw_string_literal) {
          //ファイル名変更

          //文字列リテラルから文字列を取得
          auto str = extract_string_from_strtoken((*it).token, pp_token_category::raw_string_literal <= (*it).category);
          m_replace_filename = str;

          ++it;
          if (it == end or (*it).category != pp_token_category::newline) {
            //ここにきた場合は未定義に当たるはずなので、警告出して継続する
            reporter.pp_err_report(m_filename, (*it).lextokens.front(), pp_parse_context::ControlLine_Line_ManyToken, report::report_category::warning);
          }

          return true;
        }
      }

      //マクロを展開した上で#lineディレクティブを実行する、マクロ展開を先に実装しないと・・・
      //展開後に#lineにならなければ未定義動作、エラーにしようかなあ
      assert(false);

      return false;
    }

    /**
    * @brief #errorディレクティブを実行する
    * @param reporter レポート出力オブジェクトへの参照
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template<typename Reporter, typename TokensIterator, typename TokensSentinel>
    void error(Reporter& reporter, TokensIterator& it, TokensSentinel end) const {
      assert(it != end);
      assert((*it).token == u8"error");

      auto err_token = std::move(*it);
      const auto &line_str = err_token.get_line_string();

      //#errorの次のホワイトスペースでないトークン以降を出力
      this->skip_whitespace(it, end);

      if ((*it).kind != pp_tokenize_status::NewLine) {
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
      } while (it != end and (*it).kind != pp_tokenize_status::NewLine);
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
      } while (it != end and (*it).kind == pp_tokenize_status::Whitespaces);
    }

    /**
    * @brief 現在のファイル名と行番号を取得する、テスト用
    */
    fn get_state() const -> std::pair<std::size_t, const fs::path&> {
      return {m_line, m_replace_filename};
    }

  };  
    
} // namespace kusabira::PP