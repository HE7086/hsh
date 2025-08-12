#pragma once

#include <locale>
#include <string>

namespace hsh {

extern std::locale const& LOCALE; // NOLINT

static constexpr inline std::string const PROMPT = "hsh> ";

}; // namespace hsh
