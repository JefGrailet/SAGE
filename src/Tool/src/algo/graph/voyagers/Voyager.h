/*
 * Voyager.h
 *
 *  Created on: Dec 9, 2019
 *      Author: jefgrailet
 *
 * This abstract class defines the interface for a bunch of classes which act as "graph 
 * travellers" (nicknamed voyagers) to perform various operations, such as visiting neighborhoods 
 * for alias resolution or simply numbering neighborhoods.
 *
 * This class already existed in SAGE v1 and has been very lightly changed for SAGE v2.
 */

#ifndef VOYAGER_H_
#define VOYAGER_H_

#include "../../Environment.h"
#include "../components/Graph.h"

class Voyager
{
public:

    Voyager(Environment &env);
    virtual ~Voyager();
    
    virtual void visit(Graph *g) = 0;

protected:

    Environment &env;
    
};

#endif /* VOYAGER_H_ */
