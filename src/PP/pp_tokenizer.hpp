#pragma once

#include <fstream>
#include <filesystem>
#include <memory_resource>
#include <optional>
#include <forward_list>
#include <cctype>
#include <string>

#include "common.hpp"
#include "pp_automaton.hpp"

namespace kusabira::PP {

  
  /**
  * @brief ソースコードの論理行での1行を表現する型
  * @detail 改行継続（バックスラッシュ+改行）後にソースファイルの物理行との対応を取るためのもの
  */
  struct logical_line {

    //()集成体初期化がポータブルになったならこのコンストラクタは消える定め
    logical_line(u8_pmralloc& alloc, std::size_t line_num)
      : line{alloc}
      , phisic_line{line_num}
      , line_offset{alloc}
    {}

    //論理1行の文字列
    std::pmr::u8string line;

    //元の開始行
    std::size_t phisic_line;

    //1行毎の文字列長、このvectorの長さ=継続行数
    std::pmr::vector<std::size_t> line_offset;
  };

  using maybe_line = std::optional<logical_line>;


  /**
  * @brief ソースファイルから文字列を読み出す処理を担う
  * @detail 入力はUTF-8固定、BOM有無、改行コードはどちらでもいい
  */
  class filereader {

    //入力ファイルストリーム
    std::ifstream m_srcstream;
    //char8_tの多相アロケータ
    u8_pmralloc m_alloc;
    //1行分を一時的に持っておくバッファ
    std::pmr::string m_buffer;
    //現在の物理行数
    std::size_t m_line_num = 1;

    /**
    * @brief ファイルから1行を読み込み、引数末尾に追記する
    * @param str 結果を追記する文字列
    * @return 読み込み成否を示すbool値
    */
    fn readline_impl(std::pmr::u8string& str) -> bool {
      if (std::getline(m_srcstream, m_buffer)) {
        const auto first = reinterpret_cast<char8_t*>(m_buffer.data());
        auto last = first + m_buffer.size();

#ifndef _MSC_VER
        //CRLF読み取り時にCRが残っていたら飛ばす、非Windowsの時のみ
        //バグの元になる可能性はある？要検証
        if (not m_buffer.empty() and m_buffer.back() == '\r') {
          --last;
        }
#endif
        str.append(first, last);

        //物理行数をカウント
        ++m_line_num;

        return true;
      } else {
        return false;
      }
    }

  public:

    filereader(const fs::path& filepath, std::pmr::memory_resource* mr = &kusabira::def_mr)
      : m_srcstream{filepath}
      , m_alloc{mr}
      , m_buffer{m_alloc}
    {
      m_srcstream.sync_with_stdio(false);
      m_srcstream.tie(nullptr);

      //BOMスキップ
      if (m_srcstream) {
        int first = m_srcstream.peek();
        if (first == 0xef) {
          m_srcstream.seekg(3, std::ios::beg);
        }
      }
    }

    /**
    * @brief ファイルからソースコードの論理1行を取得する
    * @detail BOMはスキップされ、末尾に改行コードは現れず、バックスラッシュによる行継続が処理済
    * @detail すなわち、翻訳フェーズ2までを完了する（1行分）
    * @return 論理行型のoptional
    */
    fn readline() -> maybe_line {
      logical_line ll{m_alloc, m_line_num};

      if (this->readline_impl(ll.line)) {
        //空行のチェック
        if (ll.line.empty() == false) {
          //バックスラッシュによる行継続と論理行生成
          while (ll.line.back() == u8'\x5c') {
            auto last = ll.line.end() - 1;
            //バックスラッシュ2つならびは行継続しない
            if (*(last - 1) == u8'\x5c') break;

            //行末尾バックスラッシュを削除
            ll.line.erase(last);
            //現在までの行の文字列長を記録
            ll.line_offset.emplace_back(ll.line.length());

            //次の行を読み込み、追記
            if (this->readline_impl(ll.line) == false) break;
            //読み込みに成功したら再び行継続チェック
          }
        }

        return maybe_line{std::in_place, std::move(ll)};
      } else {
        return std::nullopt;
      }
    }

    explicit operator bool() const noexcept {
      return bool(m_srcstream);
    }
  };

  /**
  * @brief プリプロセッシングトークン1つを表現する型
  */
  struct pp_token {
    using line_iterator = std::pmr::forward_list<logical_line>::const_iterator;

    //()集成体初期化がポータブルになったならこのコンストラクタは消える定め
    pp_token(kusabira::PP::pp_tokenize_result result, std::u8string_view view, line_iterator line)
      : kind{ std::move(result) }
      , token{ std::move(view) }
      , srcline_ref{ std::move(line) }
    {}

    //トークン種別
    kusabira::PP::pp_tokenize_result kind;

    //トークン文字列
    std::u8string_view token;

    //対応する論理行オブジェクトへのイテレータ
    line_iterator srcline_ref;
  };

  /**
  * @brief トークナイザのイテレータの番兵型
  */
  struct pp_token_iterator_end {
    using difference_type = std::size_t;
    using value_type = pp_token;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
  };

  /**
  * @brief トークナイザのイテレータ型
  * @tparam Tokenizer 対象となるトークナイザクラス
  */
  template<typename Tokenizer>
  class pp_token_iterator {
    Tokenizer* m_tokenizer = nullptr;
    std::optional<pp_token> m_token_opt = std::nullopt;

  public:
    using difference_type = std::size_t;
    using value_type = pp_token;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;

    pp_token_iterator() = default;

    pp_token_iterator(Tokenizer& ref_tokenizer)
      : m_tokenizer{&ref_tokenizer}
    {
      m_token_opt = m_tokenizer->tokenize();
    }

    pp_token_iterator(const pp_token_iterator &) = default;
    pp_token_iterator &operator=(const pp_token_iterator &) = default;

    auto operator++() -> pp_token_iterator& {
      m_token_opt = m_tokenizer->tokenize();
      return *this;
    }

    fn operator*() -> reference {
      return *m_token_opt;
    }

    fn operator==(const pp_token_iterator&) const noexcept -> bool {
      return false;
    }

    fn operator==(pp_token_iterator_end) const noexcept -> bool {
      //トークンを受けているoptionalが空ならファイル末尾に到達している
      return !m_token_opt.has_value();
    }

#ifndef __cpp_impl_three_way_comparison
    fn operator!=(pp_token_iterator_end end) const noexcept -> bool {
      return !(*this == end);
    }
#endif
  };



  /**
  * @brief ソースファイルからプリプロセッシングトークンを抽出する
  * @detail ファイルの読み込みはこのクラスでは実装しない
  */
  class tokenizer {
    using line_iterator = std::pmr::forward_list<logical_line>::const_iterator;
    using char_iterator = std::pmr::u8string::const_iterator;

    //ファイルリーダー
    filereader m_fr;
    //トークナイズのためのオートマトン
    kusabira::PP::pp_tokenizer_sm m_accepter{};
    //行バッファ
    std::pmr::forward_list<logical_line> m_lines;
    //行位置
    line_iterator m_line_pos;
    //文字位置
    char_iterator m_pos{};
    //文字の終端
    char_iterator m_end{};
    //終了判定、ファイルを読み切ったらtrue
    bool m_is_terminate = false;

    /**
    * @brief 現在の読み取り行を進める
    * @return ファイル終端に到達したか否か
    */
    fn readline() -> bool {
      if (auto line_opt = m_fr.readline(); line_opt) {
        //次の行の読み出しに成功したら、行バッファに入れておく（optionalを剥がす）
        m_line_pos = m_lines.emplace_after(m_line_pos, *std::move(line_opt));

        //現在の文字参照位置を更新
        auto& str = (*m_line_pos).line;
        m_pos = str.begin();
        m_end = str.end();

        return false;
      } else {
        return true;
      }
    }

  public:
    using iterator = pp_token_iterator<tokenizer>;

    tokenizer(const fs::path& srcpath, std::pmr::memory_resource* mr = &kusabira::def_mr) 
      : m_fr{srcpath, mr}
      , m_lines{ std::pmr::polymorphic_allocator<logical_line>{mr} }
      , m_line_pos{m_lines.before_begin()}
    {
      //とりあえず1行読んでおく
      m_is_terminate = this->readline();
    }

    /**
    * @brief トークンを一つ切り出す
    * @return 切り出したトークンのoptional
    */
    fn tokenize() -> std::optional<pp_token> {
      using kusabira::PP::pp_tokenize_status;
      using kusabira::PP::pp_tokenize_result;

      //読み込み終了のお知らせ
      if (m_is_terminate == true) return std::nullopt;

      //現在の先頭文字位置を記録
      const auto first = m_pos;

      //行末に到達するまで、1文字づつ読んでいく
      for (; m_pos != m_end; ++m_pos) {
        if (auto is_accept = m_accepter.input_char(*m_pos); is_accept) {
          //受理、エラーとごっちゃ
          return std::optional<pp_token>{std::in_place, std::move(is_accept), std::u8string_view{&*first, std::size_t(std::distance(first, m_pos))}, m_line_pos};
        } else {
          //非受理
          continue;
        }
      }
      //ここに出てきた場合、その行の文字を全て見終わったということ

      //改行入力、先にreadline()してしまうと関連変数が更新されてしまうため事前に作っておく
      std::optional<pp_token> newline_result{std::in_place, m_accepter.input_newline(), std::u8string_view{&*first, std::size_t(std::distance(first, m_pos))}, m_line_pos};

      //次の行を読み込む
      m_is_terminate = this->readline();

      return newline_result;
    }

    ffn begin(tokenizer& attached_tokenizer) -> pp_token_iterator<tokenizer> {
      return pp_token_iterator<tokenizer>{attached_tokenizer};
    }

    ffn end(const tokenizer&) -> pp_token_iterator_end {
      return {};
    }
  };

} // namespace kusabira::PP