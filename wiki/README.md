# Wiki Documentation / Wikiドキュメント

## About This Directory / このディレクトリについて

このディレクトリには、GitHubのWiki機能で使用できるMarkdownドキュメントが含まれています。

This directory contains Markdown documentation that can be used with GitHub's Wiki feature.

## How to Use These Files / これらのファイルの使い方

### Option 1: GitHub Wiki へアップロード / Upload to GitHub Wiki

1. GitHubリポジトリのWikiタブに移動 / Go to the Wiki tab in your GitHub repository
2. 各Markdownファイルの内容を新しいWikiページとして作成 / Create new Wiki pages with the content of each Markdown file
3. ファイル名からページタイトルを設定（例: `Home.md` → `Home`） / Set page titles from filenames (e.g., `Home.md` → `Home`)

### Option 2: Wiki リポジトリをクローン / Clone Wiki Repository

GitHubのWikiは、別のGitリポジトリとして管理されています：

GitHub Wikis are managed as separate Git repositories:

```bash
# Wikiリポジトリをクローン / Clone wiki repository
git clone https://github.com/biobios/mathematical-programming-implementation.wiki.git

# このディレクトリからWikiファイルをコピー / Copy wiki files from this directory
cd mathematical-programming-implementation.wiki
cp ../mathematical-programming-implementation/wiki/*.md .

# コミットしてプッシュ / Commit and push
git add .
git commit -m "Add comprehensive wiki documentation"
git push
```

### Option 3: このディレクトリを参照 / Reference This Directory

このディレクトリのMarkdownファイルは、そのままドキュメントとして読むことができます。

The Markdown files in this directory can be read as documentation as-is.

## Wiki Pages / Wikiページ

### メインページ / Main Pages

- **[Home.md](Home.md)** - Wikiのホームページ / Wiki home page
- **[Getting-Started.md](Getting-Started.md)** - プロジェクトのセットアップと使い方 / Project setup and usage
- **[Build-Instructions.md](Build-Instructions.md)** - ビルド手順の詳細 / Detailed build instructions
- **[API-Reference.md](API-Reference.md)** - 完全なAPIリファレンス / Complete API reference

### アルゴリズムページ / Algorithm Pages

- **[Simplex.md](Simplex.md)** - 線形計画法のSimplex実装 / Simplex implementation for linear programming
- **[EAX.md](EAX.md)** - EAXアルゴリズムの概要 / EAX algorithm overview
- **[EAX-STSP.md](EAX-STSP.md)** - 対称TSP用EAX / EAX for Symmetric TSP
- **[Fast-EAX.md](Fast-EAX.md)** - 高速化EAX / Optimized fast EAX
- **[Normal-EAX.md](Normal-EAX.md)** - 標準EAX / Standard EAX
- **[EAX-Tabu.md](EAX-Tabu.md)** - Tabuサーチ付きEAX / EAX with Tabu search

### ライブラリページ / Library Pages

- **[mpilib.md](mpilib.md)** - 数理計画法ライブラリ / Mathematical programming library
- **[eaxlib.md](eaxlib.md)** - EAXアルゴリズムライブラリ / EAX algorithm library

## Documentation Structure / ドキュメント構造

```
wiki/
├── Home.md                    # ホームページ / Home page
├── Getting-Started.md         # 入門ガイド / Getting started guide
├── Build-Instructions.md      # ビルド手順 / Build instructions
├── API-Reference.md           # APIリファレンス / API reference
│
├── Simplex.md                 # Simplexアルゴリズム / Simplex algorithm
│
├── EAX.md                     # EAX概要 / EAX overview
├── EAX-STSP.md               # EAX STSP実装 / EAX STSP implementation
├── Fast-EAX.md               # Fast EAX実装 / Fast EAX implementation
├── Normal-EAX.md             # Normal EAX実装 / Normal EAX implementation
├── EAX-Tabu.md               # EAX Tabu実装 / EAX Tabu implementation
│
├── mpilib.md                  # mpilibライブラリ / mpilib library
├── eaxlib.md                  # eaxlibライブラリ / eaxlib library
│
└── README.md                  # このファイル / This file
```

## Language / 言語

すべてのドキュメントは、日本語と英語のバイリンガル形式で書かれています。

All documentation is written in bilingual format (Japanese and English).

## Contributing / 貢献

ドキュメントの改善提案や誤りの修正は歓迎します。プルリクエストを送信してください。

Suggestions for documentation improvements and error corrections are welcome. Please submit a pull request.

## Maintenance / メンテナンス

- コードが変更された場合は、対応するドキュメントも更新してください
- 新しい機能を追加した場合は、ドキュメントも追加してください
- 古い情報は定期的に更新してください

When code changes:
- Update corresponding documentation
- Add documentation for new features
- Regularly update outdated information

## Additional Resources / 追加リソース

- [GitHub Wiki 公式ドキュメント / Official Documentation](https://docs.github.com/en/communities/documenting-your-project-with-wikis)
- [Markdown ガイド / Markdown Guide](https://www.markdownguide.org/)
- [リポジトリのREADME / Repository README](../README.md)
