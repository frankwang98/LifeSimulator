#include <iostream>
#include <numeric>

#include "life.h"
void check(bool ok, const char* message) {
  if (!ok)
    throw std::runtime_error(message);
}
int main() {
  try {
    life::State s;
    s.hour = 23;
    auto t = life::makeTree(life::Strategy::Money);
    t->tick(s);
    check(s.action == "sleep", "night sleep");
    s.hour = 12;
    s.health = 20;
    s.energy = 80;
    t->tick(s);
    check(s.action == "recover", "health emergency overrides work");
    for (auto mode : {life::Strategy::Money, life::Strategy::Health, life::Strategy::Balanced}) {
      auto a = life::simulate(mode, 365), b = life::simulate(mode, 365);
      int exercise = 0, family = 0, study = 0;
      double paid = 0;
      for (size_t i = 0; i < a.size(); ++i) {
        auto& d = a[i];
        auto& v = d.state;
        check(std::accumulate(d.counts.begin(), d.counts.end(), 0) == 24, "24 hours each day");
        check(v.health >= 0 && v.health <= 100 && v.energy >= 0 && v.energy <= 100 &&
                  v.relationship >= 0 && v.relationship <= 100 && v.happiness >= 0 &&
                  v.happiness <= 100,
              "state bounds");
        check(std::isfinite(v.cash) && v.debt >= 0, "financial bounds");
        check(v.cash == b[i].state.cash && d.counts == b[i].counts, "determinism");
        double previous = i ? a[i - 1].state.cash : 0;
        check(std::abs(v.cash -
                       (previous + d.income - d.expense - 15 * d.counts[1] - d.repayment)) < 1e-6,
              "cash conservation");
        exercise += d.counts[2];
        family += d.counts[3];
        study += d.counts[4];
        paid += d.repayment;
      }
      check(exercise >= 150 && family >= 150 && study >= 100, "anti starvation");
      check(paid > 0, "repayment");
    }
    auto money = life::simulate(life::Strategy::Money, 365),
         health = life::simulate(life::Strategy::Health, 365);
    check(money.back().state.debt < health.back().state.debt, "money strategy repays faster");
    bool rejected = false;
    try {
      life::simulate(life::Strategy::Balanced, 0);
    } catch (const std::invalid_argument&) {
      rejected = true;
    }
    check(rejected, "invalid duration");
    check(life::weekday(life::parseDate("2026-10-01")) == 4, "calendar starts on Thursday");
    check(life::dateString(life::nextDate(life::parseDate("2028-02-28"))) == "2028-02-29",
          "leap day");
    check(life::dateString(life::nextDate(life::parseDate("2026-12-31"))) == "2027-01-01",
          "new year");
    auto config = life::parseConfig("days=30\ntrace_day=1\nvacation_days=7");
    auto month = life::simulate(config);
    check(life::dateString(month.front().date) == "2026-10-01", "actual start date");
    for (int day = 0; day < 7; ++day) {
      check(!month[day].workday && month[day].counts[5] == 0, "vacation suppresses work");
    }
    check(month[7].workday, "October 8 is a weekday after vacation");
    check(!month[9].workday, "Saturday is not a workday");
    check(month[0].hours.size() == 24, "24 actual tick traces");
    for (const auto& hour : month[0].hours) {
      int executed = 0;
      for (const auto& event : hour.events) {
        if (event.kind == "action")
          ++executed;
      }
      check(executed == 1, "selector executes exactly one action");
      check(hour.events.back().node == "root" && hour.events.back().status == life::Status::Success,
            "root completes after selected branch");
      check(hour.after.action != "work", "trace matches holiday decision");
    }
    check(month[1].hours.empty(), "only selected day retains traces");
    config.trace_day = 8;
    auto later = life::simulate(config);
    check(later[7].hours.size() == 24 && later.back().state.cash == month.back().state.cash,
          "recording another day does not change model outcome");
    config.hourly_income = 100;
    auto higher_income = life::simulate(config);
    check(higher_income.back().state.debt < month.back().state.debt,
          "parameters change the future");
    config.work_hours = 0;
    auto no_work = life::simulate(config);
    for (const auto& day : no_work)
      check(day.counts[5] == 0, "zero working hours");
    for (const std::string invalid :
         {"health=nan", "days=1.5", "debt=-1", "start_date=2026-02-30", "unknown=1"}) {
      bool invalid_rejected = false;
      try {
        life::parseConfig(invalid);
      } catch (const std::exception&) {
        invalid_rejected = true;
      }
      check(invalid_rejected, "configuration rejects invalid values");
    }
    std::cout << "All simulation tests passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
