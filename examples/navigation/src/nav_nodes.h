#pragma once

#include <behaviortree_cpp_v3/bt_factory.h>

#include <string>

#include "pose.h"

namespace nav_tree {

/**
 * @brief Stateful action: simulate XY-plane navigation toward a goal pose.
 *
 * Each tick advances the blackboard "current_pose" one step toward the goal.
 * Returns RUNNING while moving, SUCCESS once within arrival_tolerance,
 * FAILURE after max_steps ticks.
 *
 * Goal resolution order (first non-empty wins):
 *   1. `target` input port       — set by SubTree remap or XML attribute
 *   2. `target_pose` blackboard  — set by UpdateTarget or main()
 *
 * Blackboard keys (read/write):
 *   current_pose  Pose  running position
 *   target_pose   Pose  goal position (read only when port not set)
 *
 * XML ports:
 *   target            Pose   optional goal override
 *   step_size         double (1.0)   per-tick distance
 *   arrival_tolerance double (0.5)   success threshold
 *   max_steps         int    (100)   safety cap
 */
class NavToPose : public BT::StatefulActionNode {
 public:
  NavToPose(const std::string& name, const BT::NodeConfiguration& config);

  static BT::PortsList providedPorts();

  BT::NodeStatus onStart() override;
  BT::NodeStatus onRunning() override;
  void onHalted() override;

 private:
  Pose resolveTarget();

  int steps_taken_{0};
};

/**
 * @brief Trivial sync action: print a message to stdout.
 *
 * Ports:
 *   msg  InputPort<std::string>  text to print
 */
class PrintMessage : public BT::SyncActionNode {
 public:
  PrintMessage(const std::string& name, const BT::NodeConfiguration& config);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;
};

}  // namespace nav_tree