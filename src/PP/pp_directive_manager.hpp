#pragma once

#include <charconv>
#include <functional>

#include "../common.hpp"
#include "../report_output.hpp"

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

  public:
    template <typename T = std::pmr::vector<std::u8string_view>, typename U = std::pmr::list<pp_token>>
    func_like_macro(T &&args, U &&tokens, bool is_va)
        : m_params{std::forward<T>(args)}
        , m_tokens{std::forward<U>(tokens)}
        , m_is_va{is_va}
    {}

    fn check_args_num(const std::pmr::list<pp_token>& args) const noexcept -> bool {
      if (m_is_va == true) {
        return (m_params.size() - 1u) <= args.size();
      } else {
        return m_params.size() == args.size();
      }
    }

    fn operator()(const std::pmr::list<pp_token>& args) const -> std::pmr::list<pp_token> {

      using namespace std::string_view_literals;

      //置換対象のトークンシーケンス
      std::pmr::list<pp_token> result_list{m_tokens, std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr)};
      
      //実引数リストの先頭
      auto arg_first = std::begin(args);

      //与えられたトークン文字列が仮引数名ならば、その実引数リスト上の位置を求める
      auto find_param_index = [&arglist = m_params](auto token_str) -> std::pair<bool, std::size_t> {
        for (auto i = 0u; i < arglist.size(); ++i) {
          if (arglist[i] == token_str) {
            return {true, i};
          }
        }
        return {false, 0};
      };

      //置換対象リストのトークン列から仮引数を見つけて置換する
      auto last = std::end(result_list);
      for (auto it = std::begin(result_list); it != last; ++it) {
        auto& idtoken = *it;

        //識別子だけを相手にする
        if (idtoken.category != pp_token_category::identifier) continue;

        //可変長マクロだったら・・・？
        if (m_is_va and idtoken.token.to_view() == u8"__VA_ARGS__") {
          auto [ismatch, index] = find_param_index(u8"__VA_ARGS__");
          if (ismatch) {
            //可変長実引数の先頭イテレータ
            auto arg_it = std::next(arg_first, index);
            //可変長実引数の置換リスト
            std::pmr::list<pp_token> va_list{ std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr) };
            
            //可変長引数部分をコピーしつつカンマを登録
            auto arg_last = std::end(args);
            for (; arg_it != arg_last; ++arg_it) {
              //実引数トークンの追加
              va_list.emplace_back(*arg_it);
              //カンマの追加
              auto& comma = va_list.emplace_back(pp_token_category::op_or_punc);
              comma.token = vocabulary::whimsy_str_view{ u8","sv };
            }
            
            //#演算子の処理がいる？

            //__VA_OPT__の処理

            //result_listの今の要素を消して、可変長リストをspliceする

            //関連イテレータの更新
          } else {
            //エラー、可変長マクロでは無いのに__VA_ARGS__が参照された?
            //チェックは登録時にやってほしい
            assert(false);
          }
        } else {
          //普通の関数マクロとして処理
          auto [ismatch, index] = find_param_index(idtoken.token.to_view());
          if (ismatch) {
            auto arg_it = std::next(arg_first, index);
            //トークン文字列のみを置換
            idtoken.token = (*arg_it).token;
          }
          //見つからないという事は関数マクロの仮引数ではなかったということ
        }
      }
      
      //#,##演算子の処理？
    }
  };

  /**
  * @brief マクロの管理などプリプロセッシングディレクティブの実行を担う
  */
  struct pp_directive_manager {

    using macro_map = std::pmr::unordered_map<std::u8string_view, std::pmr::list<pp_token>>;
    using pmralloc = typename macro_map::allocator_type;

    std::size_t m_line = 1;
    fs::path m_filename{};
    fs::path m_replace_filename{};
    macro_map m_objmacros{pmralloc(&kusabira::def_mr)};

    void newline() {
      ++m_line;
    }

    template<typename TokensIterator, typename TokensSentinel>
    fn include(TokensIterator& it, TokensSentinel end) -> std::pmr::list<pp_token> {
      //未実装
      assert(false);
      return {};
    }

    template<typename Reporter, typename PPTokenRange>
    fn define(Reporter& reporter, std::u8string_view macro_name, PPTokenRange&& token_range) -> bool {
      //オブジェクトマクロを登録する
      if (m_objmacros.contains(macro_name)) {
        //すでに登録されている場合
        //登録済みのトークン列の同一性を判定する
        if (token_range == m_objmacros[macro_name]) return true;

        //トークンが一致していなければエラー
        //reporter.pp_err_report(m_filename, (*it).lextokens.front(), pp_parse_context::Define_Duplicate);
        return false;
      }

      //なければそのまま登録
      m_objmacros.emplace(std::forward<PPTokenRange>(token_range));
      return true;
    }

    template<typename Reporter, typename ArgList, typename ReplacementList>
    fn define(Reporter& reporter, std::u8string_view macro_name, ArgList&& args, ReplacementList&& tokenlist) -> bool {
      //関数マクロを登録する
      if (m_objmacros.contains(macro_name)) {
        //すでに登録されている場合
        //登録済みのトークン列の同一性を判定する
        if (tokenlist == m_objmacros[macro_name]) return true;

        //トークンが一致していなければエラー
        //reporter.pp_err_report(m_filename, (*it).lextokens.front(), pp_parse_context::Define_Duplicate);
        return false;
      }

      //なければそのまま登録
      m_objmacros.emplace(std::forward<PPTokenRange>(tokenlist));
      return true;
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
          if (it != end or (*it).category != pp_token_category::newline) {
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

      auto err_token = std::move(*it);
      const auto &line_str = err_token.get_line_string();

      this->skip_whitespace(it, end);

      if ((*it).kind != pp_tokenize_status::NewLine) {
        //行は1から、列は0から・・・
        const auto [row, col] = (*it).get_phline_pos();

        assert(col < line_str.length());

        reporter.print_report({line_str.data() + col + 1, line_str.length() - (col + 1)}, m_filename, err_token);
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

  };  
    
} // namespace kusabira::PP