# nav_tree

A standalone BehaviorTree.CPP demo project. **No Baize, no ROS, no other project
coupling** — just CMake + CPM + BT.CPP v3.8.8 in one folder.

Demonstrates four core BT.CPP concepts:

1. **Custom nodes** (`NavToPose`, `PrintMessage`)
2. **Custom types** (`Pose` struct — used as a port type)
3. **Condition nodes** (`IsGoalReached` — gates control flow)
4. **Multi-tree composition** with shared blackboard

## Layout

```
nav_tree/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp            # Factory + multi-tree tick loop
│   ├── pose.h              # Pose struct + BT::convertFromString<Pose>
│   ├── nav_nodes.h         # NavToPose, PrintMessage
│   ├── nav_nodes.cpp
│   ├── condition_nodes.h   # IsGoalReached, UpdateTarget
│   └── condition_nodes.cpp
└── trees/
    ├── nav_tree.xml        # Multi-tree 1: walk to (10, 5)
    ├── chase_tree.xml      # Multi-tree 2: retarget to (20, 0) and walk there
    ├── nav_subtree.xml     # Reusable BehaviorTree (SubTree primitive)
    └── composite_tree.xml  # Composes the subtree twice with port remapping
```

## Build

Requires CMake ≥ 3.16 and a C++17 compiler (gcc 7+, clang 5+, MSVC 2019+).

```bash
cmake -B build -S .
cmake --build build -j
```

The first configure downloads CPM.cmake and BehaviorTree.CPP v3.8.8 from
GitHub (~30s on cold cache).

## Run

```bash
# Default: tick nav_tree.xml only
./build/nav_tree

# Multi-tree: tick trees in order, sharing the same blackboard
./build/nav_tree nav_tree.xml chase_tree.xml

# SubTree composition: composite_tree.xml <include>s nav_subtree.xml
# and invokes the subtree twice with different port bindings.
./build/nav_tree composite_tree.xml
```

Expected output (multi-tree run):

```
=== Loading .../nav_tree.xml ===
=== Ticking .../nav_tree.xml ===
[PrintMessage] == NavTree: heading to (10, 5) ==
[NavToPose] step=1 pos=(0.894, 0.447) remaining=10.180
...
[NavToPose] arrived at (9.839, 4.919) after 12 steps
[IsGoalReached] current=(9.839, 4.919) target=(10, 5) dist=0.180 tol=0.3 -> SUCCESS
=== Status: SUCCESS ===

=== Loading .../chase_tree.xml ===
=== Ticking .../chase_tree.xml ===
[PrintMessage] == ChaseTree: retargeting to (20, 0) ==
[UpdateTarget] target_pose set to (20, 0)
[NavToPose] step=1 pos=(10.739, 4.484) remaining=10.290
...
[NavToPose] arrived at (19.739, 0.126) after 12 steps
[IsGoalReached] ... -> SUCCESS
=== Status: SUCCESS ===
```

Note the second tree starts from where the first left off
(`current_pose = (9.839, 4.919)`) — that's blackboard sharing across trees.

## Custom nodes

| Node            | Type                | Purpose                                          |
|-----------------|---------------------|--------------------------------------------------|
| `PrintMessage`  | SyncActionNode      | Print a string to stdout                         |
| `NavToPose`     | StatefulActionNode  | Step toward `target_pose` on the blackboard      |
| `IsGoalReached` | ConditionNode       | SUCCESS if within tolerance, FAILURE otherwise    |
| `UpdateTarget`  | SyncActionNode      | Overwrite `target_pose` on the blackboard        |

### Custom type: `Pose`

```cpp
struct Pose { double x{0.0}; double y{0.0}; };
```

XML syntax: `pose="x;y"`, e.g. `pose="20.0;0.0"`. The parser lives in
`src/pose.h` as a `BT::convertFromString<Pose>` specialization (defined
inline in the header so every TU that needs it can instantiate it).

### Blackboard keys

| Key            | Type   | Owner                    |
|----------------|--------|--------------------------|
| `current_pose` | Pose   | NavToPose (read+write)   |
| `target_pose`  | Pose   | UpdateTarget (write), NavToPose / IsGoalReached (read) |

### Port summary

- **NavToPose**: `step_size` (default 1.0), `arrival_tolerance` (default 0.5),
  `max_steps` (default 100)
- **IsGoalReached**: `arrival_tolerance` (default 0.5)
- **UpdateTarget**: `pose` (Pose, required)
- **PrintMessage**: `msg` (string, required)

## Multi-tree pattern

`main.cpp` ticks each tree file in sequence, sharing one `BT::Blackboard`
instance. This is the simplest way to compose larger behaviors:

- Split a long plan into independent stages
- Switch strategy at runtime (e.g. different maneuver per obstacle type)
- Re-target and re-execute without rebuilding state from scratch

## SubTree composition

`trees/nav_subtree.xml` defines a single reusable `BehaviorTree ID="NavigateToPose"`:

```xml
<BehaviorTree ID="NavigateToPose">
  <Sequence>
    <NavToPose step_size="1.0" arrival_tolerance="0.3" max_steps="50"/>
    <IsGoalReached arrival_tolerance="0.3"/>
  </Sequence>
</BehaviorTree>
```

`trees/composite_tree.xml` pulls it in and invokes it twice with different
goal bindings — once as a literal, once via blackboard pointer:

```xml
<root BTCPP_format="3" main_tree_to_execute="CompositeTree">
  <include path="nav_subtree.xml"/>

  <BehaviorTree ID="CompositeTree">
    <Sequence>
      <PrintMessage msg="leg 1 with static target"/>
      <SubTree ID="NavigateToPose" target="10.0;5.0" __shared_blackboard="true"/>

      <UpdateTarget pose="20.0;0.0"/>
      <PrintMessage msg="leg 2 via blackboard"/>
      <SubTree ID="NavigateToPose" target="{target_pose}" __shared_blackboard="true"/>
    </Sequence>
  </BehaviorTree>
</root>
```

Two port-binding modes (BT.CPP v3):

| Form              | Meaning                                                  |
|-------------------|----------------------------------------------------------|
| `target="10;5"`   | Literal value — passed as-is to inner ports              |
| `target="{key}"`  | Blackboard pointer — read from blackboard key `key`      |

Important caveats:

- **`<SubTree>` creates a separate blackboard by default.** To share state with
  the parent (e.g. `current_pose`, `target_pose`), pass `__shared_blackboard="true"`.
  Otherwise inner nodes can't see parent blackboard keys.
- **Multiple `BehaviorTree`s in one file** need `main_tree_to_execute="..."` on
  the `<root>` element to disambiguate.
- For an alternative auto-remapping variant, see `<SubTreePlus __autoremap="true">`
  (v3.8+).

## Next steps (not yet implemented)

- **Groot2** real-time visualization (`BT::PublisherZMQ`, requires `libzmq3-dev`)
- **`.so` plugin loading** via `BT::BehaviorTreeFactory::registerFromPlugin` to
  ship nodes as dynamically loaded shared libraries (the path `tutorial/baize_cpp/`
  takes in this repo)