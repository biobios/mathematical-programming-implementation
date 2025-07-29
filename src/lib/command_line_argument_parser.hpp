#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <sstream>
#include <iostream>
#include <type_traits>
#include <memory>

namespace mpi {
    class ArgumentSpecBase {
    public:
        virtual ~ArgumentSpecBase() = default;
        virtual size_t parse(size_t argc, char* argv[], size_t start_index) = 0;
        virtual bool matches(const std::string& arg) const = 0;
        virtual void print_help() const = 0;
    private:
    };
    
    template <typename... Args>
    class ArgumentSpec : public ArgumentSpecBase {
    public:
        ArgumentSpec(Args&... placeholders)
            : placeholders(placeholders...) {}

        size_t parse(size_t argc, char* argv[], size_t start_index) override {
            return parse_args<0>(argc, argv, start_index + 1);
        }
        
        bool matches(const std::string& arg) const override {
            for (const auto& name : arg_names) {
                if (arg == name) {
                    return true;
                }
            }
            return false;
        }
        
        void print_help() const override {
            std::cout << description << std::endl;
        }
        
        void set_description(const std::string& desc) {
            description = desc;
        }

        void add_argument_name(const std::string& name) {
            arg_names.push_back(name);
        }
    private:
        template <size_t Index = 0>
        size_t parse_args(size_t argc, char* argv[], size_t index) {
            if constexpr (Index < sizeof...(Args)) {
                if (index >= argc) {
                    throw std::runtime_error("Not enough arguments provided for parsing.");
                }
                std::string arg = argv[index];
                std::stringstream ss(arg);
                if (ss >> std::get<Index>(placeholders)) {
                    return parse_args<Index + 1>(argc, argv, index + 1);
                } else {
                    throw std::runtime_error("Failed to parse argument: " + arg);
                }
            } else {
                return Index + 1;
            }
        }
        std::tuple<Args&...> placeholders;
        std::string description;
        std::vector<std::string> arg_names;
    };
    
    template <typename ArgsType>
    class ArgumentSpec<std::vector<ArgsType>> : public ArgumentSpecBase {
    public:
        ArgumentSpec(std::vector<ArgsType>& placeholder)
            : placeholder(placeholder) {}
        
        size_t parse(size_t argc, char* argv[], size_t start_index) override {
            size_t index = start_index + 1;
            // "-"で始まらない引数をすべてvectorに追加
            while (index < argc && argv[index][0] != '-') {
                ArgsType arg_value;
                std::stringstream ss(argv[index]);
                if (ss >> arg_value) {
                    placeholder.push_back(arg_value);
                } else {
                    throw std::runtime_error("Failed to parse argument: " + std::string(argv[index]));
                }
                index++;
            }
            
            return index - start_index;
        }
        
        bool matches(const std::string& arg) const override {
            for (const auto& name : arg_names) {
                if (arg == name) {
                    return true;
                }
            }
            return false;
        }

        void print_help() const override {
            std::cout << description << std::endl;
        }

        void set_description(const std::string& desc) {
            description = desc;
        }

        void add_argument_name(const std::string& name) {
            arg_names.push_back(name);
        }

    private:
        std::vector<ArgsType>& placeholder;
        std::string description;
        std::vector<std::string> arg_names;
    };
    
    template <>
    class ArgumentSpec<bool> : public ArgumentSpecBase {
    public:
        ArgumentSpec(bool& placeholder)
            : placeholder(placeholder) {}
        
        size_t parse([[maybe_unused]]size_t argc, char* argv[], size_t start_index) override {
            std::string arg = argv[start_index];
            for (const auto& name : set_arg_names) {
                if (arg == name) {
                    placeholder = true;
                    return 1; // フラグ引数は1つの引数を消費する
                }
            }
            for (const auto& name : unset_arg_names) {
                if (arg == name) {
                    placeholder = false;
                    return 1; // フラグ引数は1つの引数を消費する
                }
            }
            // matches後に呼び出されるから、ここに到達しないはず
            throw std::runtime_error("Unknown boolean argument: " + arg);
        }
        
        bool matches(const std::string& arg) const override {
            for (const auto& name : set_arg_names) {
                if (arg == name) {
                    return true;
                }
            }
            for (const auto& name : unset_arg_names) {
                if (arg == name) {
                    return true;
                }
            }
            return false;
        }
        
        void print_help() const override {
            std::cout << description << std::endl;
        }
        
        void set_description(const std::string& desc) {
            description = desc;
        }

        void add_set_argument_name(const std::string& name) {
            set_arg_names.push_back(name);
        }

        void add_unset_argument_name(const std::string& name) {
            unset_arg_names.push_back(name);
        }
    private:
        bool& placeholder;
        std::string description;
        std::vector<std::string> set_arg_names;
        std::vector<std::string> unset_arg_names;
    };
    
    class CommandLineArgumentParser {
    public:
        CommandLineArgumentParser() = default;
        
        /**
         * @brief コマンドライン引数のパーサーに引数仕様を追加する
         * @tparam Spec 引数仕様の型
         * @pre SpecはArgumentSpecBaseを継承していること
         * @pre SpecはArgumentSpecBaseではないこと
         * @pre Specがstd::forward<Spec>(spec)で初期化可能であること
         * @param spec 引数仕様のインスタンス
         */
        template <typename Spec>
            requires (std::derived_from<std::remove_cvref_t<Spec>, ArgumentSpecBase> &&
                        !std::is_same_v<std::remove_cvref_t<Spec>, ArgumentSpecBase> &&
                        std::is_constructible_v<std::remove_cvref_t<Spec>, Spec>)
        void add_argument(Spec&& spec) {
            argument_specs.push_back(std::make_unique<std::remove_cvref_t<Spec>>(std::forward<Spec>(spec)));
        }
        
        /**
         * @brief コマンドライン引数を解析する
         * @param argc 引数の数
         * @param argv 引数の配列
         */
        void parse(size_t argc, char* argv[]) {
            size_t index = 1;
            while (index < argc) {
                bool matched = false;
                for (const auto& spec : argument_specs) {
                    if (spec->matches(argv[index])) {
                        index += spec->parse(argc, argv, index);
                        matched = true;
                        break;
                    }
                }
                
                if (!matched) {
                    std::cerr << "Unknown argument: " << argv[index] << std::endl;
                    std::cerr << "To see available options, use --help." << std::endl;
                    throw std::runtime_error("Unknown argument encountered.");
                }
            }
        }
        
        void print_help() const {
            for (const auto& spec : argument_specs) {
                spec->print_help();
            }
        }
    private:
        std::vector<std::unique_ptr<ArgumentSpecBase>> argument_specs;
    };
}