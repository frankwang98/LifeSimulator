#include "condition_nodes.h"

#include <cmath>
#include <iostream>

#include "pose.h"

namespace nav_tree {

// ---------- IsGoalReached ----------

IsGoalReached::IsGoalReached(const std::string& name, const BT::NodeConfiguration& config)
    : BT::ConditionNode(name, config) {}

BT::PortsList IsGoalReached::providedPorts() {
  return {
      BT::InputPort<Pose>("target"),
      BT::InputPort<double>("arrival_tolerance", 0.5, ""),
  };
}

Pose IsGoalReached::resolveTarget() {
  if (auto port_val = getInput<Pose>("target")) {
    return port_val.value();
  }
  return config().blackboard->get<Pose>("target_pose");
}

BT::NodeStatus IsGoalReached::tick() {
  const auto bb = config().blackboard;
  const Pose current = bb->get<Pose>("current_pose");
  const Pose target = resolveTarget();
  const double tol = getInput<double>("arrival_tolerance").value_or(0.5);

  const double dist = std::hypot(target.x - current.x, target.y - current.y);
  const bool reached = dist <= tol;

  std::cout << "[IsGoalReached] current=(" << current.x << ", " << current.y << ") target=("
            << target.x << ", " << target.y << ") dist=" << dist << " tol=" << tol << " -> "
            << (reached ? "SUCCESS" : "FAILURE") << "\n";

  return reached ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

// ---------- UpdateTarget ----------

UpdateTarget::UpdateTarget(const std::string& name, const BT::NodeConfiguration& config)
    : BT::SyncActionNode(name, config) {}

BT::PortsList UpdateTarget::providedPorts() {
  return {BT::InputPort<Pose>("pose")};
}

BT::NodeStatus UpdateTarget::tick() {
  const auto pose = getInput<Pose>("pose");
  if (!pose) {
    throw BT::RuntimeError("UpdateTarget: 'pose' port must be set");
  }
  config().blackboard->set("target_pose", pose.value());
  std::cout << "[UpdateTarget] target_pose set to (" << pose->x << ", " << pose->y << ")\n";
  return BT::NodeStatus::SUCCESS;
}

}  // namespace nav_tree