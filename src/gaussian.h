//------------------------------------------------------------------------------
//
//  File:       gaussian.h
//
//  Contents:   gaussian distribution for GBM.
//
//  History:    3/26/2001   gregr created
//              2/14/2003   gregr: adapted for R implementation
//------------------------------------------------------------------------------

#ifndef __gaussian_h__
#define __gaussian_h__

//------------------------------
// Includes
//------------------------------
#include "distribution.h"
#include <memory>

//------------------------------
// Class definition
//------------------------------
class CGaussian : public CDistribution
{

public:
	//---------------------
	// Factory Function
	//---------------------
	static CDistribution* Create(DataDistParams& distParams);

	//---------------------
	// Public destructor
	//---------------------
    virtual ~CGaussian();

    //---------------------
    // Public Functions
    //---------------------
    void ComputeWorkingResponse(const CDataset& data,
    			const double *adF,
				double *adZ);

    double InitF(const CDataset& data);
    
    void FitBestConstant(const CDataset& data,
    				const double *adF,
					 unsigned long cTermNodes,
					 double* adZ,
					 CTreeComps& treeComps);

    double Deviance(const CDataset& data,
    				const double *adF,
                    bool isValidationSet=false);

    double BagImprovement(const CDataset& data,
			  const double *adF,
			  const double shrinkage,
                          const double* adFadj);
private:
    //----------------------
    // Private Constructors
    //----------------------
    CGaussian();
};

#endif // __gaussian_h__



