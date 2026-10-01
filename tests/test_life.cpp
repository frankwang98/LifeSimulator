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
    std::cout << "All simulation tests passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
