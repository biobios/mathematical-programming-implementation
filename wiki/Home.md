# Mathematical Programming Implementation Wiki

## 概要 / Overview

このリポジトリは、数理計画法の実装を含むプロジェクトです。主に、線形計画問題のための単体法（Simplex）と、巡回セールスマン問題（TSP）のためのEAX（Edge Assembly Crossover）アルゴリズムを実装しています。

This repository contains implementations of mathematical programming algorithms, primarily the Simplex method for linear programming and the EAX (Edge Assembly Crossover) algorithm for the Traveling Salesman Problem (TSP).

## プロジェクト構成 / Project Structure

- **[mpilib](mpilib)** - 数理計画法のためのユーティリティライブラリ / Mathematical programming utilities library
- **[eaxlib](eaxlib)** - EAXアルゴリズムのためのユーティリティライブラリ / EAX algorithm utilities library
- **[Simplex](Simplex)** - 線形計画問題を解くための単体法の実装 / Simplex method implementation for linear programming
- **[EAX](EAX)** - TSP問題のためのEAXアルゴリズムの実装 / EAX algorithm implementation for TSP
- **[EAX STSP](EAX-STSP)** - 対称TSP問題のためのEAXアルゴリズム / EAX algorithm for Symmetric TSP
- **[Fast EAX](Fast-EAX)** - 高速化されたEAXアルゴリズム / Optimized fast EAX algorithm
- **[Normal EAX](Normal-EAX)** - 標準的なEAXアルゴリズム / Standard EAX algorithm
- **[EAX Tabu](EAX-Tabu)** - タブーサーチを組み合わせたEAXアルゴリズム / EAX algorithm with Tabu search

## ドキュメント / Documentation

- **[はじめに / Getting Started](Getting-Started)** - プロジェクトのセットアップと使い方 / Project setup and usage
- **[ビルド手順 / Build Instructions](Build-Instructions)** - コンパイルとビルドの詳細 / Compilation and build details
- **[API リファレンス / API Reference](API-Reference)** - APIの詳細情報 / Detailed API information

## 主な機能 / Key Features

### 線形計画法 / Linear Programming
- 単体法による最適化 / Optimization using the Simplex method
- 行列ベースの実装 / Matrix-based implementation
- 実行可能性と有界性のチェック / Feasibility and boundedness checks

### 遺伝的アルゴリズム / Genetic Algorithms
- EAX交叉オペレータ / EAX crossover operator
- TSP問題への適用 / Application to TSP problems
- 2-opt局所探索 / 2-opt local search
- 複数のバリエーション / Multiple variants available

## 貢献 / Contributing

このプロジェクトへの貢献を歓迎します。問題を見つけた場合や改善の提案がある場合は、issueを作成するか、プルリクエストを送信してください。

Contributions to this project are welcome. If you find issues or have suggestions for improvements, please create an issue or submit a pull request.

## ライセンス / License

このプロジェクトのライセンスについては、リポジトリのLICENSEファイルを参照してください。

Please refer to the LICENSE file in the repository for licensing information.
