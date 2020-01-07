/*
 * SubnetInferrer.cpp
 *
 *  Created on: Nov 23, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetInferrer.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "SubnetInferrer.h"
#include "SubnetInferenceRules.h"

SubnetInferrer::SubnetInferrer(Environment &e): env(e)
{
}

SubnetInferrer::~SubnetInferrer()
{
}

void SubnetInferrer::fullShrinkage(Subnet *sn, 
                                   list<IPTableEntry*> *IPs, 
                                   IPTableEntry *oldPivot, 
                                   list<IPTableEntry*> *contrapivots)
{
    /*
     * In a specific scenario, the "old" pivot, i.e. the pivot interface of the last successful 
     * expansion, might have been alleged to be a contra-pivot after all and is therefore not 
     * listed in the Subnet object anymore and rather in the list of contra-pivots. To undo this, 
     * we simply check if the old pivot indeed appears in the list, and if yes, we remove it and 
     * restore it to the Subnet object as the selected pivot. If the "old" pivot is not part of 
     * the contra-pivots list, it is simply updated in the subnet to be labelled again as the 
     * selected pivot interface.
     */
    
    if(contrapivots != NULL && oldPivot != NULL)
    {
        bool found = false;
        for(list<IPTableEntry*>::iterator i = contrapivots->begin(); i != contrapivots->end(); ++i)
        {
            if((*i) == oldPivot)
            {
                contrapivots->erase(i--);
                sn->addInterface(oldPivot, SubnetInterface::SELECTED_PIVOT);
                found = true;
                break;
            }
        }
        if(!found)
            sn->updateInterface(oldPivot, SubnetInterface::SELECTED_PIVOT);
    }
    else if(oldPivot != NULL)
    {
        sn->updateInterface(oldPivot, SubnetInterface::SELECTED_PIVOT);
    }
    
    // Retrieves and restores IPs to the list of IPs to consider for subnet inference
    list<IPTableEntry*> restore;
    sn->shrink(&restore);
    for(list<IPTableEntry*>::iterator i = restore.begin(); i != restore.end(); ++i)
        IPs->push_back((*i));
    if(contrapivots != NULL)
    {
        for(list<IPTableEntry*>::iterator i = contrapivots->begin(); i != contrapivots->end(); ++i)
            IPs->push_back((*i));
        contrapivots->clear();
    }
    IPs->sort(IPTableEntry::compare);
}

bool SubnetInferrer::overgrowthTest(list<IPTableEntry*> contrapivots,
                                    list<IPTableEntry*> interfaces)
{
    // For the next tests, keep in mind that contrapivots is supposed to be a subset of interfaces
    if(contrapivots.size() <= 1) // Obvious
        return false;
    if(contrapivots.size() == interfaces.size()) // Obvious too
        return false;
    if(contrapivots.size() > interfaces.size()) // Should technically not happen; just in case
        return false;
    
    // Computes smallest offset between contrapivots (see special case at the end of the method)
    contrapivots.sort(IPTableEntry::compare);
    unsigned long minDiff = 65535;
    unsigned long prev = 0;
    for(list<IPTableEntry*>::iterator i = contrapivots.begin(); i != contrapivots.end(); i++)
    {
        if(prev == 0)
        {
            prev = (*i)->getULongAddress();
            continue;
        }
        unsigned long cur = (*i)->getULongAddress();
        unsigned long diff = cur - prev;
        prev = cur;
        if(diff < minDiff)
            minDiff = diff;
    }
    
    // Then, for each contrapivot
    unsigned short peerless = 0;
    for(list<IPTableEntry*>::iterator i = contrapivots.begin(); i != contrapivots.end(); i++)
    {
        IPTableEntry *curCpiv = (*i);
        
        // Grows a prefix around this contra-pivot
        InetAddress low = (InetAddress) (*curCpiv);
        InetAddress up = low;
        unsigned short prefixLen = 32;
        unsigned long size = 1;
        while(prefixLen > 20)
        {
            prefixLen--;
            
            unsigned long uintPrefix = low.getULongAddress();
            size *= 2;
            uintPrefix = uintPrefix >> (32 - prefixLen);
            uintPrefix = uintPrefix << (32 - prefixLen);
            
            InetAddress oldLow = low, oldUp = up;
            low = InetAddress(uintPrefix);
            up = InetAddress(uintPrefix + size);
            
            // Checks if any other contrapivot is encompassed by this prefix
            list<IPTableEntry*>::iterator j;
            bool overlap = false;
            for(j = contrapivots.begin(); j != contrapivots.end(); j++)
            {
                if((*i) == (*j))
                    continue;
                InetAddress curIP = (InetAddress) (*(*j));
                if(curIP >= low && curIP <= up)
                {
                    overlap = true;
                    break;
                }
            }
            
            // Gets old boundaries and gets out of loop
            if(overlap)
            {
                low = oldLow;
                up = oldUp;
                break;
            }
        }
        
        // Removes from interfaces all IPs from [low, up]
        unsigned short nbErased = 0;
        for(list<IPTableEntry*>::iterator j = interfaces.begin(); j != interfaces.end(); j++)
        {
            InetAddress curIP = (InetAddress) (*(*j));
            if(curIP >= low && curIP <= up)
            {
                interfaces.erase(j--);
                if(curIP != (InetAddress) (*curCpiv))
                    nbErased++;
            }
        }
        
        if(nbErased == 0)
            peerless++;
    }
    
    // Remaining interfaces: means we can't break down this subnet in smaller and sound subnets.
    if(interfaces.size() > 0)
        return false;
    
    /*
     * Special case: there are more contrapivots than pivots (+ outliers) in "interfaces" (or as 
     * many) and half the contrapivots are peerless, i.e., no pivots could be removed from 
     * "interfaces" after growing their respective prefix. Assuming there's an overgrowth here is 
     * dubious, because we would then likely end up with several very small and incomplete subnets 
     * (best case would be a subnet where contrapivots are the pivots, in which case the pivot(s) 
     * are outliers), and thus a larger subnet with several contra-pivots is more likely, except 
     * if the "spacing" between contra-pivots is large (then, the smaller subnets sounds like a 
     * better option).
     */
    
    unsigned int nbPivots = interfaces.size() - contrapivots.size();
    if(minDiff < 8 && nbPivots <= contrapivots.size() && peerless >= contrapivots.size() / 2)
        return false;
    
    // Otherwise, it's truly an overgrowth scenario
    return true;
}

void SubnetInferrer::process()
{
    list<IPTableEntry*> IPs = env.getIPDictionary()->listScannedIPs();
    AliasSet *aliases = env.getLattestAliases();
    list<Subnet*> *subnets = env.getSubnets();
    unsigned short outliersRatioDivisor = env.getOutliersRatioDivisor();
    
    while(IPs.size() > 0)
    {
        InetAddress prevLowBorder("255.255.255.255");
        if(subnets->size() > 0)
            prevLowBorder = subnets->front()->getLowerBorder();
        
        IPTableEntry *curPivot = IPs.back();
        IPTableEntry *prevPivot = NULL; // Pivot at the last successful expansion, see below
        Subnet *curSubnet = new Subnet(curPivot);
        IPs.pop_back();
        
        while(curSubnet->getPrefixLength() > SubnetInferenceRules::MINIMUM_PREFIX_LENGTH)
        {
            // If previous subnet creation/expansion emptied the list of IPs, breaks out of loop
            if(IPs.size() == 0)
                break;
            
            // 1) Checks the subnet doesn't overlap the last subnet we inserted
            curSubnet->expand();
            InetAddress newUpBorder = curSubnet->getUpperBorder();
            if(newUpBorder >= prevLowBorder)
            {
                curSubnet->shrink(); // This will be enough, no need for fullShrinkage()
                string stopReason = "next prefix length overlapped previously inferred subnet.";
                curSubnet->setStopDescription(stopReason);
                curSubnet->setToPostProcess(true);
                break;
            }
            
            /*
             * N.B.: in some situations, merging consecutive subnets should be considered. This 
             * operation takes place during post-processing.
             */
            
            // 2) Picks IPs in the full list that are within the new boundaries
            InetAddress newLowBorder = curSubnet->getLowerBorder();
            list<IPTableEntry*> candidates;
            IPTableEntry *candidate = IPs.back();
            InetAddress candiAsIP = (InetAddress) (*candidate);
            while(candiAsIP >= newLowBorder && candiAsIP <= newUpBorder)
            {
                IPs.pop_back();
                candidates.push_back(candidate);
                if(IPs.size() == 0)
                    break;
                
                candidate = IPs.back();
                candiAsIP = (InetAddress) (*candidate);
            }
            list<IPTableEntry*> copyCandis(candidates); // For overgrowth test (July 2019)
            
            /*
             * Some remarks for the next lines of code:
             * -rules 1 to 4 all do the additional verifications that might be needed, such as 
             *  curPivot and curCandi having the same TTL (rules 2 to 4).
             * -there are two additional pointers for IPTableEntry objects modeling the pivot 
             *  interface:
             *  1) betterPivot is used in rule 2 to refresh the pivot if the candidate IP 
             *     featured a trail with less (or zero) anomalies because it will improve the 
             *     accuracy of the result subnet. This pointer stays to NULL if the original pivot 
             *     can be kept.
             *  2) prevPivot (declared previously) is used to keep track of the pivot interface 
             *     used during the last successful expansion. Indeed, if an expansion during which 
             *     a pivot change occurred is unsuccessful and leads to a shrinkage, then the old 
             *     pivot should be restored.
             * -while they're not all essential, outside brackets while checking subnet rules are 
             *  kept for readability.
             */
            
            // 3) Reviews candidate IPs to check if they belong to the subnet
            IPTableEntry *betterPivot = NULL;
            list<IPTableEntry*> contrapivots;
            unsigned short nbNotOnSubnet = 0;
            unsigned short IPsBeforeExpansion = curSubnet->getNbInterfaces();
            while(candidates.size() > 0)
            {
                IPTableEntry *curCandi = candidates.front();
                candidates.pop_front();
                
                if(SubnetInferenceRules::rule1(curPivot, curCandi))
                {
                    curSubnet->addInterface(curCandi, SubnetInterface::RULE_1_TRAIL);
                }
                else if(SubnetInferenceRules::rule2(curPivot, curCandi, &betterPivot))
                {
                    if(betterPivot != NULL)
                    {
                        curSubnet->addInterface(curCandi, SubnetInterface::SELECTED_PIVOT);
                        curSubnet->updateInterface(curPivot, SubnetInterface::RULE_2_TTL_TIMEOUT);
                        curPivot = betterPivot;
                        betterPivot = NULL;
                    }
                    else
                        curSubnet->addInterface(curCandi, SubnetInterface::RULE_2_TTL_TIMEOUT);
                }
                else if(SubnetInferenceRules::rule3(curPivot, curCandi))
                {
                    curSubnet->addInterface(curCandi, SubnetInterface::RULE_3_ECHOES);
                }
                else if(SubnetInferenceRules::rule4(curPivot, curCandi, aliases))
                {
                    curSubnet->addInterface(curCandi, SubnetInterface::RULE_4_FLICKERING);
                }
                else if(curPivot->getTTL() > curCandi->getTTL())
                {
                    if(SubnetInferenceRules::rule5(curPivot, curCandi, aliases))
                        curSubnet->addInterface(curCandi, SubnetInterface::RULE_5_ALIAS);
                    else
                        contrapivots.push_back(curCandi);
                }
                else if(curPivot->getTTL() < curCandi->getTTL())
                {
                    // Tests rule 5 just in case
                    if(SubnetInferenceRules::rule5(curPivot, curCandi, aliases))
                    {
                        curSubnet->addInterface(curCandi, SubnetInterface::RULE_5_ALIAS);
                    }
                    else
                    {
                        // curPivot could very well be a contra-pivot (critical for small subnets)
                        unsigned short nbIPs = curSubnet->getNbInterfaces();
                        if(nbIPs == 1 && (curCandi->getTTL() - curPivot->getTTL()) == 1)
                        {
                            curSubnet->clearInterfaces();
                            curSubnet->addInterface(curCandi, SubnetInterface::SELECTED_PIVOT);
                            contrapivots.push_back(curPivot);
                            curPivot = curCandi;
                        }
                        else
                        {
                            curSubnet->addInterface(curCandi, SubnetInterface::OUTLIER);
                            nbNotOnSubnet++;
                        }
                    }
                }
                else
                {
                    curSubnet->addInterface(curCandi, SubnetInterface::OUTLIER);
                    nbNotOnSubnet++;
                }
            }
            
            // 4) Diagnoses the subnet as a whole (for each shrinkage, a motivation is written)
            unsigned short totalPivots = (unsigned short) curSubnet->getNbInterfaces();
            unsigned short totalNew = totalPivots - IPsBeforeExpansion;
            
            // Potential contra-pivots discovered: subnet stops growing or shrinks
            if(contrapivots.size() > 0)
            {
                unsigned short nbCpivots = (unsigned short) contrapivots.size();
                unsigned short minority = (totalNew + nbCpivots) / outliersRatioDivisor;
                
                // More than a minority of new IPs aren't on the same subnet
                if(totalNew > contrapivots.size() && nbNotOnSubnet > minority)
                {
                    IPTableEntry *oldPivot = (prevPivot != curPivot) ? prevPivot : NULL;
                    fullShrinkage(curSubnet, &IPs, oldPivot, &contrapivots);
                    
                    stringstream ss;
                    ss << "next prefix length ";
                    if(totalNew > nbNotOnSubnet)
                    {
                        ss << "encompassed too many outliers (" << nbNotOnSubnet;
                        ss << " out of " << totalNew << " IPs).";
                    }
                    else
                        ss << "contained no IP appearing to be on the same subnet.";
                    curSubnet->setStopDescription(ss.str());
                    break;
                }
                // Too many contra-pivots or more contra-pivots than pivots
                else if(nbCpivots > 2 && (nbCpivots > SubnetInferenceRules::MAXIMUM_NB_CONTRAPIVOTS || nbCpivots > totalPivots))
                {
                    IPTableEntry *oldPivot = (prevPivot != curPivot) ? prevPivot : NULL;
                    fullShrinkage(curSubnet, &IPs, oldPivot, &contrapivots);
                    
                    stringstream ss;
                    if(nbCpivots > 5)
                    {
                        ss << "next prefix length had too many hypothetical contra-pivot IPs (";
                        ss << nbCpivots << ").";
                    }
                    else
                    {
                        ss << "next prefix length had more contra-pivot IPs (";
                        ss << nbCpivots << ") than pivot IPs.";
                    }
                    curSubnet->setStopDescription(ss.str());
                    break;
                }
                
                /*
                 * (July 2019) On the condition "nbCpivots > 2": we discovered during additional 
                 * validation that it's possible to have a subnet with two contra-pivots and one 
                 * pivot that remains faithful to the topology (can, for instance, occur with a 
                 * critical /30 or a sparse /25). Now, SubnetInferrer considers having too many 
                 * contra-pivots only if it discovers 3 or more of them.
                 */
                
                // Checks whether contra-pivots (if several) have the same TTL or not
                bool sameTTL = true;
                unsigned char refTTL = contrapivots.front()->getTTL();
                list<IPTableEntry*>::iterator i = contrapivots.begin();
                for(++i; i != contrapivots.end(); ++i)
                {
                    if((*i)->getTTL() != refTTL)
                    {
                        sameTTL = false;
                        break;
                    }
                }
                
                if(sameTTL)
                {
                    // Checks if contra-pivots have a similar trail
                    bool similarTrail = true, aliasedTrails = false, overgrowth = false;
                    
                    if(refTTL > 1 && contrapivots.size() > 1)
                    {
                        // Finds a trail with the smallest amount of anomalies
                        IPTableEntry *refForTrail = NULL;
                        unsigned short minAnomalies = 255;
                        list<IPTableEntry*>::iterator i;
                        for(i = contrapivots.begin(); i != contrapivots.end(); ++i)
                        {
                            Trail &curTrail = (*i)->getTrail();
                            if(!curTrail.isVoid() && curTrail.getNbAnomalies() < minAnomalies)
                            {
                                minAnomalies = curTrail.getNbAnomalies();
                                refForTrail = (*i);
                            }
                        }
                        
                        // Compares trails
                        if(refForTrail != NULL) // Just in case
                        {
                            Trail &refTrail = refForTrail->getTrail();
                            for(i = contrapivots.begin(); i != contrapivots.end(); ++i)
                            {
                                Trail &curTrail = (*i)->getTrail();
                                if(curTrail.isVoid())
                                    continue;
                                
                                unsigned short curAnomalies = curTrail.getNbAnomalies();
                                if(curAnomalies == minAnomalies && !(refTrail == curTrail))
                                {
                                    // Re-using SubnetInferenceRules::rule5() to ensure IPs aren't aliases
                                    if(!SubnetInferenceRules::rule5(refForTrail, (*i), aliases))
                                    {
                                        similarTrail = false;
                                        break;
                                    }
                                    else
                                        aliasedTrails = true;
                                }
                            }
                        }
                        
                        /*
                         * (July 2019) If there are several contra-pivots, they must be placed 
                         * such that one can't discover a sub-prefix that would contain only one 
                         * contra-pivot and pivots, as this mean that stopping now will produce an 
                         * overgrown subnet. To prevent this, a special heuristic is being used in 
                         * the form of a private method "overgrowthTest()". See its documentation 
                         * in SubnetInferrer.h to get more details on how it works and how this 
                         * shows there might be an overgrowth.
                         */
                        
                        if(overgrowthTest(contrapivots, copyCandis))
                            overgrowth = true;
                    }
                    
                    if(!overgrowth)
                    {
                        /*
                         * (July 2019) Having differing trails slightly change the message. This could 
                         * be later interpreted as an hint for traffic engineering (+ need for alias 
                         * resolution during graph building ?).
                         */
                        
                        stringstream ss;
                        if(similarTrail)
                        {
                            ss << "discovered ";
                            if(nbCpivots > 1)
                            {
                                ss << nbCpivots << " sound contra-pivot IPs";
                                if(aliasedTrails)
                                    ss << " (previously aliased IPs)";
                                ss << ".";
                            }
                            else
                                ss << "a sound contra-pivot IP.";
                        }
                        else
                        {
                            ss << "discovered " << nbCpivots << " contra-pivot IPs ";
                            ss << "(note: contra-pivot IPs have differing trails).";
                            
                            /*
                             * TODO: for next steps, this should be taken into account => suggests 
                             * traffic engineering and motivates alias resolution.
                             */
                        }
                        curSubnet->setStopDescription(ss.str());
                            
                        // Finally adds the contra-pivots to the subnet
                        while(contrapivots.size() > 0)
                        {
                            IPTableEntry *toAdd = contrapivots.front();
                            contrapivots.pop_front();
                            curSubnet->addInterface(toAdd, SubnetInterface::CONTRAPIVOT);
                        }
                    }
                    else
                    {
                        IPTableEntry *oldPivot = (prevPivot != curPivot) ? prevPivot : NULL;
                        fullShrinkage(curSubnet, &IPs, oldPivot, &contrapivots);
                    
                        stringstream ss;
                        ss << "next prefix length had several contra-pivot candidates (";
                        ss << nbCpivots << ") which the spread suggested a case of overgrowth.";
                        curSubnet->setStopDescription(ss.str());
                    }
                }
                else
                {
                    IPTableEntry *oldPivot = (prevPivot != curPivot) ? prevPivot : NULL;
                    fullShrinkage(curSubnet, &IPs, oldPivot, &contrapivots);
                    
                    stringstream ss;
                    ss << "next prefix length had several contra-pivot candidates (";
                    ss << nbCpivots << ") located at different distances.";
                    curSubnet->setStopDescription(ss.str());
                }
                break;
            }
            // Only pivots or outliers were found during this expansion
            else
            {
                unsigned short minority = totalNew / outliersRatioDivisor;
                
                // IPs appearing as outliers aren't just a minority: subnet shrinks
                if(nbNotOnSubnet > minority)
                {
                    fullShrinkage(curSubnet, &IPs, prevPivot != curPivot ? prevPivot : NULL);
                    
                    stringstream ss;
                    ss << "next prefix length ";
                    if(totalNew > nbNotOnSubnet)
                    {
                        ss << "encompassed too many outliers (" << nbNotOnSubnet;
                        ss << " out of " << totalNew << " IPs).";
                    }
                    else
                        ss << "contained no IP appearing to be on the same subnet.";
                    curSubnet->setStopDescription(ss.str());
                    break;
                }
            }
            
            prevPivot = curPivot;
        }
        
        if(curSubnet->getPrefixLength() == SubnetInferenceRules::MINIMUM_PREFIX_LENGTH)
            curSubnet->setStopDescription("reached the maximum size allowed for a subnet.");
        else if(IPs.size() == 0 && curSubnet->getStopDescription().empty())
        {
            curSubnet->setStopDescription("no more IPs to expand the subnet further.");
            curSubnet->setToPostProcess(true); // Just in case
        }
        
        subnets->push_front(curSubnet);
    }
}
