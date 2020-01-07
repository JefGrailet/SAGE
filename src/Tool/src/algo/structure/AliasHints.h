/*
 * AliasHints.h
 *
 *  Created on: Nov 16, 2018
 *      Author: jefgrailet
 *
 * AliasHints gathers all the data which is collected when probing a target IP for alias 
 * resolution. This data is both used to fingerprint the target IP and apply some specific alias 
 * resolution methods on it; for instance, it stores an array of IP-IDs which are used to alias 
 * IPs with the "Ally" method.
 *
 * An AliasHints object both encapsulates all alias resolution "hints" associated to a target IP 
 * (a pointer to such object is maintained in the IPTableEntry object associated with the target) 
 * and acts as a fingerprint; i.e., in other parts of the code, AliasHints objects will be 
 * gathered in structures such as lists and sorted/analyzed according to the fingerprints they 
 * produce.
 * 
 * In previous programs involving TreeNET's alias resolution methodology, alias resolution hints 
 * used to be tied to an IPTableEntry object while fingerprints were modeled with an additional 
 * Fingerprint class.
 */

#ifndef ALIASHINTS_H_
#define ALIASHINTS_H_

#include <vector>
using std::vector;
#include <string>
using std::string;

#include "../../common/inet/InetAddress.h"

class AliasHints
{
public:

    // Constants to model probing stages
    const static unsigned short EMPTY_HINTS = 0;
    const static unsigned short DURING_SUBNET_DISCOVERY = 1;
    const static unsigned short DURING_GRAPH_BUILDING = 2; // For SAGE v2.0
    const static unsigned short DURING_FULL_ALIAS_RESOLUTION = 3; // For SAGE v2.0
    
    // Possible "classes" of IP ID counter
    enum IPIDCounterClasses
    {
        NO_IDEA, // No available IP ID data to infer anything, or not inferred yet
        HEALTHY_COUNTER, // Increases normally, at most one rollover observed in the sequence
        FAST_COUNTER, // Increases at a fast pace (more than one rollover in the sequence)
        RANDOM_COUNTER, // Increases abnomally fast; the sent IP-IDs are probably random
        ECHO_COUNTER // Echoes the IP-ID that was in the initial probe
    };
    
    AliasHints(unsigned short nbIPIDs);
    AliasHints(); // Empty hints (used to advertise absence of hints)
    AliasHints(const AliasHints &other);
    ~AliasHints();
    
    // Normally implicitely declared by compiler; provided to signal explicitely it's relevant
    AliasHints &operator=(const AliasHints &other);
    
    inline bool isEmpty() { return when == EMPTY_HINTS; }
    
    // Handles the probing stage variable
    static inline void moveStage() { probingStage++; }
    inline bool isLattestStage() { return (when >= probingStage); }
    
    // Initializes all arrays to store the IP-ID data (to use after setting "nbIPIDs" variable)
    void prepareIPIDData();
    
    // Post-processes IP-ID data to infer type of counter
    void postProcessIPIDData(unsigned short maxRollovers, double maxError);
    
    // Methods for fingerprint handling
    bool fingerprintSimilarTo(AliasHints &other);
    bool toGroupByDefault();
    
    // Comparison methods to sort objects by fingerprint or by stage
    static bool compare(AliasHints &h1, AliasHints &h2);
    inline static bool compareByStage(AliasHints &h1, AliasHints &h2) { return h1.getWhen() < h2.getWhen(); }
    
    // Output methods
    string fingerprintToString();
    string toString();
    
    /*********************
     * Accessers/setters *
     *********************/
    
    // General accessers
    inline unsigned short getWhen() { return when; }
    
    // Accessers for all data related to IP-IDs
    inline unsigned short getNbIPIDs() { return nbIPIDs; }
    inline unsigned short getNbDelays() { return nbIPIDs - 1; }
    inline vector<unsigned long> &getProbeTokens() { return probeTokens; }
    inline vector<unsigned short> &getIPIdentifiers() { return IPIdentifiers; }
    inline vector<bool> &getEchoes() { return echoMask; }
    inline vector<unsigned long> &getDelays() { return delays; }
    
    // Accessers for other alias resolution hints
    inline unsigned char getTimeExceededInitialTTL() { return timeExceededInitialTTL; }
    inline unsigned char getEchoInitialTTL() { return echoInitialTTL; }
    inline string getHostName() { return hostName; }
    inline bool repliesToTSRequest() { return replyingToTSRequest; }
    inline InetAddress getPortUnreachableSrcIP() { return portUnreachableSrcIP; }
    inline bool isUDPSecondary() { return UDPSecondary; }
    
    // Accessers for inferred alias hints
    inline double getVelocityLowerBound() { return velocityLowerBound; }
    inline double getVelocityUpperBound() { return velocityUpperBound; }
    inline unsigned short getIPIDCounterType() { return IPIDCounterType; }
    
    // Setters
    inline void setTimeExceededInitialTTL(unsigned char iTTL) { timeExceededInitialTTL = iTTL; }
    inline void setEchoInitialTTL(unsigned char iTTL) { echoInitialTTL = iTTL; }
    inline void setHostName(string hn) { hostName = hn; }
    inline void setReplyingToTSRequest(bool rttsr) { replyingToTSRequest = rttsr; }
    inline void setPortUnreachableSrcIP(InetAddress srcIP) { portUnreachableSrcIP = srcIP; }
    inline void setUDPSecondary(bool s) { UDPSecondary = s; }
    
    inline void setVelocityLowerBound(double vlb) { velocityLowerBound = vlb; }
    inline void setVelocityUpperBound(double vub) { velocityUpperBound = vub; }
    inline void setIPIDCounterType(unsigned short cType) { IPIDCounterType = cType; }
    
private:

    // Value telling when these hints have been collected
    unsigned short when;
        
    // Alias resolution hints
    unsigned short nbIPIDs;
    vector<unsigned long> probeTokens;
    vector<unsigned short> IPIdentifiers;
    vector<bool> echoMask;
    vector<unsigned long> delays;
    
    /*
     * (October 2019) AliasHints now also includes the inferred initial TTL from a "Time exceeded" 
     * reply (as described by Vanaubel et al. in "Network Fingerprinting: TTL-Based Router 
     * Signatures" published at IMC 2013). However, such TTL is only available for trail IPs (and 
     * collected by taking advantage of the route data obtained at scanning); IPs from subnets 
     * which never appear in partial traceroute records keep a value of 0 for that TTL (appearing 
     * as "*" in the fingerprint).
     * 
     * The "when" variable is now used to adapt the behavior of comparison methods of this class 
     * w.r.t. this additionnal value. It is therefore taken into account during early stages 
     * (involving only IPs discovered via traceroute probing) but ignored for the "full" alias 
     * resolution (involving IPs which didn't appear as route hops during traceroute probing).
     */
    
    unsigned char timeExceededInitialTTL, echoInitialTTL;
    string hostName;
    bool replyingToTSRequest;
    InetAddress portUnreachableSrcIP;
    bool UDPSecondary; // True when trying to alias the IP through other means if UDP doesn't work
    
    /*
     * About echo mask: it records "true" for the corresponding index when the IP-ID is the same 
     * as in the initial probe. The reason why this is done for every IP-ID is to avoid bad 
     * diagnosis when a collision (= the "echo" is a pure coincidence) occurs, which might occur 
     * during large campaigns. If every IP-ID is an echo, then it is an "echo" counter. Otherwise, 
     * it is either a healthy, fast or random counter.
     */
    
    // Data inferred from the collected IP-IDs
    double velocityLowerBound, velocityUpperBound;
    unsigned short IPIDCounterType;
    
    /*
     * Static variable to store the default value for the "when" instance variable (see below) 
     * to use upon creating a new AliasHints object. The "when" variable is only useful upon 
     * outputting the collected data.
     */
    
    static unsigned short probingStage;

};

#endif /* ALIASHINTS_H_ */
