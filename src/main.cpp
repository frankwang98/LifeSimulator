#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "json.h"

int main(int argc, char** argv) {
  try {
    life::Config config;
    bool json = false;
    for (int i = 1; i < argc; ++i) {
      if (std::string(argv[i]) == "--config") {
        if (++i >= argc)
          throw std::invalid_argument("missing config path");
        std::ifstream input(argv[i]);
        if (!input)
          throw std::runtime_error("cannot read configuration");
        std::ostringstream buffer;
        buffer << input.rdbuf();
        config = life::parseConfig(buffer.str());
      }
    }
    int days = config.days;
    std::string strategy = life::name(config.strategy), output = "output";
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--help") {
        std::cout << "LifeSimulator: --config world.conf --json --days 365 --strategy "
                     "balanced|money|health|all --output output\n";
        return 0;
      }
      if (arg == "--json") {
        json = true;
        continue;
      }
      if (i + 1 >= argc)
        throw std::invalid_argument("missing value for " + arg);
      std::string value = argv[++i];
      if (arg == "--config") {
        continue;
      }
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
    if (days < 1 || days > 3650)
      throw std::invalid_argument("days must be 1..3650");
    std::vector<life::Strategy> strategies;
    if (strategy == "all")
      strategies = {life::Strategy::Money, life::Strategy::Health, life::Strategy::Balanced};
    else
      strategies = {life::parse(strategy)};
    config.days = days;
    config.strategy = life::parse(strategy == "all" ? "balanced" : strategy);
    config.validate();
    if (json) {
      if (strategy == "all")
        throw std::invalid_argument("--json requires one strategy");
      std::cout << life::simulationJson(config) << "\n";
      return 0;
    }
    std::filesystem::create_directories(output);
    std::cout << "strategy  health energy cash debt knowledge happiness relationship\n";
    for (auto mode : strategies) {
      config.strategy = mode;
      auto data = life::simulate(config);
      auto path = std::filesystem::path(output) / (life::name(mode) + ".csv");
      std::ofstream csv(path);
      if (!csv)
        throw std::runtime_error("cannot write " + path.string());
      csv << "day,date,workday,age,health,energy,cash,debt,knowledge,happiness,relationship,income,"
             "expense,"
             "repayment";
      for (auto& a : life::actions) {
        csv << "," << a << "_hours";
      }
      csv << "\n";
      csv << std::fixed << std::setprecision(3);
      for (auto& d : data) {
        auto& s = d.state;
        csv << d.day << "," << life::dateString(d.date) << "," << d.workday << "," << s.age() << ","
            << s.health << "," << s.energy << "," << s.cash << "," << s.debt << "," << s.knowledge
            << "," << s.happiness << "," << s.relationship << "," << d.income << "," << d.expense
            << "," << d.repayment;
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
