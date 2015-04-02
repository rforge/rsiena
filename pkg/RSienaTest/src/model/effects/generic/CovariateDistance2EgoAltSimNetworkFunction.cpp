/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: CovariateDistance2EgoAltSimNetworkFunction.cpp
 *
 * Description: This file contains the implementation of the class
 * CovariateDistance2EgoAltSimNetworkFunction.
 *****************************************************************************/
#include <R_ext/Print.h>
#include "CovariateDistance2EgoAltSimNetworkFunction.h"
#include "CovariateNetworkAlterFunction.h"
#include "network/Network.h"

namespace siena
{

/**
 * Constructor.
 * @param[in] networkName the name of the network variable this function is
 * associated with
 * @param[in] covariateName the name of the covariate this function is
 * associated with
 * @param[in] parameter the value of the internal effect parameter this
 * function is associated with
 * @param[in] excludeMissing: whether to exclude missing values
 */
CovariateDistance2EgoAltSimNetworkFunction::
CovariateDistance2EgoAltSimNetworkFunction(string networkName, string
	covariateName, bool excludeMissing, bool incoming) :
	CovariateDistance2NetworkFunction(networkName, covariateName)
{
	this->lexcludeMissing = excludeMissing;
	this->lincoming = incoming;
}


/**
 * Returns the value of this function for the given alter. It is assumed
 * that the function has been initialized before and pre-processed with
 * respect to a certain ego.
 */
double CovariateDistance2EgoAltSimNetworkFunction::value(int alter)
{
	double value = 0;
	if (lincoming)
	{
		if (!(this->lexcludeMissing && this->missingInDummy(alter)))
		{
			value = this->varInAvSimilarityNetwork(this->ego(), alter);
		}
	}
	else
	{
		if (!(this->lexcludeMissing && this->missingDummy(alter)))
		{
			value = this->varOutAvSimilarityNetwork(this->ego(), alter);
		}
	}
	return value;
}
}
