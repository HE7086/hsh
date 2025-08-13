#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hsh {

std::string              trim(std::string_view s);
std::vector<std::string> splitPipeline(std::string_view line);
std::vector<std::string> tokenize(std::string_view segment);
std::optional<std::string> expandTilde(std::string_view word);

// Expand basic parameters within a pipeline segment while respecting quotes.
// - Expands $VAR and ${VAR} using getenv (unset -> empty string)
// - Expands $? to the provided last_status
// - No expansion inside single quotes; expansion allowed inside double quotes
// The returned string still contains quotes, intended to be passed to tokenize().
std::string expandParameters(std::string_view segment, int last_status);

} // namespace hsh
