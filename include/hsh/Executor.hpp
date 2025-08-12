#pragma once

#include <span>
#include <string>
#include <vector>

namespace hsh {

int runPipeline(std::span<std::vector<std::string>> commands);

} // namespace hsh
