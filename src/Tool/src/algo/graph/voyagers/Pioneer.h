/*
 * Pioneer.h
 *
 *  Created on: Dec 9, 2019
 *      Author: jefgrailet
 *
 * This simple voyager visits a newly created graph to number its vertices (i.e., neighborhoods). 
 * It proceeds gate by gate, doing the numbering recursively.
 *
 * An almost identical voyager already existed in SAGE v1.
 */

#ifndef PIONEER_H_
#define PIONEER_H_

#include "Voyager.h"

class Pioneer : public Voyager
{
public:

    Pioneer(Environment &env);
    ~Pioneer();
    
    void visit(Graph *g);

protected:

    // Counter for the numbering
    unsigned int counter;
    
    // Method to recursively visit the graph, vertex by vertex.
    void visitRecursive(Vertex *v);

};

#endif /* PIONEER_H_ */
