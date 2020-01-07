/*
 * Aggregate.cpp
 *
 *  Created on: Aug 22, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Aggregate.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "Aggregate.h"

Aggregate::Aggregate(Subnet *first)
{
    preEchoingOffset = 0;
    peersOffset = 0;
    this->addSubnet(first);
}

Aggregate::Aggregate(list<Subnet*> newSubnets)
{
    preEchoingOffset = 0;
    peersOffset = 0;
    this->addSubnets(newSubnets);
}

Aggregate::~Aggregate()
{
    // Nothing deleted here; every other pointed object is destroyed elsewhere
}

void Aggregate::updateTrails(Trail newTrail)
{
    if(newTrail.isVoid()) // "Home" subnet
        return;
    if(newTrail.isEchoing()) // Echoing trails aren't considered as valid trails for an aggregate
        return;

    /*
     * Does the new subnet have a flickering trail, and if yes, is it different than what we 
     * already know ? Because in that case, we need to update "trails" accordingly.
     */

    if(newTrail.isFlickering())
    {
        for(list<Trail>::iterator i = trails.begin(); i != trails.end(); ++i)
        {
            Trail curTrail = (*i);
            if(newTrail == curTrail)
                return;
        }
        trails.push_back(newTrail);
        trails.sort(Trail::compare);
    }
    else if(trails.size() == 0)
    {
        trails.push_back(newTrail);
    }
    
    /*
     * N.B.: in the case of a subnet built around rule 1 & rule 4, 5 (flickering IPs), the code 
     * above doesn't exhaustively list all trails seen in a same subnet. The reason is that, as 
     * long as flickering trails appear individually as the main trails towards disjoint subsets 
     * of subnets, we will end up with an aggregate tying all subsets together and very likely 
     * labelled with all flickering trails. There is still a possibility one or few flickering IPs 
     * could be missing from the label of the aggregate, but a future post-processing could 
     * complete the list (for now, it doesn't have any negative effect on neighborhood inference).
     */
}

void Aggregate::addSubnet(Subnet *newSubnet)
{
    if(newSubnet == NULL)
        return;
    subnets.push_back(newSubnet);
    this->updateTrails(newSubnet->getTrail());
}

void Aggregate::addSubnets(list<Subnet*> newSubnets)
{
    if(newSubnets.size() < 1)
        return;
    this->updateTrails(newSubnets.front()->getTrail()); // Because splice() will empty newSubnets
    subnets.splice(subnets.end(), newSubnets);
}

void Aggregate::discoverPeerIPs()
{
    // 1) Lists subnet interfaces with a (partial) route
    list<SubnetInterface> withRoutes;
    for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        list<SubnetInterface> *IPs = (*i)->getInterfacesList();
        for(list<SubnetInterface>::iterator j = IPs->begin(); j != IPs->end(); ++j)
        {
            SubnetInterface &cur = (*j);
            if(cur.partialRoute.size() > 0)
                withRoutes.push_back(cur);
        }
    }
    
    // No possible peers (gate)
    if(withRoutes.size() == 0)
        return;
    
    // 2) Finds at which offset peers can be found
    unsigned short offset = 0;
    bool foundPeers = false;
    while(!foundPeers && withRoutes.size() > 0)
    {
        offset++;
    
        for(list<SubnetInterface>::iterator i = withRoutes.begin(); i != withRoutes.end(); ++i)
        {
            vector<RouteHop> route = i->partialRoute;
            unsigned short routeLen = route.size();
            if(offset > routeLen)
                withRoutes.erase(i--); // Can't be used anymore
            
            short currentOffset = routeLen - offset;
            if(currentOffset >= 0 && route[currentOffset].isPeer())
                foundPeers = true;
        }
        
        /*
         * The code doesn't use "break" in the former loop, as completing the visit of the list of 
         * "withRoutes" guarantees it will be purged of partial routes that are too short to find 
         * a peer with the current offset.
         */
    }
    
    if(!foundPeers)
        return;
    
    // 3) Lists all peers + other IPs (might later be part of aliases)
    for(list<SubnetInterface>::iterator i = withRoutes.begin(); i != withRoutes.end(); ++i)
    {
        vector<RouteHop> route = i->partialRoute;
        short currentOffset = route.size() - offset;
        if(currentOffset >= 0)
        {
            if(route[currentOffset].isPeer())
                peerIPs.push_back(route[currentOffset].ip);
            else if(route[currentOffset].isValidHop())
                miscellaneousIPs.push_back(route[currentOffset].ip); // Will be checked w/ aliasing
        }
    }
    
    // 4) "peers" list is cleaned (removal of duplicata) and peersOffset is set
    peersOffset = offset - 1;
    peerIPs.sort(InetAddress::smaller);
    InetAddress prev(0);
    for(list<InetAddress>::iterator i = peerIPs.begin(); i != peerIPs.end(); ++i)
    {
        InetAddress cur = (*i);
        if(cur == prev)
        {
            peerIPs.erase(i--);
            continue;
        }
        prev = cur;
    }
    
    // Likewise, miscellaneousIPs is cleaned to remove potential duplicate IPs
    if(miscellaneousIPs.size() > 1)
    {
        prev = InetAddress(0);
        for(list<InetAddress>::iterator i = miscellaneousIPs.begin(); i != miscellaneousIPs.end(); ++i)
        {
            InetAddress cur = (*i);
            if(cur == prev)
            {
                miscellaneousIPs.erase(i--);
                continue;
            }
            prev = cur;
        }
    }
}

bool Aggregate::adjustPeerIPs(AliasSet *aliases)
{
    if(aliases == NULL)
        return false;
    
    // Looks for IPs which belong to previously discovered aliases
    bool adjusted = false;
    for(list<InetAddress>::iterator i = peerIPs.begin(); i != peerIPs.end(); ++i)
    {
        InetAddress cur = (*i);
        Alias *assocAlias = aliases->findAlias(cur);
        if(assocAlias != NULL)
        {
            AliasedInterface &firstInterface = assocAlias->getInterfacesList()->front();
            InetAddress firstIP = (InetAddress) *(firstInterface.ip);
            
            // Replaces the current IP with the first IP of the alias, for convenience
            if(cur != firstIP)
            {
                peerIPs.erase(i--);
                peerIPs.push_back(firstIP);
                adjusted = true;
            }
        }
    }
    
    if(!adjusted)
        return false;
    
    // Sorts and removes duplicates
    peerIPs.sort(InetAddress::smaller);
    InetAddress prev = InetAddress(0);
    for(list<InetAddress>::iterator i = peerIPs.begin(); i != peerIPs.end(); ++i)
    {
        InetAddress cur = (*i);
        if(cur == prev)
        {
            peerIPs.erase(i--);
            continue;
        }
        prev = cur;
    }
    return true;
}

void Aggregate::discoverBlindspots(IPLookUpTable *dict, AliasSet *aliases)
{
    // No need to check for blindspots or already checked them
    if(miscellaneousIPs.size() == 0 || blindspots.size() > 0)
        return;
    
    // Missing objects or no aliases: no need to do anything, either
    if(dict == NULL || aliases == NULL || aliases->getNbAliases() == 0)
        return;
    
    for(list<InetAddress>::iterator i = miscellaneousIPs.begin(); i != miscellaneousIPs.end(); ++i)
    {
        InetAddress curMisc = (*i);
        IPTableEntry *relEntry = dict->lookUp(curMisc);
        if(relEntry == NULL)
            continue;
        
        // Already marked as blindspot ?
        if(relEntry->isABlindspot())
        {
            blindspots.push_back(curMisc);
            miscellaneousIPs.erase(i--);
            continue;
        }
        
        // Finds related alias, if any, then proceeds to check if it contains peer IPs
        Alias *relAlias = aliases->findAlias(curMisc);
        if(relAlias == NULL || relAlias->getNbInterfaces() <= 1)
            continue;
        
        list<AliasedInterface> *IPs = relAlias->getInterfacesList();
        bool isBlindspot = false;
        for(list<AliasedInterface>::iterator j = IPs->begin(); j != IPs->end(); ++j)
        {
            AliasedInterface interface = (*j);
            if(interface.ip->denotesNeighborhood())
            {
                isBlindspot = true;
                break;
            }
        }
        
        if(isBlindspot)
        {
            relEntry->setAsBlindspot();
            blindspots.push_back(curMisc);
            miscellaneousIPs.erase(i--);
        }
    }
    
    if(blindspots.size() > 1)
        blindspots.sort(InetAddress::smaller);
}

void Aggregate::improvePeerIPs(Environment &env)
{
    if(peersOffset == 0 || blindspots.size() == 0)
        return; // Nothing to do here
    
    // Lists subnet interfaces with a (partial) route
    list<SubnetInterface> withRoutes;
    for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        list<SubnetInterface> *IPs = (*i)->getInterfacesList();
        for(list<SubnetInterface>::iterator j = IPs->begin(); j != IPs->end(); ++j)
        {
            SubnetInterface &cur = (*j);
            if(cur.partialRoute.size() > 0)
                withRoutes.push_back(cur);
        }
    }
    
    // No possible peers (gate)
    if(withRoutes.size() == 0)
        return;
    
    // Investigates routes before existing peers (if any) to look for blindspots
    bool hadPeers = false;
    if(peerIPs.size() > 0)
        hadPeers = true;
    unsigned short offset = 0;
    list<RouteHop> spotted;
    IPLookUpTable *dict = env.getIPDictionary();
    list<AliasSet*> *aliases = env.getAliases();
    while((!hadPeers || offset < peersOffset) && spotted.size() == 0 && withRoutes.size() > 0)
    {
        offset++;
    
        for(list<SubnetInterface>::iterator i = withRoutes.begin(); i != withRoutes.end(); ++i)
        {
            vector<RouteHop> route = i->partialRoute;
            unsigned short routeLen = route.size();
            if(offset > routeLen)
                withRoutes.erase(i--); // Can't be used anymore
            
            short currentOffset = routeLen - offset;
            if(currentOffset >= 0 && route[currentOffset].isValidHop())
            {
                InetAddress curHop = route[currentOffset].ip;
                IPTableEntry *curHopEntry = dict->lookUp(curHop);
                if(curHopEntry == NULL)
                    continue;
                
                if(!curHopEntry->isABlindspot())
                    continue;
                
                /*
                 * From this point, we now we have a blindspot, but we still need to do some 
                 * checks for the sake of consistency. The code is therefore thoroughly checking 
                 * we have a valid peer, i.e., the hop isn't:
                 * -an echo/cycle of the target interface or the trail IP (if no anomalies), 
                 * -an alias of the target interface or the trail IP.
                 */
                
                InetAddress targetInterface = (InetAddress) *(i->ip);
                Trail &targetTrail = i->ip->getTrail();
                InetAddress targetTrailIP(0);
                if(targetTrail.getNbAnomalies() == 0)
                    targetTrailIP = targetTrail.getLastValidIP();
                
                // Is it an echo/cycle ?
                if(curHop == targetInterface || curHop == targetTrailIP)
                    continue;
                
                // Is it an alias ?
                bool isAlias = false;
                for(list<AliasSet*>::iterator j = aliases->begin(); j != aliases->end(); ++j)
                {
                    AliasSet *curSet = (*j);
                    Alias *curHopAlias = curSet->findAlias(curHop);
                    if(curHopAlias != NULL)
                    {
                        if(curHopAlias->hasInterface(targetInterface))
                        {
                            isAlias = true;
                            break;
                        }
                        else if(targetTrailIP != InetAddress(0))
                        {
                            if(curHopAlias->hasInterface(targetTrailIP))
                            {
                                isAlias = true;
                                break;
                            }
                        }
                    }
                }
                
                if(isAlias)
                    continue;
                
                // Everything is OK: this is indeed a unidentified peer.
                spotted.push_back(route[currentOffset]);
            }
        }
    }
    
    // Better peers were indeed discovered. List of IPs are refreshed.
    if(spotted.size() > 0)
    {
        peerIPs.clear();
        miscellaneousIPs.clear();
        blindspots.clear();
        
        // Moves the "spotted" hops to the list of blindspots
        while(spotted.size() > 0)
        {
            RouteHop curHop = spotted.front();
            blindspots.push_back(curHop.ip);
            spotted.pop_front();
        }
        
        // Removes duplicates (if any)
        if(blindspots.size() > 1)
        {
            InetAddress prev = InetAddress(0);
            for(list<InetAddress>::iterator i = blindspots.begin(); i != blindspots.end(); ++i)
            {
                InetAddress cur = (*i);
                if(cur == prev)
                {
                    blindspots.erase(i--);
                    continue;
                }
                prev = cur;
            }
        }
        
        // Updates the offset
        peersOffset = offset - 1;
    }
}

void Aggregate::listFinalPeers(map<InetAddress, Peer*> peersMap)
{
    // Lists IPs
    list<InetAddress> pIPs;
    pIPs.insert(pIPs.end(), peerIPs.begin(), peerIPs.end());
    if(blindspots.size() > 0)
        pIPs.insert(pIPs.end(), blindspots.begin(), blindspots.end());
    pIPs.sort(InetAddress::smaller);
    
    // Reviews them to list matching Peer objects
    for(list<InetAddress>::iterator i = pIPs.begin(); i != pIPs.end(); ++i)
    {
        map<InetAddress, Peer*>::iterator res = peersMap.find((*i));
        if(res != peersMap.end())
            finalPeers.push_back(res->second);
    }
    
    // Sorts and removes duplicates
    finalPeers.sort(Peer::compare);
    if(finalPeers.size() > 1)
    {
        Peer *prev = NULL;
        for(list<Peer*>::iterator i = finalPeers.begin(); i != finalPeers.end(); ++i)
        {
            Peer *cur = (*i);
            if(cur == prev)
            {
                finalPeers.erase(i--);
                continue;
            }
            prev = cur;
        }
    }
}

list<InetAddress> Aggregate::listInitialPeerIPs()
{
    list<InetAddress> res;
    res.insert(res.end(), peerIPs.begin(), peerIPs.end());
    if(miscellaneousIPs.size() == 0)
        return res;
    res.insert(res.end(), miscellaneousIPs.begin(), miscellaneousIPs.end());
    res.sort(InetAddress::smaller);
    return res;
}

list<InetAddress> Aggregate::listTruePeerIPs()
{
    list<InetAddress> res;
    res.insert(res.end(), peerIPs.begin(), peerIPs.end());
    if(blindspots.size() == 0)
        return res;
    res.insert(res.end(), blindspots.begin(), blindspots.end());
    res.sort(InetAddress::smaller);
    return res;
}

list<InetAddress> Aggregate::listIdentifyingIPs()
{
    list<InetAddress> res;
    for(list<Trail>::iterator i = trails.begin(); i != trails.end(); ++i)
    {
        Trail &cur = (*i);
        if(cur.getNbAnomalies() > 0)
            continue;
        res.push_back(cur.getLastValidIP());
    }
    res.sort(InetAddress::smaller);
    return res;
}

bool Aggregate::compare(Aggregate *a1, Aggregate *a2)
{
    if(a1->trails.size() == 0 && a2->trails.size() == 0)
        return Subnet::compare(a1->subnets.front(), a2->subnets.front());
    else if(a2->trails.size() == 0)
        return false;
    else if(a1->trails.size() == 0)
        return true;
    Trail &firstTrail = a1->trails.front();
    Trail &secondTrail = a2->trails.front();
    return Trail::compare(firstTrail, secondTrail);
}

string Aggregate::getLabel()
{
    stringstream label;
    label << "{";
    
    if(trails.size() == 0 && subnets.front()->getPivotTTL() <= 2)
    {
        label << "This computer}";
        return label.str();
    }
    
    // Flickering trails
    if(trails.size() > 1)
    {
        for(list<Trail>::iterator i = trails.begin(); i != trails.end(); ++i)
        {
            if(i != trails.begin())
                label << ", ";
            label << i->getLastValidIP();
        }
    }
    // Echoing trails (best effort strategy used instead)
    else if(trails.size() == 0)
    {
        label << "Echo, TTL=" << (unsigned short) subnets.front()->getPivotTTL();
        label << ", Pre-echoing=";
        for(list<InetAddress>::iterator i = preEchoing.begin(); i != preEchoing.end(); ++i)
        {
            if(i != preEchoing.begin())
                label << ", ";
            label << (*i);
        }
        if(preEchoingOffset > 0)
            label << " (offset=" << preEchoingOffset << ")";
    }
    // Any other case (direct or incomplete trail common to all aggregated subnets)
    else
    {
        Trail &curTrail = trails.front();
        label << curTrail.getLastValidIP();
        if(curTrail.getNbAnomalies() > 0)
            label << " | " << curTrail.getNbAnomalies();
    }
    
    label << "}";
    return label.str();
}


string Aggregate::toString()
{
    stringstream ss;
    ss << "Aggregate for " << this->getLabel() << ":\n";
    
    // Now listing the aggregated subnets.
    
    // Case of an aggregate with flickering trails: tells which trail(s) appear in which subnet(s)
    if(trails.size() > 1)
    {
        for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
        {
            Subnet *cur = (*i);
            ss << cur->getAdjustedCIDR();
            list<InetAddress> cTrailIPs = cur->getDirectTrailIPs();
            if(cTrailIPs.size() > 1)
            {
                ss << " (trails: ";
                for(list<InetAddress>::iterator j = cTrailIPs.begin(); j != cTrailIPs.end(); ++j)
                {
                    if(j != cTrailIPs.begin())
                        ss << ", ";
                    ss << (*j);
                }
            }
            else
                ss << " (trail: " << cTrailIPs.front();
            ss << ")\n";
        }
    }
    // Case of an "Echo" aggregate with multiple pre-echoing IPs: details pre-echoing IPs
    else if(trails.size() == 0 && preEchoing.size() > 1)
    {
        for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
        {
            Subnet *cur = (*i);
            ss << cur->getAdjustedCIDR() << " (pre-echoing: ";
            list<InetAddress> preEchoes = cur->getPreTrailIPs();
            for(list<InetAddress>::iterator j = preEchoes.begin(); j != preEchoes.end(); ++j)
            {
                if(j != preEchoes.begin())
                    ss << ", ";
                ss << (*j);
            }
            ss << ")\n";
        }
    }
    // Any other situation: just prints the (adjusted) CIDR's
    else
    {
        for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
            ss << (*i)->getAdjustedCIDR() << "\n";
    }
    
    // Writes down the peer IP(s), otherwise that this aggregate could be a gate to the topology
    if(peerIPs.size() > 0 || blindspots.size() > 0)
    {
        if(peerIPs.size() > 1)
        {
            ss << "Peers: ";
            for(list<InetAddress>::iterator i = peerIPs.begin(); i != peerIPs.end(); ++i)
            {
                if(i != peerIPs.begin())
                    ss << ", ";
                ss << (*i);
            }
            if(peersOffset > 0)
                ss << " (offset=" << peersOffset << ")";
            ss << "\n";
        }
        else
        {
            ss << "Peer: " << peerIPs.front();
            if(peersOffset > 0)
                ss << " (offset=" << peersOffset << ")";
            ss << "\n";
        }
        if(blindspots.size() > 0)
        {
            ss << "Blindspot";
            if(blindspots.size() > 1)
                ss << " (miscellaneous aliases ";
            else
                ss << " (miscellaneous alias ";
            ss << "of peer IP(s)): ";
            for(list<InetAddress>::iterator i = blindspots.begin(); i != blindspots.end(); ++i)
            {
                if(i != blindspots.begin())
                    ss << ", ";
                ss << (*i);
            }
            if(peerIPs.size() == 0 && peersOffset > 0)
                ss << " (offset=" << peersOffset << ")";
            ss << "\n";
        }
        if(miscellaneousIPs.size() > 0)
        {
            ss << "Miscellaneous (seen at the same distance): ";
            for(list<InetAddress>::iterator i = miscellaneousIPs.begin(); i != miscellaneousIPs.end(); ++i)
            {
                if(i != miscellaneousIPs.begin())
                    ss << ", ";
                ss << (*i);
            }
            ss << "\n";
        }
    }
    else
        ss << "No peers; gate to the discovered topology.\n";
    
    return ss.str();
}
