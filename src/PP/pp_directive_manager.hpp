#pragma once

#include <charconv>
#include <functional>
#include <algorithm>
#include <tuple>
#include <chrono>

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

  /**
  * @brief トークンリスト上の最初の対応する閉じ括弧")"の位置を探索する
  * @details 開きかっこ"("の次のトークンから始めるものとして、間に現れているかっこのペアを無視する
  * @param it 探索開始位置のイテレータ
  * @param end 探索範囲の終端（[it, end)
  * @return 見つかった位置を指すイテレータ
  */
  template<std::forward_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
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
  * @brief プリプロセッシングトークン列の文字列化を行う
  * @param it 文字列化対象のトークン列の先頭
  * @param end 文字列化対象のトークン列の終端
  * @return 文字列化されたトークン（列）、必ず1要素になる
  */
  template<bool IsVA, std::ranges::bidirectional_range R>
  ifn pp_stringize(R&& range) -> std::pmr::list<pp_token> {
    using namespace std::string_view_literals;

    //結果のリスト
    std::pmr::list<pp_token> result{ &kusabira::def_mr };
    //文字列トークン
    pp_token& first = result.emplace_back(PP::pp_token_category::string_literal);

    //トークン列を順に文字列化するための一時文字列
    std::pmr::u8string str{u8"\""sv , &kusabira::def_mr};
    //字句トークン列挿入位置
    auto insert_pos = first.composed_tokens.before_begin();

    // 先頭と末尾のホワイトスペースとプレイスメーカートークンを無視する
    auto skip = [](const auto& token) {
      return token.category == pp_token_category::whitespaces or token.category == pp_token_category::placemarker_token;
    };

    for (auto& pptoken : range | std::views::drop_while(skip)
                               | std::views::reverse
                               | std::views::drop_while(skip)
                               | std::views::reverse)
    {
      // プレイスメーカートークンはいないものとする
      if (pptoken.category == pp_token_category::placemarker_token) continue;

      auto&& tmp = pptoken.token.to_string();

      // "と\ をエスケープする必要がある
      if (auto cat = pptoken.category; pp_token_category::charcter_literal <= cat and cat <= pp_token_category::user_defined_raw_string_literal) {
        // バックスラッシュをエスケープする
        auto bpos = tmp.find(u8'\\');
        while (bpos != std::u8string::npos) {
          tmp.replace(bpos, 1, u8R"(\\)");
          bpos = tmp.find(u8'\\', bpos + 2);
        }

        if (pp_token_category::string_literal <= cat) {
          // "をエスケープする
          auto lpos = tmp.find(u8'"');
          tmp.replace(lpos, 1, u8R"(\")");
          auto rpos = tmp.rfind(u8'"');
          tmp.replace(rpos, 1, u8R"(\")");
        }
      }
      str.append(std::move(tmp));

      if constexpr (IsVA) {
        // カンマの後にスペースを補う
        if (pptoken.token == u8","sv) {
          str.append(u8" ");
        }
      }

      // 文字列化したプリプロセッシングトークンを構成するトークン列を保存しておく
      insert_pos = first.composed_tokens.insert_after(insert_pos, pptoken);
    }

    str.append(u8"\"");
    first.token = std::move(str);

    return result;
  }

  /**
  * @brief 関数マクロ呼び出しの実引数を取得する
  * @param it 関数マクロ呼び出しの(の次の位置
  * @param end 関数マクロ呼び出しの)を含むような範囲の終端
  * @param other_token_func 記号以外のトークンを処理する関数オブジェクト
  * @return 1つの引数を表すlistを引数分保持したvector
  */
  template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel, std::invocable<Iterator&, Sentinel, std::pmr::list<pp_token>&> F>
    requires std::same_as<std::iter_value_t<Iterator>, pp_token> and
             std::convertible_to<std::invoke_result_t<F, Iterator&, Sentinel, std::pmr::list<pp_token>&>, bool>
  ifn parse_macro_args(Iterator& it, Sentinel fin, F&& other_token_func) -> std::pmr::vector<std::pmr::list<pp_token>> {
    using namespace std::string_view_literals;

    // 最初の非ホワイトスペーストークンまで進める
    it = std::ranges::find_if_not(std::move(it), fin, [](const pp_token &token) {
      // ここにくるものはプリプロセッシングトークンとして妥当なものであるはず
      // 改行はホワイトスペースになってるはずだし、それ以外に無視すべきトークンは残ってないはず
      return token.category <= pp_token_category::block_comment;
    });

    // 見つけた実引数列
    std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};

    // こうなってたら何か変なきがする
    if (it == fin) return args;

    // とりあえず引数10個分確保しておく
    args.reserve(10);

    // 実引数1つ分のリスト、作業用
    std::pmr::list<pp_token> arg_list{&kusabira::def_mr};
    // 実引数リストの区切りカンマまでの間に出現したネストかっこの数
    std::size_t inner_paren = 0;

    while (it != fin) {
      if (deref(it).category == pp_token_category::op_or_punc) {
        //カンマの出現で1つの実引数のパースを完了する
        if (inner_paren == 0 and deref(it).token == u8","sv) {
          args.emplace_back(std::move(arg_list));
          //要らないけど、一応
          arg_list = std::pmr::list<pp_token>{&kusabira::def_mr};
          //カンマは保存しない
          ++it;
          //カンマ直後のホワイトスペースは飛ばす
          it = std::ranges::find_if_not(std::move(it), fin, [](const pp_token& token) {
            return token.category <= pp_token_category::block_comment;
          });
          continue;
        } else if (deref(it).token == u8"("sv) {
          //かっこの始まり
          ++inner_paren;
        } else if (deref(it).token == u8")"sv) {
          //マクロ終了の閉じかっこ判定
          if (inner_paren == 0) {
            args.emplace_back(std::move(arg_list));
            break;
          }
          //入れ子閉じかっこ
          --inner_paren;
        }
      }

      // その他の種別のトークンを処理する、外部から渡された関数オブジェクトに委譲
      if (other_token_func(it, fin, arg_list) == false) {
        // エラーが起きていたらそこで終わる、詳細なエラーハンドリングは外の責任
        return args;
      }
    }

    // 実引数の最後が空だとしても空のリストが引数として追加される
    // args.emplace_back(std::move(arg_list));

    return args;
  }
  
  
  /**
  * @brief 関数マクロ呼び出しの実引数を取得する
  * @param it 関数マクロ呼び出しの(の次の位置
  * @param end 関数マクロ呼び出しの)を含むような範囲の終端
  * @details マクロ展開の途中と最後のタイミングで含まれるマクロを再帰的に展開する時を想定しているので、バリデーションなどは最低限
  * @todo 引数パースエラーを考慮する必要がある？
  * @return 1つの引数を表すlistを引数分保持したvector
  */
  template <std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    requires std::same_as<std::iter_value_t<Iterator>, pp_token>
  ifn parse_macro_args(Iterator& it, Sentinel fin) -> std::pmr::vector<std::pmr::list<pp_token>> { 
    return parse_macro_args(it, fin, [](Iterator& itr, Sentinel, auto& arg_list) -> bool {
      //改行はホワイトスペースになってるはずだし、ホワイトスペースは1つに畳まれているはず
      //妥当なプリプロセッシングトークン列としも構成済みのはず
      //従って、ここではその処理を行わない
      //そして、マクロ引数列中のマクロもここでは処理しない（外側マクロの置換後に再スキャンされる）

      arg_list.emplace_back(std::move(*itr));
      ++itr;
      return true;
    });
  }

  /**
  * @brief マクロ1つを表現する型
  * @details 構築に使用されたコンストラクタによってオブジェクトマクロと関数マクロを切り替える
  */
  class unified_macro {
    //仮引数リスト
    std::pmr::vector<std::u8string_view> m_params;
    //置換トークンのリスト
    std::pmr::list<pp_token> m_tokens;
    //可変長マクロですか？
    const bool m_is_va = false;
    //関数マクロですか？
    const bool m_is_func = true;
    //置換リストをチェックしてる時のエラー
    std::optional<std::pair<pp_parse_context, pp_token>> m_replist_err{};
    //{置換リストに現れる仮引数名のインデックス, 対応する実引数のインデックス, __VA_ARGS__?, __VA_OPT__?, #?, ##の左辺?, ##の右辺?, VA_OPTの中？}}
    std::pmr::vector<std::tuple<std::size_t, std::size_t, bool, bool, bool, bool, bool, bool>> m_correspond;

  public:
    
    //マクロ実行の結果型
    using macro_result_t = kusabira::expected<std::pmr::list<pp_token>, std::pair<pp_parse_context, pp_token>>;

  private:

    /**
    * @brief 置換リスト上の仮引数を見つけて、仮引数の番号との対応を取っておく
    * @tparam Is_VA 可変長マクロか否か
    * @param start 開始インデックス
    * @param end_index 終了インデックス
    * @param is_recursive 再帰中か否か（つまり、VA_OPTの中のトークンを処理している最中か否か）
    * @details 置換リストを先頭から見ていき仮引数トークンの出現を探し、引数リストのインデックスと置換リスト上でのインデックスの対応を記録する
    * @details VA_OPT内部のトークン列を処理する時に再帰呼び出しを行う
    */
    template<bool Is_VA>
    void make_id_to_param_pair(std::size_t start, std::size_t end_index, bool is_recursive = false) {
      using namespace std::string_view_literals;

      //仮引数名に対応する実引数リスト上の位置を求めるやつ
      auto find_param_index = [&arglist = m_params](auto token_str) -> std::pair<bool, std::size_t> {
        for (auto i = 0ull; i < arglist.size(); ++i) {
          if (arglist[i] == token_str) {
            return {true, i};
          }
        }
        return {false, 0};
      };

      std::bidirectional_iterator auto it = std::next(m_tokens.begin(), start);

      // 事前のマクロ定義妥当性チェック
      if (it != m_tokens.end()) {
        //先頭と末尾の##の出現を調べる、出てきたらエラー
        pp_token* ptr = nullptr;

        //エラー的にはソースコード上で最初に登場するやつを出したいので終端->先頭の順でチェック
        if (auto& back = m_tokens.back(); back.token == u8"##"sv) ptr = &back;
        if (auto& front =  *it; front.token == u8"##"sv) ptr = &front;

        if (ptr != nullptr) {
          //##トークンが置換リストの前後に出現している
          m_replist_err = std::make_pair(pp_parse_context::Define_Sharp2BothEnd, std::move(*ptr));
          return;
        }
      }

      //置換対象トークン数
      std::size_t reptoken_num = end_index;

      m_correspond.reserve(reptoken_num);

      //#と##の出現をマークする
      bool apper_sharp_op = false;
      bool apper_sharp2_op = false;

      //__VA_OPT__(...)の直後のトークンかをマーク、nulltprじゃなければそのVA_OPTが##の左辺であることをマークするbool値（6番目のbool値）への参照
      //__VA_OPT__(...) ## rhs のような結合をサポートするために__VA_OPT__(...)を指す対応に##の左辺であることを記録しておくためのもの
      bool* after_vaopt = nullptr;

      //置換リストをstartからチェックする
      for (auto index = start; index < reptoken_num; ++index, ++it) {

        // ホワイトスペース列は何らかの対象とはなり得ない、このループ処理では現れていないかのごとく扱う
        // 考慮すべきホワイトスペースは、# ## の前後にあるもののみ
        if ((*it).category == pp_token_category::whitespaces) {
          // #, ## トークンの後に出現しているホワイトスペースを削除する
          if (apper_sharp_op or apper_sharp2_op) {
            //今のトークン（ホワイトスペース）を消す
            it = m_tokens.erase(it);
            //消した分indexとトークン数を修正
            --it;
            --index;
            --reptoken_num;
          }
          continue;
        }

        //#,##をチェック
        if ((*it).category == pp_token_category::op_or_punc) {

          if ((*it).token == u8"#"sv) {
            //#も##も現れていないときだけ#を識別、#に対して#するのはエラー
            if (apper_sharp_op == false and apper_sharp2_op == false) {
              apper_sharp_op = true;
              continue;
            }
          } else if (not apper_sharp_op and (*it).token == u8"##"sv) {
            // #の後に##はエラー
            // ##の後に##が現れる場合、それを結合対象にしてしまう（未定義動作に当たるはず）

            // ##の左辺との間にあるホワイトスペースを無かったことにする
            // #, ##がトークン先頭に現れることはなく、ホワイトスペース列は1つに圧縮されているはず
            if (auto prev_it = std::prev(it); deref(prev_it).category == pp_token_category::whitespaces) {
              // これにかかる時はパースでホワイトスペース列の畳み込みができてない
              assert(deref(std::prev(prev_it)).category != pp_token_category::whitespaces);

              m_tokens.erase(prev_it);
              // 置換リストのインデックスとトークン数を修正
              --index;
              --reptoken_num;
            }

            // 1つ前のトークンを##の左辺として登録する
            // ##がトークン列の先頭に来ることは事前に弾いている
            if (after_vaopt != nullptr) {
              // 1つ前で__VA_OPT__()を処理していた時
              *after_vaopt = true;
              after_vaopt = nullptr;
            } else if (not std::ranges::empty(m_correspond) and 
                       std::get<0>(m_correspond.back()) == (index - 1)) {
              // 1つ前が仮引数名のとき
              std::get<5>(m_correspond.back()) = true;
            } else {
              // 1つ前は仮引数では無い普通のトークンの時
              // 1つ前のトークンを置換対象リストに連結対象として加える
              m_correspond.emplace_back(index - 1, std::size_t(-1), false, false, false, true, false, is_recursive);
            }

            // ##の直後であることをマークしておく
            apper_sharp2_op = true;

            continue;
          }
        }

        //ここにきてる時点で処理済みなのでリセット
        after_vaopt = nullptr;

        //1つ前の#,##トークンを削除する
        if (apper_sharp_op or apper_sharp2_op) {
          //1つ前のトークン（すなわち#,##）を消す
          m_tokens.erase(std::prev(it));
          //消した分indexとトークン数を修正
          --index;
          //N = m_tokens.size();
          --reptoken_num;
        }

        //処理終了後状態をリセット
        vocabulary::scope_exit se = [&apper_sharp_op, &apper_sharp2_op]() {
          apper_sharp_op = false;
          apper_sharp2_op = false;
        };

        //識別子以外の時、#が前に来ているのをエラーにする
        if ((*it).category != pp_token_category::identifier) {
          if (apper_sharp_op == true) {
              //#が仮引数名の前にないためエラー
              m_replist_err = std::make_pair(pp_parse_context::Define_InvalidSharp, std::move(*it));
              break;
          }
          continue;
        }

        if constexpr (Is_VA) {
          //可変引数参照のチェック
          if ((*it).token.to_view() == u8"__VA_ARGS__") {
            const auto [ismatch, va_start_index] = find_param_index(u8"...");

            //可変長マクロでは無いのに__VA_ARGS__が参照された
            if (not ismatch) {
              //エラー
              m_replist_err = std::make_pair(pp_parse_context::Define_InvalidVAARGS, std::move(*it));
              break;
            }

            //置換リストの要素番号に対して、対応する実引数位置を保存
            m_correspond.emplace_back(index, va_start_index, true, false, apper_sharp_op, false, apper_sharp2_op, is_recursive);
            continue;
          } else if ((*it).token.to_view() == u8"__VA_OPT__") {
            //__VA_OPT__は再帰しない
            if (is_recursive == true) {
              //エラー
              m_replist_err = std::make_pair(pp_parse_context::Define_VAOPTRecursive, std::move(*it));
              break;
            }
            //__VA_OPT__の前に#は来てはならない
            //if (apper_sharp_op == true) {
            //  //エラー
            //  m_replist_err = std::make_pair(pp_parse_context::Define_InvalidSharp, std::move(*it));
            //  break;
            //}

            //__VA_OPT__を見つけておく
            after_vaopt = &std::get<5>(m_correspond.emplace_back(index, 0, false, true, apper_sharp_op, false, apper_sharp2_op, false));

            //閉じかっこを探索
            std::forward_iterator auto start_pos = std::next(it, 2); //開きかっこの次のはず？
            std::forward_iterator auto close_paren = search_close_parenthesis(start_pos, m_tokens.end());
            //かっこ内の要素数、囲むかっこも含める
            std::size_t recursive_N = std::distance(start_pos, ++close_paren) + 1;

            //__VA_OPT__(...)のカッコ内だけを再帰処理、開きかっこと閉じかっこは見なくていいのでインデックス操作で飛ばす
            make_id_to_param_pair<true>(index + 2, index + recursive_N, true);

            //エラーチェック
            if (m_replist_err != std::nullopt) break;

            // トークン数を更新（__VA_OPT__の中に#や##がある場合、再帰中にトークンの削除が行われる）
            const auto old_token_num = reptoken_num;
            reptoken_num = m_tokens.size();
            // 再帰処理の過程でトークン削除が行われた場合、かっこ内の要素数は変化している
            recursive_N -= (old_token_num - reptoken_num);
            // 処理済みの分進める（この後ループで++されるので閉じかっこの次から始まる
            index += recursive_N;
            std::advance(it, recursive_N);
            continue;
          }
        }

        const auto [ismatch, param_index] = find_param_index((*it).token.to_view());
        //仮引数名ではないから次
        if (not ismatch) {
          //#が観測されているのに仮引数名ではない場合
          if (apper_sharp_op == true) {
            //エラー
            m_replist_err = std::make_pair(pp_parse_context::Define_InvalidSharp, std::move(*it));
            break;
          }
          continue;
        }

        //置換リストの要素番号に対して、対応する実引数位置を保存
        m_correspond.emplace_back(index, param_index, false, false, apper_sharp_op, false, apper_sharp2_op, is_recursive);
      }
    }

    /**
    * @brief 通常の（固定長引数）関数マクロの処理の実装
    * @param args 実引数列（カンマ区切り毎のトークン列のvector
    * @return マクロ置換後のトークンリスト
    */
    template<std::invocable<std::pmr::list<pp_token>&> F>
      requires std::same_as<bool, std::invoke_result_t<F, std::pmr::list<pp_token>&>>
    fn func_macro_impl(const std::pmr::vector<std::pmr::list<pp_token>>& args, F&& expand_macro) const -> macro_result_t {
      //置換対象のトークンシーケンスをコピー（終了後そのまま置換結果となる）
      std::pmr::list<pp_token> result_list{ m_tokens, &kusabira::def_mr };

      //置換リストの起点となる先頭位置
      const auto head = std::begin(result_list);

      //プレイスメーカートークンを挿入したかどうか
      bool should_remove_placemarker = false;

      //置換リスト-引数リスト対応を後ろから処理
      for (const auto[token_index, arg_index, ignore1, ignore2, sharp_op, sharp2_op, sharp2_op_rhs, ignore3] : m_correspond | std::views::reverse) {
        //置換リストのトークン位置
        const auto it = std::next(head, token_index);
        //##による結合時の右辺を指すイテレータ
        auto rhs = std::next(it);

        if (arg_index != std::size_t(-1)) {
          //対応する実引数のトークン列をコピー
          std::pmr::list<pp_token> arg_list{args[arg_index], &kusabira::def_mr};

          //文字列化
          if (sharp_op) {
            arg_list = pp_stringize<false>(arg_list);
          } else if (empty(arg_list)) {
            //文字列化対象ではなく引数が空の時、プレイスメーカートークンを挿入しておく
            result_list.insert(it, pp_token{ pp_token_category::placemarker_token });
            should_remove_placemarker = true;
          } else if (not (sharp2_op or sharp2_op_rhs)) {
            //##の左右のトークンはマクロ置換をしない
            //それ以外のトークンは置換の前に単体のプリプロセッシングトークン列としてマクロ置換を完了しておく
            //falseが帰ってきた場合はマクロ置換中のエラー
            if (not expand_macro(arg_list))
              return kusabira::error(std::make_pair(pp_parse_context::Funcmacro_ReplacementFail, std::move(*it)));
          } 

          //結果リストにsplice
          result_list.splice(it, std::move(arg_list));

          //置換済みトークンを消す
          rhs = result_list.erase(it);
        }

        if (sharp2_op) {
          // ##の左辺の時の処理
          auto lhs = std::prev(rhs);
          if (bool is_valid = (*lhs) += std::move(*rhs); not is_valid){
            //有効ではないプリプロセッシングトークンが生成された、エラー
            //失敗した場合、rhsに破壊的変更はされていないのでエラー出力にはそっちを使う
            return kusabira::error(std::make_pair(pp_parse_context::Define_InvalidTokenConcat, std::move(*rhs)));
          }
          result_list.erase(rhs);
        }
      }

      if (should_remove_placemarker) {
        result_list.remove_if([](const auto& pptoken) {
          return pptoken.category == pp_token_category::placemarker_token;
        });
      }

      return kusabira::ok(std::move(result_list));
    }

    /**
    * @brief 可変引数関数マクロの処理の実装
    * @param args 実引数列（カンマ区切り毎のトークン列のvector
    * @param f 引数のマクロ置換処理
    * @return マクロ置換後のトークンリスト
    */
    template<std::invocable<std::pmr::list<pp_token>&> F>
      requires std::same_as<bool, std::invoke_result_t<F, std::pmr::list<pp_token>&>>
    fn va_macro_impl(const std::pmr::vector<std::pmr::list<pp_token>>& args, F&& expand_macro) const -> macro_result_t {
      using namespace std::string_view_literals;

      //置換対象のトークンシーケンスをコピー（終了後そのまま置換結果となる）
      std::pmr::list<pp_token> result_list{ m_tokens, &kusabira::def_mr };

      //置換リストの起点となる先頭位置
      const auto head = std::begin(result_list);
      //引数の数
      const auto N = args.size();
      // 可変長引数が純粋に空かどうか
      bool is_va_empty_tmp = N < m_params.size();
      if (not is_va_empty_tmp and N == m_params.size()) {
        if (args.back().size() == 0u) {
          // F(arg1, ...)なマクロに対して、F(0,)の様に呼び出した時のケア（F(0,,)は引数ありとみなされる）
          is_va_empty_tmp = true;
        } else {
          // F(arg1, ...)なマクロに対して、F(0, EMP)のように呼び出した際の（EMPは空の置換）ケア（これは可変引数なしとみなされる）
          // EMPが関数マクロの呼び出しだとしても同様。とにかくマクロ置換を行なって結果が空になるか調べなければならない。

          // 実引数のトークン列を直接変えられると再帰マクロ展開のタイミングが異なってしまうのでコピーする
          // この結果を使いまわす手もあるけど、出現頻度が低いと思われるので微妙かも・・・
          std::pmr::list<pp_token> copylist{args.back(), &kusabira::def_mr};

          // falseが帰ってきたら置換中のエラー
          if (not expand_macro(copylist))
            return kusabira::error(std::make_pair(pp_parse_context::Funcmacro_ReplacementFail, std::move(args.back().front())));
          // 置換結果が空ならば可変長部は空
          is_va_empty_tmp = std::ranges::empty(copylist);
        }
      }
      const bool is_va_empty = is_va_empty_tmp;

      //プレイスメーカートークンを挿入したかどうか
      bool should_remove_placemarker = false;

      //プレイスメーカートークン挿入処理
      auto insert_placemarker = [&result_list, &should_remove_placemarker](auto pos) {
        result_list.insert(pos, pp_token{pp_token_category::placemarker_token});
        should_remove_placemarker = true;
      };
      //##によるトークン結合処理
      auto token_concat = [&result_list](auto rhs_pos) -> bool {
        auto &lhs = *std::prev(rhs_pos);

        if (bool is_valid = lhs += std::move(*rhs_pos); not is_valid){
          //有効ではないプリプロセッシングトークンが生成された、エラー
          //失敗した場合、rhsに破壊的変更はされていないのでエラー出力にはそっちを使う
          return false;
        }

        //結合したので次のトークンを消す
        result_list.erase(rhs_pos);

        return true;
      };

      //置換リスト-引数リスト対応を後ろから処理
      for (const auto[token_index, arg_index, va_args, va_opt, sharp_op, sharp2_op, sharp2_op_rhs, inner_vaopt] : m_correspond | std::views::reverse) {
        // 可変長引数が空の時、VA_OPT中の置換処理をスキップする
        if (inner_vaopt and is_va_empty) continue;

        //置換リストのトークン位置
        const auto it = std::next(head, token_index);
        //##の左右のどちらかの仮引数である時、そのトークン列はマクロ展開を行わない
        const bool skip_macro_expand = sharp2_op or sharp2_op_rhs;

        if (va_opt) {
          //__VA_OPT__の処理
          //__VA_OPT__が##の左右にいる時、その内部のトークン列のマクロ展開タイミングはいつ？？？？

          //事前条件
          assert((*it).token.to_view() == u8"__VA_OPT__");

          const auto replist_end = std::end(result_list);
          //__VA_OPT__の開きかっこの位置を探索（検索する必要はないかもしれない
          //auto open_paren_pos_next = std::next(it, 2);
          auto open_paren_pos_next = std::find_if(std::next(it), replist_end, [](auto &pptoken) {
            return pptoken.token == u8"("sv;
          });

          assert(open_paren_pos_next != replist_end);
          //開きかっこの"次"の位置
          ++open_paren_pos_next;

          //対応する閉じ括弧の位置を探索
          auto vaopt_close_paren = search_close_parenthesis(open_paren_pos_next, replist_end);

          // 対応する閉じかっこの存在は構文解析で保証する
          assert(vaopt_close_paren != replist_end);

          if (is_va_empty) {
            //可変長部分が空ならばVA_OPT全体を削除

            //VAOPTの全体を結果トークン列から取り除く
            auto insert_pos = result_list.erase(it, ++vaopt_close_paren);

            //##の処理のためにplacemarker tokenを置いておく
            //VA_OPTが##の左辺の時は何もしなくてもいい
            if (not sharp2_op) insert_placemarker(insert_pos);

          } else {
            //可変長部分が空でないならば、VA_OPTと対応するかっこだけを削除

            auto rhs = result_list.begin();

            if (sharp_op) {
              // #__VA_OPT__(...)の処理
              
              // 中身のトークン列だけを文字列化
              // 後ろから処理してるので中身は処理済み、VA_ARGS等考慮しなくていい
              std::pmr::list<pp_token> stringize_list = pp_stringize<false>(std::ranges::subrange{ open_paren_pos_next, vaopt_close_paren });

              // VA_OPTの全体を削除
              auto insert_pos = result_list.erase(it, ++vaopt_close_paren);

              // 文字列化トークンを挿入
              result_list.splice(insert_pos, std::move(stringize_list));

              // VA_OPT閉じかっこの次の位置、すなわち##の右辺
              rhs = insert_pos;
            } else {
              //まずは後ろの閉じ括弧を削除（イテレータが無効化するのを回避するため）
              rhs = result_list.erase(vaopt_close_paren);
              //次に開きかっことVA_OPT本体を削除
              result_list.erase(it, open_paren_pos_next);
            }

            if (sharp2_op) {
              //##によるトークンの結合処理
              if (not token_concat(rhs))
                return kusabira::error(std::make_pair(pp_parse_context::Define_InvalidTokenConcat, std::move(*rhs)));
            }
          }

          //itに対応するトークンを消してしまっているので次に行く
          continue;
        }

        //実引数リストを構成するためのリスト
        std::pmr::list<pp_token> arg_list{&kusabira::def_mr};

        //VA_ARGSと通常の置換処理
        if (va_args) {
          //spliceしていくための挿入位置
          const auto pos = std::end(arg_list);

          //可変長引数部分をコピーしつつカンマを登録
          for (std::size_t va_index = arg_index; va_index < N; ++va_index) {
            //実引数一つをコピー
            arg_list.splice(pos, std::pmr::list<pp_token>{args[va_index], &kusabira::def_mr});

            //最後の引数にはカンマをつけない
            if (va_index != (N - 1)) {
              //カンマの追加
              arg_list.emplace_back(pp_token_category::op_or_punc, u8","sv);
            }
          }
        } else if (arg_index != std::size_t(-1)) {
          //対応する実引数のトークン列をコピー
          arg_list = args[arg_index];
        }

        if (sharp_op) {
          //#演算子の処理、文字列化を行う
          if (va_args) {
            arg_list = pp_stringize<true>(arg_list);
          } else {
            arg_list = pp_stringize<false>(arg_list);
          }
        } else if (empty(arg_list)){
          //文字列化対象ではない時、プレイスメーカートークンを挿入する
          //文字列化対象の場合は空文字（""）が入っているはず
          insert_placemarker(it);
        } else if (not skip_macro_expand) {
          //##の左右のトークンはマクロ置換をしない
          //それ以外のトークンは置換の前に単体のプリプロセッシングトークン列としてマクロ置換を完了しておく
          //falseが帰ってきた場合はマクロ置換中のエラー
          if (not expand_macro(arg_list))
            return kusabira::error(std::make_pair(pp_parse_context::Funcmacro_ReplacementFail, std::move(*it)));
        } 

        //結果リストにsplice
        result_list.splice(it, std::move(arg_list));

        //##による結合時の右辺を指すイテレータ
        auto rhs = std::next(it);
        //置換済みトークンを消す
        if (arg_index != std::size_t(-1)) {
          rhs = result_list.erase(it);
        }

        if (sharp2_op) {
          //##によるトークンの結合処理
          if (not token_concat(rhs))
            return kusabira::error(std::make_pair(pp_parse_context::Define_InvalidTokenConcat, std::move(*rhs)));
        }
      }

      if (should_remove_placemarker) {
        result_list.remove_if([](const auto &pptoken) {
          return pptoken.category == pp_token_category::placemarker_token;
        });
      }

      return kusabira::ok(std::move(result_list));
      //return macro_result_t{tl::in_place, std::move(result_list)};
    }

    /**
    * @brief オブジェクトマクロの置換リスト中の##トークンを処理する
    */
    void objmacro_token_concat() {
      using namespace std::string_view_literals;

      auto first = std::begin(m_tokens);
      auto end = std::end(m_tokens);

      for (auto it = first; it != end; ++it) {
        if ((*it).category != pp_token_category::op_or_punc) continue;
        if ((*it).token != u8"##"sv) continue;
        if (it == first) {
          //先頭に##が来ているときは消して次を飛ばす
          it = m_tokens.erase(it);
          continue;
        }


        // ##を消して次のイテレータを得る
        auto rhs = m_tokens.erase(it);
        // 左辺オペランドを指すイテレータ
        auto lhs = std::prev(rhs);

        // ホワイトスペースなら消す、ホワイトスペースは1つに折り畳まれている（はず
        if (deref(lhs).category == pp_token_category::whitespaces) {
          auto tmp = m_tokens.erase(lhs); // 消した位置の次を指すイテレータが得られる
          lhs = std::prev(tmp);
        }
        if (deref(rhs).category == pp_token_category::whitespaces) {
          rhs = m_tokens.erase(rhs);
        }

        // ホワイトスペースが1つにたたまれていない、パーサがおかしい
        assert(deref(lhs).category != pp_token_category::whitespaces);
        assert(deref(rhs).category != pp_token_category::whitespaces);

        if (bool is_valid = (*lhs) += std::move(*rhs); not is_valid) {
          //有効ではないプリプロセッシングトークンが生成された、エラー
          //失敗した場合、rhsに破壊的変更はされていないのでエラー出力にはそっちを使う
          m_replist_err = std::make_pair(pp_parse_context::Define_InvalidTokenConcat, std::move(*rhs));
        }

        //結合したので次のトークンを消す
        m_tokens.erase(rhs);

        it = lhs;
      }
    }

  public:

    /**
    * @brief 関数マクロの構築
    * @param params 仮引数列
    * @param replist 置換リスト
    * @param is_va 可変引数マクロであるか否か
    */
    template <typename T = std::pmr::vector<std::u8string_view>, typename U = std::pmr::list<pp_token>>
    unified_macro(T &&params, U &&replist, bool is_va)
      : m_params{ std::forward<T>(params), &kusabira::def_mr }
      , m_tokens{ std::forward<U>(replist), &kusabira::def_mr }
      , m_is_va{is_va}
      , m_is_func{true}
      , m_correspond{ &kusabira::def_mr }
    {
      if (is_va) {
        this->make_id_to_param_pair<true>(0, m_tokens.size());
      } else {
        this->make_id_to_param_pair<false>(0, m_tokens.size());
      }
    }

    /**
    * @brief オブジェクトマクロの構築
    * @param replist 置換リスト
    */
    template <typename U = std::pmr::list<pp_token>>
    unified_macro(U &&replist)
      : m_params{ &kusabira::def_mr }
      , m_tokens{ std::forward<U>(replist), &kusabira::def_mr }
      , m_is_va{false}
      , m_is_func{false}
      , m_correspond{ &kusabira::def_mr }
    {
      this->objmacro_token_concat();
    }

    unified_macro(unified_macro&&) = default;
    unified_macro& operator=(unified_macro&&) = default;

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
    * @brief 関数マクロであるかを調べる
    * @return 関数マクロならtrue
    */
    fn is_function() const -> bool {
      return m_is_func;
    }

    /**
    * @brief マクロの実行が可能かを取得
    * @details 構築時の置換リスト上構文エラーを報告
    * @return マクロ実行がいつでも可能ならば無効値、有効値はエラー情報
    */
    fn is_ready() const -> const std::optional<std::pair<pp_parse_context, pp_token>>& {
      return m_replist_err;
    }

    /**
    * @brief 実引数の数が正しいかを調べる
    * @param args 実引数列
    * @details 可変引数マクロなら仮引数の数-1以上、それ以外の場合は仮引数の数と同一である場合に引数の数が合っているとみなす
    * @return 実引数の数が合っているか否か
    */
    fn validate_argnum(const std::pmr::vector<std::pmr::list<pp_token>>& args) const noexcept -> bool {
      if (m_is_va == true) {
        // 可変長マクロ
        // 可変長部を除いた仮引数の数以上であればok
        return (m_params.size() - 1u) <= args.size();
      } else {
        // 非可変長マクロ
        if (m_params.size() == args.size()) {
          // 合っていればそれでいい
          return true;
        } else if (m_params.size() == 0) {
          // 仮引数の数が0の時、空の実引数1つはok
          return std::ranges::size(args) == 1 and std::ranges::empty(args[0]);
        }/* else if (m_params.size() == 1) {
          // 仮引数の数が1の時
          // 引き数なしでの呼び出しはok
          return std::ranges::empty(args);
        }*/

        return false;
      }
    }

    /**
    * @brief 関数マクロを実行する
    * @param args 実引数列
    * @return 置換結果のトークンリスト
    */
    fn operator()(const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> macro_result_t {
      //可変引数マクロと処理を分ける
      if (m_is_va) {
        return this->va_macro_impl(args, [](auto&&) { return true; });
      } else {
        return this->func_macro_impl(args, [](auto &&) { return true; });
      }
    }

    /**
    * @brief 関数マクロを実行する
    * @param args 実引数列
    * @param f 引数リスト内マクロ展開用の関数（f : std::list<pp_token> -> bool）
    * @return 置換結果のトークンリスト
    */
    template<std::invocable<std::pmr::list<pp_token>&> F>
      requires std::same_as<bool, std::invoke_result_t<F, std::pmr::list<pp_token>&>>
    fn operator()(const std::pmr::vector<std::pmr::list<pp_token>>& args, F&& f) const -> macro_result_t {
      //可変引数マクロと処理を分ける
      if (m_is_va) {
        return this->va_macro_impl(args, std::forward<F>(f));
      } else {
        return this->func_macro_impl(args, std::forward<F>(f));
      }
    }
  };

  /**
  * @brief マクロの管理などプリプロセッシングディレクティブの実行を担う
  */
  struct pp_directive_manager {

    using funcmacro_map = std::pmr::unordered_map<std::u8string_view, unified_macro>;
    using timepoint_t = decltype(std::chrono::system_clock::now());

    fs::path m_filename{};
    fs::path m_replace_filename{};
    std::time_t m_datetime{};
    funcmacro_map m_funcmacros{&kusabira::def_mr};
    std::pmr::map<std::size_t, std::size_t> m_line_map{&kusabira::def_mr};

    pp_directive_manager() = default;

    pp_directive_manager(const fs::path& filename)
      : m_filename{filename}
      , m_replace_filename{filename.filename()}
      , m_datetime{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
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
    * @brief マクロを登録する
    * @param reporter メッセージ出力器
    * @param macro_name マクロ名
    * @param tokenlist 置換リスト
    * @param is_va 可変引数であるか否か（関数マクロのみ）
    * @param params 仮引数リスト（関数マクロのみ）
    * @details 後ろ2つの引数の有無でオブジェクトマクロと関数マクロを識別して登録する
    * @return 登録が恙なく完了したかどうか
    */
    template <typename Reporter, typename ReplacementList = std::pmr::list<pp_token>, typename ParamList = std::nullptr_t>
    fn define(Reporter &reporter, const PP::pp_token& macro_name, ReplacementList &&tokenlist, [[maybe_unused]] ParamList&& params = {}, [[maybe_unused]] bool is_va = false) -> bool {
      using namespace std::string_view_literals;

      bool redefinition_err = false;
      const std::pair<pp_parse_context, pp_token>* replist_err = nullptr;

      if constexpr (std::is_same_v<ParamList, std::nullptr_t>) {
        //オブジェクトマクロの登録
        const auto [pos, is_registered] = m_funcmacros.try_emplace(macro_name.token.to_view(), std::forward<ReplacementList>(tokenlist));

        if (not is_registered) {
          if ((*pos).second.is_identical({}, tokenlist)) return true;
          //置換リストが一致していなければエラー
          redefinition_err = true;
        } else {
          if (auto& opt = (*pos).second.is_ready(); bool(opt)) {
            //##による結合処理のエラーを報告
            replist_err = &*opt;
          }
        }
      } else {
        static_assert([] { return false; }() || std::is_same_v<std::remove_cvref_t<ParamList>, std::pmr::vector<std::u8string_view>>, "ParamList must be std::pmr::vector<std::u8string_view>.");
        //関数マクロの登録
        const auto [pos, is_registered] = m_funcmacros.try_emplace(macro_name.token.to_view(), std::forward<ParamList>(params), std::forward<ReplacementList>(tokenlist), is_va);

        if (not is_registered) {
          if ((*pos).second.is_identical(params, tokenlist)) return true;
          //仮引数列と置換リストが一致していなければエラー
          redefinition_err = true;
        } else {
          if (auto& opt = (*pos).second.is_ready(); bool(opt)) {
            //置換リストのパースにおいてのエラーを報告
            replist_err = &*opt;
          }
        }
      }

      if (redefinition_err) {
        //同名の異なる定義のマクロが登録された、エラー
        reporter.pp_err_report(m_filename, macro_name, pp_parse_context::Define_Duplicate);
        return false;
      }
      if (replist_err != nullptr) {
        //置換リストを見ただけで分かるエラーの報告
        const auto &[context, pptoken] = *replist_err;
        reporter.pp_err_report(m_filename, pptoken, context);
        return false;
      }

      return true;
    }

  private:

    //以降変更されることは無いはず、ムーブしたいのでconstを付けないでおく・・・
    std::unordered_map<std::u8string_view, std::u8string_view> m_predef_macro = {
      {u8"__LINE__", u8""},
      {u8"__FILE__", u8""},
      {u8"__DATE__", u8""},
      {u8"__TIME__", u8""},
      {u8"_­_­cplusplus", u8"202002L"},
      {u8"__STDC_­HOSTED__", u8"1"},
      {u8"__STDCPP_­DEFAULT_­NEW_­ALIGNMENT__", u8"16ull"},
      //{u8"__STDCPP_­STRICT_­POINTER_­SAFETY__", u8"1"},
      {u8"__STDCPP_­THREADS__", u8"1"}
    };

    /**
    * @brief 事前定義マクロを処理する
    * @details __LINE__ __FILE__ __DATE__ __TIME__ の4つは特殊処理、その他はトークン置換で生成
    * @param macro_name マクロ名
    * @return 処理結果、無効値は対象外
    */
    fn predefined_macro(const pp_token& macro_name) const -> std::optional<std::pmr::list<pp_token>> {

      //結果プリプロセッシングトークンリストを作成する処理
      auto make_result = [&macro_name, &mr = kusabira::def_mr](auto str, auto pptoken_cat) {
        std::pmr::list<pp_token> result_list{&mr};
        auto &linenum_token = result_list.emplace_back(pptoken_cat, u8"", macro_name.column, macro_name.srcline_ref);
        linenum_token.token = std::move(str);

        return result_list;
      };

      if (macro_name.token == u8"__LINE__") {
        auto line_num = macro_name.get_logicalline_num();

        //現在の実行数に対応する#lineによる行数変更を捜索        
        if (auto pos = m_line_map.upper_bound(line_num); not empty(m_line_map)) {
          //実行数よりも大きい要素を探しているので、その一つ前が適用すべき#lineの処理
          std::advance(pos, -1);
          //#line適用時点から進んだ行数
          auto diff = line_num - deref(pos).first;
          //#lineによる変更を反映し、行数を進める
          line_num = deref(pos).second + diff;
        }

        //現在の論理行番号を文字列化
        char buf[21]{};
        auto [ptr, ec] = std::to_chars(buf, std::end(buf), line_num);
        //失敗せんでしょ・・・
        assert(ec == std::errc{});
        std::pmr::u8string line_num_str{ reinterpret_cast<const char8_t*>(buf), reinterpret_cast<const char8_t*>(ptr), &kusabira::def_mr };

        return make_result(std::move(line_num_str), pp_token_category::pp_number);
      }
      if (macro_name.token == u8"__FILE__") {
        std::pmr::u8string filename_str{&kusabira::def_mr};

        //#lineによるファイル名変更を処理
        if (not m_replace_filename.empty()) {
          filename_str = std::pmr::u8string{m_replace_filename.filename().u8string(), &kusabira::def_mr};
        } else {
          filename_str = std::pmr::u8string{m_filename.filename().u8string(), &kusabira::def_mr};
        }

        return make_result(std::move(filename_str), pp_token_category::string_literal);
      }
      if (macro_name.token == u8"__DATE__") {
        //月毎の基礎文字列対応
        constexpr const char8_t* month_map[12] = {
            u8"Jan dd yyyy",
            u8"Feb dd yyyy",
            u8"Mar dd yyyy",
            u8"Apr dd yyyy",
            u8"May dd yyyy",
            u8"Jun dd yyyy",
            u8"Jul dd yyyy",
            u8"Aug dd yyyy",
            u8"Sep dd yyyy",
            u8"Oct dd yyyy",
            u8"Nov dd yyyy",
            u8"Dec dd yyyy"
        };

        //左側をスペース埋め
        auto space_pad = [](auto *pos, auto c) {
          if (*pos == c) {
            *pos = ' ';
            std::swap(*(pos - 1), *pos);
          }
        };

        //time_tをtm構造体へ変換
#ifdef _MSC_VER
        tm temp{};
        gmtime_s(&temp, &m_datetime);
        auto* utc = &temp;
#else
        auto* utc = gmtime(&m_datetime);
#endif // _MSC_VER

        //utc->tm_monは月を表す0~11の数字
        std::pmr::u8string date_str{month_map[utc->tm_mon], &kusabira::def_mr};
        auto *first = reinterpret_cast<char *>(date_str.data() + 4);

        //日付と年を文字列化
        auto [ptr1, ec1] = std::to_chars(first, first + 2, utc->tm_mday);
        space_pad(first + 1, 'd');
        auto [ptr2, ec2] = std::to_chars(first + 3, first + 7, utc->tm_year + 1900);

        //失敗しないはず・・・
        assert(ec1 == std::errc{} and ec2 == std::errc{});

        return make_result(std::move(date_str), pp_token_category::string_literal);
      }
      if (macro_name.token == u8"__TIME__") {

        std::pmr::u8string time_str{u8"hh:mm:ss", &kusabira::def_mr};
        auto *first = reinterpret_cast<char*>(time_str.data());

        //左側をゼロ埋めするやつ
        auto zero_pad = [](char* pos, char c) {
          if (*pos == c) {
            *pos = '0';
            std::swap(*(pos - 1), *pos);
          }
        };

        //time_tをtm構造体へ変換
#ifdef _MSC_VER
        tm temp{};
        gmtime_s(&temp, &m_datetime);
        auto* utc = &temp;
#else
        auto* utc = gmtime(&m_datetime);
#endif // _MSC_VER

        //時分秒を文字列化
        auto [ptr1, ec1] = std::to_chars(first, first + 2, utc->tm_hour);
        zero_pad(first + 1, 'h');
        auto [ptr2, ec2] = std::to_chars(first + 3, first + 5, utc->tm_min);
        zero_pad(first + 4, 'm');
        auto [ptr3, ec3] = std::to_chars(first + 6, first + 8, utc->tm_sec);
        zero_pad(first + 7, 's');

        //失敗しないはず・・・
        assert(ec1 == std::errc{} and ec2 == std::errc{} and ec3 == std::errc{});

        return make_result(std::move(time_str), pp_token_category::string_literal);
      }

      if (auto pos = m_predef_macro.find(macro_name.token); pos != m_predef_macro.end()) {
        //その他事前定義マクロの処理
        return make_result(deref(pos).second, pp_token_category::pp_number);
      }

      return std::nullopt;
    }

    /**
    * @brief マクロ置換処理の共通部分
    * @param reporter エラー出力先
    * @param list 引数1つのプリプロセッシングトークンのリスト
    * @param outer_macro 外側のマクロ名のメモ（この関数自体はこのメモを書き換えない、ただし再帰呼び出し先が書き換えてる）
    * @return {エラーが起きなかった, マクロのスキャンは完了（falseならば関数マクロの引数リストが閉じていない）}
    */
    template<bool Rescanning, typename Reporter>
    fn macro_replacement_impl(Reporter& reporter, std::pmr::list<pp_token>& list, std::pmr::unordered_set<std::u8string_view>& outer_macro) const -> std::pair<bool, bool> {
      auto it = std::begin(list);
      const auto fin = std::end(list);

      while (it != fin) {
        // 識別子以外は無視
        // 外側マクロを無視
        if (deref(it).category != pp_token_category::identifier) {
          ++it;
          continue;
        } else if (outer_macro.contains(deref(it).token)) {
          // メモにあったマクロのトークン種別を変更してマークしておく
          // 更に外側で再スキャンされたときにも展開を防止するため
          // 最終的にはパーサ側で元に戻す
          deref(it).category = pp_token_category::not_macro_name_identifier;
          ++it;
          continue;
        }

        // マクロ判定
        if (auto opt = this->is_macro(deref(it).token); opt) {
          std::pmr::list<pp_token> result{ &kusabira::def_mr };
          bool success = false;
          [[maybe_unused]] bool scan_complete = true;

          const bool is_funcmacro = *opt;

          // マクロの終端位置（関数マクロなら閉じかっこの次を計算して入れておく）
          auto close_pos = std::next(it);

          if (not is_funcmacro) {
            if constexpr (not Rescanning) {
              // オブジェクトマクロ置換（再スキャンしない）
              std::tie(success, std::ignore, result, std::ignore) = this->objmacro<true>(reporter, *it);
            } else {
              // オブジェクトマクロ置換（再スキャンはこの中で再帰的に行われる）
              std::tie(success, scan_complete, result) = this->objmacro<false>(reporter, *it, outer_macro);
            }
          } else {
            // マクロ引数列の先頭位置（開きかっこ）を探索
            auto start_pos = std::ranges::find_if_not(std::next(it), fin, [](const auto& pptoken) {
              return pptoken.category <= pp_token_category::block_comment;
            });

            // 終わってしまったらそこで終わり
            if (start_pos == fin) return { true, false };

            // マクロ呼び出しではなかった
            if (deref(start_pos).token != u8"(") {
              it = start_pos;
              continue;
            }

            // 開きかっこの次へ移動
            ++start_pos;

            // 終端かっこのチェック、マクロが閉じる前に終端に達した場合何もしない（外側で再処理）
            close_pos = search_close_parenthesis(start_pos, fin);
            if (close_pos == fin) return { true, false };

            // 閉じかっこの次まで進めておく
            ++close_pos;
            // マクロの引数リスト取得
            const auto args = parse_macro_args(start_pos, close_pos);

            if constexpr (not Rescanning) {
              // 関数マクロ置換（再スキャンしない）
              std::tie(success, std::ignore, result, std::ignore) = this->funcmacro<true>(reporter, *it, args);
            } else {
              // 関数マクロ置換（再スキャンはこの中で再帰的に行われる）
              std::tie(success, scan_complete, result) = this->funcmacro<false>(reporter, *it, args, outer_macro);
            }
          }

          //エラーが起きてればそのまま終わる
          if (not success) return { false, false };

          const auto erase_pos = it;

          // マクロ全体の次のトークンからスキャンを再開
          it = close_pos;

          // リストのスキャンが完了していなければ、戻ってきたリストの先頭からスキャンし関数マクロ名を探す（それより前のマクロは置換済み）
          // スキャンが未完（別のマクロの呼び出しが途中で終わってた）で、そこまでの間に同じマクロの呼び出しがある場合、おかしくなりそう・・・
          if (Rescanning and not scan_complete) {
            it = std::begin(result);
          }

          // 戻ってきたリストをspliceする
          list.splice(erase_pos, std::move(result));
          // 置換したマクロ範囲を消去
          list.erase(erase_pos, close_pos);

        } else {
          // 識別子はマクロ名ではなかった
          ++it;
        }
      }

      // 恙なく終了したときここで終わる
      return { true, true };
    }

    /**
    * @brief 関数マクロの引数内のマクロを展開する
    * @param reporter エラー出力先
    * @param list 引数1つのプリプロセッシングトークン列
    * @details マクロの引数内ではそのマクロと同じものが現れていても構わない、再帰的展開されず、再スキャン時は展開対象にならないので無限再帰に陥ることは無い
    * @details ここでは置換後の再スキャンとさらなるマクロ展開を行わない
    * @return 成功？
    */
    template<typename Reporter>
    fn macro_replacement(Reporter& reporter, std::pmr::list<pp_token>& list) const -> bool {
      std::pmr::unordered_set<std::u8string_view> memo{ &kusabira::def_mr };
      const auto [success, ignore] = macro_replacement_impl<false>(reporter, list, memo);
      return success;
    }

    /**
    * @brief マクロ置換後の結果リストに対して再スキャンとさらなる展開を行う
    * @param reporter エラー出力先
    * @param list 引数1つのプリプロセッシングトークン列
    * @param outer_macro 外側のマクロ名のメモ（この関数自体はこのメモを書き換えない、ただし再帰呼び出し先が書き換えてる）
    * @return {エラーが起きなかった, マクロのスキャンは完了した（falseならば関数マクロの引数リストが閉じていない）}
    */
    template<typename Reporter>
    fn further_macro_replacement(Reporter& reporter, std::pmr::list<pp_token>& list, std::pmr::unordered_set<std::u8string_view>& outer_macro) const -> std::pair<bool, bool> {
      return macro_replacement_impl<true>(reporter, list, outer_macro);
    }

    /**
    * @brief 対応するマクロを取り出す
    * @param macro_name マクロ名
    * @param found_op マクロが見つかった時の処理
    * @param nofound_val 見つからなかった時に返すもの
    * @return found_op()の戻り値かnofound_valを返す
    */
    template<typename Found, typename NotFound>
    fn fetch_macro(std::u8string_view macro_name, Found&& found_op, NotFound&& nofound_val) const {
      const auto pos = m_funcmacros.find(macro_name);

      //見つからなかったら、その時用の戻り値を返す
      if (pos == std::end(m_funcmacros)) {
        return nofound_val;
      }

      //見つかったらその要素を与えて処理を実行
      return found_op((*pos).second);
    }

  public:

    /**
    * @brief マクロによる置換リストを取得する
    * @tparam MacroExpandOff さらにマクロ展開を行うか否か
    * @param macro_name 識別子トークン名
    * @return 置換リストのoptional、無効地なら置換対象ではなかった
    */
    template<bool MacroExpandOff, typename Reporter>
    fn objmacro(Reporter& reporter, const pp_token& macro_name) const -> std::tuple<bool, bool, std::pmr::list<pp_token>, std::pmr::unordered_set<std::u8string_view>> {
      std::pmr::unordered_set<std::u8string_view> memo{&kusabira::def_mr};
      auto&& tuple = this->objmacro<MacroExpandOff>(reporter, macro_name, memo);
      return std::tuple_cat(std::move(tuple), std::make_tuple(std::move(memo)));
    }

    /**
    * @brief マクロによる置換リストを取得する
    * @tparam MacroExpandOff さらにマクロ展開を行うか否か
    * @param macro_name 識別子トークン名
    * @return 置換リストのoptional、無効地なら置換対象ではなかった
    */
    template<bool MacroExpandOff, typename Reporter>
    fn objmacro(Reporter& reporter, const pp_token& macro_name, std::pmr::unordered_set<std::u8string_view>& outer_macro) const -> std::tuple<bool, bool, std::pmr::list<pp_token>> {
      
      //事前定義マクロを処理（この結果には再スキャンの対象となるものは含まれていないはず）
      if (auto result = predefined_macro(macro_name); result) {
        return {true, true, std::move(*result)};
      }

      //マクロを取り出す（存在は予め調べてあるものとする）
      const auto& macro = deref(m_funcmacros.find(macro_name.token)).second;

      // ここでmemory_resourceを適切に設定しておかないと、アロケータが正しく伝搬しない
      unified_macro::macro_result_t result{tl::in_place, &kusabira::def_mr};

      //第一弾マクロ展開
      if constexpr (MacroExpandOff) {
        result = macro({});
      } else {
        // オブジェクトマクロのこっちって何？
        result = macro({}, [&, this](auto &list) { return this->macro_replacement(reporter, list); });
      }

      if constexpr (MacroExpandOff) {
        // さらなる展開をしないときはこれで終わり
        if (result) return { true, true, std::pmr::list<pp_token>{std::move(*result)} };
      }

      if (result) {
        //現在のマクロ名をメモ
        outer_macro.emplace(macro_name.token);

        //リストの再スキャンとさらなる展開
        const auto [success, complete] = this->further_macro_replacement(reporter, *result, outer_macro);

        //メモを消す
        if (complete) outer_macro.erase(macro_name.token);

        // ホワイトスペースの除去
        std::erase_if(*result, [](const auto& pptoken) {
          return pptoken.category == pp_token_category::whitespaces;
        });

        return std::make_tuple(success, complete, std::pmr::list<pp_token>{std::move(*result)});
      } else {
        //オブジェクトマクロはこっちにこないのでは？
        assert(false);
        return std::make_tuple(false, false, std::pmr::list<pp_token>{});
      }
    }

    /**
    * @brief 関数マクロによる置換リストを取得する
    * @tparam MacroExpandOff さらにマクロ展開を行うか否か
    * @param macro_name マクロ名
    * @param args 関数マクロの実引数トークン列
    * @return {エラーの有無, スキャン完了したか, 置換リスト}
    */
    template<bool MacroExpandOff, typename Reporter>
    fn funcmacro(Reporter& reporter, const pp_token& macro_name, const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::tuple<bool, bool, std::pmr::list<pp_token>, std::pmr::unordered_set<std::u8string_view>> {
      std::pmr::unordered_set<std::u8string_view> memo{&kusabira::def_mr};
      auto&& tuple = this->funcmacro<MacroExpandOff>(reporter, macro_name, args, memo);
      return std::tuple_cat(std::move(tuple) , std::make_tuple(std::move(memo)));
    }
    /**
    * @brief 関数マクロによる置換リストを取得する
    * @tparam MacroExpandOff さらにマクロ展開を行うか否か
    * @param macro_name マクロ名
    * @param args 関数マクロの実引数トークン列
    * @return {エラーの有無, スキャン完了したか, 置換リスト}
    */
    template<bool MacroExpandOff, typename Reporter>
    fn funcmacro(Reporter& reporter, const pp_token& macro_name, const std::pmr::vector<std::pmr::list<pp_token>>& args, std::pmr::unordered_set<std::u8string_view>& outer_macro) const -> std::tuple<bool, bool, std::pmr::list<pp_token>> {
      
      //マクロを取り出す（存在は予め調べてあるものとする）
      const auto& macro = deref(m_funcmacros.find(macro_name.token)).second;
      
      //引数長さのチェック
      if (not macro.validate_argnum(args)) {
        reporter.pp_err_report(m_filename, macro_name, PP::pp_parse_context::Funcmacro_InsufficientArgs);
        return { false, false, std::pmr::list<pp_token>{} };
      }

      // ここでmemory_resourceを適切に設定しておかないと、アロケータが正しく伝搬しない
      unified_macro::macro_result_t result{tl::in_place, &kusabira::def_mr};

      if constexpr (MacroExpandOff) {
        result = macro(args);
      } else {
        result = macro(args, [&, this](auto &list) { return this->macro_replacement(reporter, list); });
      }

      if constexpr (MacroExpandOff) {
        // さらなる展開をしないときはこれで終わり
        if (result) return { true, true, std::pmr::list<pp_token>{std::move(*result)} };
      }

      //置換結果取得
      if (result) {
        //現在のマクロ名をメモ
        outer_macro.emplace(macro_name.token);

        //リストの再スキャンとさらなる展開
        const auto [success, complete] = this->further_macro_replacement(reporter, *result, outer_macro);

        //メモを消す
        if (complete) outer_macro.erase(macro_name.token);

        // ホワイトスペースの除去
        std::erase_if(*result, [](const auto& pptoken) {
          return pptoken.category == pp_token_category::whitespaces;
        });

        return {success, complete, std::pmr::list<pp_token>{std::move(*result)}};
      } else {
        //エラー報告（##で不正なトークンが生成された）
        const auto [context, pptoken] = result.error();
        if (context == pp_parse_context::Funcmacro_ReplacementFail) {
          //どの引数置換時にマクロ展開に失敗したのかは報告済み、ここではどこの呼び出しで失敗したかを伝える
          reporter.pp_err_report(m_filename, macro_name, context);
        } else {
          reporter.pp_err_report(m_filename, pptoken, context);
        }
        return {false, false, std::pmr::list<pp_token>{}};
      }
    }

    /**
    * @brief 識別子がマクロ名であるか、また関数形式かをチェックする
    * @param identifier 識別子の文字列
    * @return マクロでないなら無効値、関数マクロならtrue
    */
    fn is_macro(std::u8string_view identifier) const -> std::optional<bool> {
      //事前定義マクロのチェック
      if (m_predef_macro.contains(identifier)) return false;

      //それ以外のマクロのチェック
      return fetch_macro(identifier, [](const auto& macro) -> std::optional<bool> {
        return macro.is_function();
      }, std::optional<bool>{});
    }

    /**
    * @brief #undefディレクティブを実行する
    */
    void undef(std::u8string_view macro_name) {
      //消す、登録してあったかは関係ない
      m_funcmacros.erase(macro_name);
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
        m_line_map.emplace_hint(m_line_map.end(), std::make_pair(true_line, value));
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
        m_replace_filename = str;
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
    template<typename Reporter, typename TokensIterator, typename TokensSentinel>
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