/*
 * Cassini.cpp
 *
 *  Created on: Dec 12, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Cassini.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition

#include "Cassini.h"
#include "../components/Node.h"
#include "../components/Cluster.h"
#include "../components/DirectLink.h"
#include "../components/IndirectLink.h"
#include "../components/RemoteLink.h"

Cassini::Cassini(Environment &env) : Voyager(env)
{
    this->reset();
}

Cassini::~Cassini()
{
}

void Cassini::reset()
{
    visited.clear();
    reachableNodes.clear();
    visitedGates.clear();
    
    degreeAcc = 0;
    totalNodes = 0;
    
    maxInDegree = 0;
    maxOutDegree = 0;
    maxTotDegree = 0;
    
    maxNbSubnets = 0;
    
    totalSubnets = 0;
    totalCoveredIPs = 0;
    
    maxAliasAmount = 0;
    nbSingleAlias = 0;
    maxAliasSize = 0;
    totalAliases = 0;
    
    nbDirectLinks = 0;
    nbIndirectLinks = 0;
    nbRemoteLinks = 0;
    nbLinksWithMedium = 0;
}

void Cassini::visit(Graph *g)
{
    // Resets everything if something was set
    if(visited.size() > 0)
        this->reset();
    
    // Sets the "visited" array
    totalNodes = g->getNbVertices();
    for(unsigned int i = 0; i < totalNodes; ++i)
        visited.push_back(false);
    
    // Visits the graph (node metrics)
    list<Vertice*> *gates = g->getGates();
    for(list<Vertice*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive1((*i));
    
    // For the second visit, a second array is used to now which nodes were visited last iteration
    vector<bool> visitedPrevious;
    for(unsigned int i = 0; i < totalNodes; ++i)
    {
        visited[i] = false;
        visitedPrevious.push_back(false);
    }
    
    // Visits the graph a second time to isolate connected components
    for(list<Vertice*>::iterator i = gates->begin(); i != gates->end(); ++i)
    {
        if(visitedPrevious[(*i)->getID() - 1])
            continue;
        
        // Visits recursively, listing the gates of the component in the process
        list<Vertice*> componentGates;
        this->visitRecursive2((*i), componentGates);
        
        // Counts the amount of visited nodes
        unsigned int nbNodes = 0;
        for(unsigned int j = 0; j < totalNodes; ++j)
            if(visited[j] && !visitedPrevious[j])
                nbNodes++;
        
        reachableNodes.push_back(nbNodes);
        visitedGates.push_back(componentGates);
        
        // "Commits" all currently visited nodes in visitedPrevious
        for(unsigned int j = 0; j < totalNodes; ++j)
            visitedPrevious[j] = visited[j];
    }
    
    // Visits a graph a third and final time to evaluate the depth of each component
    for(list<list<Vertice*> >::iterator i = visitedGates.begin(); i != visitedGates.end(); ++i)
    {
        list<Vertice*> gates = (*i);
        
        unsigned int depth = 0;
        for(list<Vertice*>::iterator j = gates.begin(); j != gates.end(); ++j)
        {
            // Resets visited for each visit from a given gate
            for(unsigned int i = 0; i < totalNodes; ++i)
                visited[i] = false;
            
            // Visits the component from this gate
            unsigned int curLength = this->visitRecursive3((*j), 0);
            if(curLength > depth)
                depth = curLength;
        }
        
        componentDepth.push_back(depth);
    }
}

void Cassini::visitRecursive1(Vertice *v)
{
    unsigned int ID = v->getID();
    if(visited[ID - 1])
        return;
    
    visited[ID - 1] = true;
    
    // Deals with each kind of degree, first "in" degree (#edges coming in)
    unsigned short inDegree = v->getInDegree();
    if(inDegree > maxInDegree)
    {
        maxInDegree = inDegree;
        maxInIDs.clear();
        maxInIDs.push_back(ID);
    }
    else if(inDegree == maxInDegree)
        maxInIDs.push_back(ID);
    
    // "Out" degree (#edges coming out)
    unsigned short outDegree = v->getOutDegree();
    if(outDegree > maxOutDegree)
    {
        maxOutDegree = outDegree;
        maxOutIDs.clear();
        maxOutIDs.push_back(ID);
    }
    else if(outDegree == maxOutDegree)
        maxOutIDs.push_back(ID);
    
    // Full degree (in + out)
    unsigned short totDegree = inDegree + outDegree;
    if(totDegree > maxTotDegree)
    {
        maxTotDegree = totDegree;
        maxTotIDs.clear();
        maxTotIDs.push_back(ID);
    }
    else if(totDegree == maxTotDegree)
        maxTotIDs.push_back(ID);
    
    degreeAcc += inDegree;
    
    // Subnets metrics
    unsigned short nbSubnets = v->getNbSubnets();
    totalSubnets += nbSubnets;
    if(nbSubnets > maxNbSubnets)
    {
        maxNbSubnets = nbSubnets;
        maxNbSubIDs.clear();
        maxNbSubIDs.push_back(ID);
    }
    else if(nbSubnets == maxNbSubnets)
        maxNbSubIDs.push_back(ID);
    
    if(nbSubnets > 200)
    {
        if(Cluster* cluster = dynamic_cast<Cluster*>(v))
            superClusters.push_back(cluster->getID());
        else if(Node* node = dynamic_cast<Node*>(v))
            superNodes.push_back(node->getID());
    }
    
    totalCoveredIPs += v->getSubnetCoverage();
    
    // Alias metrics
    list<Alias*> *aliases = v->getAliases();
    if(aliases->size() > 0)
    {
        unsigned int curNbAlias = aliases->size();
        if(curNbAlias > maxAliasAmount)
        {
            maxAliasAmount = curNbAlias;
            maxAmountIDs.clear();
            maxAmountIDs.push_back(ID);
        }
        else if(curNbAlias == maxAliasAmount)
            maxAmountIDs.push_back(ID);
        totalAliases += curNbAlias;
        
        unsigned short curLargest = 0;
        for(list<Alias*>::iterator i = aliases->begin(); i != aliases->end(); ++i)
        {
            unsigned short curNb = (*i)->getNbInterfaces();
            if(curNb > curLargest)
                curLargest = curNb;
        }
        if(curLargest > maxAliasSize)
            maxAliasSize = curLargest;
        
        if(aliases->size() == 1)
            nbSingleAlias++;
    }
    
    // Counting amount of direct/indirect/remote links via in edges
    list<Edge*> *edges = v->getEdges();
    for(list<Edge*>::iterator i = edges->begin(); i != edges->end(); ++i)
    {
        Edge *cur = (*i);
        if(DirectLink *direct = dynamic_cast<DirectLink*>(cur))
        {
            nbDirectLinks++;
            nbLinksWithMedium++;
            direct = NULL; // To avoid a pesky warning ("unused variable direct")
        }
        else if(IndirectLink *indirect = dynamic_cast<IndirectLink*>(cur))
        {
            nbIndirectLinks++;
            if(indirect->hasMedium())
                nbLinksWithMedium++;
        }
        else
        {
            nbRemoteLinks++;
        }
    }
    
    // Recursion
    for(list<Edge*>::iterator i = edges->begin(); i != edges->end(); ++i)
        this->visitRecursive1((*i)->getHead());
}

void Cassini::visitRecursive2(Vertice *v, list<Vertice*> &componentGates)
{
    unsigned int ID = v->getID();
    if(visited[ID - 1])
        return;
    
    visited[ID - 1] = true;
    
    // Recursion (peers; i.e. equivalent to entering edges)
    list<Vertice*> *peers = v->getPeers();
    if(peers->size() > 0)
        for(list<Vertice*>::iterator i = peers->begin(); i != peers->end(); ++i)
            this->visitRecursive2((*i), componentGates);
    else
        componentGates.push_back(v);
    
    // Recursion (out edges)
    list<Edge*> *next = v->getEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive2((*i)->getHead(), componentGates);
}

unsigned int Cassini::visitRecursive3(Vertice *v, unsigned int depth)
{
    unsigned int ID = v->getID();
    if(visited[ID - 1]) // Even if we only go "forward", there's a minor risk of cycle
        return depth; // Only to abort the method
    
    visited[ID - 1] = true;
    
    // No more out edges
    list<Edge*> *next = v->getEdges();
    if(next->size() == 0)
        return depth;
    
    // Otherwise, gets the depth for each out edge, and returns the biggest
    unsigned int deepest = 0;
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
    {
        unsigned int curLength = this->visitRecursive3((*i)->getHead(), depth + 1);
        if(curLength > deepest)
            deepest = curLength;
    }
    return deepest;
}

string Cassini::getMetrics()
{
    stringstream ss;
    
    ss << "Graph" << endl;
    ss << "-----" << endl;
    
    // In degree
    ss << "Maximum in degree: " << maxInDegree;
    if(maxInDegree > 1)
    {
        ss << " (";
        for(list<unsigned int>::iterator it = maxInIDs.begin(); it != maxInIDs.end(); ++it)
        {
            if(it != maxInIDs.begin())
                ss << ", ";
            ss << "N" << (*it);
        }
        ss << ")";
    }
    ss << endl;
    
    // Out degree
    ss << "Maximum out degree: " << maxOutDegree << " (";
    for(list<unsigned int>::iterator it = maxOutIDs.begin(); it != maxOutIDs.end(); ++it)
    {
        if(it != maxOutIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    
    // Total degree
    ss << "Maximum total degree (in + out): " << maxTotDegree << " (";
    for(list<unsigned int>::iterator it = maxTotIDs.begin(); it != maxTotIDs.end(); ++it)
    {
        if(it != maxTotIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    
    // Average degree and graph density (N.B.: average total degree = 2 * in/out average degree)
    ss << "Average out degree: " << (double) degreeAcc / (double) totalNodes << endl;
    ss << "Average total degree: " << ((double) degreeAcc * 2) / (double) totalNodes << endl;
    ss << "Graph density: " << (double) degreeAcc / (double) (totalNodes * totalNodes) << "\n" << endl;
    
    // Subnet metrics
    double totalTargetIPs = (double) env.getTotalIPsInitialTargets();
    double coverage = ((double) totalCoveredIPs / totalTargetIPs) * 100;
    double avgSubnets = (double) totalSubnets / (double) totalNodes;
    ss << "Subnets" << endl;
    ss << "-------" << endl;
    ss << "Covered IPs: " << totalCoveredIPs << " (" << coverage << "% w.r.t. targets)" << endl;
    ss << "Maximum amount of subnets per neighborhood: " << maxNbSubnets << " (";
    for(list<unsigned int>::iterator it = maxNbSubIDs.begin(); it != maxNbSubIDs.end(); ++it)
    {
        if(it != maxNbSubIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    ss << "Average amount of subnets per neighborhood: " << avgSubnets << "\n" << endl;
    
    // Alias metrics
    float ratioSingle = (((double) nbSingleAlias) / ((double) totalNodes)) * 100;
    float avgAliases = (double) totalAliases / (double) totalNodes;
    ss << "Aliases" << endl;
    ss << "-------" << endl;
    ss << "Fully aliased neighborhoods: " << nbSingleAlias << " (" << ratioSingle << "%)" << endl;
    ss << "Maximum size of an alias: " << maxAliasSize << endl;
    ss << "Maximum amount of aliases: " << maxAliasAmount << " (";
    for(list<unsigned int>::iterator it = maxAmountIDs.begin(); it != maxAmountIDs.end(); ++it)
    {
        if(it != maxAmountIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    ss << "Average amount of aliases: " << avgAliases << "\n" << endl;
    
    // Quantities/ratios for each kind of links
    double totalLinks = (double) (nbDirectLinks + nbIndirectLinks + nbRemoteLinks);
    if(totalLinks > 0)
    {
        double ratioDirect = ((double) nbDirectLinks) / totalLinks * 100;
        double ratioIndirect = ((double) nbIndirectLinks) / totalLinks * 100;
        double ratioRemote = ((double) nbRemoteLinks) / totalLinks * 100;
        double ratioWithMedium = ((double) nbLinksWithMedium) / totalLinks * 100;
        ss << "Links" << endl;
        ss << "-----" << endl;
        ss << "Direct links: " << nbDirectLinks << " (" << ratioDirect << "%)" << endl;
        ss << "Indirect links: " << nbIndirectLinks << " (" << ratioIndirect << "%)" << endl;
        ss << "Remote links: " << nbRemoteLinks << " (" << ratioRemote << "%)" << endl;
        ss << "Links with a medium: " << nbLinksWithMedium << " (" << ratioWithMedium << "%)\n" << endl;
    }
    
    // Reachable nodes per gate + islets
    ss << "Connected components" << endl;
    ss << "--------------------" << endl;
    unsigned int nbIslets = 0;
    list<list<Vertice*> >::iterator j = visitedGates.begin();
    list<unsigned int>::iterator k = componentDepth.begin();
    for(list<unsigned int>::iterator i = reachableNodes.begin(); i != reachableNodes.end(); ++i)
    {
        // Gets the total amount of visited nodes
        unsigned int nbReachable = (*i);
        
        // Gets the visited gates of the component
        list<Vertice*> visitedGates = (*j);
        j++;
        
        // Gets the depth of the component
        unsigned int depth = (*k);
        k++;
        
        // Islet scenario
        if(nbReachable == 1)
        {
            nbIslets++;
            continue;
        }
        
        // Component with more than one node, possibly with multiple gates
        double percentage = ((double) nbReachable / (double) totalNodes) * 100;
        unsigned int nbVisitedGates = visitedGates.size();
        if(nbVisitedGates > 1)
        {
            stringstream gatesStream;
            visitedGates.sort(Vertice::smallerID);
            for(list<Vertice*>::iterator l = visitedGates.begin(); l != visitedGates.end(); ++l)
            {
                if(l != visitedGates.begin())
                    gatesStream << ", ";
                gatesStream << "N" << (*l)->getID();
            }
            ss << "Via gates " << gatesStream.str() << ": ";
            ss << nbReachable << " (" << percentage << "%";
        }
        else
        {
            ss << "Via gate N" << visitedGates.front()->getID() << ": ";
            ss << nbReachable << " (" << percentage << "%";
        }
        ss << "; graph depth: ";
        if(depth > 1)
            ss << depth << " vertices";
        else
            ss << "one vertice";
        ss << ")\n";
    }
    
    if(nbIslets > 0)
    {
        double ratioIslets = (double) nbIslets / (double) totalNodes * 100;
        ss << "Isolated vertices: " << nbIslets << " (" << ratioIslets << "%)" << endl;
    }
    
    if(superNodes.size() > 0 || superClusters.size() > 0)
    {
        ss << "\n";
        ss << "Exceptional neighborhoods\n";
        ss << "-------------------------" << endl;
        
        unsigned int totalSuper = superNodes.size() + superClusters.size();
        double ratioNodes = ((double) superNodes.size() / (double) totalSuper) * 100;
        double ratioClusters = ((double) superClusters.size() / (double) totalSuper) * 100;
        ss << "Super neighborhoods (> 200 subnets): " << totalSuper << endl;
        
        stringstream detailedNodes, detailedClusters;
        if(superNodes.size() > 0)
        {
            unsigned int nbNodes = 0;
            for(list<unsigned int>::iterator i = superNodes.begin(); i != superNodes.end(); ++i)
            {
                if(nbNodes > 0)
                {
                    detailedNodes << ", ";
                    if((nbNodes % 10) == 0)
                        detailedNodes << "\n";
                }
                detailedNodes << "N" << (*i);
                nbNodes++;
            }
        }
        if(superClusters.size() > 0)
        {
            unsigned int nbClusters = 0;
            for(list<unsigned int>::iterator i = superClusters.begin(); i != superClusters.end(); ++i)
            {
                if(nbClusters > 0)
                {
                    detailedClusters << ", ";
                    if((nbClusters % 10) == 0)
                        detailedClusters << "\n";
                }
                detailedClusters << "N" << (*i);
                nbClusters++;
            }
        }
        
        if(superNodes.size() > 0 && superClusters.size() > 0)
        {
            ss << "Super nodes: " << superNodes.size() << " (" << ratioNodes << "%)" << endl;
            ss << "Super clusters: " << superClusters.size() << " (" << ratioClusters << "%)" << endl;
            ss << "Super node list: " << detailedNodes.str() << endl;
            ss << "Super cluster list: " << detailedClusters.str() << endl;
        }
        else if(superNodes.size() > 0)
            ss << "Super node list: " << detailedNodes.str() << endl;
        else
            ss << "Super cluster list: " << detailedClusters.str() << endl;
    }
    
    return ss.str();
}

void Cassini::outputMetrics(string filename)
{
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << this->getMetrics();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
