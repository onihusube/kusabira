project('kusabira', 'cpp', default_options : ['warning_level=3', 'cpp_std=c++17'], meson_version : '>=0.50.0')

#依存プロジェクト
doctest_proj = subproject('doctest')
doctest_dep = doctest_proj.get_variable('doctest_dep')

#コンパイルオプション調整
cppcompiler = meson.get_compiler('cpp').get_argument_syntax()

if cppcompiler == 'msvc'
    options = ['/std:c++latest', '/source-charset:utf-8']
elif cppcompiler == 'gcc'
    options = ['-std=c++2a']
else
    options = []
endif

#VSプロジェクトに編集しうるファイルを追加する
files = []

include_dir = include_directories('src', 'subprojects/doctest')

exe = executable('kusabira_test', 'test/kusabira_test.cpp', include_directories : include_dir, extra_files : files, cpp_args : options, dependencies : doctest_dep)