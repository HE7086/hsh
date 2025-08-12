#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace hsh {

std::string              trim(std::string_view s);
std::vector<std::string> splitPipeline(std::string_view line);
std::vector<std::string> tokenize(std::string_view segment);

} // namespace hsh
