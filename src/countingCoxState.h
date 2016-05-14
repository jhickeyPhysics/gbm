//------------------------------------------------------------------------------
//
//  File:       countingCoxState.h
//
//  Description: counting CoxPH methods
//
//	Author: 	James Hickey
//------------------------------------------------------------------------------

#ifndef __countingCoxState_h__
#define __countingCoxState_h__

//------------------------------
// Includes
//------------------------------
#include "dataset.h"
#include "distribution.h"
#include "genericCoxState.h"
#include <Rcpp.h>


//------------------------------
// Class Definition
//------------------------------
class CountingCoxState: public GenericCoxState
{
public:
	//----------------------
	// Public Constructors
	//----------------------
	CountingCoxState(CCoxPH* coxPhPtr): coxPh(coxPhPtr){};

	//---------------------
	// Public destructor
	//---------------------
	~CountingCoxState(){coxPh = NULL;};

	//---------------------
	// Public Functions
	//---------------------
	void ComputeWorkingResponse
	(
		const CDataset& data,
	    const double *adF,
	    double *adZ
	)
	{
		// Initialize parameters
		std::vector<double> martingaleResid(data.get_trainSize(), 0.0);
		double loglik = LogLikelihoodTiedTimes(data.get_trainSize(), data, adF, &martingaleResid[0], false);

		// Fill up response
		for(long i = 0; i < data.get_trainSize(); i++)
		{
			if(data.GetBagElem(i))
			{
				adZ[i] = martingaleResid[i]; // From chain rule
			}

		}
	}

	void FitBestConstant
	(
		const CDataset& data,
	    const double *adF,
	    unsigned long cTermNodes,
	    double* adZ,
	    CTreeComps& treeComps
	)
	{
		double dF = 0.0;
		double dRiskTot = 0.0;
		unsigned long i = 0;
		unsigned long k = 0;
		unsigned long m = 0;

		double dTemp = 0.0;
		bool fTemp = false;
		unsigned long K = 0;

		vector<double> vecdP;
		vector<double> vecdG;
		vector<unsigned long> veciK2Node(cTermNodes, 0);
		vector<unsigned long> veciNode2K(cTermNodes, 0);

		matrix<double> matH;
		matrix<double> matHinv;

		for(i=0; i<cTermNodes; i++)
		{
			veciNode2K[i] = 0;
			if(treeComps.GetTermNodes()[i]->cN >= treeComps.GetMinNodeObs())
			{
				veciK2Node[K] = i;
				veciNode2K[i] = K;
				K++;
			}
		}

		vecdP.resize(K);

		matH.setactualsize(K-1);
		vecdG.resize(K-1);
		vecdG.assign(K-1,0.0);

		// zero the Hessian
		for(k=0; k<K-1; k++)
		{
			for(m=0; m<K-1; m++)
			{
				matH.setvalue(k,m,0.0);
			}
		}

		// get the gradient & Hessian, Ridgeway (1999) pp. 100-101
		// correction from Ridgeway (1999): fix terminal node K-1 prediction to 0.0
		//      for identifiability
		dRiskTot = 0.0;
		vecdP.assign(K,0.0);
		for(i=0; i<data.get_trainSize(); i++)
		{
			if(data.GetBagElem(i) && (treeComps.GetTermNodes()[treeComps.GetNodeAssign()[i]]->cN >= treeComps.GetMinNodeObs()))
			{
				dF = adF[i] + ((data.offset_ptr()==NULL) ? 0.0 : data.offset_ptr()[i]);
				vecdP[veciNode2K[treeComps.GetNodeAssign()[i]]] += data.weight_ptr()[i]*std::exp(dF);
				dRiskTot += data.weight_ptr()[i]*std::exp(dF);

				if(coxPh->StatusVec()[i]==1.0)
				{
					// compute g and H
					for(k=0; k<K-1; k++)
					{
						vecdG[k] +=
							data.weight_ptr()[i]*((treeComps.GetNodeAssign()[i]==veciK2Node[k]) - vecdP[k]/dRiskTot);

						matH.getvalue(k,k,dTemp,fTemp);
						matH.setvalue(k,k,dTemp -
							data.weight_ptr()[i]*vecdP[k]/dRiskTot*(1-vecdP[k]/dRiskTot));
						for(m=0; m<k; m++)
						{
							matH.getvalue(k,m,dTemp,fTemp);
							dTemp += data.weight_ptr()[i]*vecdP[k]/dRiskTot*vecdP[m]/dRiskTot;
							matH.setvalue(k,m,dTemp);
							matH.setvalue(m,k,dTemp);
						}
					}
				}
			}
		}

		/*
		for(k=0; k<K-1; k++)
		{
			for(m=0; m<K-1; m++)
			{
				matH.getvalue(k,m,dTemp,fTemp);
				Rprintf("%f ",dTemp);
			}
			Rprintf("\n");
		}
		*/

		// one step to get leaf predictions
		matH.invert();

		for(k=0; k<cTermNodes; k++)
		{
			treeComps.GetTermNodes()[k]->dPrediction = 0.0;
		}
		for(m=0; m<K-1; m++)
		{
			for(k=0; k<K-1; k++)
			{
				matH.getvalue(k,m,dTemp,fTemp);
				if(!R_FINITE(dTemp)) // occurs if matH was not invertible
				{
					treeComps.GetTermNodes()[veciK2Node[k]]->dPrediction = 0.0;
					break;
				}
				else
				{
					treeComps.GetTermNodes()[veciK2Node[k]]->dPrediction -= dTemp*vecdG[m];
				}
			  }
		}
		// vecpTermNodes[veciK2Node[K-1]]->dPrediction = 0.0; // already set to 0.0

	}

	double Deviance
	(
		const long cLength,
		const CDataset& data,
	    const double *adF
	)
	{
		// Initialize Parameters
		double loglik = 0.0;
		std::vector<double> martingaleResid(cLength, 0.0);

		// Calculate Deviance
		loglik = LogLikelihoodTiedTimes(cLength, data, adF, &martingaleResid[0]);

		return -loglik;
	}

	double BagImprovement
	(
		const CDataset& data,
		const double *adF,
		const bag& afInBag,
	  const double shrinkage,
	  const double* adFadj
	)
	{
		// Initialize Parameters
		double loglikeNoAdj = 0.0;
		double loglikeWithAdj = 0.0;
		std::vector<double> martingaleResidNoAdj(data.get_trainSize(), 0.0);
		std::vector<double> martingaleResidWithAdj(data.get_trainSize(), 0.0);
		std::vector<double> etaAdj(data.get_trainSize(), 0.0);

		// Fill up the adjusted and shrunk eta
		for(long i = 0; i < data.get_trainSize(); i++)
		{
			if(!data.GetBagElem(i))
			{
				etaAdj[i] = adF[i] + shrinkage * adFadj[i];

			}
			else
			{
				etaAdj[i] = adF[i];
			}
		}

		// Calculate likelihoods - data not in bags
		loglikeNoAdj = LogLikelihoodTiedTimes(data.get_trainSize(), data, adF, &martingaleResidNoAdj[0], false, false);
		loglikeWithAdj = LogLikelihoodTiedTimes(data.get_trainSize(), data, &etaAdj[0], &martingaleResidWithAdj[0], false, false);

		return (loglikeWithAdj - loglikeNoAdj);
	}

private:
	CCoxPH* coxPh;
	double LogLikelihoodTiedTimes(const int n, const CDataset& data, const double* eta,
										  double* resid, bool skipBag=true, bool checkInBag=true)
	{
	    int i, j, k, ksave;
	    int person, p2, indx1, p1;
	    int istrat;
	    double cumhaz, hazard;
	    int nrisk, ndeath;
	    double deathwt, denom, temp, center;
	    double esum, dtime, e_hazard;
	    double loglik, d_denom;
	    int stratastart;    /* the first obs of each stratum */

	    /*
	    **  'person' walks through the the data from 1 to n, p2= sort2[person].
	    **     sort2[0] points to the largest stop time, sort2[1] the next, ...
	    **  'dtime' is a scratch variable holding the time of current interest
	    **  'indx1' walks through the start times.  It will be smaller than
	    **    'person': if person=27 that means that 27 subjects have time2 >=dtime,
	    **    and are thus potential members of the risk set.  If 'indx1' =9,
	    **    that means that 9 subjects have start >=time and thus are NOT part
	    **    of the risk set.  (stop > start for each subject guarrantees that
	    **    the 9 are a subset of the 27). p1 = sort1[indx1]
	    **  Basic algorithm: move 'person' forward, adding the new subject into
	    **    the risk set.  If this is a new, unique death time, take selected
	    **    old obs out of the sums, add in obs tied at this time, then update
	    **    the cumulative hazard. Everything resets at the end of a stratum.
	    **  The sort order is from large time to small, so we encounter a subjects
	    **    ending time first, then their start time.
	    **  The martingale residual for a subject is
	    **     status - (cumhaz at entry - cumhaz at exit)*score
	    */

	    istrat=0;
	    indx1 =0;
	    denom =0;  /* S in the math explanation */
	    cumhaz =0;
	    nrisk =0;   /* number at risk */
	    esum =0;  /*cumulative eta, used for rescaling */
	    center = eta[coxPh->EndTimeIndices()[0]] + data.offset_ptr()[coxPh->EndTimeIndices()[0]];
	    stratastart =0;   /* first strata starts at index 0 */
	    for (person=0; person<n; )
	    {
	    	// Check if bagging is required
			if(skipBag || (data.GetBagElem(person)==checkInBag))
			{
				p2 = coxPh->EndTimeIndices()[person];

				if (coxPh->StatusVec()[p2] ==0)
				{
					/* add the subject to the risk set */
					resid[p2] = exp(eta[p2] + data.offset_ptr()[p2] - center) * cumhaz;
					nrisk++;
					denom  += data.weight_ptr()[p2]* exp(eta[p2] + data.offset_ptr()[p2] - center);
					esum += eta[p2] + data.offset_ptr()[p2];
					person++;
				}
				else
				{
					dtime = data.y_ptr(1)[p2];  /* found a new, unique death time */

					/*
					** Remove those subjects whose start time is to the right
					**  from the risk set, and finish computation of their residual
					*/
					temp = denom;
					for (;  indx1 <person; indx1++)
					{
						if(skipBag || (data.GetBagElem(indx1)==checkInBag))
						{
							p1 = coxPh->StartTimeIndices()[indx1];
							if (data.y_ptr(0)[p1] < dtime) break; /* still in the risk set */

							nrisk--;
							resid[p1] -= cumhaz* exp(eta[p1] + data.offset_ptr()[p1] - center);
							denom  -= data.weight_ptr()[p1] * exp(eta[p1] + data.offset_ptr()[p1] - center);
							esum -= eta[p1] + data.offset_ptr()[p1];
						}

					}
					if (nrisk==0)
					{
						/* everyone was removed!
						** This happens with manufactured start/stop
						**  data sets that have some g(time) as a covariate.
						**  Just like a strata, reset the sums.
						*/
						denom =0;
						esum =0;
					}

					/*
					**        Add up over this death time, for all subjects
					*/
					ndeath =0;   /* total number of deaths at this time point */
					deathwt =0;  /* sum(wt) for the deaths */
					d_denom =0;  /*contribution to denominator for the deaths*/
					for (k=person; k< coxPh->StrataVec()[istrat]; k++)
					{
						if(skipBag || (data.GetBagElem(k)==checkInBag))
						{
							p2 = coxPh->EndTimeIndices()[k];
							if (data.y_ptr(1)[p2]  < dtime) break;  /* only tied times */

							nrisk++;
							denom += data.weight_ptr()[p2] * exp(eta[p2] + data.offset_ptr()[p2] - center);
							esum += eta[p2];
							if (coxPh->StatusVec()[p2] ==1)
							{
								ndeath ++;
								deathwt += data.weight_ptr()[p2];
								d_denom += data.weight_ptr()[p2] * exp(eta[p2] + data.offset_ptr()[p2] - center);
								loglik  += data.weight_ptr()[p2] *(eta[p2] + data.offset_ptr()[p2] - center);
							}
						}

					}
					ksave = k;

					/* compute the increment in hazard
					** hazard = usual increment
					** e_hazard = efron increment, for tied deaths only
					*/
					if (coxPh->TieApproxMethod()==0 || ndeath==1)
					{ /* Breslow */
						loglik -= deathwt*log(denom);
						hazard = deathwt /denom;
						e_hazard = hazard;
					}
					else
					{ /* Efron */
						hazard =0;
						e_hazard =0;  /* hazard experienced by a tied death */
						deathwt /= ndeath;   /* average weight of each death */
						for (k=0; k <ndeath; k++)
						{

							temp = (double)k /ndeath;    /* don't do integer division*/
							loglik -= deathwt *log(denom - temp*d_denom);
							hazard += deathwt/(denom - temp*d_denom);
							e_hazard += (1-temp) *deathwt/(denom - temp*d_denom);
						}
					}

					/* Give initial value to all intervals ending at this time
					** If tied censors are sorted before deaths (which at least some
					**  callers of this routine do), then the else below will never
					**  occur.
					*/
					temp = cumhaz + (hazard -e_hazard);
					for (; person < ksave; person++)
					{
						if(skipBag || (data.GetBagElem(person)==checkInBag))
						{
							p2 = coxPh->EndTimeIndices()[person];
							if (coxPh->StatusVec()[p2] ==1) resid[p2] = 1 + temp*exp(eta[p2] + data.offset_ptr()[p2] - center);
							else resid[p2] = cumhaz * exp(eta[p2] + data.offset_ptr()[p2] - center);
						}

					}
					cumhaz += hazard;

					/* see if we need to shift the centering (very rare case) */
					if (fabs(esum/nrisk - center) > recenter)
					{
						temp = esum/nrisk - center;
						center += temp;
						denom /=  exp(temp);
					}
				}

				/* clean up at the end of a strata */
				if (person == coxPh->StrataVec()[istrat])
				{
					stratastart = person;
					for (; indx1< coxPh->StrataVec()[istrat]; indx1++)
					{
						if(skipBag || (data.GetBagElem(indx1)==checkInBag))
						{
							p1 = coxPh->StartTimeIndices()[indx1];
							resid[p1] -= cumhaz * exp(eta[p1] + data.offset_ptr()[p1] - center);
						}

					}
					cumhaz =0;
					denom = 0;
					istrat++;
				}
			}
	    	else
			{
				// Increment person if not in bag
				person++;
			}
	    }
	    return(loglik);
	}
};
#endif //__countingCoxState_h__
