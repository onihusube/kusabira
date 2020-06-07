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
  * @brief プリプロセッシングトークン列の文字列化を行う
  * @param it 文字列化対象のトークン列の先頭
  * @param end 文字列化対象のトークン列の終端
  * @return 文字列化されたトークン（列）、必ず1要素になる
  */
  template<bool IsVA, typename Iterator, typename Sentinel>
  ifn pp_stringize(Iterator it, Sentinel end) -> std::pmr::list<pp_token> {
    using namespace std::string_view_literals;

    //結果のリスト
    std::pmr::list<pp_token> result{ &kusabira::def_mr };
    //文字列トークン
    pp_token& first = result.emplace_back(PP::pp_token_category::string_literal);

    //トークン列を順に文字列化するための一時文字列
    std::pmr::u8string str{u8"\""sv , &kusabira::def_mr};
    //字句トークン列挿入位置
    auto insert_pos = first.lextokens.before_begin();

    for (; it != end; ++it) {
      auto&& tmp = (*it).token.to_string();

      //"と\ をエスケープする必要がある
      if (auto cat = (*it).category; pp_token_category::charcter_literal <= cat and cat <= pp_token_category::user_defined_raw_string_literal) {
        //バックスラッシュをエスケープする
        constexpr auto npos = std::u8string::npos;
        auto bpos = tmp.find(u8'\\');
        while(bpos != npos) {
          tmp.replace(bpos, 1, u8R"(\\)");
          bpos = tmp.find(u8'\\', bpos + 2);
        }

        if (pp_token_category::string_literal <= cat) {
          //"をエスケープする
          auto lpos = tmp.find(u8'"');
          tmp.replace(lpos, 1, u8R"(\")");
          auto rpos = tmp.rfind(u8'"');
          tmp.replace(rpos, 1, u8R"(\")");
        }
      }
      str.append(std::move(tmp));
      
      if constexpr (IsVA) {
        //カンマの後にスペースを補う
        if ((*it).token == u8","sv) {
          str.append(u8" ");
        }
      }

      //構成する字句トークン列への参照を保存しておく
      insert_pos = first.lextokens.insert_after(insert_pos, (*it).lextokens.begin(), (*it).lextokens.end());
    }

    str.append(u8"\"");
    first.token = std::move(str);

    return result;
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
    //#トークンが仮引数名の前に来なかった
    std::optional<pp_token> m_is_sharp_op_err{};
    //{置換リストに現れる仮引数名のインデックス, 対応する実引数のインデックス, __VA_ARGS__?, __VA_OPT__?, #?, ##?}}
    std::pmr::vector<std::tuple<std::size_t, std::size_t, bool, bool, bool, bool>> m_correspond;

    /**
    * @brief 置換リスト上の仮引数を見つけて、仮引数の番号との対応を取っておく
    * @tparam Is_VA 可変長マクロか否か
    */
    template<bool Is_VA>
    void make_id_to_param_pair() {
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

      // 置換対象トークン数
      auto N = m_tokens.size();

      m_correspond.reserve(N);

      //#の出現をマークする
      bool apper_sharp_op = false;
      bool apper_sharp2_op = false;

      auto it = m_tokens.begin();
      for (auto index = 0ull; index < N; ++index, ++it) {
        //#,##をチェック
        if ((*it).category == pp_token_category::op_or_punc) {

          if ((*it).token == u8"#"sv) {
            //#も##も現れていないときだけ#を識別、#に対して#するのはエラー
            if (apper_sharp_op == false and apper_sharp2_op == false) {
              apper_sharp_op = true;
              continue;
            }
          } else if (not apper_sharp_op and (*it).token == u8"##"sv) {
            //#の後に##はエラー
            //##の後に##が現れる場合、それを結合対象にしてしまう（未定義動作に当たるはず）
            apper_sharp2_op = true;

            //最後に登録された対応関係が、1つ前だったかを調べる
            if (not empty(m_correspond) and std::get<0>(m_correspond.back()) == (index - 1)) {
              //1つ前が仮引数名のとき
              //##の左辺であることをマーク
              std::get<5>(m_correspond.back()) = true;
            } else {
              //1つ前は普通のトークン、もしくは空の時
              //置換対象リストに連結対象として加える
              m_correspond.emplace_back(index - 1, std::size_t(-1), false, false, false, true);
            }

            continue;
          }
        }

        //1つ前の#,##トークンを削除する
        if (apper_sharp_op or apper_sharp2_op) {
          //1つ前のトークン（すなわち#,##）を消す
          m_tokens.erase(std::prev(it));
          //消した分indexとトークン数を修正
          --index;
          N = m_tokens.size();
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
              m_is_sharp_op_err = std::move(*it);
              break;
          }
          continue;
        }

        if constexpr (Is_VA) {
          //可変引数参照のチェック
          if ((*it).token.to_view() == u8"__VA_ARGS__") {
            const auto [ismatch, va_start_index] = find_param_index(u8"...");

            //可変長マクロでは無いのに__VA_ARGS__が参照された?
            assert(ismatch);

            //置換リストの要素番号に対して、対応する実引数位置を保存
            m_correspond.emplace_back(index, va_start_index, true, false, apper_sharp_op, false);
            continue;
          } else if ((*it).token.to_view() == u8"__VA_OPT__") {
            //__VA_OPT__の前に#は来てはならない
            if (apper_sharp_op == true) {
              //エラー
              m_is_sharp_op_err = std::move(*it);
              break;
            }
            //__VA_OPT__を見つけておく
            m_correspond.emplace_back(index, 0, false, true, false, false);
            continue;
          }
        }

        const auto [ismatch, param_index] = find_param_index((*it).token.to_view());
        //仮引数名ではないから次
        if (not ismatch) {
          //#が観測されているのに仮引数名ではない場合
          if (apper_sharp_op == true) {
            //エラー
            m_is_sharp_op_err = std::move(*it);
            break;
          }
          continue;
        }

        //置換リストの要素番号に対して、対応する実引数位置を保存
        m_correspond.emplace_back(index, param_index, false, false, apper_sharp_op, false);
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

      //プレイスメーカートークンを挿入したかどうか
      bool should_remove_placemarker = false;

      //置換リスト-引数リスト対応を後ろから処理
      for (const auto[token_index, arg_index, ignore1, ignore2, sharp_op, sharp2_op] : views::reverse(m_correspond)) {
        //置換リストのトークン位置
        const auto it = std::next(head, token_index);
        //##による結合時の右辺を指すイテレータ
        auto rhs = std::next(it);

        if (arg_index != std::size_t(-1)) {
          //対応する実引数のトークン列をコピー
          std::pmr::list<pp_token> arg_list{args[arg_index], &kusabira::def_mr};

          //文字列化
          if (sharp_op) {
            arg_list = pp_stringize<false>(std::begin(arg_list), std::end(arg_list));
          } else if (empty(arg_list)) {
            //文字列化対象ではなく引数が空の時、プレイスメーカートークンを挿入しておく
            result_list.insert(it, pp_token{ pp_token_category::placemarker_token });
            should_remove_placemarker = true;
          }

          //結果リストにsplice
          result_list.splice(it, std::move(arg_list));

          //置換済みトークンを消す
          rhs = result_list.erase(it);
        }

        if (sharp2_op) {
          auto lhs = std::prev(rhs);
          (*lhs) += std::move(*rhs);
          result_list.erase(rhs);
        }

      }

      if (should_remove_placemarker) {
        result_list.remove_if([](const auto& pptoken) {
          return pptoken.category == pp_token_category::placemarker_token;
        });
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
      //プレイスメーカートークンを挿入したかどうか
      bool should_remove_placemarker = false;

      //プレイスメーカートークン挿入処理
      auto insert_placemarker = [&result_list, &should_remove_placemarker](auto pos) {
        result_list.insert(pos, pp_token{pp_token_category::placemarker_token});
        should_remove_placemarker = true;
      };
      //##によるトークン結合処理
      auto token_concat = [&result_list](auto rhs_pos) {
        auto &lhs = *std::prev(rhs_pos);
        lhs += std::move(*rhs_pos);
        //結合したので次のトークンを消す
        result_list.erase(rhs_pos);
      };

      //置換リスト-引数リスト対応を後ろから処理
      for (const auto[token_index, arg_index, va_args, va_opt, sharp_op, sharp2_op] : views::reverse(m_correspond)) {
        //置換リストのトークン位置
        const auto it = std::next(head, token_index);

        if (va_opt) {
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
            auto insert_pos = result_list.erase(it, ++vaopt_end);

            //##の処理のためにplacemarker tokenを置いておく
            //VA_OPTが##の左辺の時は何もしなくてもいい
            if (not sharp2_op) insert_placemarker(insert_pos);

          } else {
            //可変長部分が空でないならば、VA_OPTと対応するかっこだけを削除

            //まずは後ろの閉じ括弧を削除（イテレータが無効化するのを回避するため）
            auto rhs = result_list.erase(vaopt_end);
            //次に開きかっことVA_OPT本体を削除
            result_list.erase(it, open_paren_pos_next);

            //#__VA_OPT__はエラー
            assert(not sharp_op);

            if (sharp2_op) {
              //##によるトークンの結合処理
              token_concat(rhs);
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
              auto& comma = arg_list.emplace_back(pp_token_category::op_or_punc);
              comma.token = u8","sv;
              //エラーメッセージのためにコンテキストを補う必要がある？
            }
          }
        } else if (arg_index != std::size_t(-1)) {
          //対応する実引数のトークン列をコピー
          arg_list = args[arg_index];
        }

        if (sharp_op) {
          //#演算子の処理、文字列化を行う
          if (va_args) {
            arg_list = pp_stringize<true>(std::begin(arg_list), std::end(arg_list));
          } else {
            arg_list = pp_stringize<false>(std::begin(arg_list), std::end(arg_list));
          }
        } else if (empty(arg_list)){
          //文字列化対象ではない時、プレイスメーカートークンを挿入する
          //文字列化対象の場合は空文字（""）が入っているはず
          insert_placemarker(it);
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
          token_concat(rhs);
        }
      }

      if (should_remove_placemarker) {
        result_list.remove_if([](const auto &pptoken) {
          return pptoken.category == pp_token_category::placemarker_token;
        });
      }

      return result_list;
    }

    /**
    * @brief オブジェクトマクロの置換リスト中の##トークンを処理する
    */
    void objmacro_token_concat() {
      using namespace std::string_view_literals;

      auto it = std::begin(m_tokens);
      auto end = std::end(m_tokens);

      for (; it != end; ++it) {
        if ((*it).category != pp_token_category::op_or_punc) continue;
        if ((*it).token != u8"##"sv) continue;

        //一つ前のイテレータを得ておく
        auto prev = it;
        --prev;

        //##を消して次のイテレータを得る
        auto next = m_tokens.erase(it);

        (*prev) += std::move(*next);

        //結合したので次のトークンを消す
        m_tokens.erase(next);

        it = prev;
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
        this->make_id_to_param_pair<true>();
      } else {
        this->make_id_to_param_pair<false>();
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
    * @brief マクロの実行が可能かを取得、オブジェクトマクロはチェックの必要なし
    * @details falseの時、#の後に仮引数が続かなかった
    * @return マクロ実行がいつでも可能ならばtrue
    */
    fn is_ready() const -> const std::optional<pp_token>& {
      return m_is_sharp_op_err;
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

    using funcmacro_map = std::pmr::unordered_map<std::u8string_view, unified_macro>;

    std::size_t m_line = 1;
    fs::path m_filename{};
    fs::path m_replace_filename{};
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
    fn define(Reporter &reporter, const PP::lex_token& macro_name, ReplacementList &&tokenlist, [[maybe_unused]] ParamList&& params = {}, [[maybe_unused]] bool is_va = false) -> bool {
      using namespace std::string_view_literals;

      if (tokenlist.size() != 0) {
        //先頭と末尾の##の出現を調べる、出てきたらエラー
        pp_token* ptr = nullptr;

        //エラー的にはソースコード上で最初に登場するやつを出したい気がする
        if (auto& back = tokenlist.back(); back.token == u8"##"sv) ptr = &back;
        if (auto& front = tokenlist.front(); front.token == u8"##"sv) ptr = &front;

        if (ptr != nullptr) {
          //まあこれは起こらないと思う
          assert((*ptr).lextokens.begin() != (*ptr).lextokens.end());

          //##トークンが置換リストの前後に出現している
          reporter.pp_err_report(m_filename, (*ptr).lextokens.front(), pp_parse_context::Define_Sharp2BothEnd);
          return false;
        }
      }

      bool error = false;

      if constexpr (std::is_same_v<ParamList, std::nullptr_t>) {
        //オブジェクトマクロの登録
        const auto [pos, is_registered] = m_funcmacros.try_emplace(macro_name.token, std::forward<ReplacementList>(tokenlist));

        if (not is_registered) {
          if ((*pos).second.is_identical({}, tokenlist)) return true;
          //置換リストが一致していなければエラー
          error = true;
        }
      } else {
        static_assert([] { return false; }() || std::is_same_v<std::remove_cvref_t<ParamList>, std::pmr::vector<std::u8string_view>>, "ParamList must be std::pmr::vector<std::u8string_view>.");
        //関数マクロの登録
        const auto [pos, is_registered] = m_funcmacros.try_emplace(macro_name.token, std::forward<ParamList>(params), std::forward<ReplacementList>(tokenlist), is_va);

        if (not is_registered) {
          if ((*pos).second.is_identical(params, tokenlist)) return true;
          //仮引数列と置換リストが一致していなければエラー
          error = true;
        } else {
          if (auto& opt = (*pos).second.is_ready(); bool(opt)) {
            //#が仮引数の前ではない所に来ていた、エラー
            reporter.pp_err_report(m_filename, (*opt).lextokens.front(), pp_parse_context::Define_InvalidSharp);
            return false;
          }
        }
      }

      if (error) {
        //同名の異なる定義のマクロが登録された、エラー
        reporter.pp_err_report(m_filename, macro_name, pp_parse_context::Define_Duplicate);
        return false;
      }

      return true;
    }

  private:

    /**
    * @brief 対応するマクロを取り出す
    * @param macro_name マクロ名
    * @param found_op マクロが見つかった時の処理
    * @param nofound_val 見つからなかった時に返すもの
    * @return found_op()の戻り値かnofound_valを返す
    */
    template<typename Found, typename NotFound>
    fn fetch_macro(std::u8string_view macro_name, Found&& found_op, NotFound&& nofound_val) const {
      using std::end;

      const auto pos = m_funcmacros.find(macro_name);

      //見つからなかったら、その時用の戻り値を返す
      if (pos == end(m_funcmacros)) {
        return nofound_val;
      }

      //見つかったらその要素を与えて処理を実行
      return found_op((*pos).second);
    }

  public:

    /**
    * @brief マクロによる置換リストを取得する
    * @param macro_name 識別子トークン名
    * @return 置換リストのoptional、無効地なら置換対象ではなかった
    */
    fn objmacro(std::u8string_view macro_name) const -> std::optional<std::pmr::list<pp_token>> {
      return fetch_macro(macro_name, [&mr = kusabira::def_mr](const auto& macro) {
        //置換結果取得
        auto&& result = macro({});
        //optionalで包んで返す
        return std::optional<std::pmr::list<pp_token>>{std::in_place, std::move(result), &mr};
      }, std::optional<std::pmr::list<pp_token>>{});
    }

    /**
    * @brief 関数マクロによる置換リストを取得する
    * @param macro_name マクロ名
    * @param args 関数マクロの実引数トークン列
    * @return {仮引数の数と実引数の数があったか否か, 置換リストのoptional}
    */
    fn funcmacro(std::u8string_view macro_name, const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::pair<bool, std::optional<std::pmr::list<pp_token>>> {
      return fetch_macro(macro_name, [&mr = kusabira::def_mr, &args](const auto& macro) -> std::pair<bool, std::optional<std::pmr::list<pp_token>>> {
        //引数長さのチェック、ここでエラーにしたい
        if (not macro.validate_argnum(args)) return {false, std::nullopt};

        //置換結果取得
        auto&& result = macro(args);

        //optionalで包んで返す
        return {true, std::optional<std::pmr::list<pp_token>>{std::in_place, std::move(result), &mr}};
      }, std::make_pair(true, std::optional<std::pmr::list<pp_token>>{}));
    }

    /**
    * @brief 識別子がマクロ名であるか、また関数形式かをチェックする
    * @param identifier 識別子の文字列
    * @return マクロでないなら無効値、関数マクロならtrue
    */
    fn is_macro(std::u8string_view identifier) const -> std::optional<bool> {
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