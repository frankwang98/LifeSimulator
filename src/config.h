#pragma once

#include <cmath>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "calendar.h"
#include "state.h"

namespace life {
inline std::string name(Strategy strategy) {
  return strategy == Strategy::Money    ? "money"
         : strategy == Strategy::Health ? "health"
                                        : "balanced";
}
inline Strategy parse(const std::string& text) {
  if (text == "money") {
    return Strategy::Money;
  }
  if (text == "health") {
    return Strategy::Health;
  }
  if (text == "balanced") {
    return Strategy::Balanced;
  }
  throw std::invalid_argument("strategy must be money, health or balanced");
}
struct Config {
  State initial;
  Date start;
  Strategy strategy = Strategy::Balanced;
  int days = 365;
  int trace_day = 1;
  int vacation_days = 7;
  double hourly_income = 50;
  double daily_expense = 120;
  double work_hours = 8;
  double exercise_interval = 24;
  double family_interval = 24;
  double study_interval = 24;
  double knowledge_gain = 0.12;
  double annual_interest = 0.04;
  double cash_reserve = 3000;
  void validate() const {
    auto range = [](double value, double low, double high, const char* key) {
      if (!std::isfinite(value) || value < low || value > high) {
        throw std::invalid_argument(std::string(key) + " is outside its allowed range");
      }
    };
    range(days, 1, 3650, "days");
    range(trace_day, 1, days, "trace_day");
    range(vacation_days, 0, 365, "vacation_days");
    range(initial.initial_age, 0, 100, "age");
    for (double score : {initial.health, initial.energy, initial.happiness, initial.relationship}) {
      range(score, 0, 100, "score");
    }
    range(initial.cash, -1e8, 1e8, "cash");
    range(initial.debt, 0, 1e8, "debt");
    range(initial.knowledge, 0, 10000, "knowledge");
    range(hourly_income, 0, 10000, "hourly_income");
    range(daily_expense, 0, 100000, "daily_expense");
    range(work_hours, 0, 12, "work_hours");
    if (std::floor(work_hours) != work_hours) {
      throw std::invalid_argument("work_hours must be an integer");
    }
    for (double interval : {exercise_interval, family_interval, study_interval}) {
      range(interval, 1, 168, "activity interval");
    }
    range(knowledge_gain, 0, 10, "knowledge_gain");
    range(annual_interest, 0, 1, "annual_interest");
    range(cash_reserve, 0, 1e8, "cash_reserve");
    parseDate(dateString(start));
  }
};
// The native CLI and WASM use exactly the same key=value configuration parser.
inline Config parseConfig(const std::string& text) {
  Config config;
  std::map<std::string, double*> values = {{"age", &config.initial.initial_age},
                                           {"health", &config.initial.health},
                                           {"energy", &config.initial.energy},
                                           {"cash", &config.initial.cash},
                                           {"debt", &config.initial.debt},
                                           {"knowledge", &config.initial.knowledge},
                                           {"happiness", &config.initial.happiness},
                                           {"relationship", &config.initial.relationship},
                                           {"hourly_income", &config.hourly_income},
                                           {"daily_expense", &config.daily_expense},
                                           {"work_hours", &config.work_hours},
                                           {"exercise_interval", &config.exercise_interval},
                                           {"family_interval", &config.family_interval},
                                           {"study_interval", &config.study_interval},
                                           {"knowledge_gain", &config.knowledge_gain},
                                           {"annual_interest", &config.annual_interest},
                                           {"cash_reserve", &config.cash_reserve}};
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line.front() == '#') {
      continue;
    }
    const auto split = line.find('=');
    if (split == std::string::npos) {
      throw std::invalid_argument("configuration requires key=value lines");
    }
    const std::string key = line.substr(0, split), value = line.substr(split + 1);
    if (key == "start_date") {
      config.start = parseDate(value);
    } else if (key == "strategy") {
      config.strategy = parse(value);
    } else {
      size_t used = 0;
      const double number = std::stod(value, &used);
      if (used != value.size() || !std::isfinite(number)) {
        throw std::invalid_argument("invalid numeric value for " + key);
      }
      if (key == "days" || key == "trace_day" || key == "vacation_days") {
        if (number < 0 || number > 3650 || std::floor(number) != number) {
          throw std::invalid_argument("invalid integer value for " + key);
        }
        if (key == "days") {
          config.days = static_cast<int>(number);
        } else if (key == "trace_day") {
          config.trace_day = static_cast<int>(number);
        } else {
          config.vacation_days = static_cast<int>(number);
        }
      } else {
        const auto found = values.find(key);
        if (found == values.end()) {
          throw std::invalid_argument("unknown configuration key: " + key);
        }
        *found->second = number;
      }
    }
  }
  config.validate();
  return config;
}
}  // namespace life
