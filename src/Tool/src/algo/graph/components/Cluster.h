/*
 * Cluster.h
 *
 *  Created on: Nov 28, 2019
 *      Author: jefgrailet
 *
 * A child class of Vertice, Cluster is used to build vertices which are made of a several subnet 
 * aggregates, as opposed to Node, with a small exception: in the case where the IP identifying 
 * one aggregate was aliased with blindspot(s), a Cluster is also built despite using a single 
 * aggregate. This policy was chosen to display the fact that blindposts are ultimately "invisible 
 * peer IPs" which happened to not appear among the trail(s) of any subnet.
 *
 * Blindspots must be added after construction. Likewise, previous aliases of flickering IPs (if 
 * such IPs are among the IPs identifying this cluster) must be restored after construction as 
 * well by providing the corresponding set of aliases.
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "Vertice.h"

class Cluster : public Vertice
{
public:

    // Constructor/destructor
    Cluster(list<Aggregate*> aggregates);
    ~Cluster();
    
    void addBlindspots(list<InetAddress> blindspots);
    void addFlickeringAliases(AliasSet *aliases);
    
    // list<InetAddress> listAllInterfaces();
    string getFullLabel();
    string toString();
    
protected:

    /*
     * Additional lists of IPs used for the sake of completeness.
     * -"blindspots" list can contain said blindspots, i.e., IPs that were aliased to peering IPs 
     *  but that don't identify any neighborhood. As a consequence, there's no aggregate tied to 
     *  them during topology inference and must be added after construction for completeness.
     * -"flickering" list finally adds IPs that were aliased to some IP(s) of this cluster during 
     *  subnet inference, but which were removed during peer disambiguation to be replaced by the 
     *  first IP of their associated alias. They can be added back and maintained with this list.
     */
    
    list<InetAddress> blindspots;
    list<InetAddress> flickering;
    
};

#endif /* CLUSTER_H_ */
