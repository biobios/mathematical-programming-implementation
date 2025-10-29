#pragma once

#include <string>
#include <concepts>
#include <variant>

namespace eax {
template <typename EAXTag>
concept eax_tag = requires (EAXTag tag, std::string str) {
    { tag.to_string() } -> std::convertible_to<std::string>;
    { EAXTag::match_string(str) } -> std::convertible_to<bool>;
    EAXTag{str};
};

template <typename CharT, typename Traits, eax_tag EAXTag>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const EAXTag& tag) {
    os << tag.to_string();
    return os;
}

template <typename CharT, typename Traits, eax_tag FirstTag, eax_tag... RestTags>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const std::variant<FirstTag, RestTags...>& tag) {
    std::visit([&os](const auto& t) {
        os << t;
    }, tag);
    return os;
};

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