#pragma once

#include <list>
#include <forward_list>
#include <utility>
#include <unordered_set>
#include <cassert>
#include <algorithm>
#include <ranges>

#include "../common.hpp"
#include "file_reader.hpp"
#include "pp_automaton.hpp"
#include "pp_tokenizer.hpp"
#include "pp_directive_manager.hpp"
#include "vocabulary/scope.hpp"
#include "vocabulary/concat.hpp"

namespace kusabira::PP {

  enum class pp_parse_status : std::uint8_t {
    UnknownTokens,        //予期しないトークン入力、エラー
    Complete,             //1行分の完了
    FollowingSharpToken,  //#トークンを読んだ状態で戻っている、#if系のバックトラック
    DefineVA,             //関数マクロのパース、可変長
    DefineRparen,         //関数マクロのパース、閉じ括弧
    EndOfFile             //ファイル終端
  };

  struct pp_err_info {

    // 利便性のため
    pp_err_info()
      : token(pp_token_category::empty)
      , context(pp_parse_context::UnknownError)
    {}

    template<typename Token = pp_token>
    pp_err_info(Token&& lextoken, pp_parse_context err_from)
      : token(std::forward<Token>(lextoken))
      , context(err_from)
    {}

    pp_token token;
    pp_parse_context context;
  };

  using parse_status = kusabira::expected<pp_parse_status, pp_err_info>;

  /**
  * @brief パース中のエラーオブジェクトを作成する
  * @return エラー状態のparse_status
  */
  template<typename TokenIterator>
  ifn make_error(TokenIterator& it, pp_parse_context context) -> parse_status {
    return parse_status{tl::unexpect, pp_err_info{std::move(*it), context}};
  }



  /**
  * @brief ホワイトスペース列とブロックコメントを読み飛ばす
  * @return 有効なトークンを指していれば（終端に達していなければ）true
  */
  template <typename TokenIterator, typename Sentinel>
  cfn skip_whitespaces(TokenIterator& it, Sentinel end) -> bool {

    auto check_status = [](auto kind) -> bool {
      //ホワイトスペース列とブロックコメントはスルー
      return kind == pp_token_category::whitespace or kind == pp_token_category::block_comment;
    };

    do {
      ++it;
    } while (it != end and check_status(deref(it).category));

    return it != end;
  }

  /**
  * @brief 改行以外のホワイトスペースを飛ばす
  * @return 現れた非空白トークンを指すイテレータ
  */
  template <std::input_iterator TokenIterator, std::sentinel_for<TokenIterator> Sentinel>
  cfn skip_whitespaces_except_newline(TokenIterator&& it, Sentinel end) -> TokenIterator {
    return std::ranges::find_if_not(std::forward<TokenIterator>(it), end, [](const auto &token) {
      // トークナイズエラーはここでは考慮しないものとする
      return pp_token_category::whitespace <= token.category and token.category <= pp_token_category::block_comment;
    });
  }

//ホワイトスペース読み飛ばしのテンプレ
#define SKIP_WHITESPACE(iterator, sentinel)     \
  if (not skip_whitespaces(iterator, sentinel)) \
    return kusabira::ok(pp_parse_status::EndOfFile)

  /**
  * @brief トークン列をパースしてプリプロセッシングディレクティブの検出とCPPの実行を行う
  * @details 再帰下降構文解析によりパースする
  * @details パースしながらプリプロセスを実行し、成果物はプリプロセッシングトークン列
  * @details なるべく末尾再帰を意識したい
  * @details パースに伴ってはEOFの前に必ず改行が来る事を前提とする（トークナイザでそうなるはず）
  */
  template<
    std::ranges::input_range Tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>,
    typename ReporterFactory = report::reporter_factory<>
  >
  struct ll_paser {
    using iterator = std::ranges::iterator_t<Tokenizer>;
    using sentinel = std::ranges::sentinel_t<Tokenizer>;
    using pptoken_t = std::iter_value_t<iterator>;
    using pptoken_list_t = std::pmr::list<pptoken_t>;
    using reporter = typename ReporterFactory::reporter_t;

  private:

    Tokenizer m_tokenizer;
    pp_directive_manager m_preprocessor;
    fs::path m_filename;
    reporter m_reporter;
    pptoken_list_t m_pptoken_list{&kusabira::def_mr};

  public:

    ll_paser(fs::path filepath, report::report_lang lang = report::report_lang::ja)
      : m_tokenizer{filepath}
      , m_preprocessor{filepath}
      , m_filename{std::move(filepath)}
      , m_reporter(ReporterFactory::create(lang))
    {}

    fn get_phase4_result() const -> const pptoken_list_t& {
      return m_pptoken_list;
    }

    fn start() -> parse_status {
      auto it = std::ranges::begin(m_tokenizer);
      auto se = std::ranges::end(m_tokenizer);

      //空のファイル判定
      if (it == se) return {};
      if (auto kind = (*it).category; kind == pp_token_category::whitespace or kind == pp_token_category::block_comment) {
        //空に等しいファイルだった
        SKIP_WHITESPACE(it, se);
      }

      //モジュールファイルを識別
      if (auto& top_token = *it; top_token.category == pp_token_category::identifier) {
        using namespace std::string_view_literals;

        if (auto str = top_token.token.to_view(); str == u8"module" or str == u8"export") {
          //モジュールファイルとして読むため、終了後はそのまま終わり
          return this->module_file(it, se);
        }
      }

      //通常のファイル
      return this->group(it, se);
    }

    fn module_file([[maybe_unused]] iterator& it, [[maybe_unused]] sentinel end) -> parse_status {
      //未実装
      assert(false);
      //モジュール宣言とかを生成する
      return {pp_parse_status::Complete};
    }

    fn group(iterator& it, sentinel end) -> parse_status {
      parse_status status{ pp_parse_status::Complete };

      //ここでEOFチェックする意味ある？？？
      while (it != end and status == pp_parse_status::Complete) {
        status = group_part(it, end);
      }

      return status;
    }

    fn group_part(iterator& it, sentinel end) -> parse_status {
      using namespace std::string_view_literals;

      //ホワイトスペース列を読み飛ばす
      if (auto kind = (*it).category; kind == pp_token_category::whitespace or kind == pp_token_category::block_comment) {
        SKIP_WHITESPACE(it, end);
      }

      if (auto& token = *it; token.category == pp_token_category::op_or_punc) {
        //先頭#から始まればプリプロセッシングディレクティブ
        if (token.token == u8"#") {
          //ホワイトスペース列を読み飛ばす
          SKIP_WHITESPACE(it, end);

          //なにかしらの識別子が来てるはず
          if (auto& next_token = *it; next_token.category != pp_token_category::identifier) {
            if (next_token.category == pp_token_category::newline) {
              //空のプリプロセッシングディレクティブ
              return this->newline(it, end);
            }
            //エラーでは？？
            return make_error(it, pp_parse_context::GroupPart);
          }

          // #elif等の出現を調べるもの
          auto check_elif_etc = [](auto token) -> bool {
            return token == u8"elif" or token == u8"else" or token == u8"endif";
          };

          // #に続く識別子、何かしらのプリプロセッシングディレクティブ
          if ((*it).token.to_view().starts_with(u8"if")) {
            // if-sectionへ
            return this->if_section(it, end);
          } else if (check_elif_etc((*it).token) == true) {
            // if-sectionの内部でif-group読取中のelif等の出現、group読取の終了
            return kusabira::ok(pp_parse_status::FollowingSharpToken);
          } else {
            // control-lineへ
            return this->control_line(it, end);
          }
        }
      }

      if ((*it).category == pp_token_category::identifier) {
        if ((*it).token == u8"import"sv or (*it).token == u8"export"sv) {
            // モジュールのインポート宣言
            // pp-importに直接行ってもいい気がする
            return this->control_line(it, end);
        }
      }

      // text-lineへ
      return this->text_line(it, end);
    }

    fn control_line(iterator& it, sentinel end) -> parse_status {
      // 事前条件
      assert((*it).category == pp_token_category::identifier);
      assert(it != end);

      auto tokenstr = (*it).token.to_view();

      if (tokenstr == u8"include") {
        //#include、未実装
        assert(false);
      } else if (tokenstr == u8"define") {
        return this->control_line_define(it, end);
      } else if (tokenstr == u8"undef") {
        SKIP_WHITESPACE(it, end);
        if (deref(it).category != pp_token_category::identifier) {
          //マクロ名以外のものが指定されている
          m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
          return kusabira::error(pp_err_info{ std::move(*it), pp_parse_context::Define_InvalidDirective });
        }
        m_preprocessor.undef(deref(it).token);
        ++it;
      } else if (tokenstr == u8"line") {
        pptoken_list_t line_token_list{&kusabira::def_mr};

        //lineの次のトークンからプリプロセッシングトークンを構成する
        ++it;
        return this->pp_tokens<false>(it, end, line_token_list).and_then([&, this](auto&&) -> parse_status {
          if (auto is_err = m_preprocessor.line(*m_reporter, line_token_list); is_err) {
            return this->newline(it, end);
          } else {
            return kusabira::error(pp_err_info{ std::move(*it),  pp_parse_context::ControlLine });
          }
        });
      } else if (tokenstr == u8"error") {
        // #errorディレクティブの実行
        m_preprocessor.error(*m_reporter, it, end);
        //コンパイルエラー
        return kusabira::error(pp_err_info{ std::move(*it),  pp_parse_context::ControlLine_Error });
      } else if (tokenstr == u8"pragma") {
        // #pragmaディレクティブの実行
        m_preprocessor.pragma(*m_reporter, it, end);
      } else {
        // 知らないトークンだったら多分こっち
        return this->conditionally_supported_directive(it, end);
      }

      return this->newline(it, end);
    }

    fn control_line_define(iterator &it, sentinel end) -> parse_status {
      using namespace std::string_view_literals;
      //ホワイトスペース列を読み飛ばす
      SKIP_WHITESPACE(it, end);

      if ((*it).category != pp_token_category::identifier) {
        // #defineディレクティブのエラー、マクロ名が無い
        m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_No_Identifier);
        return kusabira::error(pp_err_info{std::move(*it),  pp_parse_context::Define_No_Identifier});
      }

      //マクロ名となるトークン
      auto macroname = std::move(*it);
      //次のトークンへ
      ++it;

      //関数マクロの場合、マクロ名と開き括弧の間にスペースは入らない
      //オブジェクトマクロは逆に必ずスペースが入る
      
      //オブジェクト形式マクロの登録
      if (deref(it).category == pp_token_category::whitespace) {
        //置換リストの取得
        return this->replacement_list(it, end).and_then([&, this](auto&& replacement_token_list) -> parse_status {
          //マクロの登録
          if (m_preprocessor.define(*m_reporter, macroname, std::move(replacement_token_list)) == false)
            return kusabira::error(pp_err_info{ std::move(macroname), pp_parse_context::ControlLine });

          return kusabira::ok(pp_parse_status::Complete);
        });
      } else if (deref(it).category == pp_token_category::newline) {
        // 空のオブジェクトマクロの登録
        if (m_preprocessor.define(*m_reporter, macroname, pptoken_list_t{ &kusabira::def_mr }) == false)
          return kusabira::error(pp_err_info{std::move(macroname), pp_parse_context::ControlLine});

        return this->newline(it, end);
      }
      
      //関数マクロの登録
      if (deref(it).category == pp_token_category::op_or_punc and deref(it).token == u8"("sv) {
        //引数リストの取得
        return this->identifier_list(it, end).and_then([&, this](auto&& value) -> parse_status {
          auto& [arg_status, arglist] = value;
          bool is_va = false;

          //可変長マクロ対応と引数リストパースの終了
          switch (arg_status) {
          case pp_parse_status::DefineRparen:
            //閉じ括弧を検出、次のトークンへ進める
            ++it;
            break;
          case pp_parse_status::DefineVA:
            //可変長引数を検出
            is_va = true;
            arglist.emplace_back(u8"...");
            //閉じかっこを探す
            SKIP_WHITESPACE(it, end);
            if (deref(it).category == pp_token_category::op_or_punc and deref(it).token == u8")"sv) {
              ++it;
            } else {
              //閉じ括弧が出現しない場合はエラー
              m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
              return kusabira::error(pp_err_info{ std::move(*it), pp_parse_context::Define_InvalidDirective });
            }
            break;
          default:
            //ここにきたらバグ
            assert(false);
          }

          //置換リストの取得
          return this->replacement_list(it, end).and_then([&, this](auto&& replacement_token_list) -> parse_status {
            //マクロの登録
            if (m_preprocessor.define(*m_reporter, macroname, std::move(replacement_token_list), arglist, is_va) == false)
              return kusabira::error(pp_err_info{ std::move(macroname), pp_parse_context::ControlLine });

            return kusabira::ok(pp_parse_status::Complete);
          });
        });
      }

      //エラー、#defineディレクティブが正しくない
      m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
      return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_InvalidDirective});
    }

    fn replacement_list(iterator &it, sentinel end) -> kusabira::expected<pptoken_list_t, pp_err_info>{
      pptoken_list_t list{std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr)};

      return this->pp_tokens<true>(it, end, list).map([&list](auto &&) -> pptoken_list_t {
        return list;
      });
    }

    fn identifier_list(iterator &it, sentinel end) -> kusabira::expected<std::pair<pp_parse_status, std::pmr::vector<std::u8string_view>>, pp_err_info> {
      using namespace std::string_view_literals;
      //仮引数文字列
      std::pmr::vector<std::u8string_view> param_list{ &kusabira::def_mr};
      param_list.reserve(10);

      for (;; ++it) {
        if (it = skip_whitespaces_except_newline(std::move(++it), end); deref(it).category == pp_token_category::newline) {
          // 改行に至るのは変
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_ParamlistBreak});
        }

        if (deref(it).category == pp_token_category::identifier) {
          //普通の識別子なら仮引数名
          param_list.emplace_back((*it).token);
        } else if (deref(it).category == pp_token_category::op_or_punc) {
          //記号が出てきたら可変長マクロか閉じ括弧の可能性がある
          if (deref(it).token == u8"..."sv) {
            return kusabira::ok(std::make_pair(pp_parse_status::DefineVA, std::move(param_list)));
          } else if ((*it).token == u8")"sv) {
            return kusabira::ok(std::make_pair(pp_parse_status::DefineRparen, std::move(param_list)));
          }

          //エラー：現れてはいけない記号が現れている
          m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_InvalidDirective});
        } else {
          //エラー：現れるべきトークンが現れなかった（ここのエラーコンテキスト、分けたほうが良くない？
          m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_InvalidDirective});
        }

        //仮引数リストの区切りとなるカンマを探す
        if (it = skip_whitespaces_except_newline(std::move(++it), end); deref(it).category == pp_token_category::newline) {
          // 改行に至るのは変
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_ParamlistBreak});
        }

        if (deref(it).category == pp_token_category::op_or_punc) {
          if (deref(it).token == u8","sv) {
            //残りの引数リストをパースする
            continue;
          }
          if ((*it).token == u8")"sv) {
            //関数マクロの終わり
            return kusabira::ok(std::make_pair(pp_parse_status::DefineRparen, std::move(param_list)));
          }
        }
  
        //エラー：現れるべきトークンが現れなかった
        m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
        return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_InvalidDirective});
      }
    }

    fn conditionally_supported_directive([[maybe_unused]] iterator& it, [[maybe_unused]] sentinel end) -> parse_status {
      //未実装
      assert(false);
      return {};
    }

    fn if_section(iterator& it, sentinel end) -> parse_status {
      using namespace std::string_view_literals;

      //ifを処理
      auto status = this->if_group(it, end);


      //#を読んだ上でここに来ているかをチェックするもの
      auto chack_status = [&status]() noexcept -> bool { return status == pp_parse_status::FollowingSharpToken; };

      //正常にここに戻った場合はすでに#を読んでいるはず
      if (chack_status() and (*it).token == u8"elif"sv) {
        //#elif
        status = this->elif_groups(it, end);
      }
      if (chack_status() and (*it).token == u8"else"sv) {
        //#else
        status = this->else_group(it, end);
      } 
      if (chack_status()) {
        //endif以外にありえない
        return this->endif_line(it, end);
      } else {
        //各セクションパース中のエラー、status == trueとなるときはどんな時だろう？
        return status ? make_error(it, pp_parse_context::IfSection) :
              status;
      }
    }

    fn if_group(iterator& it, sentinel end) -> parse_status {
      if (auto token = (*it).token.to_view(); size(token) == 3) {
        //#if

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        //定数式の処理

      } else if (token == u8"ifdef" or token == u8"ifndef") {
        //#ifdef #ifndef

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        if (auto kind = (*it).category; kind != pp_token_category::identifier) {
          //識別子以外が出てきたらエラー
          return make_error(it, pp_parse_context::IfGroup_Invalid);
        }
        //識別子を#define名としてチェックする

      } else {
        // #ifから始まるがifdefでもifndefでもない何か
        return make_error(it, pp_parse_context::IfGroup_Mistake);
      }

      //ホワイトスペース列を読み飛ばす
      SKIP_WHITESPACE(it, end);

      if (auto kind = (*it).category; kind != pp_token_category::newline) {
        //改行文字以外が出てきたらエラー
        return make_error(it, pp_parse_context::IfGroup_Invalid);
      }

      return this->group(it, end);
    }

    fn elif_groups(iterator& it, sentinel end) -> parse_status {
      parse_status status{ pp_parse_status::Complete };

      while (it != end and status == pp_parse_status::Complete) {
        status = elif_group(it, end);
      }

      return status;
    }

    fn elif_group(iterator& it, sentinel end) -> parse_status {
      //#elifを処理

      //定数式を処理

      //改行文字まで読みとばし


      return this->group(it, end);
    }

    fn else_group(iterator& it, sentinel end) -> parse_status {
      //#elseを処理
      
      //ホワイトスペース列を読み飛ばす
      SKIP_WHITESPACE(it, end);

      if (auto kind = (*it).category; kind != pp_token_category::newline) {
        //改行文字以外が出てきたらエラー
        return make_error(it, pp_parse_context::Newline_NotAppear);
      }

      return this->group(it, end);
    }

    fn endif_line(iterator& it, sentinel end) -> parse_status {
      //#endifを処理
      if (auto token = (*it).token.to_view(); token == u8"endif") {

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        //改行が現れなければならない
        if (it != end and (*it).category == pp_token_category::newline) {
          //正常終了
          return this->newline(it, end);
        } else {
          //他のトークンが出てきた
          return make_error(it, pp_parse_context::EndifLine_Invalid);
        }
      }
      //endifじゃない
      return make_error(it, pp_parse_context::EndifLine_Mistake);
    }

    fn text_line(iterator& it, sentinel end) -> parse_status {
      //1行分プリプロセッシングトークン列読み出し
      return this->pp_tokens<false>(it, end, this->m_pptoken_list);
    }

    /**
    * @brief プリプロセッシングトークン列を構成する
    * @tparam MacroExpandOff マクロの実引数パース時や#define時の置換リストパース中か否か、マクロ展開を実行せず、ホワイトスペースを残す
    * @param it 現在の先頭トークン
    * @param end トークン列の終端
    * @param list 結果を格納するリスト
    * @return {結果となるプリプロセッシングトークンリスト | エラー情報}
    */
    template<bool MacroExpandOff>
    fn pp_tokens(iterator& it, sentinel end, pptoken_list_t& list) -> parse_status {
      using namespace std::string_view_literals;

      //現在の字句トークンのカテゴリ
      auto lextoken_category = deref(it).category;

      //その行のプリプロセッシングトークン列を読み出す
      while (lextoken_category != pp_token_category::newline) {
        //トークナイザのエラーは別の所で処理するのでここでは考慮しない
        assert(pp_token_category::Unaccepted < lextoken_category);

        //プリプロセッシングトークン1つを作成する、終了後イテレータは未処理のトークンを指している
        if (auto result = this->construct_next_pptoken<MacroExpandOff>(it, end); result) {
          list.splice(list.end(), std::move(*result));
        } else {
          //エラーが起きてる
          return std::move(result).and_then([](auto &&) -> parse_status {
            //expectedを変換するためのものであって、実際にここが実行されることはない
            return parse_status{};
          });
        }

        lextoken_category = deref(it).category;
      }

      //改行で終了
      return this->newline(it, end);
    }

    /**
    * @brief プリプロセッシングトークン1つ（1部のものは複数）を構成する
    * @tparam MacroExpandOff マクロの実引数パース時や#define時の置換リストパース中か否か、マクロ展開を実行せず、ホワイトスペースを残す
    * @param it 現在の先頭トークン
    * @param it_end トークン列の終端
    * @return {結果となるプリプロセッシングトークンリスト | エラー情報}
    */
    template <bool MacroExpandOff = false, typename Iterator, typename Sentinel>
    fn construct_next_pptoken(Iterator &it, Sentinel it_end) -> kusabira::expected<pptoken_list_t, pp_err_info> {

      using namespace std::string_view_literals;

      pptoken_list_t pptoken_list{&kusabira::def_mr};

      //終了時にイテレータを進めておく（ここより深く潜らない場合にのみ進める）
      kusabira::vocabulary::scope_exit se_inc_itr = [&it]() {
        ++it;
      };

      //トークン読み出しとプリプロセッシングトークンの構成
      switch (deref(it).category) {
        case pp_token_category::identifier:
        {
          //マクロ引数の構築時はマクロ展開をしない
          if constexpr (not MacroExpandOff) {
            //識別子を処理、マクロ置換を行う
            if (auto opt = m_preprocessor.is_macro(deref(it).token); opt) {
              bool is_funcmacro = *opt;
              bool success, complete;
              // マクロ展開の結果リスト
              pptoken_list_t macro_result_list{&kusabira::def_mr};
              // 再帰的に同名のマクロ展開を行わないためのマクロ名メモ
              std::pmr::unordered_set<std::u8string_view> memo{ &kusabira::def_mr };
              // 現在注目しているマクロ名（すなわち、識別子）
              auto macro_name = std::move(*it);

              if (not is_funcmacro) {
                //オブジェクトマクロ置換
                std::tie(success, complete, macro_result_list, memo) = m_preprocessor.objmacro<true>(*m_reporter, macro_name);
              } else {
                //マクロ名の次へ進める
                ++it;
                //実引数リストの取得
                auto arg_list = this->funcmacro_args(it, it_end);
                if (not arg_list) {
                  pp_err_info& errinfo = arg_list.error();
                  if (errinfo.context == pp_parse_context::Funcmacro_NotInvoke) {
                    //マクロの呼び出しではなかった時、マクロ名を単に識別子として処理
                    pptoken_list.emplace_back(std::move(macro_name));
                    //itはマクロ名の後に出現した最初の非ホワイトスペーストークンを指している
                    se_inc_itr.release();
                    break;
                  } else {
                    //その他のエラーはexpectedを変換してそのまま返す
                    return std::move(arg_list).map([](auto &&) {
                      return pptoken_list_t{};
                    });
                  }
                }
                //関数マクロ置換
                std::tie(success, complete, macro_result_list, memo) = m_preprocessor.funcmacro<false>(*m_reporter, macro_name, *arg_list);
              }
              if (not success) {
                //マクロ実行時のエラーだが、報告済
                se_inc_itr.release();
                return kusabira::error(pp_err_info{ std::move(macro_name), pp_parse_context::ControlLine });
              }
              if (not complete) {
                //マクロ展開を継続
                auto comp_result = this->further_macro_replacement(std::move(macro_result_list), it, it_end, memo);
                macro_result_list = std::move(*comp_result);
              }

              // マクロの結果リストからホワイトスペースを取り除く
              std::erase_if(macro_result_list, [](const auto &pptoken) {
                return pptoken.category == pp_token_category::whitespace;
              });

              //置換後リストを末尾にspliceする
              pptoken_list.splice(std::end(pptoken_list), std::move(macro_result_list));
              break;
            }
          }
          //置換対象ではない
          pptoken_list.emplace_back(std::move(*it));
          break;
        }
        case pp_token_category::line_comment:  [[fallthrough]];
        case pp_token_category::block_comment: [[fallthrough]];
        case pp_token_category::whitespace:
          //マクロの引数パース時のみ処理が必要
          break;
        case pp_token_category::op_or_punc:
        {
          auto &&oppunc_list = longest_match_exception_handling(it, it_end);
          if (0u < oppunc_list.size()) {
            pptoken_list.splice(std::end(pptoken_list), std::move(oppunc_list));
          }
          //既に次のトークンを指しているので進めない
          se_inc_itr.release();
          break;
        }
        case pp_token_category::during_raw_string_literal:
        {
          //改行されている生文字列リテラルの1行目
          //生文字列リテラル全体を一つのトークンとして読み出す必要がある
          pptoken_list.emplace_back(read_rawstring_tokens(it, it_end));

          //次のトークンを調べてユーザー定義リテラルの有無を判断
          ++it;
          if (strliteral_classify(it, u8" "sv, pptoken_list.back()) == false) {
            //ファイル終端に到達した
            se_inc_itr.release();
          }
          break;
        }
        case pp_token_category::raw_string_literal: [[fallthrough]];
        case pp_token_category::string_literal:
        {
          //auto category = (deref(it).category == pp_token_category::RawStrLiteral) ? pp_token_category::raw_string_literal : pp_token_category::string_literal;
          auto& prev = pptoken_list.emplace_back(std::move(*it));

          //次のトークンを調べてユーザー定義リテラルの有無を判断
          ++it;
          if (strliteral_classify(it, prev.token, pptoken_list.back()) == false) {
            //ファイル終端に到達した
            se_inc_itr.release();
          }
          break;
        }
        case pp_token_category::empty:
          //無視
          break;
        case pp_token_category::newline:
          //来ないはず
          assert(false);
          break;
        default:
        {
          //基本はトークン1つを読み込んでプリプロセッシングトークンを構成する
          //auto category = tokenize_status_to_category(deref(it).category);
          pptoken_list.emplace_back(std::move(*it));
        }
      }

      return kusabira::ok(std::move(pptoken_list));
    }

    using expecetd_macro_args = kusabira::expected<std::pmr::vector<pptoken_list_t>, pp_err_info>;

    /**
    * @brief 関数マクロの実引数プリプロセッシングトークン列を読み出す
    * @param it 現在の先頭トークン
    * @param end トークン列の終端
    * @details 呼び出し開始の開きかっこ"("の直後から開始すること
    * @details 各引数トークン列はマクロ置換をせずにプリプロセッサへ転送する、引数内マクロはプリプロセッサ内で置換の直前に展開される
    * @return エラーが起きた場合その情報、正常終了すれば実引数リスト
    */
    template<std::ranges::range Range>
    fn funcmacro_args(Range&& rng) -> expecetd_macro_args {
      auto it = std::ranges::begin(rng);
      return this->funcmacro_args(it, std::ranges::end(rng));
    }

    /**
    * @brief 関数マクロの実引数プリプロセッシングトークンを構成する
    * @param it 現在の先頭トークン
    * @param end トークン列の終端
    * @details 呼び出し開始の開きかっこ"("の前から（マクロ名の次から）
    * @details 各引数トークン列はマクロ置換をせずにプリプロセッサへ転送する、引数内マクロはプリプロセッサ内で置換の直前に展開される
    * @return エラーが起きた場合その情報、正常終了すれば実引数リスト
    */
    template<typename Iterator, typename Sentinel>
    fn funcmacro_args(Iterator& it, Sentinel end) -> expecetd_macro_args {
      using namespace std::string_view_literals;

      //開きかっこまでトークンを進める（ここで改行すっ飛ばしていいの？
      it = std::ranges::find_if_not(std::move(it), end, [](const pptoken_t &token) {
        // トークナイズエラーはここでは考慮しないものとする
        return token.category <= pp_token_category::block_comment;
      });

      if (it == end) {
        // EOFはエラー
        return kusabira::error(pp_err_info{pptoken_t{pp_token_category::empty}, pp_parse_context::UnexpectedEOF});
      }

      if (deref(it).token != u8"("sv) {
        //開きかっこの出現前に他のトークンが出現している、エラーではなくマクロ呼び出しではなかっただけ
        return kusabira::error(pp_err_info{deref(it), pp_parse_context::Funcmacro_NotInvoke});
      }

      //開きかっこの次のトークンへ進める、少なくともここがEOFになることはない（改行がその前に来る）
      ++it;

      // 実引数パース中のエラーを保持する
      std::optional<pp_err_info> err{};

      auto args = parse_macro_args(it, end, [this, &err](Iterator& itr, Sentinel fin, auto& arg_list) -> bool {
        if (deref(itr).category == pp_token_category::newline) {
          //改行は空白文字として扱う、マクロ引数中の空白文字の並びは1つに圧縮される
          //マクロ呼び出し直後に改行来た時に死ぬよねこれ
          if (auto& prev_token = arg_list.back(); prev_token.category != pp_token_category::whitespace) {
            auto& gen_token = arg_list.emplace_back(std::move(*itr));
            gen_token.category = pp_token_category::whitespace;
            gen_token.token = u8" "sv;
          }
          ++itr;
          return true;
        } else if (deref(itr).category == pp_token_category::whitespace) {
          //マクロ引数中の空白文字の並びは1つに圧縮される
          if (not std::ranges::empty(arg_list)) {
            if (auto& prev_token = arg_list.back(); prev_token.category != pp_token_category::whitespace) {
              arg_list.emplace_back(std::move(*itr));
            }
          }
          ++itr;
          return true;
        }

        //実引数となるプリプロセッシングトークンを構成する
        if (auto result = this->construct_next_pptoken<true>(itr, fin); result) {
          arg_list.splice(arg_list.end(), std::move(*result));
        } else {
          //エラーが起きてる
          err = std::move(result).error();
          return false;
        }
        return true;
      });

      if (err) {
        return kusabira::error(*std::move(err));
      }

      //閉じかっこで終わってる筈なのでファイル終端に達するはずはない
      if (it == end) {
        return kusabira::error(pp_err_info{pptoken_t{pp_token_category::empty}, pp_parse_context::UnexpectedEOF});
      }

      return kusabira::ok(std::move(args));
    }

    /**
    * @brief マクロ展開結果の再処理時、含まれる関数マクロを適切に処理する
    * @param list マクロ展開処理途中の結果リスト、再スキャンと結果構成のために要素が移動される
    * @param it 現在の先頭トークン
    * @param end トークン列の終端
    * @param outer_macro 外側で呼び出されているマクロ名を記録したメモ
    * @details 第一段階のマクロ展開後のリストに対して再スキャンをかけさらに展開処理する際、関数マクロの呼び出しは改行等を無視するため（処理済みリストを超えて）未処理のプリプロセッシングトークン列を読み出す必要が生じる。
    * @details その必要のないマクロはプリプセッサ内で処理されるが、改行を超えて呼び出しを行っている関数マクロはここで処理する
    * @return エラーが起きた場合その情報、正常終了すればマクロ展開処理済みのプリプロセッシングトークンリスト
    */
    template<typename Iterator, typename Sentinel>
    fn further_macro_replacement(pptoken_list_t&& list, Iterator& it, Sentinel se, std::pmr::unordered_set<std::u8string_view>& outer_macro) -> kusabira::expected<pptoken_list_t, pp_err_info> {
      // 未処理トークン列のイテレータだけは参照を保持しておいてもらう
      using concat_ref = kusabira::vocabulary::concat<std::ranges::iterator_t<pptoken_list_t>, std::ranges::sentinel_t<pptoken_list_t>, Iterator &, Sentinel>;

      // 処理済みPPトークンリスト
      pptoken_list_t complete_list{&kusabira::def_mr};
      // マクロ名
      pp_token macro_name{ pp_token_category::empty };
      // 実引数リスト
      expecetd_macro_args arg_list{};
      // 展開途中の関数マクロを探す
      std::optional<bool> is_funcmacro{};
      // 未処理の先頭位置
      auto untreated_pos = list.begin();

      do {
        auto pos = std::ranges::find_if(untreated_pos, list.end(), [&](auto &token) {
          is_funcmacro = m_preprocessor.is_macro(token.token);
          return bool(is_funcmacro);
        });

        if (pos == list.end()) {
          // 見つかるはずのマクロが見つからなかった
          // 入力リストの中に必ず未処理の関数マクロがある、なければ他のマクロ処理部がおかしい
          return kusabira::error(pp_err_info{ std::move(*it), pp_parse_context::ControlLine });
        }
        // 必ず関数マクロのはず（オブジェクトマクロは処理済みなので出てこない）
        assert(*is_funcmacro);

        // マクロ位置までのトークンを移動する
        complete_list.splice(complete_list.end(), list, untreated_pos, pos);
        // 処理済みの位置を更新
        untreated_pos = pos;

        // マクロ名を記録
        // ムーブしたいがこれがマクロ呼び出しされているかはここではわからず、ループする可能性があるのでコピー
        macro_name = *pos;

        // リストの残りの関数マクロ呼び出しトークン列と未処理のPPトークン列を連結し、引数リストを構成する
        arg_list = this->funcmacro_args(concat_ref(pos, list.end(), it, se));
        if (not arg_list) {
          pp_err_info& errinfo = arg_list.error();
          if (errinfo.context == pp_parse_context::Funcmacro_NotInvoke) {
            //マクロの呼び出しではなかった時、マクロ名を単に識別子として処理
            complete_list.emplace_back(std::move(macro_name));
            //再スキャンする
            continue;
          } else {
            //なんか途中でエラー、expectedを変換してそのまま返す
            return std::move(arg_list).map([](auto &&) {
              return pptoken_list_t{};
            });
          }
        }
      } while (not arg_list);
      
      // 関数マクロ置換
      auto [success, complete, funcmacro_result] = m_preprocessor.funcmacro<true>(*m_reporter, macro_name, *arg_list, outer_macro);

      if (not success) {
        // なんか途中でエラー、expectedを変換してそのまま返す
        return kusabira::error(pp_err_info{ std::move(*it), pp_parse_context::ControlLine });
      }
      if (not complete) {
        // resultに対して再帰的に再スキャンする
        auto recursive_result = this->further_macro_replacement(std::move(funcmacro_result), it, se, outer_macro);
        if (not recursive_result) {
          return recursive_result;
        }
        // 全てのマクロ処理が完了した結果（となるはず
        funcmacro_result = std::move(*recursive_result);
      }

      // この関数で処理しているということは、入力リスト（list）の内部では収まらないマクロ呼び出しがあったということ
      // したがって、マクロ呼び出しが完了すれば入力リストは消費され尽くしていて、未処理トークンはマクロ呼び出し完了点（閉じ括弧）の次で止まってる
      // そのため、ここではマクロの置換結果として帰ってきたものだけを処理済みトークンとして扱えばいい

      // 展開結果をsplice
      complete_list.splice(complete_list.end(), std::move(funcmacro_result));

      return kusabira::ok(std::move(complete_list));
    }

    /**
    * @brief 改行処理を行う
    * @param it 現在の先頭トークン
    * @param end トークン列の終端
    * @return 1行分正常終了したことを表すステータス
    */
    fn newline(iterator& it, sentinel end) -> parse_status {
      // ホワイトスペース飛ばし
      it = skip_whitespaces_except_newline(std::move(it), end);

      // たとえ最終行でも、必ず改行がある
      assert(it != end);

      if ((*it).category != pp_token_category::newline) {
        //改行以外が出てきたらエラー
        return make_error(it, pp_parse_context::Newline_NotAppear);
      }

      //改行を保存
      this->m_pptoken_list.emplace_back(std::move(*it));
      //次の行の頭のトークンへ進めて戻る
      ++it;
      return kusabira::ok(pp_parse_status::Complete);
    }

    /**
    * @brief 文字・文字列リテラル直後のユーザー定義リテラルを検出する
    * @param it トークン列のイテレータ
    * @param prev_tokenstr 直前のトークン文字列
    * @param last_pptoken 直前のプリプロセッシングトークン
    * @details 直前が文字・文字列リテラルであることを前提に動作する、呼び出す場所に注意
    * @details trueで戻った場合itは処理済みのトークンを指している、falseで戻った場合itは未処理のトークンを指す
    * @return ユーザー定義リテラルの有無
    */
    template<typename Iterator = iterator>
    sfn strliteral_classify(const Iterator& it, std::u8string_view prev_tokenstr , pp_token& last_pptoken) -> bool {
      //以前のトークンが文字列リテラルなのか文字リテラルなのか判断
      //auto& last_pptoken = list.back();
      
      //一つ前のトークンの最後の文字が'なら文字リテラル
      if (prev_tokenstr.ends_with(u8'\'')) {
       last_pptoken.category = pp_token_category::charcter_literal;
      }

      //イテレータは既に一つ進められているものとして扱う
      if ((*it).category == pp_token_category::identifier) {
        using enum_int = std::underlying_type_t<decltype(last_pptoken.category)>;
        //文字列のすぐあとが識別子ならユーザー定義リテラル
        //どちらにせよ1つ進めれば適切なカテゴリになる
        last_pptoken.category = static_cast<pp_token_category>(static_cast<enum_int>(last_pptoken.category) + enum_int(1u));

        //ユーザー定義リテラルを文字列トークンに含める
        auto str = std::move(last_pptoken.token).to_string();
        str.append((*it).token);
        last_pptoken.token = std::move(str);

        return true;
      } else {
        //そのほかのトークンはそのまま処理してもらう
        return false;
      }
    }


    /**
    * @brief 生文字列リテラルを読み出し、改行継続を元に戻した上で連結する
    * @param it トークン列のイテレータ
    * @param end トークン列の終端イテレータ
    * @details itは処理済みトークンを指した状態で戻る
    * @return 構成した生文字列リテラルトークン
    */
    template<typename Iterator = iterator, typename Sentinel = sentinel>
    sfn read_rawstring_tokens(Iterator& it, Sentinel end) -> pptoken_t {

      //事前条件
      assert(it != end);
      assert((*it).category == pp_token_category::during_raw_string_literal);

      //生文字列リテラルの行継続を元に戻す
      auto undo_linecontinue = [](auto& iter, auto& string, std::size_t bias = 0) {
        //バックスラッシュによる行継続が行われていなければ何もしない
        if (not deref(iter).is_multiple_phlines()) return;

        //論理行オブジェクト
        auto& logical_line = deref(deref(iter).srcline_ref);
        //論理行文字列中に現在のトークンが現れるところを検索
        auto pos = logical_line.line.find_first_of(deref(iter).token);
        //これは起こりえないはず・・・
        assert(pos != std::u8string_view::npos);

        //挿入する行継続マーカとその長さ
        constexpr char8_t insert_str[] = u8"\\\n";
        constexpr auto insert_len = sizeof(insert_str) - 1;
        //行継続を修正したことによって追加された文字数の累積（行継続マーカーの文字数×修正回数）
        auto acc = 0u;

        //バックスラッシュ+改行を復元する
        for (auto newline_pos : logical_line.line_offset) {
          //newline_posは複数存在する場合は常にその論理行頭からの長さになっている
          if (pos <= newline_pos + acc) {
            //すでに構成済みの生文字列リテラルの長さ+改行継続された論理行での位置+それまで挿入したバックスラッシュと改行の長さ
            string.insert(bias + newline_pos + acc, insert_str, insert_len);
            acc += insert_len;
            pos = newline_pos + acc;
          }
        }
      };

      //プリプロセッシングトークン型を用意
      pptoken_t token{pp_token_category::raw_string_literal};
      //構成する字句トークンを保存しておくリスト（↑のメンバへの参照）
      auto& lextoken_list = token.composed_tokens;

      //現在の字句トークンを保存
      lextoken_list.push_front(*it);
      auto pos = token.composed_tokens.begin();

      //生文字列リテラルを追加していくバッファ
      std::pmr::u8string rawstr{(*it).token, &kusabira::def_mr };
      //1行目の行継続を戻す
      undo_linecontinue(it, rawstr);

      do {
        //次の行をチェックする
        ++it;
        if (it == end or (*it).category <= pp_token_category::Unaccepted) {
          //エラーかな
          return token;
        } else if ((*it).category != pp_token_category::newline) {
          //生文字列リテラルを改行する（DuringRawStrステータスは改行によって終了している）
          rawstr.push_back(u8'\n');
          std::size_t length = rawstr.length();

          //トークンをまとめて1つのPPトークンにする
          rawstr.append((*it).token);
          pos = lextoken_list.emplace_after(pos, *it);

          undo_linecontinue(it, rawstr, length);
        }
        //改行入力はスルーする

        //エラーを除いて、この3つのトークン種別以外が出てくることはないはず
        assert((*it).category == pp_token_category::raw_string_literal
                or (*it).category == pp_token_category::during_raw_string_literal
                or (*it).category == pp_token_category::newline);

      } while ((*it).category != pp_token_category::raw_string_literal);

      token.token = std::move(rawstr);

      return token;
    }

    /**
    * @brief 最長一致規則の例外処理
    * @param it トークン列のイテレータ
    * @param end トークン列の終端イテレータ
    * @details この関数の終了時、itは常に残りの未処理トークン列の先頭を指す
    * @return 構成した記号列トークン
    */
    template<typename Iterator = iterator, typename Sentinel = sentinel>
    sfn longest_match_exception_handling(Iterator& it, Sentinel) -> pptoken_list_t {

      //改行の出現をチェックする、改行ならtrue
      auto check_newline = [](auto& it) {
        return deref(it).category == pp_token_category::newline;
      };

      //事前条件
      assert((*it).category == pp_token_category::op_or_punc);

      //プリプロセッシングトークンを一時保存しておくリスト
      pptoken_list_t tmp_pptoken_list{&kusabira::def_mr};

      bool not_handle = deref(it).token.to_view() != u8"<:";
      //現在のプリプロセッシングトークン（<:）を保存
      tmp_pptoken_list.emplace_back(std::move(*it));
      //未処理のトークンを指した状態で帰るようにする
      ++it;

      if (not_handle) {
        //<:記号でなければ何もしない
        return tmp_pptoken_list;
      }

      //次のトークンをチェック、記号トークンでなければ処理の必要はない
      if (deref(it).category != pp_token_category::op_or_punc) {
        return tmp_pptoken_list;
      }

      //代替トークンの並び"<::>"はトークナイザで適切に2つのトークンに分割されているはず
      //そのため、あとは"<:::"のパターンをチェックし:以外の記号をスルーする
      not_handle = (*it).token.to_view() != u8":";

      //現在のプリプロセッシングトークン（:以外の記号）を保存
      tmp_pptoken_list.emplace_back(std::move(*it));
      //常に未処理のトークンを指した状態で帰るようにする
      ++it;

      //ここまで2つのトークンが保存されているはず
      assert(std::size(tmp_pptoken_list) == 2u);

      //:単体のトークンである場合のみ最長一致例外処理を行う必要がある
      if (not_handle or check_newline(it)) {
        return tmp_pptoken_list;
      }

      //ここにきているという事は"<:"":"という記号列が現れている
      //それを、"<"と"::"の2つのトークンにする

      auto lit = std::begin(tmp_pptoken_list);
      //前2つのトークンを"<"と"::"の2つのトークンに構成する
      (*lit).token = std::pmr::u8string{ u8"<", &kusabira::def_mr };
      ++lit;
      //2つ目のトークンを::にする
      (*lit).token = std::pmr::u8string{ u8"::", &kusabira::def_mr };
      //2トークンあるはず
      assert(std::size(tmp_pptoken_list) == 2u);

      return tmp_pptoken_list;
    }
  };

} // namespace kusabira::PP