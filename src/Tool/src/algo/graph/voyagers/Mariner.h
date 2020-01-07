/*
 * Mariner.h
 *
 *  Created on: Dec 10, 2019
 *      Author: jefgrailet
 *
 * This simple voyager visits a graph to list all its vertices, in order to output their content 
 * and later delete everything. A boolean vector is used to avoid visiting a same node several 
 * times.
 *
 * An almost identical voyager already existed in SAGE v1.
 */

#ifndef MARINER_H_
#define MARINER_H_

#include "Voyager.h"

class Mariner : public Voyager
{
public:

    Mariner(Environment &env);
    ~Mariner();
    
    inline list<Vertice*> *getVertices() { return &vertices; }
    
    void visit(Graph *g);
    
    // Methods to output the neighborhoods (i.e. vertices) and the structure of the graph itself
    void outputNeighborhoods(string filename);
    void outputGraph(string filename);
    
    // Mariner is also used to delete the listed vertices
    void cleanVertices();

protected:

    list<Vertice*> vertices;
    vector<bool> visited; // Array for avoiding visiting a vertice more than once
    
    // Method to recursively visit the graph, node by node.
    void visitRecursive(Vertice *v);

};

#endif /* MARINER_H_ */
