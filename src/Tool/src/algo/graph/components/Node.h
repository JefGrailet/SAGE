/*
 * Node.h
 *
 *  Created on: Nov 28, 2019
 *      Author: jefgrailet
 *
 * A child class of Vertex, Node is used to build vertices which are made of a single subnet 
 * aggregate, as opposed to Cluster.
 */

#ifndef NODE_H_
#define NODE_H_

#include "Vertex.h"

class Node : public Vertex
{
public:

    // Constructor/destructor
    Node(Aggregate *a);
    ~Node();
    
    inline list<InetAddress> *getPreEchoing() { return &preEchoing; }
    
    // list<InetAddress> listAllInterfaces();
    string getFullLabel();
    string toString();

protected:
    
    /*
     * Pre-echoing aggregates never ending in "Cluster" vertices, their details are exclusive to 
     * the "Node" vertices.
     */
    
    list<InetAddress> preEchoing;
    unsigned short preEchoingOffset;

};

#endif /* NODE_H_ */
