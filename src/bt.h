#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "state.h"

namespace life {
struct Node {
  virtual ~Node() = default;
  virtual Status tick(State&) = 0;
};
using Ptr = std::unique_ptr<Node>;
struct Condition : Node {
  std::string id;
  std::function<bool(const State&)> predicate;
  Condition(std::string node, std::function<bool(const State&)> test)
      : id(std::move(node)), predicate(std::move(test)) {}
  Status tick(State& state) override {
    const auto status = predicate(state) ? Status::Success : Status::Failure;
    record(state, id, "condition", status);
    return status;
  }
};
struct Action : Node {
  std::string id;
  std::string action;
  std::function<void(State&)> apply;
  Action(std::string node, std::string label, std::function<void(State&)> function)
      : id(std::move(node)), action(std::move(label)), apply(std::move(function)) {}
  Status tick(State& state) override {
    apply(state);
    state.action = action;
    record(state, id, "action", Status::Success);
    return Status::Success;
  }
};
struct Composite : Node {
  std::string id;
  std::vector<Ptr> children;
  explicit Composite(std::string node) : id(std::move(node)) {}
  void add(Ptr node) {
    children.push_back(std::move(node));
  }
};
struct Sequence : Composite {
  using Composite::Composite;
  Status tick(State& state) override {
    for (auto& child : children) {
      if (child->tick(state) == Status::Failure) {
        record(state, id, "sequence", Status::Failure);
        return Status::Failure;
      }
    }
    record(state, id, "sequence", Status::Success);
    return Status::Success;
  }
};
struct Selector : Composite {
  using Composite::Composite;
  Status tick(State& state) override {
    for (auto& child : children) {
      if (child->tick(state) == Status::Success) {
        record(state, id, "selector", Status::Success);
        return Status::Success;
      }
    }
    record(state, id, "selector", Status::Failure);
    return Status::Failure;
  }
};
}  // namespace life
