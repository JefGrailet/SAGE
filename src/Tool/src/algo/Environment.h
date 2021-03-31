/*
 * Environment.h
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * Environment is a class which sole purpose is to provide access to structures or constants (for 
 * the current execution, e.g. timeout delay used for probing) which are relevant to the different 
 * parts of the program, such that each component does not have to be passed individually each 
 * time a new object is instantiated.
 *
 * Note: it was originally first implemented in TreeNET v2.0 and named "TreeNETEnvironment".
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

#include <ostream>
using std::ostream;
#include <fstream>
using std::ofstream;

#include "../common/thread/Mutex.h"
#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "../common/inet/NetworkAddress.h"
#include "../prober/DirectProber.h"
#include "utils/StopException.h" // Not used directly here, but provided to all classes that need it this way
#include "structure/IPLookUpTable.h"
#include "structure/AliasSet.h"
#include "structure/Subnet.h"

class Environment
{
public:

    // Constants to represent probing protocols
    const static unsigned short PROBING_PROTOCOL_ICMP = 1;
    const static unsigned short PROBING_PROTOCOL_UDP = 2;
    const static unsigned short PROBING_PROTOCOL_TCP = 3;

    // Constants to denote the different modes of verbosity (introduced in TreeNET v3.0)
    const static unsigned short DISPLAY_MODE_LACONIC = 0; // Default
    const static unsigned short DISPLAY_MODE_SLIGHTLY_VERBOSE = 1;
    const static unsigned short DISPLAY_MODE_VERBOSE = 2;
    const static unsigned short DISPLAY_MODE_DEBUG = 3;

    // Mutex objects used when printing out additionnal messages (slightly verbose to debug) and triggering emergency stop
    static Mutex consoleMessagesMutex;
    static Mutex emergencyStopMutex;

    // Constructor/destructor (default values for most parameters, see implementation in .cpp file)
    Environment(ostream *consoleOut, 
                unsigned short probingProtocol, 
                InetAddress &localIPAddress, 
                unsigned char LANSubnetMask, 
                unsigned short displayMode);
    ~Environment();
    
    /************
    * Accessers *
    ************/
    
    /*
     * N.B.: the methods returning (const) references have no other point than being compatible 
     * with the libraries prober/ and common/, which were inherited from ExploreNET v2.1. 
     * Otherwise, most fields are values and can be returned as such.
     */
    
    // Data structures
    inline IPLookUpTable* getIPDictionary() { return IPDictionary; }
    inline list<Subnet*> *getSubnets() { return subnets; }
    inline list<AliasSet*>* getAliases() { return aliases; }
    
    // Probing parameters (to be set by command line options)
    inline unsigned short getProbingProtocol() { return probingProtocol; }
    inline const InetAddress &getLocalIPAddress() { return localIPAddress; }
    inline const unsigned char &getLANSubnetMask() { return LANSubnetMask; }
    
    // Probing parameters (to be set by configuration file)
    inline const TimeVal &getTimeoutPeriod() { return timeoutPeriod; }
    inline const TimeVal &getProbeRegulatingPeriod() { return probeRegulatingPeriod; }
    inline const TimeVal &getRetryDelay() { return retryDelay; }
    inline const unsigned short getMaxRetries() { return maxRetries; }
    inline bool usingFixedFlowID() { return useFixedFlowID; }
    inline string &getAttentionMessage() { return probeAttentionMessage; }
    
    // Concurrency parameters
    inline const unsigned short getMaxThreads() { return maxThreads; }
    inline const TimeVal &getProbingThreadDelay() { return probingThreadDelay; }
    
    // Display parameters
    inline unsigned short getDisplayMode() { return displayMode; }
    inline bool debugMode() { return (displayMode == DISPLAY_MODE_DEBUG); }
    
    // Parameters related to algorithms themselves
    inline bool expandingAtPrescanning() { return prescanExpand; }
    inline bool usingPrescanningThirdOpinion() { return prescanThirdOpinion; }
    
    inline unsigned char getStartTTL() { return startTTL; }
    inline unsigned short getScanningMinTargetsPerThread() { return scanMinTargetsPerThread; }
    inline unsigned short getScanningListSplitThreshold() { return scanListSplitThreshold; }
    inline unsigned short getScanningNbReprobing() { return scanNbReprobing; }
    inline unsigned short getScanningMaxFlickeringDelta() { return scanMaxFlickeringDelta; }
    
    inline unsigned short getOutliersRatioDivisor() { return outliersRatioDivisor; }
    
    inline unsigned short getMaxPeerDiscoveryPivots() { return maxPeerDiscoveryPivots; }
    
    inline unsigned short getARNbIPIDs() { return ARNbIPIDs; }
    inline unsigned short getARAllyMaxDiff() { return ARAllyMaxDiff; }
    inline unsigned short getARAllyMaxConsecutiveDiff() { return ARAllyMaxConsecutiveDiff; }
    inline unsigned short getARVelocityMaxRollovers() { return ARVelocityMaxRollovers; }
    inline double getARVelocityBaseTolerance() { return ARVelocityBaseTolerance; }
    inline double getARVelocityMaxError() { return ARVelocityMaxError; }
    inline bool usingStrictAliasResolution() { return ARStrictMode; }
    
    // Output stream and external log feature
    ostream *getOutputStream();
    inline bool usingExternalLogs() { return externalLogs; }
    
    /**********
    * Setters *
    **********/
    
    /*
     * N.B.: while accessers are listed by "categories", setters are listed by type to follow the 
     * same order of appearance as in the constants of Environment as well as in the class 
     * ConfigFileParser.
     */
    
    // Time parameters
    inline void setTimeoutPeriod(TimeVal period) { timeoutPeriod = period; }
    inline void setProbeRegulatingPeriod(TimeVal period) { probeRegulatingPeriod = period; }
    inline void setRetryDelay(TimeVal delay) { retryDelay = delay; }
    inline void setProbingThreadDelay(TimeVal delay) { probingThreadDelay = delay; }
    
    // Boolean parameters
    inline void setUsingFixedFlowID(bool val) { useFixedFlowID = val; }
    inline void setPrescanThirdOpinion(bool val) { prescanThirdOpinion = val; }
    inline void setPrescanExpansion(bool val) { prescanExpand = val; }
    inline void setARStrictMode(bool val) { ARStrictMode = val; }
    
    // String parameter
    inline void setAttentionMessage(string msg) { probeAttentionMessage = msg; }
    
    // Numeric parameters
    inline void setMaxRetries(unsigned short max) { maxRetries = max; }
    inline void setMaxThreads(unsigned short max) { maxThreads = max; }
    inline void setStartTTL(unsigned char TTL) { startTTL = TTL; }
    inline void setScanningMinTargetsPerThread(unsigned short m) { scanMinTargetsPerThread = m; }
    inline void setScanningListSplitThreshold(unsigned short t) { scanListSplitThreshold = t; }
    inline void setScanningNbReprobing(unsigned short n) { scanNbReprobing = n; }
    inline void setScanningMaxFlickeringDelta(unsigned short m) { scanMaxFlickeringDelta = m; }
    inline void setOutliersRatioDivisor(unsigned short d) { outliersRatioDivisor = d; }
    inline void setMaxPeerDiscoveryPivots(unsigned short m) { maxPeerDiscoveryPivots = m; }
    inline void setARNbIPIDs(unsigned short nb) { ARNbIPIDs = nb; }
    inline void setARAllyMaxDiff(unsigned short diff) { ARAllyMaxDiff = diff; }
    inline void setARAllyMaxConsecutiveDiff(unsigned short diff) { ARAllyMaxConsecutiveDiff = diff; }
    inline void setARVelocityMaxRollovers(unsigned short max) { ARVelocityMaxRollovers = max; }
    
    // Double parameters
    inline void setARVelocityBaseTolerance(double base) { ARVelocityBaseTolerance = base; }
    inline void setARVelocityMaxError(double max) { ARVelocityMaxError = max; }
    
    /****************
    * Other methods *
    ****************/
    
    // Methods to handle the sets of aliases
    inline void addAliasSet(AliasSet *newSet) { aliases->push_back(newSet); }
    void outputAliases(string filename, unsigned short stage = 0, bool verbose = false); // Outputs all sets except full a.r.
    AliasSet* getLattestAliases(); // Returns last alias set (NULL if none)
    
    // Methods to handle the set of subnets
    inline unsigned int getNbSubnets() { return (unsigned int) subnets->size(); }
    void outputSubnets(string filename);
    
    // Methods to handle the initial target IPs/ranges
    inline list<InetAddress> *getInitialTargetIPs() { return &itIPs; }
    inline list<NetworkAddress> *getInitialTargetRanges() { return &itRanges; }
    bool initialTargetsEncompass(InetAddress IP);
    unsigned int getTotalIPsInitialTargets();
    
    // Methods to handle total amounts of (successful) probes
    void updateProbeAmounts(DirectProber *proberObject);
    void resetProbeAmounts();
    inline unsigned int getTotalProbes() { return totalProbes; }
    inline unsigned int getTotalSuccessfulProbes() { return totalSuccessfulProbes; }
    
    // Method to handle the output stream writing in an output file.
    void openLogStream(string filename, bool message = true);
    void closeLogStream();
    
    /*
     * Method to trigger the (emergency) stop. It is a special method of Environment which is 
     * meant to force the program to quit when it cannot fully benefit from the resources of the 
     * host computer to conduct measurements/probing. It has a return result (though not used 
     * yet), a boolean one, which is true if the current context successfully triggered the stop 
     * procedure. It will return false it was already triggered by another thread.
     */
    
    bool triggerStop();
    
    // Method to check if the flag for emergency stop is raised.
    inline bool isStopping() { return this->flagEmergencyStop; }
    
private:

    // Structures
    IPLookUpTable *IPDictionary;
    list<AliasSet*> *aliases;
    list<Subnet*> *subnets;
    
    /*
     * Output streams (main console output and file stream for the external logs). Having both is 
     * useful because the console output stream will still be used to advertise the creation of a 
     * new log file and emergency stop if the user requested probing details to be written in 
     * external logs.
     */
    
    ostream *consoleOut;
    ofstream logStream;
    bool externalLogs;
    bool isExternalLogOpened;
    
    // Probing parameters
    unsigned short probingProtocol;
    InetAddress &localIPAddress;
    unsigned char LANSubnetMask;
    
    TimeVal timeoutPeriod;
    TimeVal probeRegulatingPeriod;
    TimeVal retryDelay;
    unsigned short maxRetries;
    bool useFixedFlowID;
    string probeAttentionMessage;
    
    // Algorithmic parameters
    bool prescanExpand;
    bool prescanThirdOpinion;
    
    unsigned char startTTL;
    unsigned short scanMinTargetsPerThread;
    unsigned short scanListSplitThreshold, scanNbReprobing;
    unsigned short scanMaxFlickeringDelta;
    
    unsigned short outliersRatioDivisor;
    
    unsigned short maxPeerDiscoveryPivots;
    
    unsigned short ARNbIPIDs;
    unsigned short ARAllyMaxDiff; // Max difference between two IDs for Ally application
    unsigned short ARAllyMaxConsecutiveDiff; // Max difference between two IDs for ID re-ordering
    unsigned short ARVelocityMaxRollovers;
    double ARVelocityBaseTolerance;
    double ARVelocityMaxError;
    bool ARStrictMode;
    
    // Concurrency parameters
    unsigned short maxThreads;
    TimeVal probingThreadDelay;
    
    // Lists used to maintain the initial target (it) IPs/IP ranges
    list<InetAddress> itIPs;
    list<NetworkAddress> itRanges;
    
    // Fields to record the amount of (successful) probes used during some stage (can be reset)
    unsigned int totalProbes;
    unsigned int totalSuccessfulProbes;
    
    // Display parameter (max. value = 3, amounts to debug mode)
    unsigned short displayMode;
    
    // Flag for emergency exit
    bool flagEmergencyStop;

};

#endif /* ENVIRONMENT_H_ */
