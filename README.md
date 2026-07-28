# Umoria (Japanese Localization)

_The Dungeons of Moria_ is a single player dungeon simulation originally
written by Robert Alan Koeneke, with its first public release in 1983.
The game was originally developed using VMS Pascal before being ported to the
C language by James E. Wilson in 1988, and released as _Umoria_.

Moria/Umoria has had many variants over the years, with [_Angband_](http://rephial.org/)
being the most well known. Umoria was also an inspiration for one the most
commercially successful action roguelike games, _Diablo_!

Supported Platforms:

  - Windows
  - macOS
  - Linux (Ubuntu/Debian)

Compiling and limited testing has been done for other Linux based system
including NetBSD 8.1 and Fedora 32.

_Note: the platforms above describe the base (English) game. The Japanese
localization added by this fork has only been verified on Linux; Windows and
macOS are untested for Japanese display. See "About this fork" below._


## About this fork / 本フォークについて

This repository is a Japanese localization fork of
[`dungeons-of-moria/umoria`](https://github.com/dungeons-of-moria/umoria). It
is not an official upstream project. The fork adds a Japanese message
catalog and the display-width-aware rendering needed to show multi-byte
text correctly in a terminal, but makes no intentional changes to gameplay.

This fork is Japanese-only, not a general-purpose internationalization
framework: a handful of language-specific branches (`lang::currentLanguage()
== "ja"`, 11 call sites under `src/`) are hardcoded for Japanese grammar
handling rather than driven by a generic rule engine. See
[`docs/TRANSLATING.md`](docs/TRANSLATING.md) for the technical details a
future contributor would need to add another language.

**RPM package naming**: this fork's `umoria-jp.spec` builds an RPM package
called `umoria-jp` (not `umoria`), letting it install alongside the
upstream `umoria` package without file conflicts — the installed command
is `umoria-jp`, with a per-user config/save directory of
`~/.config/umoria-jp` (separate from upstream's `~/.config/umoria`). Note
the asymmetry if you build directly with CMake instead of the RPM: the
CMake target itself stays named `umoria` (to minimize the diff against
upstream), so a from-source build still produces a binary named `umoria`,
not `umoria-jp` — only the RPM-packaged install uses the `umoria-jp` name.

Where to report issues: translation mistakes or Japanese-display bugs belong
in this fork's [Issues](https://github.com/whitehara/umoria-jp/issues);
gameplay/engine bugs that also affect the English original should go to
[upstream's issue tracker](https://github.com/dungeons-of-moria/umoria/issues)
instead.

本リポジトリは [`dungeons-of-moria/umoria`](https://github.com/dungeons-of-moria/umoria)
の日本語ローカライズフォークであり、upstreamの公式プロジェクトではありません。
日本語メッセージカタログと、端末上でマルチバイト文字を正しく表示するための
表示幅対応の描画処理を追加していますが、ゲームプレイ自体への意図的な変更は
行っていません。

本フォークは汎用的な多言語対応基盤ではなく、日本語専用です。`src/`配下の
11箇所で`lang::currentLanguage() == "ja"`という言語判定のハードコード分岐を
使い、日本語特有の文法処理を行っています。新しい言語を追加する場合に必要な
技術的詳細は [`docs/TRANSLATING.md`](docs/TRANSLATING.md) を参照してください。

**RPMパッケージ名について**: 本フォークの`umoria-jp.spec`は、RPMパッケージ名を
（`umoria`ではなく）`umoria-jp`として生成します。upstreamの`umoria`パッケージと
ファイル衝突せず共存でき、インストールされるコマンド名は`umoria-jp`、
設定・セーブデータのディレクトリは`~/.config/umoria-jp`（upstreamの
`~/.config/umoria`とは別）になります。CMakeで直接ビルドする場合との非対称に
注意してください: CMakeのターゲット名は（upstreamとの差分を最小限にするため）
`umoria`のままなので、ソースから直接ビルドしたバイナリ名は`umoria-jp`ではなく
`umoria`のままです（名前が変わるのはRPMパッケージ経由のインストール時のみ）。

不具合の報告先: 翻訳の誤りや日本語表示の不具合は本フォークの
[Issues](https://github.com/whitehara/umoria-jp/issues)へ、英語版にも共通する
ゲームロジック上の不具合は
[upstreamのIssueトラッカー](https://github.com/dungeons-of-moria/umoria/issues)
へお願いします。


## Umoria 5.7.x releases

The main focus of the `5.7.0` release was to provide support for the three
main operating systems: Windows, macOS, and Linux. Support for all other
outdated computer systems such as MS DOS, "Classic" Mac OS (pre OSX), Amiga,
and Atari ST was removed.

_Note: there have been no intentional gameplay changes in the 5.7.x releases._

Since the initial 5.7 release, a great deal of _code restoration_ has been
undertaken in the hope of aiding future development of the game. Some examples
of the work done include reformatting the source code with the help of
`clang-tidy` and `clang-format`, modernizing the code to use standard C types,
breaking apart most large functions (many of which had hundreds of lines of code)
into smaller, easier to read functions, and fixing all compiler warnings when
compiling against recent versions of GCC and Clang.

Full details of all changes can be found in the [CHANGELOG](CHANGELOG.md), and
by browsing the commit history.

Due to its lack of Windows and macOS support Moria was inaccessible to many
people. Hopefully these changes will give many more people a chance to play
this classic roguelike game.


## Notes on Compiling Umoria

Umoria has been tested against GCC (`10` and `11`) and with `ncurses 6.x`,
although recent earlier versions should also work fine.

You will need these as well as `CMake` and the C++ build tools for your system.


### macOS and Linux

Change to the `umoria` game directory and enter the following commands at the
terminal:

    $ mkdir build && cd build
    $ cmake ..
    $ make

NOTE: use `make -j $(nproc)` to speed up compilation on Linux.

An `umoria` directory will be created in the current directory containing the
game binary and data files, which can then be moved to any other location, such
as the `home` directory.


### Windows

MinGW is used to provide GCC and GNU Binutils for compiling on the Windows platform.
The easiest solution to get set up is to use the [MSYS2 Installer](http://msys2.github.io/).
Once installed, `pacman` can be used to install `GCC`, `ncurses`, and the
`make`/`cmake` build tools.

At present an environment variable for the MinGW system being compiled on will
need to be specified. This will be either `mingw64` or `mingw32`.

At the command prompt type the following, being sure to add the correct label
to `MINGW=`:

    $ MINGW=mingw64 cmake .
    $ make

To perform an out-of-source build, type the following:

    $ mkdir build
    $ cd build
    $ MINGW=mingw64 cmake ..
    $ make

As with the macOS/Linux builds, all files will be installed into an `umoria` directory.


### Building an RPM / SRPM package

`scripts/make-srpm.sh` builds a source RPM (`.src.rpm`) for `umoria-jp` directly
from a git ref (default `HEAD`), without touching your working tree. Run it
from anywhere inside the repository:

    $ bash scripts/make-srpm.sh

The generated files are placed under `srpm-out/` (set the `outdir` environment
variable to change this). Building the binary RPM from the resulting SRPM
requires `mock` or `rpmbuild` and is not covered here.

`scripts/make-srpm.sh` は、作業ツリーを変更することなく、指定した git の ref
（既定は `HEAD`）から `umoria-jp` のソース RPM（`.src.rpm`）を直接生成します。
リポジトリ内のどこからでも次のように実行できます。

    $ bash scripts/make-srpm.sh

生成物は `srpm-out/`（`outdir` 環境変数で変更可能）以下に置かれます。
生成された SRPM からバイナリ RPM を作る手順（`mock` または `rpmbuild` が必要）は
本書の対象外です。

### Installing from Copr (Fedora)

Pre-built packages for Fedora (currently fedora-43, fedora-44, and
fedora-rawhide on x86_64) are published via
[Copr](https://copr.fedorainfracloud.org/coprs/whitehara/umoria-jp/):

    $ sudo dnf copr enable whitehara/umoria-jp
    $ sudo dnf install umoria-jp

This installs alongside the upstream `umoria` package (if present) without
conflict — the binary, config directory (`~/.config/umoria-jp`), and save
data are all kept separate from `umoria`'s.

Fedora（現時点では fedora-43・fedora-44・fedora-rawhide の x86_64）向けの
ビルド済みパッケージを
[Copr](https://copr.fedorainfracloud.org/coprs/whitehara/umoria-jp/) で
配布しています。

    $ sudo dnf copr enable whitehara/umoria-jp
    $ sudo dnf install umoria-jp

英語版の `umoria` パッケージがインストール済みでも衝突なく併存できます
（バイナリ・設定ディレクトリ（`~/.config/umoria-jp`）・セーブデータはいずれも
`umoria` とは別に管理されます）。


### Installing the .deb (Debian / Ubuntu)

Pre-built `.deb` packages (Ubuntu 22.04+ / Debian 12+, x86_64) are attached
to each [GitHub Release](https://github.com/whitehara/umoria-jp/releases).
Download the latest `umoria-jp_<version>_amd64.deb` and install it with:

    $ sudo apt install ./umoria-jp_<version>_amd64.deb

Using `apt install ./...` (rather than `dpkg -i`) is important: the leading
`./` tells `apt` to resolve and install the package's dependencies
automatically. As with the Copr package, this installs alongside the
upstream `umoria` package (if present) without conflict.

Debian/Ubuntu 向けのビルド済み `.deb` パッケージ（Ubuntu 22.04 以降 /
Debian 12 以降、x86_64）は、各
[GitHub Release](https://github.com/whitehara/umoria-jp/releases) に
添付されています。最新の `umoria-jp_<version>_amd64.deb` をダウンロードし、
次のようにインストールしてください。

    $ sudo apt install ./umoria-jp_<version>_amd64.deb

`dpkg -i` ではなく `apt install ./...` を使うのが重要です（先頭の `./` に
より `apt` が依存関係を自動的に解決してインストールします）。Copr版と
同様、英語版の `umoria` パッケージと衝突なく併存できます。


## Language / 言語設定

Umoria supports a Japanese translation alongside the original English. The
active language is chosen, in order of priority: the `-l <lang>` command
line flag, the `MORIA_LANG` environment variable, the system `LANG`/`LC_ALL`
locale, falling back to English if none of those resolve to a translation.

    $ umoria -l ja
    $ MORIA_LANG=ja umoria

Translation data lives under `data/lang/<lang>/`. A language with no
catalog, or an untranslated string within one, falls back to English
automatically. Building with a wide-character ncurses (`ncursesw`) is
required for correct display; this is handled automatically by the CMake
build on Linux/macOS.

Adding a new language catalog requires touching some Japanese-specific
hardcoded logic; see [`docs/TRANSLATING.md`](docs/TRANSLATING.md) for the
details.

Umoriaは英語版に加えて日本語版に対応しています。使用する言語は優先順位順に、
`-l <lang>` コマンドラインオプション、`MORIA_LANG` 環境変数、システムの
`LANG`/`LC_ALL` ロケールから決まり、いずれも該当しない場合は英語になります。

    $ umoria -l ja
    $ MORIA_LANG=ja umoria

翻訳データは `data/lang/<lang>/` 以下にあります。カタログが無い言語、または
カタログ内に未翻訳の文字列がある場合は、自動的に英語表示にフォールバックします。

新しい言語カタログを追加するには、日本語専用のハードコード処理にも手を
入れる必要があります。詳細は [`docs/TRANSLATING.md`](docs/TRANSLATING.md)
（英語）を参照してください。

## Historical Documents

Most of the original document files included in the Umoria 5.6 sources have
been placed in the [historical](historical) directory. You will even find the
old CHANGELOG, which tracks all code changes made between versions 4.81 and
5.5.2 (1987-2008). If you'd like to learn more on the development history of
Umoria, these can make for interesting reading.

There is also the original Moria Manual and FAQ. Although these are a little
outdated now they are certainly worth reading as they contain a lot of
interesting and useful information.


## Code of Conduct and Contributions

See here for details on our [Code of Conduct](CODE_OF_CONDUCT.md).

For details on how to contribute to the Umoria project, please read our
[contributing](CONTRIBUTING.md) guide.


## License Information

Umoria is released under the [GNU General Public License v3.0](LICENSE).

In 2007 Ben Asselstine and Ben Shadwick started the
[_free-moria_](http://free-moria.sourceforge.net/) project to re-license
UMoria 5.5.2 under GPL-2 by obtaining permission from all the contributing
authors. A year later they succeeded in their goal and in late 2008 official
maintainer David Grabiner released Umoria 5.6 under a GPL-3.0-or-later license.

### Japanese translation data and Hengband attribution

The Japanese translation data under `data/lang/ja/` is released under the
same GPL-3.0-or-later license as the rest of this project.

A portion of the monster/item/class-rank-title vocabulary was originally
adapted from [Hengband](https://hengband.github.io/), a separate, unrelated
Moria/Angband variant distributed under a non-commercial-only license that
is not GPL-compatible. Each borrowed entry was individually reviewed for
creative content: entries that are plain dictionary-level translations or
pure katakana phonetic transliterations (i.e. not creative or original
expression, and so not considered Hengband's copyrightable work) were kept
and are credited to Hengband here as a courtesy; entries that involved a
creative or interpretive translation choice were replaced with original
wording not derived from Hengband.

`data/lang/ja/`にある日本語翻訳データは、本プロジェクトの他の部分と同じ
GPL-3.0-or-laterライセンスで公開されています。

モンスター名・アイテム名・クラスランク称号の語彙の一部は、別プロジェクトである
[Hengband](https://hengband.github.io/)（非商用限定ライセンスで配布されており、
GPL とは互換性がありません）から流用しています。流用した各語について個別に
創作性を検討し、辞書的な直訳や純粋なカタカナ音写（＝創作的・独自の表現とは
言えず、Hengbandの著作物とは考えにくいもの）はそのまま維持した上でHengbandへの
謝辞として明記し、意訳や独自の解釈を伴う創作的な訳語選択が含まれるものは
Hengbandに由来しない独自の新訳に差し替えています。
