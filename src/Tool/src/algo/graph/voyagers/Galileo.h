/*
 * Galileo.h
 *
 *  Created on: Dec 19, 2019
 *      Author: jefgrailet
 *
 * This voyager visits a graph to conduct the final and full alias resolution. It stores the final 
 * aliases and provides a method to output them in a ".aliases-f" file.
 *
 * A very similar voyager already existed in SAGE v1.
 */

#ifndef GALILEO_H_
#define GALILEO_H_

#include "Voyager.h"
#include "../../aliasresolution/AliasHintsCollector.h"
#include "../../aliasresolution/AliasResolver.h"

class Galileo : public Voyager
{
public:

    Galileo(Environment &env);
    ~Galileo();
    
    void visit(Graph *g);
    
    // Method to output the aliases
    void figaro(string filename);

protected:

    AliasHintsCollector ahc;
    AliasResolver ar;
    
    // In order to output aliases by neighborhood, the vertices having aliases are listed.
    list<Vertex*> withAliases;
    vector<bool> visited; // Array for avoiding evaluating a vertex more than once
    
    // Method to recursively visit the graph, node by node.
    void visitRecursive(Vertex *v);
    
    // Additional private method to add trail IPs to alias candidates of a vertex (contra-pivots)
    list<IPTableEntry*> getAliasCandidates(Vertex *v);

};

#endif /* GALILEO_H_ */
