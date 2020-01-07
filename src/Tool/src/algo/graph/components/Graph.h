/*
 * Graph.h
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * As the name suggests, this class models a network graph, where vertices model neighborhoods and 
 * edges the links that exist between them.
 *
 * This class already existed in SAGE v1 (originally written in November 2017) but has been 
 * refreshed and somewhat simplified in this newer version.
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include <list>
using std::list;
#include <map>
using std::map;

#include "SubnetVerticeMapping.h"

class Graph
{
public:

    /*
     * Size of the subnetMap array (array of lists), which is used for fast look-up of a subnet 
     * listed in a vertice on the basis of an interface it should contain. Its size is based on 
     * the fact that no subnet of a prefix length shorter than /20 was ever found in the 
     * measurements. Therefore, the 20 first bits of any interface in a subnet are used to access 
     * a list of subnets sharing the same prefix in O(1), the list containing at most 2048 subnets 
     * (2048 subnets of prefix length /31). This dramatically speeds up the look-up for a subnet 
     * present in a graph (in comparison with a more trivial method where one visits the graph), 
     * at the cost of using more memory (~8Mo).
     */

    const static unsigned int SIZE_SUBNET_MAP = 1048576;
    
    static SubnetVerticeMapping VOID_MAPPING;
    
    Graph();
    ~Graph();
    
    inline list<Vertice*> *getGates() { return &gates; }
    inline void addGate(Vertice *gate) { gates.push_back(gate); }
    
    inline unsigned int getNbVertices() { return nbVertices; }
    inline void setNbVertices(unsigned int nb) { nbVertices = nb; }
    
    // Inserts the mappings subnet -> vertice for a given vertice
    void createSubnetMappingsTo(Vertice *v);
    void sortMappings();
    
    // Gets a subnet found in the graph which contains the given input address
    SubnetVerticeMapping &getSubnetContaining(InetAddress needle);
    
private:

    /*
     * Private fields:
     * -gates: Vertice objects modeling the first neighborhoods to appear in the topology with 
     *  respects to the (partial) traceroute measurements.
     * -subnetMap: array of lists for fast subnet look-up (cf. large comment in public section).
     */
    
    list<Vertice*> gates;
    list<SubnetVerticeMapping> *subnetMap;
    
    unsigned int nbVertices; // Total amount; set after a visit by Pioneer

};

#endif /* GRAPH_H_ */
