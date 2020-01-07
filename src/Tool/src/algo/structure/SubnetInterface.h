/*
 * SubnetInterface.h
 *
 *  Created on: Nov 27, 2018
 *      Author: jefgrailet
 *
 * A simple class to represent a single interface that is part of a subnet.
 * 
 * This class has no connection with the SubnetSiteNode class used in TreeNET/SAGE v1.0.
 */

#ifndef SUBNETINTERFACE_H_
#define SUBNETINTERFACE_H_

#include "IPTableEntry.h"
#include "RouteHop.h"

class SubnetInterface
{
public:

    // Status/rules through which this interface has been added to the subnet
    enum InterfaceStatus
    {
        VOID, // Used as an equivalent of nullptr (nullptr only exists starting from C++11)
        SELECTED_PIVOT, 
        CONTRAPIVOT, 
        RULE_1_TRAIL, // Same trail as the pivot interface, therefore same subnet
        RULE_2_TTL_TIMEOUT, // Same distance, but one trail got anomalies (= timeout)
        RULE_3_ECHOES, // Trails are echoing target IPs and interfaces are at the same distance
        RULE_4_FLICKERING, // Flickering trails which IPs were aliased together
        RULE_5_ALIAS, // Different trail than the pivot, but IPs were aliased (flickering/warping)
        OUTLIER, 
        ALTERNATIVE_CONTRAPIVOT // (July 2019) Alternate definition of a contra-pivot, see below
    };
    
    /*
     * About alternative definition of a contra-pivot: usually, a contra-pivot is inferred on the 
     * basis of the TTL distance. However, in situations where traffic engineering causes the 
     * distances of pivot IPs to vary wildly, the contra-pivot(s) might not appear sooner than 
     * pivot IPs and might at first appear as outlier(s). A new post-processing step now detects 
     * subnets that are missing a contra-pivot and where the TTL distances of pivot IPs vary a 
     * lot to re-label the outliers as contra-pivots (if there is only one outlier or very few 
     * of them).
     */

    SubnetInterface(IPTableEntry *ip, unsigned short status);
    SubnetInterface(); // Creates a "void" interface
    ~SubnetInterface();
    
    inline bool isVoid() { return status == VOID; }
    
    // Most essential data
    IPTableEntry *ip;
    unsigned short status;
    
    // Used during neighborhood discovery
    vector<RouteHop> partialRoute;
    
    // Comparison method
    static bool smaller(SubnetInterface &i1, SubnetInterface &i2);
    
    // Method for convenient output of route data (if any; returns "" if none)
    string routeToString();

};

#endif /* SUBNETINTERFACE_H_ */
