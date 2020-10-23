/*
 * Cassini.h
 *
 *  Created on: Dec 12, 2019
 *      Author: jefgrailet
 *
 * This voyager visits a network graph to compute a variety of early metrics, including various 
 * notions of degree and the amount of nodes that can be reached for each gate. A boolean array 
 * is used to avoid visiting a same node several times.
 *
 * A similar voyager already existed in SAGE v1.
 */

#ifndef CASSINI_H_
#define CASSINI_H_

#include "Voyager.h"

class Cassini : public Voyager
{
public:

    Cassini(Environment &env);
    ~Cassini();
    
    void visit(Graph *g);
    
    // Methods to get metrics in string format, or as an output file
    string getMetrics();
    void outputMetrics(string filename);

protected:

    // Vectors and lists to census visited nodes (overall or per gate)
    vector<bool> visited;
    list<unsigned int> reachableNodes; // Per component
    list<list<Vertex*> > visitedGates; // Vertex* and not just IDs (useful for depth evaluation)
    list<unsigned int> componentDepth; // Longest paths (#vertices) in each connected component

    // Fields to handle the metrics
    unsigned int degreeAcc, totalNodes; // Acc for "accumulator"
    
    /*
     * N.B.: only the "in" degree is summed to degreeAcc during the visit. Indeed, whether we sum 
     * in or out degree, the total of edges will be the same in both cases. The average total 
     * degree will consist in twice the total amount of edges divided by the amount of nodes, 
     * therefore average in (or out) degree times 2.
     */
    
    unsigned short maxInDegree;
    list<unsigned int> maxInIDs;
    
    unsigned short maxOutDegree;
    list<unsigned int> maxOutIDs;
    
    unsigned short maxTotDegree;
    list<unsigned int> maxTotIDs;

    unsigned short maxNbSubnets;
    list<unsigned int> maxNbSubIDs;
    
    unsigned int totalSubnets, totalCoveredIPs;
    unsigned int nbDirectLinks, nbIndirectLinks, nbRemoteLinks, nbLinksWithMedium;
    
    // Metrics about "super-"neighborhoods (i.e., > 200 subnets)
    list<unsigned int> superClusters, superNodes;
    
    // TODO for later: more details about subnets ?
    
    // Metrics on aliases
    unsigned int maxAliasAmount, nbSingleAlias;
    unsigned short maxAliasSize;
    list<unsigned int> maxAmountIDs;
    unsigned int totalAliases;
    
    /*
     * Private methods:
     * -reset(), as the name implies, resets all numeric fields to 0 and resets vectors/lists.
     * -visitRecursive1() travels the graph recursively in order to compute the minimum/maximum 
     *  in/out degree along the minimum/maximum amount of subnets per neighborhood. In other 
     *  words, it computes node metrics.
     * -visitRecursive2() also travels the graph but is meant to go through a gate then mark as 
     *  visited any node connected to the current component; i.e., it will also look at the in 
     *  edges for each node. When the method reaches a gate (i.e., no "in" edge), it appends it 
     *  to the list "componentGates" passed by reference to the method.
     * -visitRecursive3() travels again the graph from a vertex, but only forward, and evaluates 
     *  the longest path (or depth) starting from this vertex (typically, a gate).
     */
    
    void reset();
    void visitRecursive1(Vertex *v);
    void visitRecursive2(Vertex *v, list<Vertex*> &componentGates);
    unsigned int visitRecursive3(Vertex *v, unsigned int depth);

};

#endif /* CASSINI_H_ */
