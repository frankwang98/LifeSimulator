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
inline Behavior makeBehavior(const Config& config) {
  Behavior tree;
  const auto exercise = [](State& state) {
    state.health += 0.7;
    state.energy -= 6;
    state.happiness += 0.6;
    state.last_exercise = state.hour;
  };
  const auto family = [](State& state) {
    state.relationship += 2;
    state.happiness += 1;
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
      [](const State& state) { return state.hour - state.last_family >= 48; }, family);
  tree.add(
      "study_floor", "至少每 72 小时学习一次", "study",
      [](const State& state) { return state.hour - state.last_study >= 72; }, study);
  if (config.strategy != Strategy::Money) {
    tree.add(
        "exercise", "到达设定的运动间隔", "exercise",
        [interval = config.exercise_interval](const State& state) {
          return state.hour - state.last_exercise >= interval;
        },
        exercise);
  }
  const int adjustment = config.strategy == Strategy::Money    ? 2
                         : config.strategy == Strategy::Health ? -2
                                                               : 0;
  const int limit = std::clamp(static_cast<int>(config.work_hours) + adjustment, 0, 12);
  tree.add(
      "work", "工作日 09:00–21:00，未超过每日 " + std::to_string(limit) + " 小时上限", "work",
      [limit](const State& state) {
        return state.workday && state.hour % 24 >= 9 && state.hour % 24 < 21 &&
               state.work_today < limit;
      },
      [income = config.hourly_income](State& state) {
        const double wage = income * (1 + state.knowledge / 100);
        state.cash += wage;
        state.income_today += wage;
        ++state.work_today;
        state.energy -= 7;
        state.health -= 0.12;
        state.happiness -= 0.2;
      });
  if (config.strategy != Strategy::Money) {
    tree.add(
        "family", "到达设定的陪伴间隔", "family",
        [interval = config.family_interval](const State& state) {
          return state.hour - state.last_family >= interval;
        },
        family);
    tree.add(
        "study", "到达设定的学习间隔", "study",
        [interval = config.study_interval](const State& state) {
          return state.hour - state.last_study >= interval;
        },
        study);
  }
  tree.add(
      "leisure", "其他行为未被选中", "leisure", [](const State&) { return true; },
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
  auto behavior = makeBehavior(config);
  Date date = config.start;
  std::vector<Day> result;
  result.reserve(config.days);
  for (int day = 0; day < config.days; ++day) {
    state.work_today = 0;
    state.income_today = 0;
    state.workday = day >= config.vacation_days && weekday(date) != 0 && weekday(date) != 6;
    state.record_trace = day + 1 == config.trace_day;
    std::vector<int> counts(actions.size());
    std::vector<Hour> hours;
    for (int hour = 0; hour < 24; ++hour) {
      state.trace.clear();
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
