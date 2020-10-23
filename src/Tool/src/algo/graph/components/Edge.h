/*
 * Edge.h
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Models a (directed) edge in a network graph and acts as a superclass for all kinds of edges 
 * (direct, indirect and remote). This class already existed in SAGE v1 but has been simplified.
 */

#ifndef EDGE_H_
#define EDGE_H_

#include "Vertex.h"
class Vertex; // Forward declaration

class Edge
{
public:

    Edge(Vertex *tail, Vertex *head);
    virtual ~Edge();
    
    inline Vertex* getTail() { return tail; }
    inline Vertex* getHead() { return head; }
    
    // Comparison method for sorting (e.g. used in a method of the "Mariner" voyager)
    static bool compare(Edge *e1, Edge *e2);
    
    // To be implemented by child class
    virtual string toString(bool verbose = false) = 0;

protected:
    
    /*
     * In graph theory, given an edge u -> v, u is the tail while v is the head.
     */
    
    Vertex *tail, *head;
    
};

#endif /* EDGE_H_ */
