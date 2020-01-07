/*
 * Environment.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * This file implements the class defined in Environment.h. See this file for more details on the 
 * goals of such class.
 */

#include <math.h> // For pow() function
#include <sys/stat.h> // For CHMOD edition

#include "Environment.h"

Mutex Environment::consoleMessagesMutex(Mutex::ERROR_CHECKING_MUTEX);
Mutex Environment::emergencyStopMutex(Mutex::ERROR_CHECKING_MUTEX);

Environment::Environment(ostream *cOut, 
                         unsigned short protocol, 
                         InetAddress &localIP, 
                         NetworkAddress &lan, 
                         unsigned short mode): localIPAddress(localIP), LAN(lan)
{
    IPDictionary = new IPLookUpTable();
    subnets = new list<Subnet*>();
    aliases = new list<AliasSet*>();

    consoleOut = cOut;
    externalLogs = false;
    isExternalLogOpened = false;
    probingProtocol = protocol;
    
    timeoutPeriod = TimeVal(2, TimeVal::HALF_A_SECOND); // 2,5s
    probeRegulatingPeriod = TimeVal(0, 250000); // 0,25s
    retryDelay = TimeVal(1, 0); // 1s
    maxRetries = 2; // One retry by default (there can be more)
    useFixedFlowID = true;
    probeAttentionMessage = "NOT AN ATTACK (e-mail: Jean-Francois.Grailet@uliege.be)";
    
    prescanExpand = false;
    prescanThirdOpinion = false;
    
    startTTL = (unsigned char) 1;
    scanMinTargetsPerThread = 32;
    scanListSplitThreshold = 256;
    scanNbReprobing = 1;
    scanMaxFlickeringDelta = 4;
    
    outliersRatioDivisor = 3;
    
    maxPeerDiscoveryPivots = 5; // Up to 5 (e.g. if /31 subnet, only one possible VP)
    
    ARNbIPIDs = 4;
    ARAllyMaxDiff = 13107; // 1/5 * 2^16
    ARAllyMaxConsecutiveDiff = 100;
    ARVelocityMaxRollovers = 10;
    ARVelocityBaseTolerance = 0.2;
    ARVelocityMaxError = 0.35;
    ARStrictMode = false;
    
    maxThreads = 256;
    probingThreadDelay = TimeVal(0, TimeVal::HALF_A_SECOND); // 0,5s
    
    totalProbes = 0;
    totalSuccessfulProbes = 0;
    flagEmergencyStop = false;
    
    displayMode = mode;
}

Environment::~Environment()
{
    delete IPDictionary;
    for(list<Subnet*>::iterator i = subnets->begin(); i != subnets->end(); i++)
        delete (*i);
    delete subnets;
    for(list<AliasSet*>::iterator i = aliases->begin(); i != aliases->end(); ++i)
        delete (*i);
    delete aliases;
}

ostream* Environment::getOutputStream()
{
    if(externalLogs || isExternalLogOpened)
        return &logStream;
    return consoleOut;
}

void Environment::outputAliases(string filename, unsigned short stage, bool verbose)
{
    if(aliases->size() == 0)
        return;
    
    for(list<AliasSet*>::iterator i = aliases->begin(); i != aliases->end(); ++i)
    {
        AliasSet *curSet = (*i);
        if(curSet->getNbAliases() > 0)
        {
            unsigned short when = curSet->getWhen();
            if(when == AliasSet::FULL_ALIAS_RESOLUTION)
                break;
            if(when < stage)
                continue;
            switch(when)
            {
                // No need to get a specific output stream here.
                case AliasSet::SUBNET_DISCOVERY:
                    cout << "Aliases discovered during subnet discovery have been written in ";
                    cout << filename << ".aliases-1.\n";
                    curSet->output(filename + ".aliases-1", verbose);
                    break;
                // For SAGE v2.0
                case AliasSet::GRAPH_BUILDING:
                    cout << "Aliases discovered during graph building have been written in ";
                    cout << filename << ".aliases-2.\n";
                    curSet->output(filename + ".aliases-2", verbose);
                    break;
                default:
                    break;
            }
        }
    }
}

AliasSet* Environment::getLattestAliases()
{
    if(aliases->size() == 0)
        return NULL;
    return aliases->back();
}

void Environment::outputSubnets(string filename)
{
    string output = "";
    
    for(list<Subnet*>::iterator i = subnets->begin(); i != subnets->end(); i++)
        output += (*i)->toString() + "\n";
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

bool Environment::initialTargetsEncompass(InetAddress IP)
{
    for(list<InetAddress>::iterator it = itIPs.begin(); it != itIPs.end(); ++it)
    {
        if(IP == (*it))
            return true;
    }
    
    for(list<NetworkAddress>::iterator it = itRanges.begin(); it != itRanges.end(); ++it)
    {
        NetworkAddress curRange = (*it);
        if(IP >= curRange.getLowerBorderAddress() && IP <= curRange.getUpperBorderAddress())
            return true;
    }
    
    return false;
}

unsigned int Environment::getTotalIPsInitialTargets()
{
    unsigned int totalIPs = itIPs.size();
    for(list<NetworkAddress>::iterator it = itRanges.begin(); it != itRanges.end(); ++it)
    {
        double power = 32 - (double) ((unsigned short) (*it).getPrefixLength());
        totalIPs += (unsigned int) pow(2, power);
    }
    return totalIPs;
}

void Environment::updateProbeAmounts(DirectProber *proberObject)
{
    totalProbes += proberObject->getNbProbes();
    totalSuccessfulProbes += proberObject->getNbSuccessfulProbes();
}

void Environment::resetProbeAmounts()
{
    totalProbes = 0;
    totalSuccessfulProbes = 0;
}

void Environment::openLogStream(string filename, bool message)
{
    logStream.open(filename.c_str());
    isExternalLogOpened = true;
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    if(message)
        (*consoleOut) << "Log of current phase is being written in " << filename << ".\n" << endl;
}

void Environment::closeLogStream()
{
    logStream.close();
    logStream.clear();
    isExternalLogOpened = false;
}

bool Environment::triggerStop()
{
    /*
     * In case of loss of connectivity, it is possible several threads calls this method. To avoid 
     * multiple contexts launching the emergency stop, there is the following condition (in 
     * addition to a Mutex object).
     */
    
    if(flagEmergencyStop)
    {
        return false;
    }

    flagEmergencyStop = true;
    
    consoleMessagesMutex.lock();
    (*consoleOut) << "\nWARNING: emergency stop has been triggered because of connectivity ";
    (*consoleOut) << "issues.\nThe tool will wait for all remaining threads to complete before ";
    (*consoleOut) << "exiting without carrying out remaining probing tasks.\n" << endl;
    consoleMessagesMutex.unlock();
    
    return true;
}
