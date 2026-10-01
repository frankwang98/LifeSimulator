#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "life.h"

int main(int argc, char** argv) {
  try {
    int days = 365;
    std::string strategy = "balanced", output = "output";
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--help") {
        std::cout << "Life Tree: --days 365 --strategy balanced|money|health|all --output output\n";
        return 0;
      }
      if (i + 1 >= argc)
        throw std::invalid_argument("missing value for " + arg);
      std::string value = argv[++i];
      if (arg == "--days") {
        size_t used = 0;
        days = std::stoi(value, &used);
        if (used != value.size())
          throw std::invalid_argument("invalid days");
      } else if (arg == "--strategy")
        strategy = value;
      else if (arg == "--output")
        output = value;
      else
        throw std::invalid_argument("unknown option " + arg);
    }
    if (days < 1 || days > 36500)
      throw std::invalid_argument("days must be 1..36500");
    std::vector<life::Strategy> strategies;
    if (strategy == "all")
      strategies = {life::Strategy::Money, life::Strategy::Health, life::Strategy::Balanced};
    else
      strategies = {life::parse(strategy)};
    std::filesystem::create_directories(output);
    std::cout << "strategy  health energy cash debt knowledge happiness relationship\n";
    for (auto mode : strategies) {
      auto data = life::simulate(mode, days);
      auto path = std::filesystem::path(output) / (life::name(mode) + ".csv");
      std::ofstream csv(path);
      if (!csv)
        throw std::runtime_error("cannot write " + path.string());
      csv << "day,age,health,energy,cash,debt,knowledge,happiness,relationship,income,expense,"
             "repayment";
      for (auto& a : life::actions) {
        csv << "," << a << "_hours";
      }
      csv << "\n";
      csv << std::fixed << std::setprecision(3);
      for (auto& d : data) {
        auto& s = d.state;
        csv << d.day << "," << s.age() << "," << s.health << "," << s.energy << "," << s.cash << ","
            << s.debt << "," << s.knowledge << "," << s.happiness << "," << s.relationship << ","
            << d.income << "," << d.expense << "," << d.repayment;
        for (int n : d.counts)
          csv << "," << n;
        csv << "\n";
      }
      csv.close();
      if (!csv)
        throw std::runtime_error("failed writing " + path.string());
      auto& s = data.back().state;
      std::cout << std::fixed << std::setprecision(1) << life::name(mode) << " " << s.health << " "
                << s.energy << " " << s.cash << " " << s.debt << " " << s.knowledge << " "
                << s.happiness << " " << s.relationship << "\n";
      std::cout << "Daily records: " << path << "\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
