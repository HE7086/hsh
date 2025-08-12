#include "hsh/Builtins.hpp"
#include "hsh/Constants.hpp"
#include "hsh/Executor.hpp"
#include "hsh/Signals.hpp"
#include "hsh/Util.hpp"

#include <cstdio>
#include <functional>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <unistd.h>

int main() {
  hsh::setParentSignals();
  bool        interactive = isatty(STDIN_FILENO) != 0;
  int         last_status = 0;
  std::string line;

  while (true) {
    if (interactive) {
      fmt::print(hsh::PROMPT);
      std::fflush(stdout);
    }
    if (!std::getline(std::cin, line)) {
      if (interactive) {
        fmt::println("");
      }
      break;
    }

    line = hsh::trim(line);
    if (line.empty()) {
      continue;
    }

    auto commands =
        hsh::splitPipeline(line) |
        std::views::transform([](std::string const& s) { return hsh::tokenize(s); }) |
        std::views::filter(std::not_fn(&std::vector<std::string>::empty)) |
        std::ranges::to<std::vector>();

    // Expand aliases on each simple command in the pipeline
    for (auto& cmd : commands) {
      hsh::expandAliases(cmd);
    }

    if (commands.empty() || (commands.size() == 1 && hsh::handleBuiltin(commands[0], last_status))) {
      continue;
    }

    last_status = hsh::runPipeline(commands);
  }
}
