#include "nav_nodes.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "pose.h"

namespace nav_tree {

NavToPose::NavToPose(const std::string& name, const BT::NodeConfiguration& config)
    : BT::StatefulActionNode(name, config) {}

BT::PortsList NavToPose::providedPorts() {
  return {
      BT::InputPort<Pose>("target"),
      BT::InputPort<double>("step_size", 1.0, ""),
      BT::InputPort<double>("arrival_tolerance", 0.5, ""),
      BT::InputPort<int>("max_steps", 100, ""),
  };
}

Pose NavToPose::resolveTarget() {
  // Port wins over blackboard — lets SubTree remap set the goal per call.
  if (auto port_val = getInput<Pose>("target")) {
    return port_val.value();
  }
  return config().blackboard->get<Pose>("target_pose");
}

BT::NodeStatus NavToPose::onStart() {
  steps_taken_ = 0;
  return onRunning();
}

BT::NodeStatus NavToPose::onRunning() {
  steps_taken_++;

  const auto bb = config().blackboard;
  Pose current = bb->get<Pose>("current_pose");
  Pose target = resolveTarget();

  const double tol = getInput<double>("arrival_tolerance").value_or(0.5);
  const double step = getInput<double>("step_size").value_or(1.0);
  const int cap = getInput<int>("max_steps").value_or(100);

  const double dx = target.x - current.x;
  const double dy = target.y - current.y;
  const double dist = std::hypot(dx, dy);

  if (dist <= tol) {
    std::cout << "[NavToPose] arrived at (" << current.x << ", " << current.y << ") after "
              << steps_taken_ << " steps\n";
    return BT::NodeStatus::SUCCESS;
  }
  if (steps_taken_ >= cap) {
    std::cout << "[NavToPose] FAILURE: exceeded max_steps=" << cap << " (remaining dist=" << dist
              << ")\n";
    return BT::NodeStatus::FAILURE;
  }

  // Move one step toward target (clamped so we don't overshoot).
  const double ratio = std::min(step / dist, 1.0);
  current.x += dx * ratio;
  current.y += dy * ratio;
  bb->set("current_pose", current);

  std::cout << "[NavToPose] step=" << steps_taken_ << " pos=(" << current.x << ", " << current.y
            << ")" << " remaining=" << (dist - step) << "\n";

  return BT::NodeStatus::RUNNING;
}

void NavToPose::onHalted() {
  std::cout << "[NavToPose] halted at step=" << steps_taken_ << "\n";
  steps_taken_ = 0;
}

PrintMessage::PrintMessage(const std::string& name, const BT::NodeConfiguration& config)
    : BT::SyncActionNode(name, config) {}

BT::PortsList PrintMessage::providedPorts() {
  return {BT::InputPort<std::string>("msg")};
}

BT::NodeStatus PrintMessage::tick() {
  const auto msg = getInput<std::string>("msg").value_or("(empty)");
  std::cout << "[PrintMessage] " << msg << "\n";
  return BT::NodeStatus::SUCCESS;
}

}  // namespace nav_tree