/*
 * AliasResolver.cpp
 *
 *  Created on: Oct 20, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in AliasResolver.h (see this file to learn further about the goals 
 * of such a class).
 */

#include <list>
using std::list;
#include <cmath>

#include "AliasResolver.h"

AliasResolver::AliasResolver(Environment &e): env(e)
{
}

AliasResolver::~AliasResolver()
{
}

unsigned short AliasResolver::Ally(IPTableEntry *ip1, IPTableEntry *ip2, unsigned short maxDiff)
{
    if(ip1 == NULL || ip2 == NULL) // Just in case
        return ALLY_NO_SEQUENCE;

    AliasHints &ah1 = ip1->getLattestHints();
    AliasHints &ah2 = ip2->getLattestHints();

    // The first condition is just in case; normally calling code checks it first
    if(!ah1.isEmpty() && !ah2.isEmpty())
    {
        unsigned short nbIPIDs = env.getARNbIPIDs();
        
        vector<unsigned long> &probeTokens1 = ah1.getProbeTokens();
        vector<unsigned long> &probeTokens2 = ah2.getProbeTokens();
        
        vector<unsigned short> &IPIDs1 = ah1.getIPIdentifiers();
        vector<unsigned short> &IPIDs2 = ah2.getIPIdentifiers();
        
        /*
         * February 2018: thanks to the new IP-ID collection mechanism introduced back in 2017, 
         * looking for an interleaving sequence of IP-IDs between both IPs is now much simpler. 
         * Indeed, thanks to the "round probing", no second IP-ID in a sequence for an IP should 
         * come earlier than any first IP-ID, and this also holds through for second and third 
         * IP-ID, etc. Therefore, one can just compare IP-ID by IP-ID. This also allows for a 
         * more thorough resolution, as it is also easier to check that the whole sequence of 
         * IP-IDs for both IPs match.
         */
        
        unsigned short curIndex = 0;
        unsigned short nbRollovers = 0;
        
        // 1) Checks the token;IP-ID pairs are sound w.r.t. each other
        while(curIndex < nbIPIDs)
        {
            // IP-ID from first IP comes first
            if(probeTokens1[curIndex] < probeTokens2[curIndex])
            {
                // But IP-ID from first IP is bigger: is it a rollover scenario ?
                if(IPIDs1[curIndex] > IPIDs2[curIndex])
                {
                    unsigned int diff = (65535 - IPIDs1[curIndex]) + IPIDs2[curIndex];
                
                    /*
                     * If the difference is too large, then it cannot be a rollover scenario. The 
                     * alias can't be validated.
                     */
                    
                    if(diff > maxDiff) 
                        return ALLY_REJECTED;
                    nbRollovers++;
                }
            }
            // IP-ID from second IP comes first
            else
            {
                if(IPIDs2[curIndex] > IPIDs1[curIndex])
                {
                    unsigned int diff = (65535 - IPIDs2[curIndex]) + IPIDs1[curIndex];
                    if(diff > maxDiff) 
                        return ALLY_REJECTED;
                    nbRollovers++;
                }
            }
            
            // More than one rollover suggests this alias cannot be aliased soundly with Ally.
            if(nbRollovers > 1)
                return ALLY_REJECTED;
            
            curIndex++;
        }
        
        // 2) Checks the sequence is sound
        curIndex = 0;
        nbRollovers = 0;
        while(curIndex < nbIPIDs - 1)
        {
            // Possible rollover ? If yes, the code checks if the difference.
            if(IPIDs1[curIndex] > IPIDs2[curIndex + 1])
            {
                unsigned int diff = (65535 - IPIDs1[curIndex]) + IPIDs2[curIndex + 1];
                
                /*
                 * If the difference is too large, then it cannot be a rollover scenario. The 
                 * alias can't be validated.
                 */
                
                if(diff > maxDiff) 
                    return ALLY_REJECTED;
                nbRollovers++;
            }
            
            // Same logic applies to the other couple
            if(IPIDs2[curIndex] > IPIDs1[curIndex + 1])
            {
                unsigned int diff = (65535 - IPIDs2[curIndex]) + IPIDs1[curIndex + 1];
                if(diff > maxDiff)
                    return ALLY_REJECTED;
                nbRollovers++;
            }
            
            // Too many rollovers suggest this alias cannot be aliased soundly with Ally.
            if(nbRollovers > 2)
                return ALLY_REJECTED;
            
            curIndex++;
        }
        
        return ALLY_ACCEPTED;
    }
    return ALLY_NO_SEQUENCE;
}

unsigned short AliasResolver::groupAlly(IPTableEntry *isolatedIP, 
                                        list<IPTableEntry*> group, 
                                        unsigned short maxDiff)
{
    bool accepted = false;
    for(list<IPTableEntry*>::iterator it = group.begin(); it != group.end(); ++it)
    {
        IPTableEntry *curIP = (*it);
        unsigned short pairAllyRes = this->Ally(curIP, isolatedIP, maxDiff);
        if(pairAllyRes == ALLY_ACCEPTED)
            accepted = true;
        else if(pairAllyRes == ALLY_REJECTED)
            return ALLY_REJECTED;
    }
    
    /*
     * N.B.: nothing occurs in the loop above in case of ALLY_NO_SEQUENCE, but there must be at 
     * least one ALLY_ACCEPTED to consider the alias is possible.
     */
    
    if(accepted)
        return ALLY_ACCEPTED;
    return ALLY_NO_SEQUENCE;
}

bool AliasResolver::velocityOverlap(IPTableEntry *ip1, IPTableEntry *ip2)
{
    if(ip1 == NULL || ip2 == NULL)
        return false;
    
    AliasHints &ah1 = ip1->getLattestHints();
    AliasHints &ah2 = ip2->getLattestHints();
    
    if(ah1.isEmpty() || ah2.isEmpty())
        return false;

    double low1 = ah1.getVelocityLowerBound();
    double low2 = ah2.getVelocityLowerBound();
    double up1 = ah1.getVelocityUpperBound();
    double up2 = ah2.getVelocityUpperBound();

    if(up1 == 0.0 || up2 == 0.0)
        return false;

    // Particular case of "infinite" velocity (should no longer be evaluated, but just in case)
    if(up1 == 65535.0 || up2 == 65535.0)
    {
        if(up1 == up2)
            return true;
        return false;
    }
    
    // Computes tolerance value
    double tolerance = env.getARVelocityBaseTolerance();
    if((low1 / (tolerance * 10)) > 1.0 && (low2 / (tolerance * 10)) > 1.0)
        tolerance *= (low1 / (tolerance * 10));
    
    // Finds largest intervall and extends it with tolerance
    double low1Bis = low1, low2Bis = low2, up1Bis = up1, up2Bis = up2;
    if((up1 - low1) > (up2 - low2))
    {
        low1Bis = low1 - tolerance;
        up1Bis = up1 + tolerance;
    }
    else
    {
        low2Bis = low2 - tolerance;
        up2Bis = up2 + tolerance;
    }
    
    // If overlaps with tolerance is OK...
    if(low1Bis <= up2Bis && up1Bis >= low2Bis)
    {
        unsigned short nbIPIDs = env.getARNbIPIDs();
        
        /*
         * (Short validation added in March 2017)
         *
         * If we denote ah_1 as the hints of first IP to have been probed, and ah_2 hints of the 
         * second, we validate the association by checking if ah_2 first IP-ID belongs to the interval:
         *
         * [ah_1 last IP-ID, ah_1 last IP-ID + speed (inc./token) * 1,25 * (ah_2 first token - ah_1 last token)]
         *
         * The 1,25 ensures slight changes in speed do not disturb the final results. Overlaps 
         * must also be considered in the process.
         */
        
        AliasHints ah_1(ah1), ah_2(ah2);
        vector<unsigned long> &tokens1 = ah_1.getProbeTokens();
        vector<unsigned long> &tokens2 = ah_2.getProbeTokens();
        unsigned long diffTokens = 0;
        if(tokens1[nbIPIDs - 1] > tokens2[0])
        {
            ah_1 = AliasHints(ah2);
            ah_2 = AliasHints(ah1);
            
            tokens1 = ah_1.getProbeTokens();
            tokens2 = ah_2.getProbeTokens();
        }
        diffTokens = tokens2[0] - tokens1[nbIPIDs - 1];
        
        // We compute a new speed, as increase per tokens, from the first IP.
        vector<unsigned short> &IDs1 = ah_1.getIPIdentifiers();
        vector<unsigned short> &IDs2 = ah_2.getIPIdentifiers();
        double speedPerToken = 0.0;
        unsigned int totalDiff = 0;
        for(unsigned short i = 0; i < nbIPIDs - 1; i++)
        {
            if(IDs1[i + 1] < IDs1[i])
                totalDiff += (unsigned int) IDs1[i + 1] + 65535 - (unsigned int) IDs1[i];
            else
                totalDiff += (unsigned int) (IDs1[i + 1] - IDs1[i]);
        }
        speedPerToken = (double) totalDiff / (double) (tokens1[nbIPIDs - 1] - tokens1[0]);
        speedPerToken *= 2;
        
        bool validated = false;
        unsigned short fIPID = IDs1[nbIPIDs - 1];
        unsigned short lIPID = IDs2[0];
        unsigned int prediction = (unsigned int) fIPID + (unsigned int) (speedPerToken * (double) diffTokens);
        if(prediction > 65535)
        {
            unsigned short endIPID = (unsigned short) (prediction % 65535);
            if((lIPID > fIPID) || (lIPID > 0 && lIPID <= endIPID))
                validated = true;
        }
        else
        {
            unsigned short endIPID = (unsigned short) prediction;
            if(lIPID > fIPID && lIPID <= endIPID)
                validated = true;
        }
        
        return validated;
    }
    return false;
}

bool AliasResolver::groupVelocity(IPTableEntry *isolatedIP, list<IPTableEntry*> group)
{
    for(list<IPTableEntry*>::iterator it = group.begin(); it != group.end(); ++it)
        if(!this->velocityOverlap(isolatedIP, (*it)))
            return false;
    return true;
}

bool AliasResolver::reverseDNS(IPTableEntry *ip1, IPTableEntry *ip2)
{
    // Just in case
    if(ip1 == NULL || ip2 == NULL)
        return false;
    
    AliasHints &ah1 = ip1->getLattestHints();
    AliasHints &ah2 = ip2->getLattestHints();
    
    if(ah1.isEmpty() || ah2.isEmpty())
        return false;
    
    string hostName1 = ah1.getHostName();
    string hostName2 = ah2.getHostName();
    
    if(!hostName1.empty() && !hostName2.empty())
    {
        list<string> hn1Chunks;
        list<string> hn2Chunks;
        string element;
        
        stringstream hn1(hostName1);
        while (std::getline(hn1, element, '.'))
            hn1Chunks.push_front(element);
        
        stringstream hn2(hostName2);
        while (std::getline(hn2, element, '.'))
            hn2Chunks.push_front(element);
        
        if(hn1Chunks.size() == hn2Chunks.size())
        {
            unsigned short size = (unsigned short) hn1Chunks.size();
            unsigned short similarities = 0;
            
            list<string>::iterator itBis = hn1Chunks.begin();
            for(list<string>::iterator it = hn2Chunks.begin(); it != hn2Chunks.end(); ++it)
            {
                if((*it).compare((*itBis)) == 0)
                {
                    similarities++;
                    itBis++;
                }
                else
                    break;
            }
            
            if(similarities >= (size - 1))
                return true;
            return false;
        }
    }
    return false;
}

list<Alias*> AliasResolver::resolve(list<IPTableEntry*> IPs, bool strict)
{
    list<Alias*> result;
    this->discover(IPs, &result, strict);
    return result;
}

void AliasResolver::discover(list<IPTableEntry*> IPs, list<Alias*> *results, bool strict)
{
    // Removes duplicata (can exist; an ingress interface of a neighborhood can be a contra-pivot)
    IPTableEntry *prev = NULL;
    for(list<IPTableEntry*>::iterator i = IPs.begin(); i != IPs.end(); ++i)
    {
        IPTableEntry *cur = (*i);
        if(cur == prev)
            IPs.erase(i--);
        prev = cur;
    }
    
    // Filters out IPs which don't have hints from lattest alias resolution phase
    list<IPTableEntry*> finalIPs;
    for(list<IPTableEntry*>::iterator i = IPs.begin(); i != IPs.end(); ++i)
    {
        IPTableEntry *cur = (*i);
        AliasHints &curHints = cur->getLattestHints();
        if(!curHints.isEmpty() && curHints.isLattestStage())
            finalIPs.push_back(cur);
    }
    finalIPs.sort(IPTableEntry::compareLattestFingerprints);
    
    /*
     * Alias resolution starts here. One IP is picked from the list, then a list of IPs which have 
     * a similar fingerprint, after what the alias hints of the former are checked to see which 
     * alias resolution approach should be applied. There are similarities between some cases, but 
     * each has its own specific additional checks to maximize accuracy, hence why the code is not 
     * more generic (for now).
     */
    
    while(finalIPs.size() > 0)
    {
        IPTableEntry *cur = finalIPs.front();
        finalIPs.pop_front();
        
        // No more IPs: creates a single alias for cur (outside strict mode)
        if(finalIPs.size() == 0)
        {
            if(!strict)
                results->push_back(new Alias(cur));
            break;
        }
        
        // Lists IPs which the fingerprints are similar, taking advantage of the sorting.
        list<IPTableEntry*> similar;
        AliasHints &refHints = cur->getLattestHints();
        IPTableEntry *next = finalIPs.front();
        while(finalIPs.size() > 0 && refHints.fingerprintSimilarTo(next->getLattestHints()))
        {
            similar.push_back(next);
            finalIPs.pop_front();
            if(finalIPs.size() > 0)
                next = finalIPs.front();
        }
        
        /*
         * If cur has its field "portUnreachableSrcIP" different from 0.0.0.0, we also look for 
         * a fingerprint which the IP is identical to the value of this field to add it to the 
         * "similar" list. This will lead by design to the second alias resolution scenario, in 
         * which case an alias featuring at least both IPs will be created.
         */
        
        InetAddress UDPSrcIP(0);
        if(!refHints.isUDPSecondary())
        {
            UDPSrcIP = refHints.getPortUnreachableSrcIP();
            if(UDPSrcIP != InetAddress(0))
            {
                for(list<IPTableEntry*>::iterator i = finalIPs.begin(); i != finalIPs.end(); ++i)
                {
                    IPTableEntry *subCur = (*i);
                    if((InetAddress) (*subCur) == UDPSrcIP)
                    {
                        similar.push_back(subCur);
                        finalIPs.erase(i--);
                    }
                }
            }
        }
        
        if(similar.size() == 0)
        {
            /*
             * Case 1: there is no similar fingerprint in the list, therefore one creates a Alias 
             * object for this IP only, except if it features a source IP for the UDP port 
             * unreachable method. In this case, the corresponding AliasHints object sees its 
             * "UDPSecondary" flag raised, in order to try other alias resolution methods (e.g., 
             * IP-ID-based methods).
             */
            
            if(UDPSrcIP != InetAddress(0))
            {
                refHints.setUDPSecondary(true);
                finalIPs.push_front(cur);
                finalIPs.sort(IPTableEntry::compareLattestFingerprints);
                continue;
            }
            else if(!strict)
                results->push_back(new Alias(cur)); // Only created outside strict mode
        }
        else if(!refHints.isUDPSecondary() && UDPSrcIP != InetAddress(0))
        {
            if(similar.size() > 0)
            {
                /*
                 * Case 2: similar fingerprints for which we have the data needed for the UDP 
                 * port unreachable alias resolution method. Due to the sorting process and the usage 
                 * of fingerprintSimilarTo() while picking similar fingerprints (that is, the source 
                 * IP in the Port Unreachable response is the same for all fingerprints), one can just 
                 * create a single alias with all the listed IPs and "cur".
                 */
            
                Alias *newAlias = new Alias(cur);
                newAlias->addInterfaces(&similar, AliasedInterface::UDP_PORT_UNREACHABLE);
                results->push_back(newAlias);
            }
            // Otherwise, single IP: hints are set to ignore UDP to try other aliasing techniques.
            else
            {
                refHints.setUDPSecondary(true);
                finalIPs.push_front(cur);
                finalIPs.sort(IPTableEntry::compareLattestFingerprints);
                continue;
            }
        }
        else if(refHints.getIPIDCounterType() == AliasHints::HEALTHY_COUNTER)
        {
            /*
             * Case 3: similar fingerprint and "healthy" IP-ID counter. The Ally method is used.
             */
            
            list<IPTableEntry*> toProcess = similar;
            list<IPTableEntry*> excluded;
            list<IPTableEntry*> grouped;
            while(toProcess.size() > 0)
            {
                IPTableEntry *subCur = toProcess.front();
                toProcess.pop_front();
                
                grouped.push_front(cur);
                unsigned short AllyRes = this->groupAlly(subCur, grouped, env.getARAllyMaxDiff());
                grouped.pop_front();
                
                if(AllyRes == ALLY_ACCEPTED)
                    grouped.push_back(subCur);
                else
                    excluded.push_back(subCur);
                
                /*
                 * When toProcess gets empty, we have some additional checks to perform to obtain 
                 * accurate results. The reason why it's a condition at the end of the body of the 
                 * while(toProcess.size() > 0) loop rather than additional code after the loop is 
                 * because toProcess might be refilled with IPs which were excluded from an alias, 
                 * in order to try to alias them with other IPs, re-using this loop. The same 
                 * logic applies to other loops in this method.
                 */
                
                if(toProcess.size() == 0)
                {
                    /*
                     * Looks after another alias obtained through the address-based method with 
                     * "healthy" IPs, because we might miss aliases with some of these IPs. To 
                     * check if the aliasing "cur" and a previously discovered alias makes sense, 
                     * a "merging pivot" is selected in said alias (see the Alias class).
                     */
                    
                    bool fusionOccurred = false;
                    for(list<Alias*>::iterator it = results->begin(); it != results->end(); ++it)
                    {
                        Alias *listed = (*it);
                        IPTableEntry *pivot = listed->getMergingPivot();
                        if(pivot != NULL)
                        {
                            unsigned short maxDiff = env.getARAllyMaxDiff();
                            grouped.push_front(cur);
                            unsigned short AllyResult = this->groupAlly(pivot, grouped, maxDiff);
                            if(AllyResult == ALLY_ACCEPTED)
                            {
                                fusionOccurred = true;
                                listed->addInterfaces(&grouped, AliasedInterface::ALLY);
                                break;
                            }
                            else
                                grouped.pop_front();
                        }
                    }
                    
                    /*
                     * If no fusion occurred, creates an alias with cur and the IPs found in 
                     * grouped except if grouped is empty and strict mode is enabled.
                     */
                    
                    if(!fusionOccurred && (!strict || grouped.size() > 0))
                    {
                        Alias *newAlias = new Alias(cur);
                        newAlias->addInterfaces(&grouped, AliasedInterface::ALLY);
                        results->push_back(newAlias);
                    }
                    
                    // Takes care of excluded IPs.
                    if(excluded.size() > 0)
                    {
                        cur = excluded.front();
                        excluded.pop_front();
                        if(excluded.size() > 0) // Re-fills toProcess
                        {
                            while(excluded.size() > 0)
                            {
                                toProcess.push_back(excluded.front());
                                excluded.pop_front();
                            }
                        }
                        else if(!strict) 
                            results->push_back(new Alias(cur)); // Except if in strict mode
                    }
                }
            }
        }
        else if(!strict && refHints.getIPIDCounterType() == AliasHints::FAST_COUNTER)
        {
           /*
            * Case 4 (never performed if strict mode): similar fingerprint and "fast" counter: 
            * the velocity-based method is used.
            */
            
            list<IPTableEntry*> toProcess = similar;
            list<IPTableEntry*> excluded;
            list<IPTableEntry*> grouped;
            while(toProcess.size() > 0)
            {
                IPTableEntry *subCur = toProcess.front();
                toProcess.pop_front();
                
                grouped.push_front(cur);
                bool aliasable = this->groupVelocity(subCur, grouped);
                grouped.pop_front();
                
                if(aliasable)
                    grouped.push_back(subCur);
                else
                    excluded.push_back(subCur);
                
                // When we are done, creates the current alias and re-fills toProcess if necessary.
                if(toProcess.size() == 0)
                {
                    Alias *newAlias = new Alias(cur);
                    newAlias->addInterfaces(&grouped, AliasedInterface::IPID_VELOCITY);
                    results->push_back(newAlias);
                    
                    // Takes care of excluded IPs
                    if(excluded.size() > 0)
                    {
                        cur = excluded.front();
                        excluded.pop_front();
                        if(excluded.size() > 0)
                        {
                            while(excluded.size() > 0)
                            {
                                toProcess.push_back(excluded.front());
                                excluded.pop_front();
                            }
                        }
                        else if(!strict)
                            results->push_back(new Alias(cur));
                    }
                }
            }  
        }
        else if(!strict && refHints.toGroupByDefault())
        {
            /*
             * Case 5 (never performed if strict mode): similar fingerprints and "group by 
             * default" policy (applied to echo and random IP-ID counters). The fingerprints with 
             * DNS are double checked to exclude IPs with similar fingerprints but host names that 
             * do not match. The process is repeated as long as there are excluded IPs.
             *
             * If "cur" or the next IP have no host name, then the whole group can be assumed to 
             * have not enough host names to perform reverse DNS (due to the sorting strategy).
             */
            
            string firstHostName = similar.front()->getLattestHints().getHostName();
            if(!refHints.getHostName().empty() && !firstHostName.empty())
            {
                list<IPTableEntry*> toProcess = similar;
                list<IPTableEntry*> excluded;
                list<IPTableEntry*> grouped;
                while(toProcess.size() > 0)
                {
                    IPTableEntry *subCur = toProcess.front();
                    toProcess.pop_front();
                    
                    if(!subCur->getLattestHints().getHostName().empty())
                    {
                        if(this->reverseDNS(cur, subCur))
                            grouped.push_back(subCur);
                        else
                            excluded.push_back(subCur);
                    }
                    else
                        grouped.push_back(subCur);
                    
                    if(toProcess.size() == 0)
                    {
                        // Creates the alias with the IPs found in "grouped" + cur.
                        Alias *newAlias = new Alias(cur);
                        while(grouped.size() > 0)
                        {
                            IPTableEntry *head = grouped.front();
                            AliasHints &headHints = head->getLattestHints();
                            grouped.pop_front();
                            unsigned short aliasMethod = AliasedInterface::GROUP_ECHO_DNS;
                            if(headHints.getIPIDCounterType() == AliasHints::RANDOM_COUNTER)
                            {
                                if(!headHints.getHostName().empty())
                                    aliasMethod = AliasedInterface::GROUP_RANDOM_DNS;
                                else
                                    aliasMethod = AliasedInterface::GROUP_RANDOM;
                            }
                            else
                            {
                                if(!headHints.getHostName().empty())
                                    aliasMethod = AliasedInterface::GROUP_ECHO_DNS;
                                else
                                    aliasMethod = AliasedInterface::GROUP_ECHO;
                            }
                            newAlias->addInterface(head, aliasMethod);
                        }
                        results->push_back(newAlias);
                        
                        // Takes care of excluded IPs
                        if(excluded.size() > 0)
                        {
                            cur = excluded.front();
                            excluded.pop_front();
                            if(excluded.size() > 0)
                            {
                                while(excluded.size() > 0)
                                {
                                    toProcess.push_back(excluded.front());
                                    excluded.pop_front();
                                }
                            }
                            else
                                results->push_back(new Alias(cur));
                        }
                    }
                }
            }
            else
            {
                // Single alias made of cur and every IP listed in "similar"
                Alias *newAlias = new Alias(cur);
                
                unsigned short aliasMethod = AliasedInterface::GROUP_ECHO;
                if(refHints.getIPIDCounterType() == AliasHints::RANDOM_COUNTER)
                    aliasMethod = AliasedInterface::GROUP_RANDOM;
                
                newAlias->addInterfaces(&similar, aliasMethod);
                results->push_back(newAlias);
            }
        }
        else if(!strict && refHints.getIPIDCounterType() == AliasHints::NO_IDEA)
        {
            /*
             * Case 6 (never performed if strict mode): case of IPs for which no IP-ID data could 
             * be reliably collected. There are two possibilites:
             * -either there is no host name (easily checked with cur, since fingerprints with 
             *  host names should come first), then a single alias is created for each IP,
             * -either there are host names, in which case one separates the IPs with host names 
             *  from the others (for those, it is the same outcome as in previous point) and 
             *  attempts gathering them through reverse DNS.
             */
            
            // Creates an alias for each IP which no host name data could be collected
            if(refHints.getHostName().empty())
            {
                // In this situation, due to sorting, no IP will have a host name.
                while(similar.size() > 0)
                {
                    results->push_back(new Alias(similar.front()));
                    similar.pop_front();
                }
            }
            else
            {
                // IPs following cur might still have a host name in this situation (or not)
                for(list<IPTableEntry*>::iterator it = similar.begin(); it != similar.end(); ++it)
                {
                    IPTableEntry *subCur = (*it);
                    AliasHints &subCurHints = subCur->getLattestHints();
                    if(!subCurHints.isEmpty() && !subCurHints.getHostName().empty())
                    {
                        results->push_back(new Alias(subCur));
                        similar.erase(it--);
                    }
                }
            }
            
            // If there remains only cur, creates an alias for it and moves on
            if(similar.size() == 0)
            {
                results->push_back(new Alias(cur));
                continue;
            }
            
            // Now takes care of IPs with a known host name
            list<IPTableEntry*> toProcess = similar;
            list<IPTableEntry*> excluded;
            list<IPTableEntry*> grouped;
            while(toProcess.size() > 0)
            {
                IPTableEntry *subCur = toProcess.front();
                toProcess.pop_front();
                
                if(this->reverseDNS(cur, subCur))
                    grouped.push_back(subCur);
                else
                    excluded.push_back(subCur);
                
                if(toProcess.size() == 0)
                {
                    // Creates an alias with cur and the IPs found in grouped
                    Alias *newAlias = new Alias(cur);
                    newAlias->addInterfaces(&grouped, AliasedInterface::REVERSE_DNS);
                    results->push_back(newAlias);
                    
                    // Takes care of excluded IPs
                    if(excluded.size() > 0)
                    {
                        cur = excluded.front();
                        excluded.pop_front();
                        if(excluded.size() > 0)
                        {
                            while(excluded.size() > 0)
                            {
                                toProcess.push_back(excluded.front());
                                excluded.pop_front();
                            }
                        }
                        else
                            results->push_back(new Alias(cur));
                    }
                }
            }
        }
        else
        {
            /*
             * Case 7: if we are in "strict" mode and reach this block, nothing happens besides 
             * having removed several alias candidates from "finalIPs" list. We want to only 
             * produce Alias objects from UDP-based method and Ally method with at least two 
             * interfaces per object.
             */
        }
    }
}
