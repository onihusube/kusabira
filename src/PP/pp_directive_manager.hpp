#pragma once

#include <charconv>
#include <functional>
#include <algorithm>
#include <tuple>

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

  /**
  * @brief トークンリスト上の最初の対応する閉じ括弧")"の位置を探索する
  * @details 開きかっこ"("の次のトークンから始めるものとして、間に現れているかっこのペアを無視する
  * @param it 探索開始位置のイテレータ
  * @param end 探索範囲の終端（[it, end)
  * @return 見つかった位置を指すイテレータ
  */
  template<typename Iterator, typename Sentinel>
  ifn search_close_parenthesis(Iterator it, Sentinel end) -> Iterator {
    using namespace std::string_view_literals;

    //間に現れている開きかっこの数
    std::size_t inner_paren_depth = 0;

    for (; it != end; ++it) {
      if ((*it).token == u8")"sv) {
        //開きかっこが現れる前に閉じればそれ
        if (inner_paren_depth == 0) return it;
        --inner_paren_depth;
      } else if ((*it).token == u8"("sv) {
        ++inner_paren_depth;
      }
    }
    return end;
  }

  /**
  * @brief 関数マクロ1つを表現する型
  */
  class func_like_macro {
    //仮引数リスト
    std::pmr::vector<std::u8string_view> m_params;
    //置換トークンのリスト
    std::pmr::list<pp_token> m_tokens;
    //可変長マクロですか？
    bool m_is_va = false;
    //{置換リストに現れる仮引数名のインデックス, 対応する実引数のインデックス, __VA_ARGS__?, __VA_OPT__?}
    std::pmr::vector<std::tuple<std::size_t, std::size_t, bool, bool>> m_correspond;

    /**
    * @brief 置換リスト上の仮引数を見つけて、仮引数の番号との対応を取っておく
    * @tparam Is_VA 可変長マクロか否か
    */
    template<bool Is_VA>
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

        if constexpr (Is_VA) {
          //可変引数参照のチェック
          if ((*it).token.to_view() == u8"__VA_ARGS__") {
            const auto [ismatch, va_start_index] = find_param_index(u8"...");

            //可変長マクロでは無いのに__VA_ARGS__が参照された?
            assert(ismatch);

            //置換リストの要素番号に対して、対応する実引数位置を保存
            m_correspond.emplace_back(index, va_start_index, true, false);
            continue;
          } else if ((*it).token.to_view() == u8"__VA_OPT__") {
            //__VA_OPT__を見つけておく
            m_correspond.emplace_back(index, 0, false, true);
            continue;
          }
        }

        const auto [ismatch, param_index] = find_param_index((*it).token.to_view());
        //仮引数名ではないから次
        if (not ismatch) continue;

        //置換リストの要素番号に対して、対応する実引数位置を保存
        m_correspond.emplace_back(index, param_index, false, false);
      }
    }

    /**
    * @brief 通常の関数マクロの処理の実装
    * @param args 実引数列（カンマ区切り毎のトークン列のvector
    * @return マクロ置換後のトークンリスト
    */
    fn func_macro_impl(const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::pmr::list<pp_token> {
      //置換対象のトークンシーケンスをコピー（終了後そのまま置換結果となる）
      std::pmr::list<pp_token> result_list{ m_tokens, &kusabira::def_mr };

      //置換リストの起点となる先頭位置
      const auto head = std::begin(result_list);

      //置換リスト-引数リスト対応を後ろから処理
      for (const auto[token_index, arg_index, ignore1, ignore2] : views::reverse(m_correspond)) {
        //置換リストのトークン位置
        const auto it = std::next(head, token_index);

        //対応する実引数のトークン列をコピー
        std::pmr::list<pp_token> arg_list{args[arg_index], &kusabira::def_mr};

        //結果リストにsplice
        result_list.splice(it, std::move(arg_list));

        //置換済みトークンを消す
        result_list.erase(it);
      }

      return result_list;
    }

    /**
    * @brief 可変引数関数マクロの処理の実装
    * @param args 実引数列（カンマ区切り毎のトークン列のvector
    * @return マクロ置換後のトークンリスト
    */
    fn va_macro_impl(const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::pmr::list<pp_token> {
      using namespace std::string_view_literals;

      //置換対象のトークンシーケンスをコピー（終了後そのまま置換結果となる）
      std::pmr::list<pp_token> result_list{ m_tokens, &kusabira::def_mr };

      //置換リストの起点となる先頭位置
      const auto head = std::begin(result_list);
      //引数の数
      const auto N = args.size();

      //置換リスト-引数リスト対応を後ろから処理
      for (const auto[token_index, arg_index, va_args, va_opt] : views::reverse(m_correspond)) {
        //置換リストのトークン位置
        const auto it = std::next(head, token_index);

        if (va_args) {
          //可変長引数部分をコピーしつつカンマを登録
          for (std::size_t va_index = arg_index; va_index < N; ++va_index) {
            //対応する実引数のトークン列をコピー
            std::pmr::list<pp_token> arg_list{args[va_index], &kusabira::def_mr};

            //最後の引数にはカンマをつけない
            if (va_index != (N - 1)) {
              //カンマの追加
              auto& comma = arg_list.emplace_back(pp_token_category::op_or_punc);
              comma.token = u8","sv;
              //エラーメッセージのためにコンテキストを補う必要がある？
            }

            //結果リストにsplice
            result_list.splice(it, std::move(arg_list));
          }
        } else if (va_opt) {
          //__VA_OPT__の処理

          //事前条件
          assert((*it).token.to_view() == u8"__VA_OPT__");

          const auto replist_end = std::end(result_list);
          //__VA_OPT__の開きかっこの位置を探索
          auto open_paren_pos_next = std::find_if(std::next(it), replist_end, [](auto &pptoken) {
            return pptoken.token == u8"("sv;
          });

          assert(open_paren_pos_next != replist_end);
          //開きかっこの"次"の位置
          ++open_paren_pos_next;

          //対応する閉じ括弧の位置を探索
          auto vaopt_end = search_close_parenthesis(open_paren_pos_next, replist_end);

          // 対応する閉じかっこの存在は構文解析で保証する
          assert(vaopt_end != replist_end);

          //可変長引数が純粋に空の時の条件
          bool is_va_empty = N < m_params.size();
          //F(arg1, ...)なマクロに対して、F(0,)の様に呼び出した時のケア（F(0,,)は引数ありとみなされる）
          is_va_empty |= (N == m_params.size()) and (args.back().size() == 0ull);

          if (is_va_empty) {
            //可変長部分が空ならばVA_OPT全体を削除

            //VAOPTの全体を結果トークン列から取り除く
            result_list.erase(it, ++vaopt_end);
          } else {
            //可変長部分が空でないならば、VA_OPTと対応するかっこだけを削除

            //まずは後ろの閉じ括弧を削除（イテレータが無効化するのを回避するため）
            result_list.erase(vaopt_end);
            //次に開きかっことVA_OPT本体を削除
            result_list.erase(it, open_paren_pos_next);
          }

          //itに対応するトークンを消してしまっているので次に行く
          continue;
        } else {
          //対応する実引数のトークン列をコピー
          std::pmr::list<pp_token> arg_list{args[arg_index], &kusabira::def_mr};

          //結果リストにsplice
          result_list.splice(it, std::move(arg_list));
        }

        //置換済みトークンを消す
        result_list.erase(it);
      }

      return result_list;
    }


  public:

    /**
    * @brief コンストラクタ
    * @param params 仮引数列
    * @param is_va 可変引数マクロであるか否か
    */
    template <typename T = std::pmr::vector<std::u8string_view>, typename U = std::pmr::list<pp_token>>
    func_like_macro(T &&params, U &&tokens, bool is_va)
        : m_params{ std::forward<T>(params), &kusabira::def_mr }
        , m_tokens{ std::forward<U>(tokens), &kusabira::def_mr }
        , m_is_va{is_va}
        , m_correspond{ &kusabira::def_mr }
    {
      if (is_va) {
        this->make_id_to_param_pair<true>();
      } else {
        this->make_id_to_param_pair<false>();
      }
    }

    func_like_macro(func_like_macro&&) = default;
    func_like_macro& operator=(func_like_macro&&) = default;

    /**
    * @brief 仮引数列と置換リストから別のマクロとの同一性を判定する
    * @param params 仮引数列
    * @param tokens 置換リスト
    * @return マクロとして同一であるか否か
    */
    fn is_identical(const std::pmr::vector<std::u8string_view>& params, const std::pmr::list<pp_token>& tokens) const -> bool {
      return m_params == params and m_tokens == tokens;
    }

    /**
    * @brief 実引数の数が正しいかを調べる
    * @param args 実引数列
    * @details 可変引数マクロなら仮引数の数-1以上、それ以外の場合は仮引数の数と同一である場合に引数の数が合っているとみなす
    * @return 実引数の数が合っているか否か
    */
    fn validate_argnum(const std::pmr::vector<std::pmr::list<pp_token>>& args) const noexcept -> bool {
      if (m_is_va == true) {
        return (m_params.size() - 1u) <= args.size();
      } else {
        return m_params.size() == args.size();
      }
    }

    /**
    * @brief 関数マクロを実行する
    * @param args 実引数列
    * @return 置換結果のトークンリスト
    */
    fn operator()(const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::pmr::list<pp_token> {
      //可変引数マクロと処理を分ける
      if (m_is_va) {
        return this->va_macro_impl(args);
      } else {
        return this->func_macro_impl(args);
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
      if (not funcmacro.validate_argnum(args)) return {false, std::nullopt};

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