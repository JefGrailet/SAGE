/*
 * MergingCandidate.cpp
 *
 *  Created on: Jul 4, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in MergingCandidate.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "MergingCandidate.h"

MergingCandidate::MergingCandidate()
{
    candidate = NULL;
    compatibility = UNMERGEABLE;
    
    nbPivots = 0;
    nbContrapivots = 0;
    nbOutliers = 0;
}

MergingCandidate::MergingCandidate(Subnet *candidate, unsigned short compatibility)
{
    this->candidate = candidate;
    this->compatibility = compatibility;
    
    /*
     * Counts the interfaces by type to ease the decision process in SubnetPostProcessor; 
     * compatibility type matters here, e.g. compatibility through the contra-pivots means we 
     * should count contra-pivots as pivots and all other interfaces of the candidate as outliers.
     */
    
    vector<unsigned short> interfaces = candidate->countAllInterfaces();
    if(compatibility == COMPATIBLE_CONTRAPIVOT)
    {
        nbPivots = interfaces[1];
        nbContrapivots = 0;
        nbOutliers = interfaces[0] + interfaces[2];
    }
    else if(compatibility == COMPATIBLE_OUTLIER)
    {
        /*
         * N.B.: by design (i.e., see testCandidate() in SubnetPostProcessor), a subnet fitting 
         * this scenario only features pivots and outliers. Thus, pivots become contra-pivots, as 
         * outliers are at the very least at the same TTL distance as pivots (again, by design).
         */
    
        nbPivots = interfaces[2];
        nbContrapivots = interfaces[0];
        nbOutliers = 0;
    }
    else if(compatibility == OUTLIERS)
    {
        nbPivots = 0;
        nbContrapivots = 0;
        nbOutliers = interfaces[0]; // In this scenario, only "pivots" exist on this subnet
    }
    // Remaining case is equivalent to the initial pivot being compatible.
    else
    {
        nbPivots = interfaces[0];
        nbContrapivots = interfaces[1];
        nbOutliers = interfaces[2];
    }
}

MergingCandidate::MergingCandidate(const MergingCandidate &other)
{
    *this = other;
}

MergingCandidate::~MergingCandidate()
{
}

MergingCandidate& MergingCandidate::operator=(const MergingCandidate &other)
{
    this->candidate = other.candidate;
    this->compatibility = other.compatibility;
    
    this->nbPivots = other.nbPivots;
    this->nbContrapivots = other.nbContrapivots;
    this->nbOutliers = other.nbOutliers;
    return *this;
}

unsigned char MergingCandidate::getPivotTTL()
{
    if(compatibility == COMPATIBLE_CONTRAPIVOT)
        return candidate->getPivotTTL(1);
    if(compatibility == COMPATIBLE_OUTLIER)
        return candidate->getPivotTTL(2);
    return candidate->getPivotTTL();
}

string MergingCandidate::toString()
{
    stringstream ss;
    ss << candidate->getCIDR() << " - ";
    if(compatibility == COMPATIBLE_PIVOT)
        ss << "Compatible by pivot";
    else if(compatibility == COMPATIBLE_CONTRAPIVOT)
        ss << "Compatible by contra-pivot";
    else if(compatibility == COMPATIBLE_OUTLIER)
        ss << "Compatible by outlier";
    else
        ss << "Potential outlier(s)";
    ss << " - ";
    ss << nbPivots << " / " << nbContrapivots << " / " << nbOutliers;
    return ss.str();
}

bool MergingCandidate::smaller(MergingCandidate &c1, MergingCandidate &c2)
{
    return Subnet::compare(c1.candidate, c2.candidate);
}
