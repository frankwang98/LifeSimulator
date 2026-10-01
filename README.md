# Life Tree · 人生行为树模拟器

用 C++17 行为树模拟一个人如何分配时间，在工作、健康、学习和家庭之间做选择，并观察长期结果。

`nav_tree` 仓库现在以人生模拟为主；原来的 BehaviorTree.CPP 导航示例保留在 [`examples/navigation`](examples/navigation)，独立构建。自动驾驶方向由 ros2drive 承载。

## 快速开始

主程序无第三方依赖、无需联网下载。需要 CMake 3.16+、C++17 编译器。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
./build/life_tree --strategy all --days 365 --output output
ctest --test-dir build --output-on-failure
```

没有 CMake 也可以：

```bash
g++ -std=c++17 -O2 src/main.cpp -o life_tree
./life_tree --strategy balanced --days 365
```

程序打印各策略的最终状态，生成 `money.csv`、`health.csv`、`balanced.csv`。CSV 每行是一天，包含状态、收入、生活支出、还贷额和七种行为的小时数。可以用 Excel、Python 或其他绘图工具查看趋势。

| 参数 | 默认 | 含义 |
|---|---|---|
| `--strategy` | `balanced` | `money` / `health` / `balanced` / `all` |
| `--days` | `365` | 1 至 36500 天 |
| `--output` | `output` | CSV 保存目录 |
| `--help` | — | 查看用法 |

## 模拟规则

一个 tick 是一小时；行为即时执行，每天恰好 24 个 tick。三种策略从同一个初始状态开始，模型是确定性的，重复运行得到相同结果。

Blackboard 由 `life::State` 承载：年龄、健康、精力、现金、债务、知识、幸福、关系以及活动时间记录。初始年龄 28 岁，现金 0，债务 300000；这是演示参数，可以在 `src/life.h` 中调整。

行为树使用 `Selector` 选择第一个可执行分支，分支通过 `Sequence(Condition, Action)` 表示。当前行为均为一小时原子行为，因此只需 SUCCESS / FAILURE；尚未实现跨 tick 的 RUNNING、halt 或异步行为。主程序采用自己的轻量行为树内核，原版 BehaviorTree.CPP 示例仍在保留目录中。

| 决策顺序 | 条件与行为 |
|---|---|
| 1 | 23:00—07:00 或精力低于 20：睡觉 |
| 2 | 健康低于 35：休养，有额外护理支出 |
| 3 | 48 小时未运动：运动 |
| 4 | 48 小时未陪伴：陪伴家人 |
| 5 | 72 小时未学习：学习 |
| 6 | 健康、均衡策略：每日运动 |
| 7 | 工作日 09:00—21:00：按策略工作时数上限工作 |
| 8 | 健康、均衡策略：每日陪伴、学习 |
| 9 | 其他时间：休闲 |

活动间隔约束先于工作检查，防止低优先级活动长期饥饿；睡眠和紧急休养仍可以覆盖这些约束。

| 策略 | 每日工作上限 | 取舍 |
|---|---|---|
| money | 10 小时 | 较快还贷，运动和学习以最低间隔为主 |
| health | 6 小时 | 每日运动，留出更多恢复时间 |
| balanced | 8 小时 | 每日运动、学习和陪伴 |

工作收入随知识增加；工作消耗精力和健康，学习增加知识。每天扣生活支出 120 元，债务按年利率 4% 日计息，每 30 天用超过 3000 元现金储备的部分还贷。护理每小时另扣 15 元；现金允许为负，代表未覆盖的生活开支，不会自动变成新贷款。知识没有上限，其他评分限制为 0—100。

**这些参数是可修改的演示假设，不是现实收入估计或人生预测。** 第一版没有疾病概率、失业、就业门槛、家庭成员、死亡或通胀。健康、幸福、关系只是简化评分；模型结果主要用来讨论规则和取舍。

## 结构

```text
src/life.h                  状态、行为树节点、策略与时间推进
src/main.cpp                命令行、摘要与每日 CSV
tests/test_life.cpp         模拟规则测试
.github/workflows/ci.yml     自动编译、测试、全年模拟
examples/navigation/        保留的 BT.CPP 导航示例
```

测试覆盖：睡眠与健康优先级、每天时间守恒、现金守恒、状态范围、可复现性、最低活动频率、还贷和策略差异。CI 在每次 push / pull request 运行。

## 后续方向

- 参数文件和小时级事件记录，方便复盘每次决策。
- 多小时动作与 RUNNING / halt 生命周期。
- 加入带固定随机种子的失业、疾病和学习机会。
- 趋势可视化和可解释的行为树执行轨迹。
- 再逐步扩展家庭、多 Agent 与社会环境。

先保持单人、确定性、规则透明，不在第一版引入社会模拟的复杂度。

## License

MIT
