#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace life {
enum class Status { Success, Failure };
enum class Strategy { Money, Health, Balanced };
inline std::string name(Strategy s) { return s == Strategy::Money ? "money" : s == Strategy::Health ? "health" : "balanced"; }
inline Strategy parse(const std::string& s) {
  if(s=="money") return Strategy::Money;
  if(s=="health") return Strategy::Health;
  if(s=="balanced") return Strategy::Balanced;
  throw std::invalid_argument("strategy must be money, health or balanced");
}
struct State {
  double health=70, energy=80, cash=0, debt=300000, knowledge=20, happiness=60, relationship=60;
  int hour=0, work_today=0, last_exercise=-24, last_family=-24, last_study=-24;
  double income_today=0;
  std::string action;
  double age() const { return 28.0 + hour/(24.0*365); }
};
struct Node { virtual ~Node()=default; virtual Status tick(State&)=0; };
using Ptr=std::unique_ptr<Node>;
struct Condition : Node {
  std::function<bool(const State&)> predicate;
  explicit Condition(std::function<bool(const State&)> p):predicate(std::move(p)){}
  Status tick(State& s) override {return predicate(s)?Status::Success:Status::Failure;}
};
struct Action : Node {
  std::string label; std::function<void(State&)> apply;
  Action(std::string l,std::function<void(State&)> f):label(std::move(l)),apply(std::move(f)){}
  Status tick(State& s) override { apply(s); s.action=label; return Status::Success; }
};
struct Composite : Node { std::vector<Ptr> children; void add(Ptr n){children.push_back(std::move(n));} };
struct Sequence : Composite {
  Status tick(State& s) override {for(auto& n:children) if(n->tick(s)==Status::Failure) return Status::Failure; return Status::Success;}
};
struct Selector : Composite {
  Status tick(State& s) override {for(auto& n:children) if(n->tick(s)==Status::Success) return Status::Success; return Status::Failure;}
};
inline Ptr branch(std::function<bool(const State&)> p,std::string label,std::function<void(State&)> f){
  auto n=std::make_unique<Sequence>(); n->add(std::make_unique<Condition>(std::move(p))); n->add(std::make_unique<Action>(std::move(label),std::move(f))); return n;
}
inline std::unique_ptr<Selector> makeTree(Strategy strategy) {
  auto root=std::make_unique<Selector>();
  root->add(branch([](const State&s){return s.hour%24>=23||s.hour%24<7||s.energy<20;},"sleep",[](State&s){s.energy+=13;s.health+=0.10;}));
  root->add(branch([](const State&s){return s.health<35;},"recover",[](State&s){s.health+=0.8;s.energy+=4;s.cash-=15;}));
  // Floors prevent lower-priority activities from starving indefinitely.
  root->add(branch([](const State&s){return s.hour-s.last_exercise>=48;},"exercise",[](State&s){s.health+=0.7;s.energy-=6;s.happiness+=0.6;s.last_exercise=s.hour;}));
  root->add(branch([](const State&s){return s.hour-s.last_family>=48;},"family",[](State&s){s.relationship+=2;s.happiness+=1;s.energy-=1;s.last_family=s.hour;}));
  root->add(branch([](const State&s){return s.hour-s.last_study>=72;},"study",[](State&s){s.knowledge+=0.12;s.energy-=4;s.last_study=s.hour;}));
  const int limit=strategy==Strategy::Money?10:strategy==Strategy::Health?6:8;
  auto work=[limit](const State&s){return s.hour%24>=9&&s.hour%24<21&&s.hour/24%7<5&&s.work_today<limit;};
  auto earn=[](State&s){double wage=50*(1+s.knowledge/100);s.cash+=wage;s.income_today+=wage;s.work_today++;s.energy-=7;s.health-=0.12;s.happiness-=0.2;};
  if(strategy!=Strategy::Money) root->add(branch([](const State&s){return s.hour-s.last_exercise>=24;},"exercise",[](State&s){s.health+=0.7;s.energy-=6;s.happiness+=0.6;s.last_exercise=s.hour;}));
  root->add(branch(work,"work",earn));
  if(strategy!=Strategy::Money){
    root->add(branch([](const State&s){return s.hour-s.last_family>=24;},"family",[](State&s){s.relationship+=2;s.happiness+=1;s.last_family=s.hour;}));
    root->add(branch([](const State&s){return s.hour-s.last_study>=24;},"study",[](State&s){s.knowledge+=0.12;s.energy-=4;s.last_study=s.hour;}));
  }
  root->add(std::make_unique<Action>("leisure",[](State&s){s.energy+=3;s.happiness+=0.2;}));
  return root;
}
inline void clamp(State&s){for(double* v:{&s.health,&s.energy,&s.happiness,&s.relationship}) *v=std::clamp(*v,0.0,100.0);}
struct Day { int day; State state; double income,expense,repayment; std::vector<int> counts; };
inline const std::vector<std::string> actions={"sleep","recover","exercise","family","study","work","leisure"};
inline std::vector<Day> simulate(Strategy strategy,int days){
  if(days<1||days>36500) throw std::invalid_argument("days must be 1..36500");
  State s; auto tree=makeTree(strategy); std::vector<Day> result;
  for(int d=0;d<days;++d){
    s.work_today=0;s.income_today=0;std::vector<int> counts(actions.size());
    for(int h=0;h<24;++h){tree->tick(s);++counts.at(std::find(actions.begin(),actions.end(),s.action)-actions.begin());clamp(s);++s.hour;}
    const double expense=120; s.cash-=expense;
    // Annual debt interest 4%, daily accrual; repayment retains a cash reserve.
    s.debt*=1+0.04/365;
    double repayment=0;
    if((d+1)%30==0){repayment=std::min(s.debt,std::max(0.0,s.cash-3000));s.cash-=repayment;s.debt-=repayment;}
    s.health-=0.65;s.happiness-=0.5;s.relationship-=0.8;
    if(s.cash<0){s.happiness-=1;s.health-=0.1;}
    clamp(s);result.push_back({d+1,s,s.income_today,expense,repayment,counts});
  }
  return result;
}
}
