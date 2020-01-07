/*
 * SubnetPostProcessor.cpp
 *
 *  Created on: Jul 3, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetPostProcessor.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "SubnetPostProcessor.h"
#include "SubnetInferenceRules.h"

MergingCandidate SubnetPostProcessor::NOT_MERGEABLE = MergingCandidate();

SubnetPostProcessor::SubnetPostProcessor(Environment &e): env(e)
{
}

SubnetPostProcessor::~SubnetPostProcessor()
{
}

bool SubnetPostProcessor::compatiblePivots(IPTableEntry **refPivot, IPTableEntry *cmpPivot)
{
    AliasSet *aliases = env.getLattestAliases(); // For SubnetInferenceRules::rule5()
    IPTableEntry *betterPivot = NULL;
    bool mergeOK = false;
    if(SubnetInferenceRules::rule1((*refPivot), cmpPivot)) // Same trail
        mergeOK = true;
    else if(SubnetInferenceRules::rule2((*refPivot), cmpPivot, &betterPivot)) // Same TTL, but one got timeout
        mergeOK = true;
    else if(SubnetInferenceRules::rule3((*refPivot), cmpPivot)) // Echoing IPs
        mergeOK = true;
    else if(SubnetInferenceRules::rule5((*refPivot), cmpPivot, aliases)) // Aliased flickering/warping IPs
        mergeOK = true;
    
    if(!mergeOK)
        return false;
    
    if(betterPivot != NULL)
        (*refPivot) = betterPivot; // Updates reference pivot if rule2() detected a better one
    return true;
}

bool SubnetPostProcessor::compatiblePivots(list<IPTableEntry*> pivots)
{
    if(pivots.size() <= 1)
        return true;
    
    IPTableEntry *refPivot = pivots.front();
    for(list<IPTableEntry*>::iterator i = pivots.begin(); i != pivots.end(); ++i)
    {
        if(i == pivots.begin())
            continue;
        if(!this->compatiblePivots(&refPivot, (*i)))
            return false;
    }
    return true;
}

MergingCandidate SubnetPostProcessor::testCandidate(IPTableEntry **refPivot, Subnet *candidate)
{
    // Same pivot and pivots are compatible (ideal scenario)
    IPTableEntry *candiPivot = candidate->getSelectedPivot();
    if(this->compatiblePivots(refPivot, candiPivot))
        return MergingCandidate(candidate, MergingCandidate::COMPATIBLE_PIVOT);
    
    // Does the subnet only have pivots ? Will appear as outliers (need to consider whole scenario)
    if(candidate->hasOnlyPivots())
        return MergingCandidate(candidate, MergingCandidate::OUTLIERS);
    
    // Does the subnet have contra-pivots ? If yes, are they compatible with the reference pivot ?
    if(candidate->hasContrapivots())
    {
        list<IPTableEntry*> contrapivots = candidate->getContrapivots();
        if(this->compatiblePivots(contrapivots))
        {
            if(this->compatiblePivots(refPivot, contrapivots.front()))
                return MergingCandidate(candidate, MergingCandidate::COMPATIBLE_CONTRAPIVOT);
            return NOT_MERGEABLE;
        }
        return NOT_MERGEABLE;
    }
    
    // What about outliers ?
    if(!candidate->hasOutliers())
        return NOT_MERGEABLE;
    
    list<IPTableEntry*> outliers = candidate->getOutliers();
    if(this->compatiblePivots(outliers))
    {
        if(this->compatiblePivots(refPivot, outliers.front()))
            return MergingCandidate(candidate, MergingCandidate::COMPATIBLE_OUTLIER);
        return NOT_MERGEABLE;
    }
    return NOT_MERGEABLE;
}

void SubnetPostProcessor::relabel(MergingCandidate &cand)
{
    // Checks if a relabel is indeed needed
    unsigned short c = cand.compatibility;
    if(c == MergingCandidate::OUTLIERS)
    {
        // Simplest case: all IPs become outliers of the post-processed subnet
        list<SubnetInterface> *IPs = cand.candidate->getInterfacesList();
        for(list<SubnetInterface>::iterator i = IPs->begin(); i != IPs->end(); ++i)
            i->status = SubnetInterface::OUTLIER;
        return;
    }
    if(c != MergingCandidate::COMPATIBLE_OUTLIER && c != MergingCandidate::COMPATIBLE_CONTRAPIVOT)
        return;
    
    unsigned short typeToSwitch = SubnetInterface::OUTLIER;
    if(c != MergingCandidate::COMPATIBLE_OUTLIER)
        typeToSwitch = SubnetInterface::CONTRAPIVOT;
    
    // Processes all interfaces of the merging subnet to re-label interfaces correctly
    AliasSet *aliases = env.getLattestAliases(); // For SubnetInferenceRules::rule5()
    IPTableEntry *refPivot = NULL;
    list<SubnetInterface> *IPs = cand.candidate->getInterfacesList();
    for(list<SubnetInterface>::iterator i = IPs->begin(); i != IPs->end(); ++i)
    {
        SubnetInterface &cur = (*i);
        if(cur.status != typeToSwitch)
        {
            if(typeToSwitch == SubnetInterface::OUTLIER)
                cur.status = SubnetInterface::CONTRAPIVOT;
            else
                cur.status = SubnetInterface::OUTLIER;
            continue;
        }
        
        if(refPivot == NULL)
        {
            cur.status = SubnetInterface::SELECTED_PIVOT;
            refPivot = cur.ip;
            continue;
        }
        
        unsigned short rule = SubnetInterface::OUTLIER;
        IPTableEntry *betterPivot = NULL;
        if(SubnetInferenceRules::rule1(refPivot, cur.ip)) // Same trail
            rule = SubnetInterface::RULE_1_TRAIL;
        else if(SubnetInferenceRules::rule2(refPivot, cur.ip, &betterPivot)) // Same TTL, but one got timeout
            rule = SubnetInterface::RULE_2_TTL_TIMEOUT;
        else if(SubnetInferenceRules::rule3(refPivot, cur.ip)) // Echoing IPs
            rule = SubnetInterface::RULE_3_ECHOES;
        else if(SubnetInferenceRules::rule4(refPivot, cur.ip, aliases)) // Aliased flickering IPs
            rule = SubnetInterface::RULE_4_FLICKERING;
        else if(SubnetInferenceRules::rule5(refPivot, cur.ip, aliases)) // Aliased flickering/warping IPs
            rule = SubnetInterface::RULE_5_ALIAS;
        
        if(betterPivot != NULL)
            refPivot = betterPivot;
        cur.status = rule;
    }
}

void SubnetPostProcessor::process()
{
    list<Subnet*> processedSubnets; // Already processed subnets
    list<Subnet*> *subnets = env.getSubnets();
    unsigned short outliersRatioDivisor = env.getOutliersRatioDivisor();
    
    while(subnets->size() > 0)
    {
        Subnet *curSub = subnets->front();
        subnets->pop_front();
        
        // Doesn't seem to require post-processing: moves on
        if(!curSub->needsPostProcessing())
        {
            processedSubnets.push_back(curSub);
            continue;
        }
        
        vector<unsigned short> curAmounts = curSub->countAllInterfaces();
        IPTableEntry *refPivot = curSub->getSelectedPivot();
        
        // Initializes variables for growing the hypothetical merged subnet
        InetAddress fusionLowBorder = curSub->getLowerBorder();
        InetAddress fusionUpBorder = curSub->getUpperBorder();
        unsigned short fusionPrefixLength = curSub->getPrefixLength();
        
        // Decreases prefix length of the subnet considered for post-processing
        bool encompassesContrapivots = false;
        list<MergingCandidate> mergeable; // Currently OK candidates (i.e., will be merged)
        while(fusionPrefixLength > SubnetInferenceRules::MINIMUM_PREFIX_LENGTH)
        {
            // Gets new borders of the subnet and lists overlapped candidate subnet(s) (if any)
            list<Subnet*> newCandidates;
            InetAddress oldLowBorder = fusionLowBorder, oldUpBorder = fusionUpBorder;
            fusionPrefixLength--;
    
            // New low border
            unsigned long mask = ~0;
	        mask = mask >> (32 - fusionPrefixLength);
	        mask = mask << (32 - fusionPrefixLength);
            fusionLowBorder = InetAddress(fusionLowBorder.getULongAddress() & mask);
            
            // New up border
            mask = ~0;
	        mask = mask >> fusionPrefixLength;
	        fusionUpBorder = InetAddress(fusionLowBorder.getULongAddress() ^ mask);
            
            // Expansion made the subnet "expand to the left" of the IP scope
            if(oldLowBorder > fusionLowBorder)
            {
                // Lists subnets that are overlapped in processedSubnets
                for(list<Subnet*>::reverse_iterator i = processedSubnets.rbegin(); i != processedSubnets.rend(); ++i)
                {
                    Subnet *cur = (*i);
                    if(cur->getLowerBorder() >= fusionLowBorder)
                        newCandidates.push_back(cur);
                    else
                        break;
                }
            }
            // Otherwise, it "expanded to the right" of the scope
            else
            {
                // Lists subnets that are overlapped in *subnets
                for(list<Subnet*>::iterator i = subnets->begin(); i != subnets->end(); ++i)
                {
                    Subnet *cur = (*i);
                    if(cur->getUpperBorder() <= fusionUpBorder)
                        newCandidates.push_back(cur);
                    else
                        break;
                }
            }
            
            /*
             * Two remarks on the code above:
             * -candidates are removed from the lists only after a successful expansion, therefore 
             *  there's no need to "skip" anything upon listing new candidates during the next 
             *  expansion. This also avoid putting them back in case of a failed expansion.
             * -there might be a way to simplify the code above, but out of laziness I didn't.
             */
            
            // No new candidates: skip to next prefix length
            if(newCandidates.size() == 0)
                continue;
            
            // Ensures we encompass at most one subnet with contra-pivot(s) (i.e. initially sound)
            bool breakOut = false;
            for(list<Subnet*>::iterator i = newCandidates.begin(); i != newCandidates.end(); ++i)
            {
                if((*i)->hasContrapivots())
                {
                    if(encompassesContrapivots)
                    {
                        breakOut = true;
                        break;
                    }
                    else
                        encompassesContrapivots = true;
                }
            }
            
            if(breakOut)
                break;
            
            // Processes the subnets in newCandidates
            list<MergingCandidate> candis;
            for(list<Subnet*>::iterator i = newCandidates.begin(); i != newCandidates.end(); ++i)
            {
                MergingCandidate candi = this->testCandidate(&refPivot, (*i));
                if(candi.isUnmergeable())
                    break;
                candis.push_back(candi);
            }
            
            // At least one subnet doesn't fit at all: quits expansion
            if(candis.size() != newCandidates.size())
            {
                candis.clear();
                break;
            }
            
            // Full list of merging candidates to evaluate the situation as a whole
            list<MergingCandidate> fullList;
            for(list<MergingCandidate>::iterator i = candis.begin(); i != candis.end(); ++i)
                fullList.push_back((*i));
            for(list<MergingCandidate>::iterator i = mergeable.begin(); i != mergeable.end(); ++i)
                fullList.push_back((*i));
            
            // Goes through the full list of candidates
            bool doublyPostProcessed = false;
            unsigned short nbCompatibleSubnets = 0, totalPivots = curAmounts[0];
            unsigned short totalContrapivots = curAmounts[1];
            unsigned short totalIPs = curAmounts[0] + curAmounts[1] + curAmounts[2];
            for(list<MergingCandidate>::iterator i = fullList.begin(); i != fullList.end(); ++i)
            {
                MergingCandidate candi = (*i);
                if(candi.candidate->isPostProcessed())
                {
                    doublyPostProcessed = true;
                    break;
                }
                
                if(candi.isCompatible())
                    nbCompatibleSubnets++;
                
                totalPivots += candi.nbPivots;
                totalContrapivots += candi.nbContrapivots;
                totalIPs += candi.nbPivots + candi.nbContrapivots + candi.nbOutliers;
            }
            
            /*
             * Stops if:
             * -we detect potential double post-processing, 
             * -there are too many potential contra-pivots in this scenario, 
             * -there is no subnet with compatible pivots, only outliers.
             * These various conditions are meant to minimize ill cases of merging as much as 
             * possible.
             */
            
            bool badScenario = false;
            if(doublyPostProcessed)
                badScenario = true;
            else if(totalContrapivots > SubnetInferenceRules::MAXIMUM_NB_CONTRAPIVOTS)
                badScenario = true;
            else if(nbCompatibleSubnets == 0)
                badScenario = true;
            
            if(badScenario)
            {
                candis.clear();
                break;
            }
            
            // Stops if too few pivots overall (depends on prefix length)
            double ratioPivots = (double) totalPivots / (double) totalIPs;
            double idealRatio = (double) (outliersRatioDivisor-1) / (double) outliersRatioDivisor;
            if((fusionPrefixLength < 29 && ratioPivots < idealRatio) || ratioPivots < 0.5)
            {
                candis.clear();
                break;
            }
            
            // Final check: if pivots all have the same TTL, outliers shouldn't be located sooner.
            unsigned char outlierTTL = 255, pivotTTL = curSub->getPivotTTL();
            if(pivotTTL != 255)
            {
                for(list<MergingCandidate>::iterator i = candis.begin(); i != candis.end(); ++i)
                {
                    MergingCandidate &cur = (*i);
                    if(!cur.isCompatible())
                    {
                        unsigned char minTTL = cur.getSmallestTTL();
                        if(minTTL < outlierTTL)
                            outlierTTL = minTTL;
                    }
                    else
                    {
                        unsigned char curTTL = cur.getPivotTTL();
                        if(curTTL == 255 || curTTL != pivotTTL)
                        {
                            pivotTTL = 255; // Means varying TTLs for pivots
                            break;
                        }
                    }
                }
                
                // Outliers are sooner than pivots: that's unlikely.
                if(pivotTTL != 255 && outlierTTL < pivotTTL)
                {
                    candis.clear();
                    break;
                }
            }
            
            /*
             * N.B.: code doesn't check if said outliers are contra-pivots, because the inference 
             * would normally have discovered them earlier.
             */
            
            /*
             * At this point in the code, merging with this prefix length is OK. The 
             * post-processing will evaluate smaller prefix lengths (if bigger than 20) to see 
             * if the expansion can go further, but before that, the code will:
             * -reset the encompassesContrapivots flag to false if a "COMPATIBLE_CONTRAPIVOT" 
             *  subnet was among the candidates (by design it's the only subnet with 
             *  contra-pivot(s) up to now) since said contra-pivot(s) are actually pivots, 
             * -move all MergingCandidate objects from "candis" to "mergeable",
             * -pop subnets turned into MergingCandidate objects from the initial lists.
             */
            
            for(list<MergingCandidate>::iterator i = candis.begin(); i != candis.end(); ++i)
            {
                if(i->compatibility == MergingCandidate::COMPATIBLE_CONTRAPIVOT)
                {
                    encompassesContrapivots = false;
                    break;
                }
            }
            
            while(candis.size() > 0)
            {
                mergeable.push_back(candis.front());
                candis.pop_front();
            }
            
            if(newCandidates.size() > 0)
            {
                for(unsigned short i = 0; i < newCandidates.size(); i++)
                {
                    if(oldLowBorder > fusionLowBorder)
                        processedSubnets.pop_back();
                    else
                        subnets->pop_front();
                }
            }
        }
        
        // No possible merging here: moves on
        if(mergeable.size() == 0)
        {
            processedSubnets.push_back(curSub);
            continue;
        }
        
        /*
         * At this point, a merging scenario is envisioned. The code turns the subnets listed in 
         * "mergeable" into a regular list of subnets, and performs at the same time the "pivot 
         * switch" on MergingCandidate where only contra-pivot(s) or outliers were evaluated as 
         * compatible with the reference pivot and re-label IPs of outlying candidates as outliers.
         */
        
        list<Subnet*> toMerge;
        toMerge.push_back(curSub);
        while(mergeable.size() > 0)
        {
            MergingCandidate curCand = mergeable.front();
            mergeable.pop_front();
            
            this->relabel(curCand); // Will do nothing if no relabeling work is needed
            toMerge.push_back(curCand.candidate);
        }
        
        processedSubnets.push_back(new Subnet(toMerge)); // Will also delete listed Subnet objects
    }
    
    /*
     * Re-copies the final subnets, performing two last minor post-processings:
     * -subnet sizes are adjusted to the smallest prefix length encompassing all their IPs. This 
     *  meant to avoid obviously overgrown subnets. For instance, an isolated IP (or some isolated 
     *  IPs) can see its (their) encompassing subnet growing up to /20 because there are no other 
     *  IPs to check around. Keeping a /20 in the data is very optimistic; therefore the subnet 
     *  size is adjusted at the very end to keep a result faithful to the observations.
     * -subnets which contain pivot IPs appearing at varying distances and feature no contra-pivot 
     *  IP(s) are spotted to re-label outliers as contra-pivots (alternative definition) if and 
     *  only if such outliers are a minority (ideally, only one).
     */
    
    while(processedSubnets.size() > 0)
    {
        Subnet *curSub = processedSubnets.front();
        processedSubnets.pop_front();
        curSub->findAdjustedPrefix();
        curSub->findAlternativeContrapivot(SubnetInferenceRules::MAXIMUM_NB_CONTRAPIVOTS);
        subnets->push_back(curSub);
    }
}
