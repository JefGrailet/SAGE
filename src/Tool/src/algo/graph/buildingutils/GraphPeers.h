/*
 * GraphPeers.h
 *
 *  Created on: Nov 28, 2019
 *      Author: jefgrailet
 *
 * Used during graph building, this specific class ties together the final peers of one aggregate 
 * or a set of aggregates and their offset. Its constructors also perform the task of processing 
 * the peers of multiple aggregates (if there are several) to only keep the closest peers and to 
 * remove possible duplicates.
 *
 * The goal is to isolate the final peers of a Vertex, still as Peer objects, while all Vertex 
 * objects are built aside without their final peering details. Setting the peering details of 
 * the vertices (with peers being provided as Vertex, rather than Peer) after all vertices have 
 * been built is easier than building them "on the fly" - which brings not only algorithmical 
 * challenges, but also consistency issues if stuff like cycles appear inside the graph.
 *
 * Important note: this class assumes the aggregate(s) given to the constructors necessarily have 
 * peers. Peerless aggregates/vertices shouldn't have an associated GraphPeers object.
 */

#ifndef GRAPHPEERS_H_
#define GRAPHPEERS_H_

#include "Aggregate.h" // Indirectly, also brings Peer

class GraphPeers
{
public:

    // Constructor/destructor
    GraphPeers(Aggregate *aggregate);
    GraphPeers(list<Aggregate*> aggregates);
    ~GraphPeers();
    
    list<Peer*> peers;
    unsigned short offset;
    
};

#endif /* GRAPHPEERS_H_ */
