#pragma once

#include <behaviortree_cpp_v3/basic_types.h>

#include <string>

namespace nav_tree {

/**
 * @brief 2D pose on the XY plane.
 *
 * Used as a custom port type so nodes can carry (x, y) atomically instead
 * of splitting into current_x/current_y. BT.CPP needs to know how to parse
 * Pose from a string — see the convertFromString specialization below.
 */
struct Pose {
  double x{0.0};
  double y{0.0};
};

}  // namespace nav_tree

// Tell BT.CPP how to parse Pose from a string.
// Used both for XML port values (e.g. pose="10.0;5.0") and for blackboard
// entries stored as strings.
//
// Defining the body in the header (not in a .cpp) is required for template
// specializations: the symbol is only emitted where it's instantiated, and
// keeping it inline lets every TU that touches `convertFromString<Pose>`
// produce its own copy.
namespace BT {
template <>
inline nav_tree::Pose convertFromString<nav_tree::Pose>(StringView str) {
  // Format: "x;y", e.g. "10.0;5.0"
  const auto sep = str.find(';');
  if (sep == StringView::npos) {
    throw BT::RuntimeError(
        std::string("Pose string must be 'x;y' format, got: '") +
        std::string(str) + "'");
  }
  const StringView x_part = str.substr(0, sep);
  const StringView y_part = str.substr(sep + 1);
  return nav_tree::Pose{convertFromString<double>(x_part),
                        convertFromString<double>(y_part)};
}
}  // namespace BT