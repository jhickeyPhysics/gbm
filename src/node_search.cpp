//------------------------------------------------------------------------------
//  GBM by Greg Ridgeway  Copyright (C) 2003
//
//  File:       node_search.cpp
//
//------------------------------------------------------------------------------
//-----------------------------------
// Includes
//-----------------------------------
#include "node_search.h"

//----------------------------------------
// Function Members - Public
//----------------------------------------
CNodeSearch::CNodeSearch(int treeDepth, int numColData, unsigned long minObs):
variableSplitters(2*treeDepth+1, VarSplitter(minObs))//variableSplitters(numColData, VarSplitter(minObs))
{
    cTerminalNodes = 1;
    minNumObs = minObs;
    totalCache = 0;

}


CNodeSearch::~CNodeSearch()
{
}

void CNodeSearch::GenerateAllSplits
(
		vector<CNode*>& vecpTermNodes,
		const CDataset& data,
		double* adZ,
		vector<unsigned long>& aiNodeAssign
)
{
	unsigned long iWhichObs = 0;
	const CDataset::index_vector colNumbers(data.random_order());
	const CDataset::index_vector::const_iterator final = colNumbers.begin() + data.get_numFeatures();

	for(long iNode = 0; iNode < cTerminalNodes; iNode++)
	{
		// If node has split then skip
		if(vecpTermNodes[iNode]->splitAssigned) continue;
		variableSplitters[iNode].Set(*vecpTermNodes[iNode]);
		/*variableSplitters[iNode].Reset();
		variableSplitters[iNode].SetForNode(*vecpTermNodes[iNode]);*/

	}

	/*// Loop over all variables
	for(CDataset::index_vector::const_iterator it=colNumbers.begin();
			it != final;
			it++)
	{

		for(long iNode =0; iNode < cTerminalNodes; iNode++)
		{
			if(vecpTermNodes[iNode]->splitAssigned)
			{

				continue;
			}
			variableSplitters[iNode].SetForVariable(*it, data.varclass(*it));
		}

		// Get observation and add to split if needed
		for(long iOrderObs = 0; iOrderObs < data.get_trainSize(); iOrderObs++)
		{
			iWhichObs = data.order_ptr()[(*it)*data.get_trainSize() + iOrderObs];
			const int iNode = aiNodeAssign[iWhichObs];

			// Check if node already has split cached or not
			if(data.GetBag()[iWhichObs])
			{
				const double dX = data.x_value(iWhichObs, *it);
				if(vecpTermNodes[iNode]->splitAssigned) continue;
				variableSplitters[iNode].IncorporateObs(dX,
						adZ[iWhichObs],
						data.weight_ptr()[iWhichObs],
						data.monotone(*it));
			}
		}

		for(long iNode = 0; iNode < cTerminalNodes; iNode++)
		{
			if(vecpTermNodes[iNode]->splitAssigned) continue;
			if(data.varclass(*it) != 0) // evaluate if categorical split
			{
			  variableSplitters[iNode].EvaluateCategoricalSplit();
			}
		}

	}*/

	  for(CDataset::index_vector::const_iterator it=colNumbers.begin();
	      it != final;
	      it++)
	    {
	      const int iVar = *it;
	      const int cVarClasses = data.varclass(iVar);

	      for(long iNode=0; iNode < cTerminalNodes; iNode++)
	        {
		  variableSplitters[iNode].ResetForNewVar(iVar, cVarClasses);
	        }

	      // distribute the observations in order to the correct node search
	      for(long iOrderObs=0; iOrderObs < data.get_trainSize(); iOrderObs++)
	        {
		  iWhichObs = data.order_ptr()[iVar*data.get_trainSize() + iOrderObs];
		  if(vecpTermNodes[aiNodeAssign[iWhichObs]]->splitAssigned) continue;
		  if(data.GetBagElem(iWhichObs))
	            {
		      const int iNode = aiNodeAssign[iWhichObs];
		      const double dX = data.x_value(iWhichObs, iVar);
		      variableSplitters[iNode].IncorporateObs(dX,
							adZ[iWhichObs],
							data.weight_ptr()[iWhichObs],
							data.monotone(iVar));
	            }
	        }
	        for(long iNode=0; iNode<cTerminalNodes; iNode++)
	        {
	        	if(vecpTermNodes[iNode]->splitAssigned) continue;
	            if(cVarClasses != 0) // evaluate if categorical split
	            {
		      variableSplitters[iNode].EvaluateCategoricalSplit();
	            }
	            variableSplitters[iNode].WrapUpCurrentVariable();
	        }
	    }

	   /* // search for the best split
	    iBestNode = 0;
	    dBestNodeImprovement = 0.0;
	    for(iNode=0; iNode<cTerminalNodes; iNode++)
	    {
	        aNodeSearch[iNode].SetToSplit();
	        if(aNodeSearch[iNode].BestImprovement() > dBestNodeImprovement)
	        {
	            iBestNode = iNode;
	            dBestNodeImprovement = aNodeSearch[iNode].BestImprovement();
	        }
	    }*/

}


double CNodeSearch::CalcImprovement
(
	long &iBestNode,
	vector<CNode*>& vecpTermNodes
)
{
	// search for the best split
	/*long iBestNode = 0;*/
	double dBestNodeImprovement = 0.0;
	for(long iNode=0; iNode < cTerminalNodes; iNode++)
	{
		//variableSplitters[iNode].SetToSplit();
		vecpTermNodes[iNode]->SplitAssign();
		if(variableSplitters[iNode].BestImprovement() > dBestNodeImprovement)//vecpTermNodes[iNode]->SplitImprovement())
		{
			iBestNode = iNode;
			dBestNodeImprovement = variableSplitters[iNode].BestImprovement();//vecpTermNodes[iNode]->SplitImprovement();
			//vecpTermNodes[iNode]->childrenParams =  variableSplitters[iNode].GetBestSplit();
		}

		//if(vecpTermNodes[iNode]->splitAssigned) continue;
		//vecpTermNodes[iNode]->SplitAssign();

	}
	// Split Node if improvement is non-zero
	/*if(dBestNodeImprovement != 0.0)
	{
		//Split Node
		//vecpTermNodes[iBestNode]->SplitNode();
		variableSplitters[iBestNode].SetupNewNodes(*vecpTermNodes[iBestNode]);
		cTerminalNodes += 2;

		// Move data to children nodes
		ReAssignData(iBestNode, vecpTermNodes, data, aiNodeAssign);

		// Add children to terminal node list
		vecpTermNodes[cTerminalNodes-2] = vecpTermNodes[iBestNode]->pRightNode;
		vecpTermNodes[cTerminalNodes-1] = vecpTermNodes[iBestNode]->pMissingNode;
		vecpTermNodes[iBestNode] = vecpTermNodes[iBestNode]->pLeftNode;

	}*/

	return dBestNodeImprovement;
}

void CNodeSearch::Split(long& iBestNode, vector<CNode*>& vecpTermNodes, const CDataset& data,
		vector<unsigned long>& aiNodeAssign)
{
	// Split Node if improvement is non-zero
		//Split Node
		//vecpTermNodes[iBestNode]->SplitNode();
		variableSplitters[iBestNode].SetupNewNodes(*vecpTermNodes[iBestNode]);
		cTerminalNodes += 2;

		// Move data to children nodes
		ReAssignData(iBestNode, vecpTermNodes, data, aiNodeAssign);

		// Add children to terminal node list
		vecpTermNodes[cTerminalNodes-2] = vecpTermNodes[iBestNode]->pRightNode;
		vecpTermNodes[cTerminalNodes-1] = vecpTermNodes[iBestNode]->pMissingNode;
		vecpTermNodes[iBestNode] = vecpTermNodes[iBestNode]->pLeftNode;

		/*variableSplitters[cTerminalNodes-2].Set(*vecpTermNodes[cTerminalNodes-2]);
		variableSplitters[cTerminalNodes-1].Set(*vecpTermNodes[cTerminalNodes-1]);
		variableSplitters[iBestNode].Set(*vecpTermNodes[iBestNode]);*/


}

//----------------------------------------
// Function Members - Private
//----------------------------------------
void CNodeSearch::ReAssignData
(
		long splittedNodeIndex,
		vector<CNode*>& vecpTermNodes,
		const CDataset& data,
		vector<unsigned long>& aiNodeAssign
)
{
	// assign observations to the correct node
	for(long iObs=0; iObs < data.get_trainSize(); iObs++)
	{
		if(aiNodeAssign[iObs]==splittedNodeIndex)
		{
		  signed char schWhichNode = vecpTermNodes[splittedNodeIndex]->WhichNode(data,iObs);
		  if(schWhichNode == 1) // goes right
		  {
			  aiNodeAssign[iObs] = cTerminalNodes-2;
		  }
		  else if(schWhichNode == 0) // is missing
		  {
			  aiNodeAssign[iObs] = cTerminalNodes-1;
		  }
		  // those to the left stay with the same node assignment
		  }
	}
}



