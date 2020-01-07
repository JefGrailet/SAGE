/*
 * IPClusterer.h
 *
 *  Created on: Aug 30, 2019
 *      Author: jefgrailet
 *
 * A class specifically designed for the algorithmical needs of neighborhood inference and graph 
 * building (in SAGE), IPClusterer is meant to gather and uniformize lists of IPs that are related 
 * in some manner (pre-trail IPs of "echoing" subnets, partial aliases discovered during graph 
 * building) while guaranteeing fast access to them.
 *
 * In particular, it is used for aggregating "pre-echoing" IPs to build neighborhoods consisting 
 * of subnets discovered through the third rule of inference (echo rule).
 */

#include <list>
using std::list;
#include <map>
using std::map;

#ifndef IPCLUSTERER_H_
#define IPCLUSTERER_H_

#include "../../../common/inet/InetAddress.h"

class IPClusterer
{
public:

    // Constructor, destructor
    IPClusterer();
    ~IPClusterer();
    
    // Methods for interacting with the clusterer
    void update(list<InetAddress> cluster);
    list<InetAddress> getCluster(InetAddress needle);
    string getClusterString(InetAddress needle);
    list<list<InetAddress> > listClusters();

private:

    // Map of IPs -> clusters
    map<InetAddress, list<InetAddress>* > haystack;
    
    /*
     * Merge two clusters together and update the haystack accordingly.
     *
     * @param list<InetAddress>* c1  First cluster (will be updated)
     * @param list<InetAddress>* c2  Second cluster (will be deleted)
     */
    
    void mergeClusters(list<InetAddress> *c1, list<InetAddress> *c2);

};

#endif /* IPCLUSTERER_H_ */
