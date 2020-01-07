/*
 * TopologyInferrer.h
 *
 *  Created on: Aug 22, 2019
 *      Author: jefgrailet
 *
 * Class implementing the inference of the topology of a target domain as a graph where vertices 
 * are neighborhoods while edges describe which subnets/paths are crossed to go from a 
 * neighborhood to another.
 *
 * After building the initial aggregates of subnets (i.e., future neighborhoods) and discovering 
 * their peer IPs, thanks to the additional (partial) traceroute data collected by PeerScanner, 
 * the code conducts alias resolution on groups of peer IPs (built from list of peer IPs for every 
 * aggregate, using the property of "alias transitivity") to ensure these interfaces belong to 
 * the same devices or not, as discovering aliases allows us to identify possibly incomplete 
 * neighborhoods (i.e. composed of several aggregates) and the final peers of each (group of) 
 * aggregate(s). Once the final neighborhoods and their peers are known, the construction of the 
 * resulting graph begins.
 *
 * Remarks:
 * --------
 * -the class exists since August 22 because it was originally created for WISE and only inferred 
 *  aggregates of subnets and their peers, in order to conduct a preliminary study of these 
 *  concepts "in the wild".
 * -it essentially consists of one big method. This part of the code is separated from the rest of 
 *  the program for the sake of readability, i.e., all actual graph building steps are isolated 
 *  here while PeerScanner isolates the additional probing and while the Main.cpp file describes 
 *  the sequence of all main algorithmical steps.
 * -for the sake of readability, said method is also heavily commented to highlight the different 
 *  "sub-parts" of the inference and their purpose.
 */

#ifndef TOPOLOGYINFERRER_H_
#define TOPOLOGYINFERRER_H_

#include "../Environment.h"
#include "components/Graph.h"

class TopologyInferrer
{
public:

    // Constructor, destructor
    TopologyInferrer(Environment &env);
    ~TopologyInferrer();
    
    Graph *infer();

private:

    Environment &env;

};

#endif /* TOPOLOGYINFERRER_H_ */
