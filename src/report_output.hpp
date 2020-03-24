#pragma once

#include "common.hpp"

namespace kusabira::report
{

  enum class report_type : std::uint8_t {

    error,
    warning

  };


  inline void error_directive(std::u8string_view msg, const PP::lex_token& lextoken) {

  }

  namespace detail {

    ifn print_u8string(std::u8string_view str) -> std::basic_ostream<char>& {
      auto punned_str = reinterpret_cast<const char*>(str.data());
      return (std::cout << punned_str);
    }

  }

  inline void print_report(std::u8string_view message, fs::path& filename, PP::lex_token context, report_type kind = report_type::error) {
    //<file名>:<行>:<列>: <メッセージのカテゴリ>: <本文>
    //の形式で出力
    
    //ファイル名を出力
    detail::print_u8string(filename.filename().u8string()) << ":";

    //物理行番号、列番号を出力


  }

} // namespace kusabira::report
