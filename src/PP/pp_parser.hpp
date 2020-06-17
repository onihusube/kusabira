#pragma once

#include <list>
#include <forward_list>
#include <utility>
#include <cassert>

#include "../common.hpp"
#include "file_reader.hpp"
#include "pp_automaton.hpp"
#include "pp_tokenizer.hpp"
#include "pp_directive_manager.hpp"

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

    template<typename Token = lex_token>
    pp_err_info(Token&& lextoken, pp_parse_context err_from)
      : token(std::forward<Token>(lextoken))
      , context(err_from)
    {}

    lex_token token;
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
      return kind == pp_tokenize_status::Whitespaces or kind == pp_tokenize_status::BlockComment;
    };

    do {
      ++it;
    } while (it != end and check_status(deref(it).kind));

    return it != end;
  }


//ホワイトスペース読み飛ばしのテンプレ
#define SKIP_WHITESPACE(iterator, sentinel)     \
  if (not skip_whitespaces(iterator, sentinel)) \
    return kusabira::ok(pp_parse_status::EndOfFile)

//トークナイザのエラーチェック
#define TOKNIZE_ERR_CHECK(iterator)                                               \
  if (auto status_kind = (*iterator).kind.status; status_kind < pp_tokenize_status::Unaccepted) \
  return make_error(iterator, pp_parse_context{static_cast<std::underlying_type_t<decltype(status_kind)>>(status_kind)})

//イテレータを一つ進めるとともに終端チェック
#define EOF_CHECK(iterator, sentinel) ++it; if (iterator == sentinel) return {pp_parse_status::EndOfFile}



  using std::begin;
  using std::end;

  /**
  * @brief トークン列をパースしてプリプロセッシングディレクティブの検出とCPPの実行を行う
  * @details 再帰下降構文解析によりパースする
  * @details パースしながらプリプロセスを実行し、成果物はプリプロセッシングトークン列
  * @details なるべく末尾再帰を意識したい
  */
  template<
    typename Tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>,
    typename ReporterFactory = report::reporter_factory<>
  >
  struct ll_paser {
    //using Tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;
    using iterator = decltype(begin(std::declval<Tokenizer &>()));
    using sentinel = decltype(end(std::declval<Tokenizer&>()));
    using pptoken_conteiner = std::pmr::list<pp_token>;
    using reporter = typename ReporterFactory::reporter_t;

    pptoken_conteiner pptoken_list{std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr)};
    pp_directive_manager preprocessor{};

  private:

    fs::path m_filename{};
    reporter m_reporter;

  public:

    ll_paser(report::report_lang lang = report::report_lang::ja)
      : m_reporter(ReporterFactory::create(lang))
    {}

    fn start(Tokenizer& pp_tokenizer) -> parse_status {
      auto it = begin(pp_tokenizer);
      auto se = end(pp_tokenizer);

      //空のファイル判定
      if (it == se) return {};
      if (auto kind = (*it).kind; kind == pp_tokenize_status::Whitespaces or kind == pp_tokenize_status::BlockComment) {
        //空に等しいファイルだった
        SKIP_WHITESPACE(it, se);
      }

      //モジュールファイルを識別
      if (auto& top_token = *it; top_token.kind == pp_tokenize_status::Identifier) {
        using namespace std::string_view_literals;

        if (auto str = top_token.token; str == u8"module" or str == u8"export") {
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
      if (auto kind = (*it).kind; kind == pp_tokenize_status::Whitespaces or kind == pp_tokenize_status::BlockComment) {
        SKIP_WHITESPACE(it, end);
      }

      if (auto& token = *it; token.kind == pp_tokenize_status::OPorPunc) {
        //先頭#から始まればプリプロセッシングディレクティブ
        if (token.token == u8"#") {
          //ホワイトスペース列を読み飛ばす
          SKIP_WHITESPACE(it, end);

          //なにかしらの識別子が来てるはず
          if (auto& next_token = *it; next_token.kind != pp_tokenize_status::Identifier) {
            if (next_token.kind == pp_tokenize_status::NewLine) {
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
          if ((*it).token.starts_with(u8"if")) {
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

      if ((*it).kind == pp_tokenize_status::Identifier) {
        if ((*it).token == u8"import" or (*it).token == u8"export") {
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
      assert((*it).kind == pp_tokenize_status::Identifier);
      assert(it != end);

      auto tokenstr = (*it).token;

      if (tokenstr == u8"include") {
        //#include、未実装
        assert(false);
      } else if (tokenstr == u8"define") {
        return this->control_line_define(it, end).and_then([&, this](auto&&) {
          return this->newline(it, end);
        });
      } else if (tokenstr == u8"undef") {
        SKIP_WHITESPACE(it, end);
        if (deref(it).kind != pp_tokenize_status::Identifier) {
          //マクロ名以外のものが指定されている
          m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
          return kusabira::error(pp_err_info{ std::move(*it), pp_parse_context::Define_InvalidDirective });
        }
        preprocessor.undef(deref(it).token);
      } else if (tokenstr == u8"line") {
        pptoken_conteiner line_token_list{ &kusabira::def_mr };

        //lineの次のトークンからプリプロセッシングトークンを構成する
        ++it;
        return this->pp_tokens(it, end, line_token_list).and_then([&, this](auto&&) -> parse_status {
          if (auto is_err = preprocessor.line(*m_reporter, line_token_list); is_err) {
            return this->newline(it, end);
          } else {
            return kusabira::error(pp_err_info{ std::move(*it),  pp_parse_context::ControlLine });
          }
        });
      } else if (tokenstr == u8"error") {
        // #errorディレクティブの実行
        preprocessor.error(*m_reporter, it, end);
        //コンパイルエラー
        return kusabira::error(pp_err_info{ std::move(*it),  pp_parse_context::ControlLine_Error });
      } else if (tokenstr == u8"pragma") {
        // #pragmaディレクティブの実行
        preprocessor.pragma(*m_reporter, it, end);
      } else {
        // 知らないトークンだったら多分こっち
        return this->conditionally_supported_directive(it, end);
      }

      return this->newline(it, end);
    }

    fn control_line_define(iterator &it, sentinel end) -> parse_status {
      //ホワイトスペース列を読み飛ばす
      SKIP_WHITESPACE(it, end);

      if ((*it).kind != pp_tokenize_status::Identifier) {
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
      if (deref(it).kind == pp_tokenize_status::Whitespaces) {
        //置換リストの取得
        return this->replacement_list(it, end).and_then([&, this](auto&& replacement_token_list) -> parse_status {
          //マクロの登録
          if (preprocessor.define(*m_reporter, macroname, std::move(replacement_token_list)) == false)
            return kusabira::error(pp_err_info{ std::move(macroname), pp_parse_context::ControlLine });

          return kusabira::ok(pp_parse_status::Complete);
        });
      }
      
      //関数マクロの登録
      if (deref(it).kind == pp_tokenize_status::OPorPunc and deref(it).token == u8"(") {
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
            if (deref(it).kind == pp_tokenize_status::OPorPunc and deref(it).token == u8")") {
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
            if (preprocessor.define(*m_reporter, macroname, std::move(replacement_token_list), arglist, is_va) == false)
              return kusabira::error(pp_err_info{ std::move(macroname), pp_parse_context::ControlLine });

            return kusabira::ok(pp_parse_status::Complete);
          });
        });
      }

      //エラー、#defineディレクティブが正しくない
      m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
      return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_InvalidDirective});
    }

    fn replacement_list(iterator &it, sentinel end) -> kusabira::expected<pptoken_conteiner, pp_err_info>{
      pptoken_conteiner list{std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr)};
      
      return this->pp_tokens(it, end, list).map([&list](auto&&) -> pptoken_conteiner {
        return list;
      });
    }

    fn identifier_list(iterator &it, sentinel end) -> kusabira::expected<std::pair<pp_parse_status, std::pmr::vector<std::u8string_view>>, pp_err_info> {
      //仮引数文字列
      std::pmr::vector<std::u8string_view> param_list{ &kusabira::def_mr};
      param_list.reserve(10);

      for (;; ++it) {
        if (skip_whitespaces(it, end) == false)
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::UnexpectedEOF});

        if (deref(it).kind == pp_tokenize_status::Identifier) {
          //普通の識別子なら仮引数名
          param_list.emplace_back((*it).token);
        } else if (deref(it).kind == pp_tokenize_status::OPorPunc) {
          //記号が出てきたら可変長マクロか閉じ括弧の可能性がある
          if (deref(it).token == u8"...") {
            return kusabira::ok(std::make_pair(pp_parse_status::DefineVA, std::move(param_list)));
          } else if ((*it).token == u8")") {
            return kusabira::ok(std::make_pair(pp_parse_status::DefineRparen, std::move(param_list)));
          }

          //エラー：現れてはいけない記号が現れている
          m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_InvalidDirective});
        } else {
          //エラー：現れるべきトークンが現れなかった
          m_reporter->pp_err_report(m_filename, *it, pp_parse_context::Define_InvalidDirective);
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::Define_InvalidDirective});
        }

        //仮引数リストの区切りとなるカンマを探す
        if (skip_whitespaces(it, end) == false)
          return kusabira::error(pp_err_info{std::move(*it), pp_parse_context::UnexpectedEOF});

        if (deref(it).kind == pp_tokenize_status::OPorPunc) {
          if (deref(it).token == u8",") {
            //残りの引数リストをパースする
            continue;
          }
          if ((*it).token == u8")") {
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
      //ifを処理
      auto status = this->if_group(it, end);


      //#を読んだ上でここに来ているかをチェックするもの
      auto chack_status = [&status]() noexcept -> bool { return status == pp_parse_status::FollowingSharpToken; };

      //正常にここに戻った場合はすでに#を読んでいるはず
      if (chack_status() and (*it).token == u8"elif") {
        //#elif
        status = this->elif_groups(it, end);
      }
      if (chack_status() and (*it).token == u8"else") {
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
      if (auto token = (*it).token; token.length() == 3) {
        //#if

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        //定数式の処理

      } else if (token == u8"ifdef" or token == u8"ifndef") {
        //#ifdef #ifndef

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        if (auto kind = (*it).kind; kind != pp_tokenize_status::Identifier) {
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

      if (auto kind = (*it).kind; kind != pp_tokenize_status::NewLine) {
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

      if (auto kind = (*it).kind; kind != pp_tokenize_status::NewLine) {
        //改行文字以外が出てきたらエラー
        return make_error(it, pp_parse_context::ElseGroup);
      }

      return this->group(it, end);
    }

    fn endif_line(iterator& it, sentinel end) -> parse_status {
      //#endifを処理
      if (auto token = (*it).token; token == u8"endif") {

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        //改行が現れなければならない
        if (it != end and (*it).kind == pp_tokenize_status::NewLine) {
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
      return this->pp_tokens(it, end, this->pptoken_list);
    }

    fn pp_tokens(iterator& it, sentinel end, pptoken_conteiner& list) -> parse_status {
      using namespace std::string_view_literals;
      //プリプロセッシングトークン列を読み出す
      auto lextoken_category = (*it).kind;
      while (lextoken_category != pp_tokenize_status::NewLine) {
        //トークナイザのエラーならそこで終わる
        if (lextoken_category < pp_tokenize_status::Unaccepted) break;

        //トークン読み出しとプリプロセッシングトークンの構成
        switch ((*it).kind.status) {
          case pp_tokenize_status::Identifier:
          {
            //識別子を処理、マクロ置換を行う
            if (auto opt = preprocessor.is_macro(deref(it).token); opt) {
              bool is_funcmacro = *opt;
              if (not is_funcmacro) {
                //オブジェクトマクロ置換
                if (auto res_list = preprocessor.objmacro(deref(it)); res_list) {
                  //置換後リストを末尾にspliceする
                  list.splice(std::end(list), std::move(*res_list));
                }
              } else {
                //関数マクロ
                //マクロ名を取得し次へ
                auto macro_name = deref_inc(it);
                //実引数リストの取得
                auto&& arg_list = this->funcmacro_args(it, end);

                if (auto [success, res_list] = preprocessor.funcmacro(*m_reporter, macro_name, *arg_list); success and res_list) {
                  //置換後リストを末尾にspliceする
                  list.splice(std::end(list), std::move(*res_list));
                } else if (not success) {
                  return kusabira::error(pp_err_info{std::move(macro_name), pp_parse_context::ControlLine});
                }
              }
            } else {
              //置換対象ではない
              list.emplace_back(pp_token_category::identifier, std::move(*it));
            }
            break;
          }
          case pp_tokenize_status::LineComment:  [[fallthrough]];
          case pp_tokenize_status::BlockComment: [[fallthrough]];
          case pp_tokenize_status::Whitespaces:  [[fallthrough]];
          case pp_tokenize_status::Empty:
            //これらのトークンは無視する
            break;
          case pp_tokenize_status::OPorPunc:
          {
            auto&& oppunc_list = longest_match_exception_handling(it, end);
            if (0u < oppunc_list.size()) {
              list.splice(std::end(list), std::move(oppunc_list));
            }
            if (it == end) {
              return { pp_parse_status::EndOfFile };
            }
            lextoken_category = (*it).kind;
            continue;
          }
          case pp_tokenize_status::DuringRawStr:
          {
            //生文字列リテラル全体を一つのトークンとして読み出す必要がある
            auto&& tmp_pptoken = read_rawstring_tokens(it, end);

            TOKNIZE_ERR_CHECK(it);

            list.emplace_back(std::move(tmp_pptoken));

            EOF_CHECK(it, end);

            //次のトークンを調べてユーザー定義リテラルの有無を判断
            if (strliteral_classify(it, u8" "sv, list.back()) == true) {
              //そのままおわる
              break;
            } else {
              //falseの時は次のトークンを通常の手順で処理して頂く
              lextoken_category = (*it).kind;
              continue;
            }
            break;
          }
          case pp_tokenize_status::RawStrLiteral: [[fallthrough]];
          case pp_tokenize_status::StringLiteral:
          {
            auto category = (lextoken_category == pp_tokenize_status::RawStrLiteral) ? pp_token_category::raw_string_literal : pp_token_category::string_literal;
            auto& prev = list.emplace_back(category, std::move(*it));

            EOF_CHECK(it, end);

            //次のトークンを調べてユーザー定義リテラルの有無を判断
            if (strliteral_classify(it, (*prev.lextokens.begin()).token, list.back()) == true) {
              //そのままおわる
              break;
            } else {
              //falseの時は次のトークンを通常の手順で処理して頂く
              lextoken_category = (*it).kind;
              continue;
            }
          }
          default:
          {
            //基本はトークン1つを読み込んでプリプロセッシングトークンを構成する
            auto category = tokenize_status_to_category(lextoken_category.status);
            list.emplace_back(category, std::move(*it));
          }
        }

        //トークンを進める
        EOF_CHECK(it, end);
        lextoken_category = (*it).kind;
      }

      TOKNIZE_ERR_CHECK(it);

      //改行で終了しているはず
      return this->newline(it, end);
    }

    fn coonvert_pp_token(iterator &it, pptoken_conteiner &list)->kusabira::expected<pp_token, pp_err_info> {
      
    }

    fn funcmacro_args(iterator& it, sentinel) -> kusabira::expected<std::pmr::vector<std::pmr::list<pp_token>>, pp_err_info> {
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
      while (deref(it).category == pp_tokenize_status::OPorPunc and deref(it).token == u8")") {

      }
      return kusabira::ok(std::move(args));
    }

    /**
    * @brief 改行処理を行う
    * @param it 現在の先頭トークン
    * @param end トークン列の終端
    * @return 1行分正常終了したことを表すステータス
    */
    fn newline(iterator& it, sentinel end) -> parse_status {
      //事前条件
      assert(it != end);
      assert((*it).kind == pp_tokenize_status::NewLine);

      //プリプロセッサへ行が進んだことを伝える
      //preprocessor.newline();
      //次の行の頭のトークンへ進めて戻る
      ++it;
      return kusabira::ok(pp_parse_status::Complete);
    }


    /**
    * @brief トークナイザの出力分類をプリプセッシングトークン分類に変換する
    * @param status トークナイザの出力分類
    * @return プリプセッシングトークン分類
    */
    sfn tokenize_status_to_category(pp_tokenize_status status) -> pp_token_category {
      switch (status)
      {
      case kusabira::PP::pp_tokenize_status::Identifier:
        return pp_token_category::identifier;
      case kusabira::PP::pp_tokenize_status::NumberLiteral:
        return pp_token_category::pp_number;
      case kusabira::PP::pp_tokenize_status::OPorPunc:
        return pp_token_category::op_or_punc;
      case kusabira::PP::pp_tokenize_status::OtherChar:
        return pp_token_category::other_character;
      case kusabira::PP::pp_tokenize_status::NewLine:
        return pp_token_category::newline;
      default:
        //ここに来たら上でバグってる
        assert(false);
        return pp_token_category{};
        break;
      }
    }


    /**
    * @brief 文字・文字列リテラル直後のユーザー定義リテラルを検出する
    * @param it トークン列のイテレータ
    * @param prev_tokenstr 直前のトークン文字列
    * @param last_pptoken 直前のプリプロセッシングトークン
    * @details 直前が文字・文字列リテラルであることを前提に動作する、呼び出す場所に注意
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
      if ((*it).kind == pp_tokenize_status::Identifier) {
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
    * @return 構成した生文字列リテラルトークン
    */
    template<typename Iterator = iterator, typename Sentinel = sentinel>
    sfn read_rawstring_tokens(Iterator& it, Sentinel end) -> pp_token {

      //事前条件
      assert(it != end);
      assert((*it).kind == pp_tokenize_status::DuringRawStr or (*it).kind < pp_tokenize_status::Unaccepted);

      //生文字列リテラルの行継続を元に戻す
      auto undo_rawstr = [](auto& iter, auto& string, std::size_t bias = 0) {
        if ((*iter).is_multiple_phlines()) {
          //バックスラッシュによる行継続が行われている
          auto &line = *(*iter).srcline_ref;

          auto pos = line.line.find_first_of((*iter).token);
          auto acc = 0u;
          //これは起こりえないはず・・・
          assert(pos != std::u8string_view::npos);

          //バックスラッシュ+改行を復元する
          for (auto newline_pos : line.line_offset) {
            //newline_posは複数存在する場合は常にその論理行頭からの長さになっている
            if (pos <= newline_pos + acc) {
              //挿入する行継続マーカとその長さ
              constexpr char8_t insert_str[] = u8"\\\n";
              constexpr auto insert_len = sizeof(insert_str) - 1;
              //すでに構成済みの生文字列リテラルの長さ+改行継続された論理行での位置+それまで挿入したバックスラッシュと改行の長さ
              string.insert(bias + newline_pos + acc, insert_str, insert_len);
              acc += insert_len;
              pos = newline_pos + acc;
            }
          }
        }
      };

      //プリプロセッシングトークン型を用意
      pp_token token{pp_token_category::raw_string_literal};
      auto& list = token.lextokens;

      list.push_front(*it);
      auto pos = token.lextokens.begin();

      //生文字列リテラルを構成する
      std::pmr::u8string rawstr{(*it).token, std::pmr::polymorphic_allocator<char8_t>{&kusabira::def_mr}};
      //1行目を戻す
      undo_rawstr(it, rawstr);

      do {
        ++it;
        if (it == end or (*it).kind < pp_tokenize_status::Unaccepted) {
          //エラーかな
          return token;
        } else if ((*it).kind != pp_tokenize_status::NewLine) {
          //生文字列リテラルを改行する
          rawstr.push_back(u8'\n');
          std::size_t length = rawstr.length();

          //トークンをまとめて1つのPPトークンにする
          rawstr.append((*it).token);
          pos = list.emplace_after(pos, *it);

          undo_rawstr(it, rawstr, length);
        }
        //改行入力はスルーする

        //エラーを除いて、この3つ以外のトークン種別が出てくることはないはず
        assert((*it).kind == pp_tokenize_status::RawStrLiteral
                or (*it).kind == pp_tokenize_status::DuringRawStr
                or (*it).kind == pp_tokenize_status::NewLine);

      } while ((*it).kind != pp_tokenize_status::RawStrLiteral);

      token.token = std::move(rawstr);

      return token;
    }

    /**
    * @brief 最長一致規則の例外処理
    * @param it トークン列のイテレータ
    * @param end トークン列の終端イテレータ
    * @return 構成した記号列トークン
    */
    template<typename Iterator = iterator, typename Sentinel = sentinel>
    sfn longest_match_exception_handling(Iterator& it, Sentinel end) -> pptoken_conteiner {

      //事前条件
      assert(it != end);
      assert((*it).kind == pp_tokenize_status::OPorPunc);

      //プリプロセッシングトークンを一時保存しておくリスト
      pptoken_conteiner tmp_pptoken_list{std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr)};

      bool is_not_handle = (*it).token != u8"<:";
      //現在のプリプロセッシングトークンを保存
      tmp_pptoken_list.emplace_back(pp_token_category::op_or_punc, std::move(*it));
      //未処理のトークンを指した状態で帰るようにする
      ++it;

      if (is_not_handle) {
        //<:記号でなければ何もしない
        return tmp_pptoken_list;
      }

      //次のトークンをチェック、記号トークンでなければ処理の必要はない
      if (it == end or (*it).kind != pp_tokenize_status::OPorPunc) {
        return tmp_pptoken_list;
      }

      //代替トークンの並び"<::>"はトークナイザで適切に2つのトークンに分割されているはず
      //そのため、あとは"<:::"のパターンと:以外の記号をスルーする
      is_not_handle = (*it).token != u8":";

      //現在のプリプロセッシングトークンを保存
      tmp_pptoken_list.emplace_back(pp_token_category::op_or_punc, std::move(*it));
      //常に未処理のトークンを指した状態で帰るようにする
      ++it;

      //ここまで2つのトークンが保存されているはず
      assert(std::size(tmp_pptoken_list) == 2u);

      //:単体のトークンである場合のみ最長一致例外処理を行う必要がある
      if (is_not_handle or it == end) {
        return tmp_pptoken_list;
      }

      //ここにきているという事は"<:"":"という記号列が現れている
      //それを、"<"と"::"の2つのトークンにする

      auto lit = std::begin(tmp_pptoken_list);
      //前2つのトークンを"<"と"::"の2つのトークンに構成する
      (*lit).token = std::pmr::u8string{ u8"<", std::pmr::polymorphic_allocator<char8_t>{&kusabira::def_mr} };
      ++lit;
      //2つ目のトークンを::にする
      (*lit).token = std::pmr::u8string{ u8"::", std::pmr::polymorphic_allocator<char8_t>{&kusabira::def_mr} };
      //2トークンあるはず
      assert(std::size(tmp_pptoken_list) == 2u);

      return tmp_pptoken_list;
    }
  };

} // namespace kusabira::PP