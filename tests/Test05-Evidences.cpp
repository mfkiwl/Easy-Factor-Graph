#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <sstream>

#include "Utils.h"
#include <EasyFactorGraph/structure/EvidenceManager.h>
#include <EasyFactorGraph/structure/FactorsManager.h>

using namespace EFG;
using namespace EFG::categoric;
using namespace EFG::distribution;
using namespace EFG::strct;
using namespace EFG::test;

namespace {
class EvidenceTest : public EvidenceSetter,
                     public EvidenceRemover,
                     protected FactorsAdder {
public:
  VariablesSoup uVars;
  VariablesSoup mVars;
  VariablesSoup lVars;

  EvidenceTest() {
    auto connect = [this](const VariablePtr &a, const VariablePtr &b) {
      addConstFactor(make_corr_factor2(a, b));
    };

    auto createVariable = [](const std::string &name, std::size_t id) {
      std::stringstream s;
      s << name << id;
      return make_variable(2, s.str());
    };

    this->uVars.reserve(3);
    this->mVars.reserve(4);
    this->lVars.reserve(3);

    this->uVars.push_back(createVariable("A", 0));
    this->mVars.push_back(createVariable("M", 0));
    this->lVars.push_back(createVariable("L", 0));
    connect(this->uVars.back(), this->mVars.back());
    connect(this->lVars.back(), this->mVars.back());

    for (std::size_t k = 1; k < 3; ++k) {
      std::size_t s = this->mVars.size();
      this->uVars.push_back(createVariable("A", k));
      this->mVars.push_back(createVariable("M", k));
      this->lVars.push_back(createVariable("L", k));
      connect(this->uVars.back(), this->mVars.back());
      connect(this->lVars.back(), this->mVars.back());
      connect(this->uVars[s - 1], this->mVars.back());
      connect(this->lVars[s - 1], this->mVars.back());
    }
  };

  void clusterExists(const VariablesSet &vars) {
    std::set<Variable *> to_find;
    for (const auto &var : vars) {
      to_find.emplace(var.get());
    }
    std::vector<std::set<Variable *>> clusters_as_vars;
    for (const auto &cluster : getState().clusters) {
      auto &set = clusters_as_vars.emplace_back();
      for (const auto *node : cluster.nodes) {
        set.emplace(node->variable.get());
      }
    }
    if (std::find(clusters_as_vars.begin(), clusters_as_vars.end(), to_find) ==
        clusters_as_vars.end()) {
      std::stringstream stream;
      stream << "Hidden cluster: <";
      for (const auto &var : vars) {
        stream << ' ' << var->name();
      }
      stream << '>';
      stream << " was not found";
      throw Error{stream.str()};
    }
  };
};
} // namespace

TEST_CASE("testing evidence managing", "[evidence]") {
  EvidenceTest model;
  model.clusterExists(VariablesSet{model.getAllVariables().begin(),
                                   model.getAllVariables().end()});
  CHECK(model.getEvidences().empty());

  model.setEvidence(model.mVars[1], 0);
  model.clusterExists(
      VariablesSet{model.uVars[0], model.mVars[0], model.lVars[0]});
  model.clusterExists(VariablesSet{model.uVars[2], model.mVars[2],
                                   model.lVars[2], model.uVars[1],
                                   model.lVars[1]});
  {
    Evidences expected;
    expected.emplace(model.mVars[1], 0);
    CHECK(model.getEvidences() == expected);
  }

  model.setEvidence(model.mVars[2], 1);
  model.clusterExists(
      VariablesSet{model.uVars[0], model.mVars[0], model.lVars[0]});
  model.clusterExists(VariablesSet{model.uVars[1]});
  model.clusterExists(VariablesSet{model.uVars[2]});
  model.clusterExists(VariablesSet{model.lVars[1]});
  model.clusterExists(VariablesSet{model.lVars[2]});
  {
    Evidences expected;
    expected.emplace(model.mVars[1], 0);
    expected.emplace(model.mVars[2], 1);
    CHECK(model.getEvidences() == expected);
  }
}

TEST_CASE("evidence individual reset", "[evidence]") {
  EvidenceTest model;

  model.setEvidence(model.mVars[1], 0);
  model.setEvidence(model.mVars[2], 0);

  model.removeEvidence(model.mVars[2]);
  model.clusterExists(
      VariablesSet{model.uVars[0], model.mVars[0], model.lVars[0]});
  model.clusterExists(VariablesSet{model.uVars[1], model.uVars[2],
                                   model.lVars[1], model.lVars[2],
                                   model.mVars[2]});
  {
    Evidences expected;
    expected.emplace(model.mVars[1], 0);
    CHECK(model.getEvidences() == expected);
  }

  model.removeEvidence(model.mVars[1]);
  model.clusterExists(VariablesSet{model.getAllVariables().begin(),
                                   model.getAllVariables().end()});
  CHECK(model.getEvidences().empty());
}

TEST_CASE("evidence group reset", "[evidence]") {
  EvidenceTest model;

  model.setEvidence(model.mVars[1], 0);
  model.setEvidence(model.mVars[2], 0);

  model.removeEvidences(VariablesSet{model.mVars[1], model.mVars[2]});
  model.clusterExists(VariablesSet{model.getAllVariables().begin(),
                                   model.getAllVariables().end()});
  CHECK(model.getEvidences().empty());
}

TEST_CASE("evidence total reset", "[evidence]") {
  EvidenceTest model;

  model.setEvidence(model.mVars[1], 0);
  model.setEvidence(model.mVars[2], 0);

  model.removeAllEvidences();
  model.clusterExists(VariablesSet{model.getAllVariables().begin(),
                                   model.getAllVariables().end()});
  CHECK(model.getEvidences().empty());
  // CHECK_THROWS_AS(model.removeEvidence(model.mVars[2]), Error); // TODO find
  // a way to make it work
}
