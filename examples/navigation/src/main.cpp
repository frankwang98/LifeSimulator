#include <behaviortree_cpp_v3/bt_factory.h>

#include <filesystem>
#include <iostream>
#include <vector>

#include "condition_nodes.h"
#include "nav_nodes.h"
#include "pose.h"

int main(int argc, char** argv) {
  using namespace nav_tree;

  // --- 1. Build factory and register custom node types ---
  BT::BehaviorTreeFactory factory;
  factory.registerNodeType<NavToPose>("NavToPose");
  factory.registerNodeType<PrintMessage>("PrintMessage");
  factory.registerNodeType<IsGoalReached>("IsGoalReached");
  factory.registerNodeType<UpdateTarget>("UpdateTarget");

  // --- 2. Pre-populate the blackboard with initial pose ---
  // current_pose / target_pose are Pose (custom-typed) blackboard keys,
  // mutated by nodes across ticks. Both trees share this same instance,
  // so the second tree picks up wherever the first one stopped.
  BT::Blackboard::Ptr blackboard = BT::Blackboard::create();
  blackboard->set("current_pose", Pose{0.0, 0.0});
  blackboard->set("target_pose", Pose{10.0, 5.0});

  // --- 3. Resolve tree files ---
  // Default: nav_tree.xml. Otherwise: every CLI arg is a tree file path.
  // Multiple trees tick sequentially, sharing the same blackboard — this is
  // how you compose larger behaviors from smaller ones.
  std::vector<std::filesystem::path> tree_files;
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) {
      tree_files.emplace_back(std::filesystem::absolute(argv[i]));
    }
  } else {
    tree_files.emplace_back(std::filesystem::absolute("nav_tree.xml"));
  }

  // --- 4. Tick each tree in sequence ---
  int exit_code = 0;
  for (const auto& tree_file : tree_files) {
    if (!std::filesystem::exists(tree_file)) {
      std::cerr << "Tree file not found: " << tree_file << "\n";
      exit_code = 1;
      continue;
    }

    std::cout << "\n=== Loading " << tree_file << " ===\n";
    BT::Tree tree = factory.createTreeFromFile(tree_file.string(), blackboard);

    std::cout << "=== Ticking " << tree_file << " ===\n";
    const BT::NodeStatus status = tree.tickRootWhileRunning();

    std::cout << "=== Status: " << BT::toStr(status) << " ===\n";
    const Pose current = blackboard->get<Pose>("current_pose");
    const Pose target = blackboard->get<Pose>("target_pose");
    std::cout << "=== After tick: current=(" << current.x << ", " << current.y << ") target=("
              << target.x << ", " << target.y << ") ===\n";

    if (status != BT::NodeStatus::SUCCESS) {
      exit_code = 1;
    }
  }

  return exit_code;
}