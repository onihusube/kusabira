#pragma once

#include <charconv>
#include <functional>
#include <algorithm>
#include <tuple>
#include <chrono>

#include "../common.hpp"
#include "../report_output.hpp"
#include "unified_macro.hpp"

namespace kusabira::PP {

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
      const auto name_str = macro_name.token.to_view();

      if constexpr (std::is_same_v<ParamList, std::nullptr_t>) {
        //オブジェクトマクロの登録
        const auto [pos, is_registered] = m_funcmacros.try_emplace(name_str, name_str, std::forward<ReplacementList>(tokenlist));

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
        const auto [pos, is_registered] = m_funcmacros.try_emplace(name_str, name_str, std::forward<ParamList>(params), std::forward<ReplacementList>(tokenlist), is_va);

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
    template<bool MacroExpandOff = false, typename Reporter>
    fn objmacro(Reporter& reporter, const pp_token& macro_name) const -> std::tuple<bool, bool, std::pmr::list<pp_token>, std::pmr::unordered_set<std::u8string_view>> {
      std::pmr::unordered_set<std::u8string_view> memo{&kusabira::def_mr};
      auto&& tuple = this->objmacro<MacroExpandOff>(reporter, macro_name, memo);
      return std::tuple_cat(std::move(tuple), std::make_tuple(std::move(memo)));
    }

    /**
    * @brief マクロによる置換リストを取得する
    * @tparam MacroExpandOff さらにマクロ展開（展開結果の再スキャン）を行うか否か
    * @param macro_name 識別子トークン名
    * @return 置換リストのoptional、無効地なら置換対象ではなかった
    */
    template<bool MacroExpandOff = false, typename Reporter>
    fn objmacro(Reporter& reporter, const pp_token& macro_name, std::pmr::unordered_set<std::u8string_view>& outer_macro) const -> std::tuple<bool, bool, std::pmr::list<pp_token>> {
      
      //事前定義マクロを処理（この結果には再スキャンの対象となるものは含まれていないはず）
      if (auto result = predefined_macro(macro_name); result) {
        return {true, true, std::move(*result)};
      }

      //マクロを取り出す（存在は予め調べてあるものとする）
      const auto& macro = deref(m_funcmacros.find(macro_name.token)).second;

      // ここでmemory_resourceを適切に設定しておかないと、アロケータが正しく伝搬しない
      unified_macro::macro_result_t result{tl::in_place, &kusabira::def_mr};

      // 第一弾マクロ展開（オブジェクトマクロでは引数内マクロ置換は常に不要）
      result = macro({});
      
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
    * @tparam MacroExpandOff さらにマクロ展開（引数内マクロの再帰展開と展開結果の再スキャン）を行うか否か
    * @param macro_name マクロ名
    * @param args 関数マクロの実引数トークン列
    * @return {エラーの有無, スキャン完了したか, 置換リスト}
    */
    template<bool MacroExpandOff = false, typename Reporter>
    fn funcmacro(Reporter& reporter, const pp_token& macro_name, const std::pmr::vector<std::pmr::list<pp_token>>& args) const -> std::tuple<bool, bool, std::pmr::list<pp_token>, std::pmr::unordered_set<std::u8string_view>> {
      std::pmr::unordered_set<std::u8string_view> memo{&kusabira::def_mr};
      auto&& tuple = this->funcmacro<MacroExpandOff>(reporter, macro_name, args, memo);
      return std::tuple_cat(std::move(tuple) , std::make_tuple(std::move(memo)));
    }
    /**
    * @brief 関数マクロによる置換リストを取得する
    * @tparam MacroExpandOff さらにマクロ展開（引数内マクロの再帰展開と展開結果の再スキャン）を行うか否か
    * @param macro_name マクロ名
    * @param args 関数マクロの実引数トークン列
    * @return {エラーの有無, スキャン完了したか, 置換リスト}
    */
    template<bool MacroExpandOff = false, typename Reporter>
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