#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "bt.h"
#include "config.h"

namespace life {
struct BranchInfo {
  std::string id;
  std::string condition;
  std::string action;
};
struct Behavior {
  std::unique_ptr<Selector> root = std::make_unique<Selector>("root");
  std::vector<BranchInfo> branches;
  void add(std::string id, std::string condition, std::string action,
           std::function<bool(const State&)> test, std::function<void(State&)> apply) {
    auto sequence = std::make_unique<Sequence>(id);
    sequence->add(std::make_unique<Condition>(id + ".condition", std::move(test)));
    sequence->add(std::make_unique<Action>(id + ".action", action, std::move(apply)));
    root->add(std::move(sequence));
    branches.push_back({std::move(id), std::move(condition), std::move(action)});
  }
};
inline double utilityFor(const State& state, const Config& config, const std::string& action) {
  const int clock = state.hour % 24;
  const int adjustment = config.strategy == Strategy::Money    ? 2
                         : config.strategy == Strategy::Health ? -2
                                                               : 0;
  const int work_limit = std::clamp(static_cast<int>(config.work_hours) + adjustment, 0, 12);
  if (action == "work") {
    if (!state.workday || clock < 9 || clock >= 21 || state.work_today >= work_limit) {
      return -1;
    }
    return 35 + std::min(45.0, state.debt / 10000) + (config.strategy == Strategy::Money ? 35 : 0) +
           state.energy * 0.12 + (state.economy_index - 1) * 30;
  }
  if (action == "exercise") {
    return (100 - state.health) * 0.75 +
           std::max(0.0, state.hour - state.last_exercise - config.exercise_interval) * 0.8 +
           (config.strategy == Strategy::Health ? 30 : 0);
  }
  if (action == "family") {
    if (!state.companion.enabled) {
      return -1;
    }
    return ((100 - state.relationship) * 0.7 +
            std::max(0.0, state.hour - state.last_family - config.family_interval) * 0.9 +
            (state.companion.action == "connect" ? 18 : 0)) *
           config.social_weight;
  }
  if (action == "study") {
    return std::max(0.0, 65 - state.knowledge) * 0.45 +
           std::max(0.0, state.hour - state.last_study - config.study_interval) * 0.7;
  }
  return (100 - state.happiness) * 0.45 + (100 - state.energy) * 0.38 + 12;
}
inline void updateUtilities(State& state, const Config& config) {
  state.utilities.clear();
  for (const char* action : {"work", "exercise", "family", "study", "leisure"}) {
    state.utilities.push_back({action, utilityFor(state, config, action)});
  }
}
inline std::string utilityChoice(const State& state) {
  return std::max_element(state.utilities.begin(), state.utilities.end(),
                          [](const UtilityScore& left, const UtilityScore& right) {
                            return left.score < right.score;
                          })
      ->action;
}
inline void tickCompanion(State& state) {
  auto& agent = state.companion;
  if (!agent.enabled) {
    agent.action = "away";
    agent.utilities.clear();
    return;
  }
  const int clock = state.hour % 24;
  agent.utilities = {
      {"sleep", clock >= 23 || clock < 7 ? 1000.0 : 100 - agent.energy},
      {"work", state.workday && clock >= 9 && clock < 18 ? 70.0 : -1.0},
      {"connect", (100 - state.relationship) * 0.8 + (clock >= 18 && clock < 23 ? 30 : 0)},
      {"leisure", (100 - agent.happiness) * 0.45 + (100 - agent.energy) * 0.35 + 10}};
  agent.action = std::max_element(agent.utilities.begin(), agent.utilities.end(),
                                  [](const UtilityScore& left, const UtilityScore& right) {
                                    return left.score < right.score;
                                  })
                     ->action;
  if (agent.action == "sleep") {
    agent.energy += 12;
  } else if (agent.action == "work") {
    agent.energy -= 6;
    agent.happiness -= 0.15;
  } else if (agent.action == "connect") {
    agent.energy -= 1;
    agent.happiness += 0.5;
  } else {
    agent.energy += 3;
    agent.happiness += 0.25;
  }
  agent.energy = std::clamp(agent.energy, 0.0, 100.0);
  agent.happiness = std::clamp(agent.happiness, 0.0, 100.0);
}
inline Behavior makeBehavior(const Config& config) {
  Behavior tree;
  const auto exercise = [](State& state) {
    state.health += 0.7;
    state.energy -= 6;
    state.happiness += 0.6;
    state.last_exercise = state.hour;
  };
  const auto family = [](State& state) {
    const double together = state.companion.enabled && (state.companion.action == "connect" ||
                                                        state.companion.action == "leisure")
                                ? 1.0
                                : 0.0;
    state.relationship += 1.4 + together;
    state.happiness += 0.8 + together * 0.4;
    if (state.companion.enabled) {
      state.companion.happiness += 0.5 + together * 0.5;
    }
    state.energy -= 1;
    state.last_family = state.hour;
  };
  const auto study = [gain = config.knowledge_gain](State& state) {
    state.knowledge += gain;
    state.energy -= 4;
    state.last_study = state.hour;
  };
  tree.add(
      "sleep", "23:00–07:00，或精力低于 20", "sleep",
      [](const State& state) {
        return state.hour % 24 >= 23 || state.hour % 24 < 7 || state.energy < 20;
      },
      [](State& state) {
        state.energy += 13;
        state.health += 0.1;
      });
  tree.add(
      "recover", "健康低于 35", "recover", [](const State& state) { return state.health < 35; },
      [](State& state) {
        state.health += 0.8;
        state.energy += 4;
        state.cash -= 15;
      });
  tree.add(
      "exercise_floor", "至少每 48 小时运动一次", "exercise",
      [](const State& state) { return state.hour - state.last_exercise >= 48; }, exercise);
  tree.add(
      "family_floor", "至少每 48 小时陪伴一次", "family",
      [](const State& state) {
        return state.companion.enabled && state.hour - state.last_family >= 48;
      },
      family);
  tree.add(
      "study_floor", "至少每 72 小时学习一次", "study",
      [](const State& state) { return state.hour - state.last_study >= 72; }, study);
  const int adjustment = config.strategy == Strategy::Money    ? 2
                         : config.strategy == Strategy::Health ? -2
                                                               : 0;
  const int limit = std::clamp(static_cast<int>(config.work_hours) + adjustment, 0, 12);
  tree.add(
      "utility_work", "Utility AI 当前最高分：工作（每日上限 " + std::to_string(limit) + " 小时）",
      "work", [](const State& state) { return utilityChoice(state) == "work"; },
      [income = config.hourly_income](State& state) {
        const double wage = income * state.economy_index * (1 + state.knowledge / 100);
        state.cash += wage;
        state.income_today += wage;
        ++state.work_today;
        state.energy -= 7;
        state.health -= 0.12;
        state.happiness -= 0.2;
      });
  tree.add(
      "utility_exercise", "Utility AI 当前最高分：运动", "exercise",
      [](const State& state) { return utilityChoice(state) == "exercise"; }, exercise);
  tree.add(
      "utility_family", "Utility AI 当前最高分：陪伴", "family",
      [](const State& state) { return utilityChoice(state) == "family"; }, family);
  tree.add(
      "utility_study", "Utility AI 当前最高分：学习", "study",
      [](const State& state) { return utilityChoice(state) == "study"; }, study);
  tree.add(
      "utility_leisure", "Utility AI 当前最高分：休闲", "leisure",
      [](const State& state) { return utilityChoice(state) == "leisure"; },
      [](State& state) {
        state.energy += 3;
        state.happiness += 0.2;
      });
  return tree;
}
inline std::unique_ptr<Selector> makeTree(Strategy strategy) {
  Config config;
  config.strategy = strategy;
  return makeBehavior(config).root;
}
inline void clamp(State& state) {
  for (double* value : {&state.health, &state.energy, &state.happiness, &state.relationship}) {
    *value = std::clamp(*value, 0.0, 100.0);
  }
  state.companion.happiness = std::clamp(state.companion.happiness, 0.0, 100.0);
  state.companion.energy = std::clamp(state.companion.energy, 0.0, 100.0);
}
struct Hour {
  int hour;
  State before;
  State after;
  std::vector<TraceEvent> events;
};
struct Day {
  int day;
  Date date;
  bool workday;
  State state;
  double income;
  double expense;
  double repayment;
  std::vector<int> counts;
  std::vector<Hour> hours;
};
inline const std::vector<std::string> actions = {"sleep", "recover", "exercise", "family",
                                                 "study", "work",    "leisure"};
inline std::vector<Day> simulate(const Config& config) {
  config.validate();
  State state = config.initial;
  state.companion.enabled = config.companion_enabled == 1;
  auto behavior = makeBehavior(config);
  Date date = config.start;
  std::vector<Day> result;
  result.reserve(config.days);
  for (int day = 0; day < config.days; ++day) {
    // A deterministic world signal: the user-selected baseline plus a gentle
    // half-year economic cycle. It affects both the work decision and wages.
    state.economy_index = std::clamp(
        config.economy_index + 0.08 * std::sin(day * 2 * 3.14159265358979323846 / 180), 0.5, 1.5);
    state.work_today = 0;
    state.income_today = 0;
    state.workday = day >= config.vacation_days && weekday(date) != 0 && weekday(date) != 6;
    state.record_trace = day + 1 == config.trace_day;
    std::vector<int> counts(actions.size());
    std::vector<Hour> hours;
    for (int hour = 0; hour < 24; ++hour) {
      state.trace.clear();
      tickCompanion(state);
      updateUtilities(state, config);
      State before = state;
      behavior.root->tick(state);
      ++counts.at(std::find(actions.begin(), actions.end(), state.action) - actions.begin());
      clamp(state);
      if (state.record_trace) {
        State after = state;
        after.trace.clear();
        hours.push_back({hour, std::move(before), std::move(after), state.trace});
      }
      ++state.hour;
    }
    state.trace.clear();
    state.cash -= config.daily_expense;
    state.debt *= 1 + config.annual_interest / 365;
    double repayment = 0;
    if ((day + 1) % 30 == 0) {
      repayment = std::min(state.debt, std::max(0.0, state.cash - config.cash_reserve));
      state.cash -= repayment;
      state.debt -= repayment;
    }
    state.health -= 0.65;
    state.happiness -= 0.5;
    state.relationship -= 0.8;
    if (state.cash < 0) {
      state.happiness -= 1;
      state.health -= 0.1;
    }
    clamp(state);
    result.push_back({day + 1, date, state.workday, state, state.income_today, config.daily_expense,
                      repayment, std::move(counts), std::move(hours)});
    date = nextDate(date);
  }
  return result;
}
inline std::vector<Day> simulate(Strategy strategy, int days) {
  Config config;
  config.strategy = strategy;
  config.days = days;
  return simulate(config);
}
}  // namespace life
