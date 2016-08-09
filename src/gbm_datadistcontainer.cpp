//-----------------------------------
//
// File: gbmDataContainer.cpp
//
// Description: class that contains the data and distribution.
//
//-----------------------------------

//------------------------------
// Includes
//------------------------------
#include "gbm_datadistcontainer.h"
#include "pairwise.h"

//----------------------------------------
// Function Members - Public
//----------------------------------------
//-----------------------------------
// Function: CGBMDataContainer
//
// Returns: none
//
// Description: Default constructor for gbm data container.
//
// Parameters: ...
//-----------------------------------
CGBMDataDistContainer::CGBMDataDistContainer(DataDistParams& datadist_config)
    : data_(datadist_config),
      databag_(datadist_config),
      distfactory_(new DistributionFactory()),
      distptr_(distfactory_->CreateDist(datadist_config)) {
  // Initialize the factory and then use to get the disribution
  distptr_->Initialize(data_);
}

//-----------------------------------
// Function: InitializeFunctionEstimate
//
// Returns: none
//
// Description: Initialize the function fit.
//
// Parameters: double& - reference to the initial function estimate (a constant)
//    unsigned long - the number of predictors the fit must provide response
//    estimates for
//
//-----------------------------------
double CGBMDataDistContainer::InitialFunctionEstimate() {
  return get_dist()->InitF(data_);
}

//-----------------------------------
// Function: ComputeResiduals
//
// Returns: nonei = 0; i < data.getNumUniquePatient()
//
// Description: Compute the residuals associated with the distributions loss
// function.
//
// Parameters: const double ptr - ptr to the function estimates for each
// predictor
//    CTreeComps ptr - ptr to the tree components container in the gbm.
//
//-----------------------------------
void CGBMDataDistContainer::ComputeResiduals(const double* kFuncEstimate,
                                             std::vector<double>& residuals) {
  get_dist()->ComputeWorkingResponse(get_data(), get_bag(), kFuncEstimate,
                                     residuals);
}

//-----------------------------------
// Function: ComputeBestTermNodePreds
//
// Returns: none
//
// Description: Fit the best constants (predictions) to the terminal nodes.
//
// Parameters: const double ptr - ptr to function estimates for each predictor
//    CTreeComps ptr - ptr to the tree components container in the gbm
//    int& - reference to the number of nodes in the tree.
//-----------------------------------
void CGBMDataDistContainer::ComputeBestTermNodePreds(
    const double* kFuncEstimate, std::vector<double>& residuals,
    CCARTTree& tree) {
  get_dist()->FitBestConstant(
      get_data(), get_bag(), &kFuncEstimate[0],
      (2 * tree.size_of_tree() + 1) / 3,  // number of terminal nodes
      residuals, tree);
}

//-----------------------------------
// Function: ComputeDeviance
//
// Returns: double
//
// Description: Compute the deviance (error) of the fit on training/validation
// data.
//
// Parameters: const double ptr - ptr to function estimates for each predictor
//    CTreeComps ptr - ptr to the tree components container in the gbm
//    bool - bool which indicates whether it is the training or validation data
//    used.
//
//-----------------------------------
double CGBMDataDistContainer::ComputeDeviance(const double* kFuncEstimate,
                                              bool is_validationset) {
  double deviance = 0.0;
  if (!(is_validationset)) {
    deviance = get_dist()->Deviance(get_data(), get_bag(), kFuncEstimate);
  } else {
    // Shift to validation set, calculate deviance and shift back
    shift_datadist_to_validation();
    deviance = get_dist()->Deviance(get_data(), get_bag(),
                                    kFuncEstimate + data_.get_trainsize());
    shift_datadist_to_train();
  }
  return deviance;
}

//-----------------------------------
// Function: ComputeBagImprovement
//
// Returns: double
//
// Description: Compute the improvement from combining decision trees
//
// Parameters: const double ptr - ptr to function estimates for each predictor
//    CTreeComps ptr - ptr to the tree components container in the gbm
//
//-----------------------------------
double CGBMDataDistContainer::ComputeBagImprovement(
    const double* kFuncEstimate, const double kShrinkage,
    const std::vector<double>& kDeltaEstimate) {
  return get_dist()->BagImprovement(get_data(), get_bag(), &kFuncEstimate[0],
                                    kShrinkage, kDeltaEstimate);
}

//-----------------------------------
// Function: BagData
//
// Returns: none
//
// Description: put data into bags.
//
// Parameters: bool - determines if distribution is pairwise
//    CDistribution ptr - pointer to the distribution + data
//
//-----------------------------------
void CGBMDataDistContainer::BagData() {
  get_bag().clear();
  get_dist()->BagData(get_data(), get_bag());
}
