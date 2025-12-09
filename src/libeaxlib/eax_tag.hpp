#pragma once

#include <string>
#include <concepts>
#include <variant>

namespace eax {
/**
 * @brief EAXタグのコンセプト
 * @tparam EAXTag EAXタグの型
 * @requires EAXTag が以下の要件を満たすこと
 * - メンバ関数 `std::string to_string() const` を持つ
 * - 静的メンバ関数 `static bool match_string(const std::string& str)` を持つ
 * - コンストラクタ `EAXTag(const std::string& str)` を持つ
 */
template <typename EAXTag>
concept eax_tag = requires (EAXTag tag, std::string str) {
    { tag.to_string() } -> std::convertible_to<std::string>;
    { EAXTag::match_string(str) } -> std::convertible_to<bool>;
    EAXTag{str};
};

/**
 * @brief EAXタグをストリームに出力する演算子
 * @tparam CharT 文字型
 * @tparam Traits 文字特性型
 * @tparam EAXTag EAXタグの型
 * @param os 出力ストリーム
 * @param tag EAXタグ
 * @return 出力ストリーム
 */
template <typename CharT, typename Traits, eax_tag EAXTag>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const EAXTag& tag) {
    os << tag.to_string();
    return os;
}

/**
 * @brief EAXタグのバリアントをストリームに出力する演算子
 * @tparam CharT 文字型
 * @tparam Traits 文字特性型
 * @tparam FirstTag バリアントの最初のEAXタグの型
 * @tparam RestTags バリアントの残りのEAXタグの型
 * @param os 出力ストリーム
 * @param tag EAXタグのバリアント
 * @return 出力ストリーム
 */
template <typename CharT, typename Traits, eax_tag FirstTag, eax_tag... RestTags>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const std::variant<FirstTag, RestTags...>& tag) {
    std::visit([&os](const auto& t) {
        os << t;
    }, tag);
    return os;
};

/**
 * @brief 文字列からEAXタグのバリアントを生成する
 * @tparam Variant EAXタグのバリアントの型
 * @tparam I 現在のインデックス (デフォルト値: 0)
 * @param str EAXタグの文字列
 * @return EAXタグのバリアント
 * @throws std::runtime_error 指定された文字列に対応するEAXタグが存在しない場合
 */
template <typename Variant, std::size_t I = 0>
Variant create_eax_tag_from_string(const std::string& str) {
    if constexpr (I >= std::variant_size_v<Variant>) {
        throw std::runtime_error("Unknown EAX tag: " + str);
    } else {
        using CurrentTag = std::variant_alternative_t<I, Variant>;
        if (CurrentTag::match_string(str)) {
            return CurrentTag{str};
        } else {
            return create_eax_tag_from_string<Variant, I + 1>(str);
        }
    }
}

}