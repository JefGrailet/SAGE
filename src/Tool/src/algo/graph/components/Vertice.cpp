/*
 * Vertice.cpp
 *
 *  Created on: Nov 22, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Vertice.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "Vertice.h"

Vertice::Vertice()
{
    ID = 0; // Set later
    peersOffset = 0;
}

Vertice::~Vertice()
{
    // Other pointed objects (e.g. subnets, peers) are deleted by other parts of the program
    for(list<Alias*>::iterator i = aliases.begin(); i != aliases.end(); ++i)
        delete (*i);
    aliases.clear();
    
    for(list<Edge*>::iterator i = edges.begin(); i != edges.end(); ++i)
        delete (*i);
    edges.clear();
}

unsigned int Vertice::getSubnetCoverage()
{
    unsigned int totalIPs = 0;
    for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
        totalIPs += (*i)->getTotalCoveredIPs();
    return totalIPs;
}

void Vertice::setPeers(GraphPeers *peersInfo, map<Peer*, Vertice*> peersMap)
{
    peersOffset = peersInfo->offset;
    for(list<Peer*>::iterator i = peersInfo->peers.begin(); i != peersInfo->peers.end(); ++i)
    {
        map<Peer*, Vertice*>::iterator res = peersMap.find((*i));
        if(res != peersMap.end())
            peers.push_back(res->second);
    }
    
    /*
     * No sorting here, because Vertice objects are only numbered when the graph is finally 
     * built, and the Peer objects normally already come sorted by their identifying IPs. The 
     * actual sorting takes place in Aggregate::listFinalPeers(); sorted peers are then copied 
     * in the GraphPeers object with the same order.
     *
     * However, the final list of peers is filtered in order to remove potential pointer(s) to 
     * this object. Indeed, while peer scanning has efficient mechanisms to prevent self-peering, 
     * the construction of Cluster vertices from several aggregates can still lead to 
     * self-peering: because of TE (Traffic Engineering), an aggregate can peer to another 
     * aggregate that will be listed alongside it in order to build a cluster. This filter also 
     * works as a kind of final consistency check for all vertices (and also because method 
     * override doesn't exist prior to C++11...).
     */
    
    for(list<Vertice*>::iterator i = peers.begin(); i != peers.end(); ++i)
        if((*i) == this)
            peers.erase(i--);
}

Subnet* Vertice::getConnectingSubnet(InetAddress nextHop)
{
    for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        Subnet *cur = (*i);
        if(cur->contains(nextHop))
            return cur;
    }
    return NULL;
}

list<vector<RouteHop> > Vertice::findUniqueRoutes(list<Trail> peerIPs)
{
    // Lists all routes
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
    
    // Lists routes where the peer IP matches the argument(s)
    list<vector<RouteHop> > nonUnique;
    for(list<Trail>::iterator i = peerIPs.begin(); i != peerIPs.end(); ++i)
    {
        InetAddress curIP = i->getLastValidIP();
        for(list<SubnetInterface>::iterator j = withRoutes.begin(); j != withRoutes.end(); ++j)
        {
            vector<RouteHop> route = j->partialRoute;
            
            /*
             * By design, the first hop in the route is a peer IP or a TTL=1 hop (both if the 
             * target topology is next to the vantage point). The code stores in "nonUnique" all 
             * routes which start with "curIP" (which should be, also by design, a peer IP) and 
             * which contains more than one hop.
             */
            
            if(route[0].ip == curIP && route.size() > 1)
                nonUnique.push_back(route);
        }
    }
    
    list<vector<RouteHop> > resList;
    if(nonUnique.size() == 0)
        return resList; // Cuts execution short if no intermediate routes
    
    // Using a side method and a map (route as a string -> route), isolates the unique routes
    map<string, vector<RouteHop> > unique;
    for(list<vector<RouteHop> >::iterator i = nonUnique.begin(); i != nonUnique.end(); ++i)
    {
        vector<RouteHop> route = (*i);
        
        stringstream routeStream;
        for(unsigned short j = 1; j < route.size(); j++) // 1st hop skipped because it's a peer IP
        {
            if(j > 1)
                routeStream << ", ";
            routeStream << route[j];
        }
        string routeStr = routeStream.str();
        
        map<string, vector<RouteHop> >::iterator res = unique.find(routeStr);
        if(res == unique.end())
            unique.insert(pair<string, vector<RouteHop> >(routeStr, route));
    }
    
    // Turns the map into a list and returns it
    for(map<string, vector<RouteHop> >::iterator i = unique.begin(); i != unique.end(); ++i)
        resList.push_back(i->second);
    return resList;
}

list<IPTableEntry*> Vertice::getAliasCandidates()
{
    list<IPTableEntry*> res;
    
    for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        list<IPTableEntry*> contrapivots = (*i)->getContrapivots();
        if(contrapivots.size() > 0)
            res.insert(res.end(), contrapivots.begin(), contrapivots.end());
    }
    
    res.sort(IPTableEntry::compare);
    return res;
}

void Vertice::storeAliases(list<Alias*> newAliases)
{
    bool needsSorting = false;
    if(aliases.size() > 0)
        needsSorting = true;
    aliases.insert(aliases.end(), newAliases.begin(), newAliases.end());
    if(needsSorting)
        aliases.sort(Alias::compare);
}

bool Vertice::smallerID(Vertice *v1, Vertice *v2)
{
    return v1->getID() < v2->getID();
}

string Vertice::peersToString()
{
    if(peers.size() == 0)
        return "No peers; gate to the topology.\n";
    
    // Sorting needed ?
    if(peers.front()->getID() != 0)
        peers.sort(Vertice::smallerID);
    
    stringstream ss;
    if(peers.size() > 1)
    {
        ss << "Peers";
        if(peersOffset > 0)
            ss << " (offset=" << peersOffset << ")";
        ss << ":\n";
        for(list<Vertice*>::iterator i = peers.begin(); i != peers.end(); ++i)
        {
            Vertice *cur = (*i);
            ss << cur->getFullLabel() << "\n";
        }
    }
    else
    {
        ss << "Peer";
        if(peersOffset > 0)
            ss << " (offset=" << peersOffset << ")";
        ss << ":\n" << peers.front()->getFullLabel() << "\n";
    }
    return ss.str();
}
