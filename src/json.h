#pragma once

#include <iomanip>
#include <sstream>

#include "life.h"

namespace life {
inline void quote(std::ostream& output, const std::string& text) {
  output << '"';
  for (const unsigned char character : text) {
    switch (character) {
      case '"':
        output << "\\\"";
        break;
      case '\\':
        output << "\\\\";
        break;
      case '\n':
        output << "\\n";
        break;
      case '\r':
        output << "\\r";
        break;
      case '\t':
        output << "\\t";
        break;
      default:
        if (character < 32) {
          output << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << int(character)
                 << std::dec << std::setfill(' ');
        } else {
          output << character;
        }
    }
  }
  output << '"';
}
inline void stateJson(std::ostream& output, const State& state) {
  output << "{\"age\":" << state.age() << ",\"health\":" << state.health
         << ",\"energy\":" << state.energy << ",\"cash\":" << state.cash
         << ",\"debt\":" << state.debt << ",\"knowledge\":" << state.knowledge
         << ",\"happiness\":" << state.happiness << ",\"relationship\":" << state.relationship
         << '}';
}
inline std::string simulationJson(const Config& config) {
  const auto days = simulate(config);
  const auto behavior = makeBehavior(config);
  std::ostringstream output;
  output << std::fixed << std::setprecision(6);
  output << "{\"engine\":\"cpp-behavior-tree\",\"start_date\":";
  quote(output, dateString(config.start));
  output << ",\"strategy\":";
  quote(output, name(config.strategy));
  output << ",\"initial\":";
  stateJson(output, config.initial);
  output << ",\"branches\":[";
  for (size_t index = 0; index < behavior.branches.size(); ++index) {
    if (index)
      output << ',';
    const auto& branch = behavior.branches[index];
    output << "{\"id\":";
    quote(output, branch.id);
    output << ",\"condition\":";
    quote(output, branch.condition);
    output << ",\"action\":";
    quote(output, branch.action);
    output << '}';
  }
  output << "],\"days\":[";
  for (size_t index = 0; index < days.size(); ++index) {
    if (index)
      output << ',';
    const auto& day = days[index];
    output << "{\"day\":" << day.day << ",\"date\":";
    quote(output, dateString(day.date));
    output << ",\"weekday\":" << weekday(day.date)
           << ",\"workday\":" << (day.workday ? "true" : "false") << ",\"state\":";
    stateJson(output, day.state);
    output << ",\"income\":" << day.income << ",\"expense\":" << day.expense
           << ",\"repayment\":" << day.repayment << ",\"counts\":[";
    for (size_t action = 0; action < day.counts.size(); ++action) {
      if (action)
        output << ',';
      output << day.counts[action];
    }
    output << "]}";
  }
  output << "],\"trace_day\":" << config.trace_day << ",\"hours\":[";
  const auto& hours = days.at(config.trace_day - 1).hours;
  for (size_t index = 0; index < hours.size(); ++index) {
    if (index)
      output << ',';
    const auto& hour = hours[index];
    output << "{\"hour\":" << hour.hour << ",\"action\":";
    quote(output, hour.after.action);
    output << ",\"before\":";
    stateJson(output, hour.before);
    output << ",\"after\":";
    stateJson(output, hour.after);
    output << ",\"events\":[";
    for (size_t event = 0; event < hour.events.size(); ++event) {
      if (event)
        output << ',';
      const auto& trace = hour.events[event];
      output << "{\"node\":";
      quote(output, trace.node);
      output << ",\"kind\":";
      quote(output, trace.kind);
      output << ",\"status\":";
      quote(output, trace.status == Status::Success ? "SUCCESS" : "FAILURE");
      output << '}';
    }
    output << "]}";
  }
  output << "]}";
  return output.str();
}
inline std::string errorJson(const std::string& message) {
  std::ostringstream output;
  output << "{\"error\":";
  quote(output, message);
  output << '}';
  return output.str();
}
}  // namespace life
