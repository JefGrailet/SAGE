/*
 * Subnet.h
 *
 *  Created on: Nov 27, 2018
 *      Author: jefgrailet
 *
 * A simple class to model a subnet, as a list of SubnetInterface to which a prefix IP and a 
 * prefix length are associated. The class also provides various methods to evaluate it and output 
 * it. It will be later expanded to integrate it into SAGE v2.0.
 * 
 * This class has no connection with the SubnetSite class from TreeNET/SAGE v1.0, since subnet 
 * inference is re-built from scratch here.
 *
 * N.B.: this class presents similarities with Alias, as the interfaces list is handled in the 
 * same manner. However, since it only makes a up a small part of the code, there was no strong 
 * incentive to use inheritance (and for instance create classes to respectively model an 
 * interface and a list of interfaces).
 */

#ifndef SUBNET_H_
#define SUBNET_H_

#include "SubnetInterface.h"

class Subnet
{
public:

    static SubnetInterface VOID_INTERFACE;
    
    /*
     * General remark on constructors: it's never checked whether the resulting subnet will be 
     * empty or not (e.g., if the submitted pivot is a NULL pointer, or if the list of subnets is 
     * empty for the second constructor). It's simply because the only classes creating subnets 
     * (in the src/algo/subnetinference/ module) already ensure these scenarii never occur, 
     * directly or indirectly. Otherwise, the proper way to handle these scenarii would be to 
     * throw an exception when they occur.
     */
    
    Subnet(IPTableEntry *pivot); // Initializes a /32 subnet
    Subnet(list<Subnet*> subnets); // Merge several subnets together
    ~Subnet();
    
    // Methods to handle interfaces in general
    void addInterface(IPTableEntry *interface, unsigned short subnetRule);
    void updateInterface(IPTableEntry *interface, unsigned short newRule);
    void clearInterfaces();
    IPTableEntry *getSelectedPivot();
    IPTableEntry *getInterface(InetAddress interface); // NULL if no such interface
    inline unsigned short getNbInterfaces() { return (unsigned short) interfaces.size(); }
    
    // Additional methods for handling interfaces, especially useful for subnet post-processing
    inline bool hasContrapivots() { return hasInterfaces(SubnetInterface::CONTRAPIVOT); }
    inline unsigned short getNbContrapivots() { return countInterfaces(SubnetInterface::CONTRAPIVOT); }
    inline list<IPTableEntry*> getContrapivots() { return listInterfaces(SubnetInterface::CONTRAPIVOT); }
    
    inline bool hasOutliers() { return hasInterfaces(SubnetInterface::OUTLIER); }
    inline unsigned short getNbOutliers() { return countInterfaces(SubnetInterface::OUTLIER); }
    inline list<IPTableEntry*> getOutliers() { return listInterfaces(SubnetInterface::OUTLIER); }
    
    bool hasOnlyPivots();
    vector<unsigned short> countAllInterfaces(); // 0 = #pivots, 1 = #contra-pivots, 2 = #outliers
    list<IPTableEntry*> listBetween(InetAddress low, InetAddress up); // IPs in ]low, up[
    
    // Accessers
    inline list<SubnetInterface> *getInterfacesList() { return &interfaces; }
    inline InetAddress getInitialPivot() { return initialPivot; }
    inline InetAddress getPrefix() { return prefix; }
    inline unsigned short getPrefixLength() { return prefixLength; }
    inline string getStopDescription() { return stopDescription; }
    inline bool needsPostProcessing() { return toPostProcess; }
    inline void setStopDescription(string desc) { stopDescription = desc; }
    inline void setToPostProcess(bool pp) { toPostProcess = pp; }
    inline bool isPostProcessed() { return postProcessed; }
    
    // Methods to handle borders in the wide sense
    InetAddress getLowerBorder(bool withLimits = true); // Include network/broadcast addresses
    InetAddress getUpperBorder(bool withLimits = true);
    bool contains(InetAddress interface); // True if given interface is within boundaries of this
    bool overlaps(Subnet *other);
    
    // Shrinks/expands the subnet
    void expand();
    void shrink(list<IPTableEntry*> *out = NULL); // (out => IPs no longer within the borders)
    
    /*
     * (July 2019) Methods used during subnet post-processing.
     * -getPivotTTL() finds the TTL of pivot IPs, if always the same, and returns 255 otherwise. 
     *  The method has the particularity of allowing a parameter to tell which type of interface 
     *  is considered as a pivot (notion of pivot can change during post-processing). 0 means 
     *  we use the already known pivot, 1 means we use contra-pivot(s) as pivot(s) and 2 means 
     *  we use outlier(s) as pivot(s).
     * -getSmallestTTL() finds the overall smallest TTL value seen for an interface in this 
     *  subnet.
     * -findAdjustedPrefix() computes the maximum prefix length such that the corresponding 
     *  subnet encompasses all discovered interfaces. The purpose of this method is to minimize 
     *  large prefixes which encompasses few interfaces (e.g. a /20 with one interface) as they 
     *  are often the result of a lack of responsive interfaces rather than realistic predictions.
     * -findAlternativeContrapivot() detects "alternative" contra-pivot(s) if the distance of 
     *  pivots vary within the subnet (due to warping IPs) and if no contra-pivot has been found 
     *  yet. Note the maxNb variable, which is meant to be the maximum allowed amount of 
     *  contra-pivots within a subnet.
     */
    
    unsigned char getPivotTTL(unsigned short otherType = 0);
    unsigned char getSmallestTTL();
    void findAdjustedPrefix();
    void findAlternativeContrapivot(unsigned short maxNb);
    
    /*
     * (August 2019) Methods used for neighborhood discovery.
     * -getTrail() returns the trail of the subnet, i.e., the trail tied to the selected pivot.
     * -getDirectTrailIPs() returns a list of all IPs appearing in the trails (without anomalies) 
     *  of the pivot interfaces of this subnet. This consists in checking if there are any pivots 
     *  identified via Rule 4 or 5 and listing them along with the selected pivot (or one selected 
     *  pivot if the subnet was built during subnet post-processing).
     * -getPeerDiscoveryPivots() returns a list of SubnetInterface objects (as SubnetInterface* to 
     *  easily update the objects afterwards) which correspond to the selected pivot and other 
     *  pivot IPs which were deemed as being on the same subnet through rules other than rule 2 
     *  (timeout rule). It takes as a parameter the maximum amount of such interfaces to pick (in 
     *  large subnets, getting all interfaces is not interesting). These interfaces will then be 
     *  used as target IPs for probes which aim at discovering the peer(s) of the neighborhood 
     *  encompassing this subnet.
     * -findPreTrailIPs() finds and stores the first valid IPs (i.e. non-anonymous hops) that 
     *  appear before the trail, ideally one hop away from it. It is mainly used to find handle 
     *  "pre-echoing IPs" (for subnets built around rule 3).
     */
    
    Trail &getTrail();
    list<InetAddress> getDirectTrailIPs();
    list<SubnetInterface*> getPeerDiscoveryPivots(unsigned short maxNb);
    void findPreTrailIPs();
    inline unsigned short getPreTrailOffset() { return preTrailOffset; }
    inline list<InetAddress> getPreTrailIPs() { return preTrailIPs; } // No pointer because won't be edited
    
    // Various outputs methods
    string getCIDR(bool verbose = false); // verbose => non-adjusted prefix will be given too
    string getAdjustedCIDR(); // Returns adjusted prefix, or default prefix if not adjusted
    unsigned short getAdjustedPrefixLength(); // Same spirit, but just returns the prefix length
    unsigned int getTotalCoveredIPs(); // Gets the amount of IPs covered by this (adjusted) subnet
    string toString();
    
    // Methods to sort and compare subnets
    static inline bool compare(Subnet *s1, Subnet *s2) { return s1->prefix < s2->prefix; }
    
private:

    // Initial pivot IP, prefix, prefix length and list of interfaces
    InetAddress initialPivot, prefix;
    unsigned short prefixLength;
    list<SubnetInterface> interfaces;
    
    // String containing a short but detailed description of why subnet growth stopped.
    string stopDescription;
    
    /*
     * Special flags used during post-processing.
     * -toPostProcess signals that the growth stopped because the subnet started overlapping a 
     *  previously inferred subnet, potentially compromising the accuracy of the latter. However, 
     *  due to contra-pivot positioning, having an actually undergrown subnet remains a 
     *  possibility and needs to be mitigated as much as possible (this is the purpose of subnet 
     *  post-processing).
     * -postProcessed is set to true when the subnet has already been built during post-processing 
     *  as a merger of several subnets discovered during inference. This flag is needed to avoid 
     *  merging subnets several times (can happen due to overlap), as post-processing already 
     *  ensures we get the largest and best possible result. Building an even larger merged subnet 
     *  would likely result in an incoherent final result.
     */
    
    bool toPostProcess;
    bool postProcessed;
    
    // Adjusted prefix (inferred at the end of post-processing; optional)
    InetAddress adjustedPrefix;
    unsigned short adjustedPrefixLength;
    
    // Pre-trail stuff (see above)
    unsigned short preTrailOffset;
    list<InetAddress> preTrailIPs;
    
    // Gets selected pivot as SubnetInterface&
    SubnetInterface &getSelectedSubnetInterface();
    
    // Re-computes prefix (called after expanding/shrinking)
    void resetPrefix();
    
    // Checks the presence, counts the amount of or lists a certain type of interface
    bool hasInterfaces(unsigned short type);
    unsigned short countInterfaces(unsigned short type);
    list<IPTableEntry*> listInterfaces(unsigned short type);

};

#endif /* SUBNET_H_ */
