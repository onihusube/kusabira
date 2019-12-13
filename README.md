# 茸 - kusabira - 🍄

kusabiraはC++コンパイラを目指すキノコです。

### 機能

- [x] CPPトークナイザ
- [ ] CPP構文解析
- [ ] CPP意味解析
- [ ] プリプロセッサ
- [ ] C++構文解析
- [ ] C++意味解析
- [ ] 中間コード生成
- [ ] LLVMバックエンドへ投げる

### ビルド（現在はテスト実行のみ）

- 必要なもの
  - [Meson](https://github.com/mesonbuild/meson)
  - Ninja (Windowsではない場合)
  - VC++2019 latest (Windowsの場合)

1. どこかのディレクトリにこのリポジトリをチェックアウトし、そこに移動します
    - Windowsの場合は*x64 Native Tools Command Prompt*を使用してください
2. コマンドラインで`meson build`を実行します
    - Windowsの場合は`meson build --backend vs`を実行します
3. するとそのディレクトリに`build`というディレクトリができるので、そこに移動します
4. `ninja`を実行するか、 Visual Studioのソリューションファイル(`kusabira.sln`)を開きビルドします

### 開発に使用しているコンパイラ

- VC++2019 Preview latest
- GCC 9.2 (on MacOS)

### ふわっとした方針

- 日本語でコメントを残す
- 当面処理速度よりも単純さを
  - すでにあやしい・・・
- やったことややっていることなどをドキュメント化しておきたい・・・

### 貢献

C++コンパイラ開発に興味がある方の御参画をお待ちしております。一緒にC++コンパイラ作ってみませんか？
