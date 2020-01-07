/*
 * TopologyInferrer.cpp
 *
 *  Created on: Aug 22, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in TopologyInferrer.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "TopologyInferrer.h"

#include "../aliasresolution/AliasHintsCollector.h"
#include "../aliasresolution/AliasResolver.h"

#include "buildingutils/IPClusterer.h"
#include "buildingutils/Aggregate.h"
#include "buildingutils/GraphPeers.h"
#include "components/Node.h"
#include "components/Cluster.h"
#include "components/DirectLink.h"
#include "components/IndirectLink.h"
#include "components/RemoteLink.h"
#include "voyagers/Pioneer.h"

TopologyInferrer::TopologyInferrer(Environment &e): env(e)
{
}

TopologyInferrer::~TopologyInferrer()
{
}

Graph *TopologyInferrer::infer()
{
    // Graph has been already built at some point: stops
    if(env.getLattestAliases() != NULL)
        if(env.getLattestAliases()->getWhen() > AliasSet::SUBNET_DISCOVERY)
            return NULL;

    ostream *out = env.getOutputStream();
    IPLookUpTable *dict = env.getIPDictionary();
    AliasSet *subInfAliases = env.getLattestAliases();

    /*
     * Step 1: subnet aggregation
     * --------------------------
     * The code first aggregates subnet that have an identical trail, excepts for subnets built 
     * around rule 3 (echoing trails) which are put away in a different structure to be processed 
     * later (see below). Aggregates (i.e., simplest form of neighborhoods) are then built based 
     * on these lists, taking in account the possibility that a trail might be flickering, and 
     * thus, we have to merge into a single neighborhood lists of subnets that have trails 
     * flickering with each other.
     */
    
    (*out) << "Building initial aggregates of subnets... " << std::flush;
    
    list<Subnet*> *initialList = env.getSubnets();
    if(initialList->size() == 0)
    {
        (*out) << "No subnet, no network graph." << endl;
        return NULL;
    }
    
    list<Subnet*> toProcess(*initialList); // Copies the list of subnets
    list<Aggregate*> aggregates; // As long as there are subnets, there will always be at least one
    
    map<string, list<Subnet*>* > subnetsByTrail;
    list<Subnet*> homeSubnets; // Subnets with no trail (subnets close to this computer)
    list<Subnet*> rule3Subnets; // Subnets
    while(toProcess.size() > 0)
    {
        Subnet *sub = toProcess.front();
        toProcess.pop_front();
        
        Trail &subTrail = sub->getTrail();
        if(subTrail.isVoid())
        {
            homeSubnets.push_back(sub);
            continue;
        }
        else if(subTrail.isEchoing())
        {
            rule3Subnets.push_back(sub);
            continue;
        }
        string trailStr = subTrail.toString();
        
        // Looks for existing list in the map; creates it if not found
        map<string, list<Subnet*>* >::iterator res = subnetsByTrail.find(trailStr);
        if(res != subnetsByTrail.end())
        {
            list<Subnet*> *existingList = res->second;
            existingList->push_back(sub);
        }
        else
        {
            list<Subnet*> *newList = new list<Subnet*>();
            newList->push_back(sub);
            
            subnetsByTrail.insert(pair<string, list<Subnet*>* >(trailStr, newList));
        }
    }
    
    // Creates the aggregate for "void" subnets
    if(homeSubnets.size() > 0)
    {
        Aggregate *newAgg = new Aggregate(homeSubnets);
        aggregates.push_back(newAgg);
    }
    
    // Creates the (first) aggregates
    while(subnetsByTrail.size() > 0)
    {
        map<string, list<Subnet*>* >::iterator it = subnetsByTrail.begin();
        list<Subnet*> *toAggregate = it->second;
        toAggregate->sort(Subnet::compare);
        Trail &refTrail = toAggregate->front()->getTrail();
        
        // Creates the aggregate from this first list
        Aggregate *newAgg = new Aggregate((*toAggregate));
        subnetsByTrail.erase(it);
        delete toAggregate;
        
        // Do we have flickering trails ? If yes, we need to look for associated lists
        if(subInfAliases != NULL && refTrail.isFlickering() && refTrail.getNbAnomalies() == 0)
        {
            Alias *assocAlias = subInfAliases->findAlias(refTrail.getLastValidIP());
            if(assocAlias != NULL)
            {
                list<AliasedInterface> *peers = assocAlias->getInterfacesList();
                for(list<AliasedInterface>::iterator j = peers->begin(); j != peers->end(); ++j)
                {
                    InetAddress peer = (InetAddress) (*(j->ip));
                    if(peer == refTrail.getLastValidIP())
                        continue; // Just in case
                    
                    stringstream madeUpTrail;
                    madeUpTrail << "[" << peer << "]";
                    string oTrail = madeUpTrail.str();
                    
                    map<string, list<Subnet*>* >::iterator res = subnetsByTrail.find(oTrail);
                    if(res != subnetsByTrail.end()) // Other list with flickering peer exists
                    {
                        list<Subnet*> *additionalSubnets = res->second;
                        newAgg->addSubnets((*additionalSubnets));
                        subnetsByTrail.erase(res);
                        delete additionalSubnets;
                    }
                }
            }
        }
        
        aggregates.push_back(newAgg);
    }
    
    // Deals with the special case of "rule 3" subnets
    if(rule3Subnets.size() > 0)
    {
        // 1) Finds pre-echoing IPs
        for(list<Subnet*>::iterator i = rule3Subnets.begin(); i != rule3Subnets.end(); ++i)
            (*i)->findPreTrailIPs();
        
        // 2) Further discriminates the subnets by using TTL distance and offset of pre-trail IPs
        map<string, list<Subnet*>* > sortedSubs;
        while(rule3Subnets.size() > 0)
        {
            Subnet *sub = rule3Subnets.front();
            rule3Subnets.pop_front();
            stringstream ss;
            ss << sub->getPivotTTL() << "," << sub->getPreTrailOffset();
            string classifyingLabel = ss.str();
            
            map<string, list<Subnet*>* >::iterator res = sortedSubs.find(classifyingLabel);
            if(res != sortedSubs.end())
            {
                list<Subnet*> *existingList = res->second;
                existingList->push_back(sub);
            }
            else
            {
                list<Subnet*> *newList = new list<Subnet*>();
                newList->push_back(sub);
                
                sortedSubs.insert(pair<string, list<Subnet*>* >(classifyingLabel, newList));
            }
        }
        
        // 3) For each sub-group of subnets...
        while(sortedSubs.size() > 0)
        {
            map<string, list<Subnet*>* >::iterator it = sortedSubs.begin();
            list<Subnet*> *subGroup = it->second;
            sortedSubs.erase(it);
            if(subGroup == NULL || subGroup->size() == 0)
            {
                if(subGroup != NULL)
                    delete subGroup;
                continue;
            }
            
            // 3.1) Clusters pre-trail IPs to have exhaustive neighborhoods
            IPClusterer *clusterer = new IPClusterer();
            for(list<Subnet*>::iterator i = subGroup->begin(); i != subGroup->end(); ++i)
                clusterer->update((*i)->getPreTrailIPs());
            
            // 3.2) Just like during initial aggregation, builds a map string -> list of subnets
            map<string, list<Subnet*>* > subnetsByPretrail;
            while(subGroup->size() > 0)
            {
                Subnet *sub = subGroup->front();
                InetAddress needleSub = sub->getPreTrailIPs().front();
                subGroup->pop_front();
                
                // Shouldn't produce an empty string by design
                string pretrailStr = clusterer->getClusterString(needleSub);
                
                map<string, list<Subnet*>* >::iterator res = subnetsByPretrail.find(pretrailStr);
                if(res != subnetsByPretrail.end())
                {
                    list<Subnet*> *existingList = res->second;
                    existingList->push_back(sub);
                }
                else if(pretrailStr.length() > 0) // Just in case
                {
                    list<Subnet*> *newList = new list<Subnet*>();
                    newList->push_back(sub);
                    
                    subnetsByPretrail.insert(pair<string, list<Subnet*>* >(pretrailStr, newList));
                }
            }
            delete subGroup;
            
            // 3.3) Creates the aggregates
            while(subnetsByPretrail.size() > 0)
            {
                map<string, list<Subnet*>* >::iterator itBis = subnetsByPretrail.begin();
                list<Subnet*> *toAggregate = itBis->second;
                if(toAggregate->size() == 0) // Just in case
                {
                    subnetsByPretrail.erase(itBis);
                    delete toAggregate;
                    continue;
                }
                toAggregate->sort(Subnet::compare);
                
                // Gets an IP to get the cluster of pre-echoing IPs
                InetAddress needle(0);
                unsigned short preEchoingOffset = 0;
                for(list<Subnet*>::iterator i = toAggregate->begin(); i != toAggregate->end(); ++i)
                {
                    list<InetAddress> curPreEcho = (*i)->getPreTrailIPs();
                    if(curPreEcho.size() > 0)
                    {
                        needle = curPreEcho.front();
                        preEchoingOffset = (*i)->getPreTrailOffset();
                        break;
                    }
                }
                
                if(needle == InetAddress(0)) // Artefact (normally, it shouldn't occur)
                {
                    subnetsByPretrail.erase(itBis);
                    delete toAggregate;
                    continue;
                }
                
                list<InetAddress> preEchoing = clusterer->getCluster(needle);
                if(preEchoing.size() == 0) // Ditto: shouldn't occur by design
                {
                    subnetsByPretrail.erase(itBis);
                    delete toAggregate;
                    continue;
                }
                
                // Creates the aggregate
                Aggregate *newAgg = new Aggregate((*toAggregate));
                newAgg->setPreEchoing(preEchoing);
                newAgg->setPreEchoingOffset(preEchoingOffset);
                aggregates.push_back(newAgg);
                subnetsByPretrail.erase(itBis);
                delete toAggregate;
            }
            
            delete clusterer;
        }
    }
    
    (*out) << "Done." << endl;
    
    /*
     * Step 2: peer discovery
     * ----------------------
     * The newly created aggregates are processed in order to find potential peers, using the 
     * partial routes attached to interfaces of the aggregated subnets. This part of the inference 
     * is entirely carried out by the Aggregate class via the discoverPeerIPs() method (see this 
     * class for more details).
     *
     * A second method adjustPeerIPs() is also called on the aggregates, with the alias set of 
     * subnet inference as a parameter (if any). The purpose of this method is to find if any 
     * peer IP belongs to a previously discovered alias, and if any, replace it with the first IP 
     * (w.r.t. IP scope) of the alias, then remove the duplicates (if any). The main motivation 
     * for doing this is that there's no guarantee an alias will re-appear at this algorithmic 
     * step, meaning we could have several peer IPs not belonging to the same final peer that 
     * would be referring to a same aggregate (because subnets built on flickering IPs end up in 
     * a same aggregate, identified with all said flickering IPs).
     *
     * Replacing all IPs of a former alias by a single IP ensures that
     * -each aggregate will be mapped to single peer, 
     * -by the property of alias transitivity, aliasing more IPs to the single IP is equivalent to 
     *  aliasing the same IPs with any IP of the former alias.
     */
    
    aggregates.sort(Aggregate::compare);
    unsigned int directPeers = 0, remotePeers = 0, noPeers = 0, adjustedPeers = 0;
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        Aggregate *curAgg = (*i);
        curAgg->discoverPeerIPs();
        
        size_t nbPeers = curAgg->getNbPeerIPs();
        unsigned short offset = curAgg->getPeersOffset();
        if(nbPeers > 0)
        {
            if(offset > 0)
                remotePeers++;
            else
                directPeers++;
        }
        else
            noPeers++;
        
        if(nbPeers > 0 && subInfAliases != NULL)
            if(curAgg->adjustPeerIPs(subInfAliases))
                adjustedPeers++;
    }
    
    // Some console display to inform user about peer IPs discovered so far.
    if(noPeers < aggregates.size())
    {
        if(directPeers > 0)
        {
            (*out) << "Discovered direct peer IP(s) for ";
            if(directPeers > 1)
                (*out) << directPeers << " aggregates";
            else
                (*out) << "one aggregate";
            (*out) << "." << endl;
        }
        if(remotePeers > 0)
        {
            (*out) << "Discovered remote peer IP(s) for ";
            if(remotePeers > 1)
                (*out) << remotePeers << " aggregates";
            else
                (*out) << "one aggregate";
            (*out) << "." << endl;
        }
        if(noPeers > 0)
        {
            if(noPeers > 1)
                (*out) << noPeers << " aggregates don't have any peer." << endl;
            else
                (*out) << "One aggregate doesn't have any peer." << endl;
        }
        if(adjustedPeers > 0)
        {
            (*out) << "Peer IPs have been adjusted for ";
            if(adjustedPeers > 1)
                (*out) << adjustedPeers << " aggregates";
            else
                (*out) << "one aggregate";
            (*out) << " using previously discovered aliases." << endl;
        }
    }
    else
    {
        (*out) << "Didn't discover any peer at all." << endl;
    }
    
    /*
     * Step 3: peer disambiguation
     * ---------------------------
     * Due to traffic engineering, it's possible several peers for an aggregate are actually 
     * interfaces from a same device, i.e. the device is a convergence point in a load-balancer 
     * and interfaces are different from a measurement to another. To keep the topology inference 
     * as thorough as possible, an IPClusterer is used to discover sets of peer IPs that could be 
     * from a same device (either because appearing for a same aggregate, either by transitivity) 
     * and submit them to alias resolution to guarantee the peers come from distinct devices or 
     * the opposite.
     */
    
    // 1) Clustering peer IPs to get sets of alias candidates
    IPClusterer *aliasCandis = new IPClusterer();
    
    // N.B.: if no peer was discovered, the following code will basically do nothing
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
        aliasCandis->update((*i)->listInitialPeerIPs());
    list<list<InetAddress> > clusters = aliasCandis->listClusters();
    
    // Turns InetAddress into IPTableEntry objects, only keep sets of >= 2 candidates
    list<list<IPTableEntry*> > candis;
    for(list<list<InetAddress> >::iterator i = clusters.begin(); i != clusters.end(); ++i)
    {
        list<InetAddress> cluster = (*i);
        if(cluster.size() < 2)
            continue;
        cluster.sort(InetAddress::smaller);
        
        list<IPTableEntry*> hypothesis;
        for(list<InetAddress>::iterator j = cluster.begin(); j != cluster.end(); ++j)
        {
            IPTableEntry *relatedEntry = dict->lookUp((*j));
            if(relatedEntry == NULL) // Shouldn't occur by design, just in case
                continue;
            hypothesis.push_back(relatedEntry);
        }
        candis.push_back(hypothesis);
    }
    
    delete aliasCandis;
    
    // Next steps only occurs if we have aggregates with more than one peer (+ misc. IPs).
    AliasSet *newAliases = NULL;
    if(candis.size() > 0)
    {
        (*out) << "\nThere are aggregates of subnets with more than one peer and/or with ";
        (*out) << "miscellaneous IPs observed at the same distance.\n";
        (*out) << "SAGE will conduct alias resolution to test whether these IPs belong to the ";
        (*out) << "same devices or not (peer disambiguation).\n" << endl;
    
        // 2) Collecting hints for the clusters
        AliasHintsCollector ahc(env);
        for(list<list<IPTableEntry*> >::iterator i = candis.begin(); i != candis.end(); ++i)
        {
            list<IPTableEntry*> hypothesis = (*i);
            if(hypothesis.size() < 2) // Shouldn't occur by design (consequence of problem above)
                continue;
            (*out) << "Collecting hints for ";
            for(list<IPTableEntry*>::iterator j = hypothesis.begin(); j != hypothesis.end(); ++j)
            {
                if(j != hypothesis.begin())
                    (*out) << ", ";
                (*out) << *(*j);
            }
            (*out) << "... " << std::flush;
            ahc.setIPsToProbe(hypothesis);
            ahc.collect();
        }
        
        // 3) Resolving the clusters
        newAliases = new AliasSet(AliasSet::GRAPH_BUILDING);
        AliasResolver ar(env);
        for(list<list<IPTableEntry*> >::iterator i = candis.begin(); i != candis.end(); ++i)
        {
            list<IPTableEntry*> hypothesis = (*i);
            if(hypothesis.size() < 2) // Shouldn't occur by design (consequence of problem above)
                continue;
            
            list<Alias*> discoveredAliases = ar.resolve(hypothesis, env.usingStrictAliasResolution());
            newAliases->addAliases(discoveredAliases, true); // Only actual aliases (#IPs >= 2) should appear
        }
        env.addAliasSet(newAliases);
        
        size_t nbAliases = newAliases->getNbAliases();
        if(nbAliases > 0)
        {
            if(nbAliases > 1)
                (*out) << nbAliases << " aliases could be discovered." << endl;
            else
                (*out) << "One alias could be discovered." << endl;
        }
        else
            (*out) << "No alias could be discovered." << endl;
        
        /*
         * Additional step: blindspot detection
         * ------------------------------------
         * Again, because of traffic engineering, an IP interface which appears in routes can be 
         * at the same time an alias of a peer IP and not be denoting any neighborhood, meaning 
         * it can't be properly identified as a peer. Such an IP is called a "blindspot" and is a 
         * problem for properly linking neighborhoods together. Using the IP dictionary and the 
         * aliases that have been just collected, SAGE identifies these "blindspots" in order to 
         * fix the shortcomings of peer discovery.
         */
        
        unsigned int withBlindspots = 0;
        for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
        {
            Aggregate *curAgg = (*i);
            curAgg->discoverBlindspots(dict, newAliases);
            if(curAgg->getNbBlindspots() > 0)
                withBlindspots++;
        }
        
        if(withBlindspots > 0)
        {
            (*out) << "\n";
            (*out) << "Identified ";
            if(withBlindspots > 1)
                (*out) << withBlindspots << " aggregates ";
            else
                (*out) << "one aggregate ";
            (*out) << "with blindspots (previously unidentified peers)." << endl;
            
            unsigned int nbImproved = 0;
            for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
            {
                Aggregate *curAgg = (*i);
                unsigned short curPeersOffset = curAgg->getPeersOffset();
                if(curPeersOffset == 0)
                    continue;
                
                curAgg->improvePeerIPs(env);
                if(curAgg->getPeersOffset() < curPeersOffset)
                    nbImproved++;
            }
            
            if(nbImproved > 0)
            {
                if(nbImproved > 1)
                    (*out) << nbImproved << " neighborhoods got better peer(s) via blindspot detection." << endl;
                else
                    (*out) << "One neighborhood got better peer(s) via blindspot detection." << endl;
            }
        }
    }
    
    /*
     * Step 4: vertice creation
     * ------------------------
     * The next step consists in creating the final vertices of the graph, knowing that due to 
     * peer disambiguation, there might be one or several neighborhoods that will be made of 
     * several aggregates (called clusters). This operation needs to be carefully conducted and 
     * involves several temporar data structures (in particular, "Peer" and "GraphPeers"). The 
     * general idea is to instantiate Peer objects which abstract "peering" aggregates and 
     * clusters before they become vertices and which are pointed to by every relevant aggregate. 
     * Afterwards, aggregates and their peers are "split" as new Vertice objects (aggregates which 
     * don't act as peers are identified as "termini" and become "Node" vertices) and GraphPeers 
     * objects, with maps keeping track of which Peer corresponds to which Vertice and which 
     * Vertice corresponds to which GraphPeers. It's only after building all Vertice objects that 
     * the peering between vertices (i.e., the peers of a Vertice are known as Vertice objects 
     * directly, rather than Peer objects) is achieved by using the GraphPeers objects.
     */
     
    (*out) << "Building the vertices of the graph... " << std::flush;
    
    /*
     * General remark: like other pieces of code above, the next instructions will do nothing 
     * besides creating "terminus" vertices if no peer was discovered; running this code should 
     * therefore be safe in all situations.
     */
    
    // Creates a map of Peer objects, using peers of aggregates and the newly discovered aliases
    map<InetAddress, Peer*> peersMap; // Map: one IP -> Peer object that contains it
    list<Peer*> allPeers; // To ease deletion later
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        list<InetAddress> peerIPs = (*i)->listTruePeerIPs();
        for(list<InetAddress>::iterator j = peerIPs.begin(); j != peerIPs.end(); ++j)
        {
            InetAddress curIP = (*j);
            map<InetAddress, Peer*>::iterator res = peersMap.find(curIP);
            if(res != peersMap.end())
                continue; // A peer is already mapped to this IP, and by design, there's no overlap
            
            Alias *peerAlias = NULL;
            if(newAliases != NULL)
                peerAlias = newAliases->findAlias(curIP);
            
            if(peerAlias != NULL)
            {
                Peer *Gynt = new Peer(peerAlias);
                allPeers.push_back(Gynt);
                list<AliasedInterface> *IPs = peerAlias->getInterfacesList();
                for(list<AliasedInterface>::iterator k = IPs->begin(); k != IPs->end(); ++k)
                {
                    AliasedInterface curAlias = (*k);
                    InetAddress curInterface = (InetAddress) *(curAlias.ip);
                    peersMap.insert(pair<InetAddress, Peer*>(curInterface, Gynt));
                }
            }
            else
            {
                Peer *Gynt = new Peer(curIP);
                allPeers.push_back(Gynt);
                peersMap.insert(pair<InetAddress, Peer*>(curIP, Gynt));
            }
        }
    }
    
    /*
     * With a single loop: 
     * -lists the final peers (as Peer objects) for all aggregates, 
     * -identifies "terminus" aggregates (neighborhoods that won't be peers to others), 
     * -with the non-terminus neighborhoods, fills a map InetAddress -> Aggregate that will be 
     *  used to build the final peer neighborhoods/vertices of the graph.
     */
    
    map<InetAddress, Aggregate*> peerAggs;
    list<Aggregate*> termini;
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        Aggregate *curAgg = (*i);
        curAgg->listFinalPeers(peersMap);
        
        list<InetAddress> IDs = curAgg->listIdentifyingIPs();
        if(IDs.size() > 0)
        {
            bool isPeer = false;   
            for(list<InetAddress>::iterator j = IDs.begin(); j != IDs.end(); ++j)
            {
                InetAddress curIP = (*j);
                map<InetAddress, Peer*>::iterator found = peersMap.find(curIP);
                if(found != peersMap.end())
                {
                    peerAggs.insert(pair<InetAddress, Aggregate*>(curIP, curAgg));
                    isPeer = true;
                }
            }
            if(!isPeer)
                termini.push_back(curAgg);
        }
        else
        {
            termini.push_back(curAgg);
        }
    }
    termini.sort(Aggregate::compare);
    
    // Maps filled and used to create vertices and peer them all together after
    map<Peer*, Vertice*> verticesMap;
    map<Vertice*, GraphPeers*> peeringMap;
    
    // Full list of vertices
    list<Vertice*> vertices;
    
    // First, creates the "termini" vertices (Node instances, by design)
    for(list<Aggregate*>::iterator i = termini.begin(); i != termini.end(); ++i)
    {
        Aggregate *cur = (*i);
        Vertice *terminus = new Node(cur);
        vertices.push_back(terminus);
        
        if(!cur->hasPeers())
            continue;
        
        GraphPeers *peering = new GraphPeers(cur);
        peeringMap.insert(pair<Vertice*, GraphPeers*>(terminus, peering));
    }
    
    // No peers, only "terminus" aggregates: stops here
    if(peeringMap.size() == 0)
    {
        (*out) << "Done." << endl;
        
        // Builds a simplified graph, only consisting of "gates" (due to absence of peers)
        Graph *graph = new Graph();
        for(list<Vertice*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
            graph->addGate((*i));
        
        /*
         * Removes utility objects from memory. By design, in this case, there should be no Peer 
         * or GraphPeers objects. Therefore, only Aggregate objects needs to be deleted.
         */
        
        for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
            delete (*i);
        aggregates.clear();
        
        Pioneer p(env);
        p.visit(graph); // This will number the vertices
        return graph;
    }
    
    // Second, the peers (can be Node or Cluster)
    for(list<Peer*>::iterator i = allPeers.begin(); i != allPeers.end(); ++i)
    {
        Peer *curPeer = (*i);
        
        // List aggregates (at least one; there can be only one with several IPs due to blindspots)
        list<InetAddress> *IPs = curPeer->getInterfaces();
        list<Aggregate*> aggs;
        list<InetAddress> blindspots;
        for(list<InetAddress>::iterator j = IPs->begin(); j != IPs->end(); ++j)
        {
            map<InetAddress, Aggregate*>::iterator found = peerAggs.find((*j));
            if(found != peerAggs.end())
                aggs.push_back(found->second);
            else
                blindspots.push_back((*j));
        }
        
        if(aggs.size() == 0)
            continue; // Unlikely by design; just in case
        
        Vertice *newVertice = NULL;
        
        // Creates a Node
        if(aggs.size() == 1 && blindspots.size() == 0)
        {
            Aggregate *a = aggs.front();
            newVertice = new Node(a);
            
            if(a->hasPeers())
            {
                GraphPeers *peering = new GraphPeers(a);
                peeringMap.insert(pair<Vertice*, GraphPeers*>(newVertice, peering));
            }
        }
        // Creates a Cluster; by design, in this case, we have 2+ aggs or at least one blindspot
        else
        {
            Cluster *newCluster = new Cluster(aggs);
            if(blindspots.size() > 0)
                newCluster->addBlindspots(blindspots); // For completeness
            if(subInfAliases != NULL)
                newCluster->addFlickeringAliases(subInfAliases); // Ditto
            newVertice = newCluster;
            
            bool clusterWithPeers = false;
            for(list<Aggregate*>::iterator j = aggs.begin(); j != aggs.end(); j++)
            {
                if((*j)->hasPeers())
                {
                    clusterWithPeers = true;
                    break;
                }
            }
            
            if(clusterWithPeers)
            {
                GraphPeers *peering = new GraphPeers(aggs);
                peeringMap.insert(pair<Vertice*, GraphPeers*>(newVertice, peering));
            }
        }
        
        vertices.push_back(newVertice);
        verticesMap.insert(pair<Peer*, Vertice*>(curPeer, newVertice));
    }
    
    // Binds all vertices together, using the various maps that were built
    for(list<Vertice*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
    {
        Vertice *v = (*i);
        map<Vertice*, GraphPeers*>::iterator peersLookUp = peeringMap.find(v);
        if(peersLookUp != peeringMap.end()) // Peerless (gate) otherwise
        {
            v->setPeers(peersLookUp->second, verticesMap);
            delete peersLookUp->second;
        }
    }
    peeringMap.clear();
    
    // Cleans the memory from Aggregate and Peer objects
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
        delete (*i);
    aggregates.clear();
    
    for(list<Peer*>::iterator i = allPeers.begin(); i != allPeers.end(); ++i)
        delete (*i);
    allPeers.clear();
    
    (*out) << "Done." << endl;
    
    /*
     * Step 5: edge creation
     * ---------------------
     * The final graph object is created and initially filled with two pieces of information: 
     * -the "gates" of the topology (vertices without peers), 
     * -the mappings between a given subnet and the vertice arounds which it appears.
     * Then, vertices created during the previous step are processed in order to create the edges 
     * which connect them with their peers, effectively binding all vertices together. The very 
     * final step consists in running a first visitor of the graph, "Pionneer", which will number 
     * the vertices in order to simplify the text output.
     */
    
    (*out) << "Building the edges of the graph... " << std::flush;
    
    Graph *graph = new Graph();
    list<Vertice*> toConnect;
    while(vertices.size() > 0)
    {
        Vertice *v = vertices.front();
        vertices.pop_front();
        graph->createSubnetMappingsTo(v);
        
        if(v->getNbPeers() == 0)
            graph->addGate(v);
        
        toConnect.push_back(v);
    }
    
    while(toConnect.size() > 0)
    {
        Vertice *v = toConnect.front();
        toConnect.pop_front();
        
        list<Vertice*> *vPeers = v->getPeers();
        unsigned short vPeersOffset = v->getPeersOffset();
        // v has direct peers: the edges with its peers will be direct or indirect links
        if(vPeersOffset == 0)
        {
            list<Trail> *vT = v->getTrails(); // T for Trails
            for(list<Vertice*>::iterator j = vPeers->begin(); j != vPeers->end(); ++j)
            {
                Vertice *u = (*j);
                
                // Do we have a direct connecting subnet ?
                Subnet *medium = NULL;
                if(vT->size() == 0)
                {
                    // "Rule 3" neighborhood: checks pre-echoing IPs
                    if(Node *node = dynamic_cast<Node*>(v))
                    {
                        list<InetAddress> *pE = node->getPreEchoing();
                        for(list<InetAddress>::iterator k = pE->begin(); k != pE->end(); k++)
                        {
                            medium = u->getConnectingSubnet((*k));
                            if(medium != NULL)
                                break;
                        }
                    }
                }
                else
                {
                    // Checks trails
                    for(list<Trail>::iterator k = vT->begin(); k != vT->end(); ++k)
                    {
                        medium = u->getConnectingSubnet(k->getLastValidIP());
                        if(medium != NULL)
                            break;
                    }
                }
                
                // Yes: establishes a direct link between u and v
                if(medium != NULL)
                {
                    u->addEdge(new DirectLink(u, v, medium));
                }
                // No: creates an indirect link; looks for a medium among the mappings of the graph
                else
                {
                    SubnetVerticeMapping m = SubnetVerticeMapping(); // Empty mapping
                    
                    if(vT->size() == 0)
                    {
                        // "Rule 3" neighborhood: checks pre-echoing IPs
                        if(Node *node = dynamic_cast<Node*>(v))
                        {
                            list<InetAddress> *pE = node->getPreEchoing();
                            for(list<InetAddress>::iterator k = pE->begin(); k != pE->end(); k++)
                            {
                                m = graph->getSubnetContaining((*k));
                                if(!m.isEmpty())
                                    break;
                            }
                        }
                    }
                    else
                    {
                        // Checks trails
                        for(list<Trail>::iterator k = vT->begin(); k != vT->end(); ++k)
                        {
                            m = graph->getSubnetContaining(k->getLastValidIP());
                            if(!m.isEmpty())
                                break;
                        }
                    }
                    
                    if(m.isEmpty())
                        u->addEdge(new IndirectLink(u, v));
                    else
                        u->addEdge(new IndirectLink(u, v, m));
                }
            }
            continue;
        }
        
        /*
         * Otherwise, there can only exist remote links between v and its peers. A specific method 
         * of the Vertice class is used to easily discover the unique routes which exist between 
         * them and to store them as additional data in the RemoteLink object.
         */
        
        for(list<Vertice*>::iterator j = vPeers->begin(); j != vPeers->end(); ++j)
        {
            Vertice *u = (*j);
            list<Trail> *uT = u->getTrails();
            
            list<vector<RouteHop> > uniqueRoutes = v->findUniqueRoutes(*uT);
            u->addEdge(new RemoteLink(u, v, uniqueRoutes));
        }
    }
    
    (*out) << "Done." << endl;
    
    Pioneer p(env);
    p.visit(graph); // This will number the vertices
    return graph;
}
