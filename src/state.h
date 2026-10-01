#pragma once

#include <string>
#include <vector>

namespace life {
enum class Status { Success, Failure };
enum class Strategy { Money, Health, Balanced };
struct TraceEvent {
  std::string node;
  std::string kind;
  Status status;
};
struct State {
  double health = 70;
  double energy = 80;
  double cash = 0;
  double debt = 300000;
  double knowledge = 20;
  double happiness = 60;
  double relationship = 60;
  double initial_age = 28;
  int hour = 0;
  int work_today = 0;
  int last_exercise = -24;
  int last_family = -24;
  int last_study = -24;
  bool workday = false;
  bool record_trace = false;
  double income_today = 0;
  std::string action;
  std::vector<TraceEvent> trace;
  double age() const {
    return initial_age + hour / (24.0 * 365.2425);
  }
};
inline void record(State& state, const std::string& node, const std::string& kind, Status status) {
  if (state.record_trace) {
    state.trace.push_back({node, kind, status});
  }
}
}  // namespace life
