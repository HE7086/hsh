module;

#include <memory>

export module hsh.executor;

import hsh.core;

export namespace hsh::executor {

using core::Result;

// Abstract interface for executable AST nodes to avoid circular dependency
class ExecutableNode {
public:
  virtual ~ExecutableNode()                                     = default;
  virtual auto clone() const -> std::unique_ptr<ExecutableNode> = 0;
  virtual auto type_name() const noexcept -> std::string_view   = 0;
};
using ExecutableNodePtr = std::unique_ptr<ExecutableNode>;

// Interface class to avoid circular dependencies
class SubshellExecutor {
public:
  virtual ~SubshellExecutor() = default;
  
  virtual auto execute_subshell_body(ExecutableNodePtr const& body) -> Result<int> = 0;
};


} // namespace hsh::executor