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

#ifndef __bernoulli_h__
#define __bernoulli_h__

//------------------------------
// Includes
//------------------------------

#include "distribution.h"
#include "buildinfo.h"
#include <memory>

//------------------------------
// Class definition
//------------------------------
class CBernoulli : public CDistribution
{

public:
	//---------------------
	// Factory Function
	//---------------------
	static CDistribution* Create(DataDistParams& distParams);

	//---------------------
	// Public destructor
	//---------------------
    virtual ~CBernoulli();

    //---------------------
    // Public Functions
    //---------------------
    void ComputeWorkingResponse(const CDataset& data,
    		const double *adF,
				double *adZ);

    double Deviance(const CDataset& data,
    				const double *adF,
                    bool isValidationSet=false);

    double InitF(const CDataset& data);

    void FitBestConstant(const CDataset& data,
    		const double *adF,
			 unsigned long cTermNodes,
				double* adZ, CTreeComps& treeComps);
    
    double BagImprovement(const CDataset& data,
    					  const double *adF,
    					  const bag& afInBag,
                          const double shrinkage, const double* adFadj);

private:
    //----------------------
    // Private Constructors
    //----------------------
    CBernoulli();

    //-------------------
    // Private Variables
    //-------------------
    bool fCappedPred;
};

#endif // BERNOULLI_H



