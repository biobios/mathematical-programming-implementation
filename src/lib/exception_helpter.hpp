#pragma once

#include <string>
#include <source_location>

namespace mpi
{
    namespace exception
    {

#if defined(__cpp_exceptions) || defined(__EXCEPTIONS)

        template <typename E>
        [[noreturn]] constexpr void throw_exception(const std::string& message, const std::source_location& location = std::source_location::current())
        {
            std::string full_message = std::string(location.file_name()) + ":" + std::to_string(location.line()) + ":" + std::to_string(location.column()) + ": exception: " + message + "\n" +
                                        "Occurred in " + location.function_name() + "\n";

            throw E(full_message);
        }

#else

        template <typename E>
        [[noreturn]] constexpr void throw_exception(const std::string& message, const std::source_location& location = std::source_location::current())
        {
            std::string full_message = std::string(location.file_name()) + ":" + std::to_string(location.line()) + ":" + std::to_string(location.column()) + ": exception: " + message + "\n" +
                                        "Occurred in " + location.function_name() + "\n";
            std::cerr << full_message << std::endl;
            std::terminate();
        }

#endif

    }
}