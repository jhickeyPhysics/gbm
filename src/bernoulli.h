//------------------------------------------------------------------------------
//
//  File:       bernoulli.h
//
//  Description:   bernoulli distribution class used in GBM
//
//  History:    3/26/2001   gregr created
//              2/14/2003   gregr: adapted for R implementation
//
//------------------------------------------------------------------------------

#ifndef BERNOULLI_H
#define BERNOULLI_H

//------------------------------
// Includes
//------------------------------
#include "distribution.h"
#include <memory>

//------------------------------
// Class definition
//------------------------------
class CBernoulli : public CDistribution {
 public:
  //---------------------
  // Factory Function
  //---------------------
  static CDistribution* Create(DataDistParams& distparams);

  //---------------------
  // Public destructor
  //---------------------
  virtual ~CBernoulli();

  //---------------------
  // Public Functions
  //---------------------
  void ComputeWorkingResponse(const CDataset& kData, const Bag& kBag,
                              const double* kFuncEstimate,
                              std::vector<double>& residuals);

  double Deviance(const CDataset& kData, const Bag& kBag,
                  const double* kFuncEstimate);

  double InitF(const CDataset& kData);

  void FitBestConstant(const CDataset& kData, const Bag& kBag,
                       const double* kFuncEstimate,
                       unsigned long num_terminalnodes,
                       std::vector<double>& residuals, CCARTTree& tree);

  double BagImprovement(const CDataset& kData, const Bag& kBag,
                        const double* kFuncEstimate, const double kShrinkage,
                        const std::vector<double>& kDeltaEstimate);

 private:
  //----------------------
  // Private Constructors
  //----------------------
  CBernoulli(const parallel_details& parallel);

  //-------------------
  // Private Variables
  //-------------------
  bool terminalnode_capped_;
  double terminalnode_cap_level_;
};

#endif  // BERNOULLI_H
