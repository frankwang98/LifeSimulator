#pragma once

#include <behaviortree_cpp_v3/bt_factory.h>

#include "pose.h"

namespace nav_tree {

/**
 * @brief ConditionNode: returns SUCCESS if current_pose is within
 *        arrival_tolerance of the goal.
 *
 * Goal resolution order (first non-empty wins):
 *   1. `target` input port       — set by SubTree remap or XML attribute
 *   2. `target_pose` blackboard  — set by UpdateTarget or main()
 *
 * Use in Sequence / Fallback / Inverter to gate branches.
 */
class IsGoalReached : public BT::ConditionNode {
 public:
  IsGoalReached(const std::string& name, const BT::NodeConfiguration& config);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

 private:
  Pose resolveTarget();
};

/**
 * @brief SyncAction: writes a Pose value (from XML or upstream port) to the
 *        "target_pose" blackboard key.
 *
 * Useful for retargeting mid-plan — e.g. a second tree updates the target
 * before re-running NavToPose, picking up from wherever the first tree left
 * the current_pose.
 */
class UpdateTarget : public BT::SyncActionNode {
 public:
  UpdateTarget(const std::string& name, const BT::NodeConfiguration& config);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;
};

}  // namespace nav_tree