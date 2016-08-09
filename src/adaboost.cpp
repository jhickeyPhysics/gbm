//-----------------------------------
//
// File: adaboost.cpp
//
// Description: distribution used for adaboosting.
//
//-----------------------------------

//-----------------------------------
// Includes
//-----------------------------------
#include "adaboost.h"
#include <memory>

//----------------------------------------
// Function Members - Private
//----------------------------------------
CAdaBoost::CAdaBoost(const parallel_details& parallel)
    : CDistribution(parallel) {}

//----------------------------------------
// Function Members - Public
//----------------------------------------
CDistribution* CAdaBoost::Create(DataDistParams& distparams) {
  return new CAdaBoost(distparams.parallel);
}

CAdaBoost::~CAdaBoost() {}

void CAdaBoost::ComputeWorkingResponse(const CDataset& kData, const Bag& kBag,
                                       const double* kFuncEstimate,
                                       std::vector<double>& residuals) {
#pragma omp parallel for schedule(static, get_array_chunk_size()) \
  num_threads(get_num_threads())
  for (unsigned long i = 0; i < kData.get_trainsize(); i++) {
    residuals[i] = -(2 * kData.y_ptr()[i] - 1) *
                   std::exp(-(2 * kData.y_ptr()[i] - 1) *
                            (kData.offset_ptr()[i] + kFuncEstimate[i]));
  }
}

double CAdaBoost::InitF(const CDataset& kData) {
  double numerator = 0.0;
  double denominator = 0.0;

#pragma omp parallel for schedule(static, get_array_chunk_size()) \
    reduction(+ : numerator, denominator) num_threads(get_num_threads())
  for (unsigned long i = 0; i < kData.get_trainsize(); i++) {
    if (kData.y_ptr()[i] == 1.0) {
      numerator += kData.weight_ptr()[i] * std::exp(-kData.offset_ptr()[i]);
    } else {
      denominator += kData.weight_ptr()[i] * std::exp(kData.offset_ptr()[i]);
    }
  }

  return 0.5 * std::log(numerator / denominator);
}

double CAdaBoost::Deviance(const CDataset& kData, const Bag& kBag,
                           const double* kFuncEstimate) {
  double loss = 0.0;
  double weight = 0.0;

  // Switch to validation set if necessary
  unsigned long num_of_rows_in_set = kData.get_size_of_set();

#pragma omp parallel for schedule(static, get_array_chunk_size()) \
    reduction(+ : loss, weight) num_threads(get_num_threads())
  for (unsigned long i = 0; i < num_of_rows_in_set; i++) {
    loss += kData.weight_ptr()[i] *
            std::exp(-(2 * kData.y_ptr()[i] - 1) *
                     (kData.offset_ptr()[i] + kFuncEstimate[i]));
    weight += kData.weight_ptr()[i];
  }

  // TODO: Check if weights are all zero for validation set
  if ((weight == 0.0) && (loss == 0.0)) {
    return nan("");
  } else if (weight == 0.0) {
    return HUGE_VAL;
  }

  return loss / weight;
}

void CAdaBoost::FitBestConstant(const CDataset& kData, const Bag& kBag,
                                const double* kFuncEstimate,
                                unsigned long num_terminalnodes,
                                std::vector<double>& residuals,
                                CCARTTree& tree) {
  unsigned long obs_num = 0;
  unsigned long node_num = 0;
  numerator_bestconstant_.resize(num_terminalnodes);
  numerator_bestconstant_.assign(numerator_bestconstant_.size(), 0.0);
  denominator_bestconstant_.resize(num_terminalnodes);
  denominator_bestconstant_.assign(denominator_bestconstant_.size(), 0.0);

  for (obs_num = 0; obs_num < kData.get_trainsize(); obs_num++) {
    if (kBag.get_element(obs_num)) {
      const double deltafunc_est =
          kFuncEstimate[obs_num] + kData.offset_ptr()[obs_num];
      numerator_bestconstant_[tree.get_node_assignments()[obs_num]] +=
          kData.weight_ptr()[obs_num] * (2 * kData.y_ptr()[obs_num] - 1) *
          std::exp(-(2 * kData.y_ptr()[obs_num] - 1) * deltafunc_est);
      denominator_bestconstant_[tree.get_node_assignments()[obs_num]] +=
          kData.weight_ptr()[obs_num] *
          std::exp(-(2 * kData.y_ptr()[obs_num] - 1) * deltafunc_est);
    }
  }

  for (node_num = 0; node_num < num_terminalnodes; node_num++) {
    if (tree.has_node(node_num)) {
      if (denominator_bestconstant_[node_num] == 0) {
        tree.get_terminal_nodes()[node_num]->set_prediction(0.0);
      } else {
        tree.get_terminal_nodes()[node_num]->set_prediction(
            numerator_bestconstant_[node_num] /
            denominator_bestconstant_[node_num]);
      }
    }
  }
}

double CAdaBoost::BagImprovement(const CDataset& kData, const Bag& kBag,
                                 const double* kFuncEstimate,
                                 const double kShrinkage,
                                 const std::vector<double>& kDeltaEstimate) {
  double returnvalue = 0.0;
  double weight = 0.0;

#pragma omp parallel for schedule(static, get_array_chunk_size()) \
    reduction(+ : returnvalue, weight) num_threads(get_num_threads())
  for (unsigned long i = 0; i < kData.get_trainsize(); i++) {
    if (!kBag.get_element(i)) {
      const double func_est = kFuncEstimate[i] + kData.offset_ptr()[i];

      returnvalue +=
          kData.weight_ptr()[i] *
          (std::exp(-(2 * kData.y_ptr()[i] - 1) * func_est) -
           std::exp(-(2 * kData.y_ptr()[i] - 1) *
                    (func_est + (kShrinkage) * (kDeltaEstimate[i]))));
      weight += kData.weight_ptr()[i];
    }
  }

  return returnvalue / weight;
}
