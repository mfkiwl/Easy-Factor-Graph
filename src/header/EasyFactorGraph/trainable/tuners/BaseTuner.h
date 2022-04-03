/**
 * Author:    Andrea Casalino
 * Created:   01.01.2021
 *
 * report any bug to andrecasa91@gmail.com.
 **/

#pragma once

#include <EasyFactorGraph/distribution/CombinationFinder.h>
#include <EasyFactorGraph/trainable/tuners/Tuner.h>

namespace EFG::train {
class BaseTuner : public Tuner {
public:
  std::vector<FactorExponentialPtr> getFactors() const final {
    return {factor};
  }

  void setTrainSetIterator(const TrainSet::Iterator &iter) final;

  float getGradientAlpha() final { return gradientAlpha; };
  void setWeight(const float &w) final { factor->setWeight(w); }
  float getWeight() const final { return factor->getWeight(); };
  bool isHereGroup(const categoric::VariablesSet &group) const final {
    return factor->getVariables().getVariablesSet() == group;
  }

protected:
  BaseTuner(const FactorExponentialPtr &factor,
            const categoric::VariablesSoup &variables_in_model);

  float dotProduct(const std::vector<float> &prob) const;

  const distribution::FactorExponential &getFactor() const { return *factor; }

private:
  FactorExponentialPtr factor;

  const TrainSet::Iterator *train_set_iterator = nullptr;
  const distribution::CombinationFinder finder;
  float gradientAlpha = 0;
};
} // namespace EFG::train
