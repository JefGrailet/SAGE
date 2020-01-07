/*
 * ConfigFileParser.cpp
 *
 *  Created on: Sept 22, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in ConfigFileParser.h.
 */

#include <cstdlib> // atoi()
#include <algorithm> // transform()
#include <sstream>
using std::stringstream;
#include <iomanip>
using std::setprecision;
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::endl;
using std::flush;

#include "ConfigFileParser.h"
#include "../../common/date/TimeVal.h"
#include "../../common/utils/StringUtils.h"

const unsigned short ConfigFileParser::N_PARAM_TIMEVAL = 4;
const string ConfigFileParser::PARAM_TIMEVAL[] = {
"probingTimeoutPeriod", 
"probingRegulatingDelay", 
"probingRetryDelay", 
"concurrencyThreadDelay"
};

const unsigned short ConfigFileParser::N_PARAM_BOOL = 4;
const string ConfigFileParser::PARAM_BOOL[] = {
"probingFixedFlowParis", 
"prescanningThirdOpinion", 
"prescanningExpansion", 
"aliasResolutionStrictMode"
};

const unsigned short ConfigFileParser::N_PARAM_STRING = 1;
const string ConfigFileParser::PARAM_STRING[] = {
"probingPayloadMessage"
};

const unsigned short ConfigFileParser::N_PARAM_INTEGER = 13;
const string ConfigFileParser::PARAM_INTEGER[] = {
"probingMaxRetries", 
"concurrencyMaxThreads", 
"scanningStartTTL", 
"scanningMinimumTargetsPerThread", 
"scanningTargetListSplitThreshold", 
"scanningNumberOfReprobing", 
"scanningMaximumFlickeringDelta", 
"inferenceOutliersRatioDivisor",
"peerDiscoveryMaxPivots", 
"aliasResolutionNbIPIDs", 
"aliasResolutionAllyMaxDifference", 
"aliasResolutionAllyMaxConsecutiveDifference", 
"aliasResolutionVelocityMaxRollovers"
};

const unsigned short ConfigFileParser::N_PARAM_DOUBLE = 2;
const string ConfigFileParser::PARAM_DOUBLE[] = {
"aliasResolutionVelocityOverlapTolerance", 
"aliasResolutionVelocityMaxError"
};

ConfigFileParser::ConfigFileParser(Environment &e): env(e)
{
}

ConfigFileParser::~ConfigFileParser()
{
}

list<string> ConfigFileParser::explode(string input, char delimiter)
{
    list<string> result;

    // Checks if delimiter is there; if not return a single-element list
    size_t pos = input.find(delimiter);
    if(pos == std::string::npos)
    {
        result.push_back(input);
        return result;
    }
    
    // Splits the input string according to delimiter
    stringstream ss(input);
    string chunk;
    while(std::getline(ss, chunk, delimiter))
        result.push_back(chunk);
    
    return result;
}

int ConfigFileParser::inArray(string needle, const string haystack[], unsigned short sizeHaystack)
{
    for(unsigned short i = 0; i < sizeHaystack; i++)
        if(needle.compare(haystack[i]) == 0)
            return i;
    return -1;
}

void ConfigFileParser::parse(string inputFileName)
{
    ostream *out = env.getOutputStream();
    
    string inputFileContent = "";
    ifstream inFile;
    inFile.open((inputFileName).c_str());
    if(inFile.is_open())
    {
        inputFileContent.assign((std::istreambuf_iterator<char>(inFile)),
                                (std::istreambuf_iterator<char>()));
        
        inFile.close();
    }
    else
    {
        (*out) << "Configuration file " << inputFileName << " does not exist.\n";
        (*out) << "Default parameters will be used instead.\n" << endl;
        return;
    }
    
    stringstream ss(inputFileContent);
    string targetStr;
    
    unsigned short nbLine = 0;
    unsigned short nbErrors = 0;
    while(std::getline(ss, targetStr, '\n'))
    {
        list<string> keyValuePair = explode(targetStr, '=');
        
        string key = keyValuePair.front();
        string value = keyValuePair.back();
        
        // TimeVal parameters
        int index = inArray(key, PARAM_TIMEVAL, N_PARAM_TIMEVAL);
        if(index != -1)
        {
            TimeVal res(1, 0);
            unsigned long val = 1000 * StringUtils::string2Ulong(value);
            if(val > 0)
            {
                unsigned long sec = val / TimeVal::MICRO_SECONDS_LIMIT;
                unsigned long microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
                res.setTime(sec, microSec);
                
                switch(index)
                {
                    case 0:
                        if(val > 10000000)
                        {
                            (*out) << "Warning for key \"" << PARAM_TIMEVAL[0] << "\": you ";
                            (*out) << "provided a timeout larger than 10 seconds. Default value ";
                            (*out) << "will be used instead." << endl;
                            nbErrors++;
                            break;
                        }
                        env.setTimeoutPeriod(res);
                        break;
                    case 1:
                        if(val > 1000000)
                        {
                            (*out) << "Warning for key \"" << PARAM_TIMEVAL[1] << "\": you ";
                            (*out) << "provided a delay above the second. Default value will be ";
                            (*out) << "used instead." << endl;
                            nbErrors++;
                            break;
                        }
                        env.setProbeRegulatingPeriod(res);
                        break;
                    case 2:
                        if(val > 10000000)
                        {
                            (*out) << "Warning for key \"" << PARAM_TIMEVAL[2] << "\": you ";
                            (*out) << "provided a delay above 10 seconds. Default value will be ";
                            (*out) << "used instead." << endl;
                            nbErrors++;
                            break;
                        }
                        env.setRetryDelay(res);
                        break;
                    case 3:
                        if(val > 1000000)
                        {
                            (*out) << "Warning for key \"" << PARAM_TIMEVAL[3] << "\": you ";
                            (*out) << "provided a delay above the second. Default value will be ";
                            (*out) << "used instead." << endl;
                            nbErrors++;
                            break;
                        }
                        env.setProbingThreadDelay(res);
                        break;
                    default:
                        break; // Unlikely
                }
            }
            else
            {
                (*out) << "Malformed value for key \"" << PARAM_TIMEVAL[index] << "\": please ";
                (*out) << "input a integer amount. Default value will be used." << endl;
                nbErrors++;
            }
            nbLine++;
            continue;
        }
        
        // Boolean parameters
        index = inArray(key, PARAM_BOOL, N_PARAM_BOOL);
        if(index != -1)
        {
            std::transform(value.begin(), value.end(), value.begin(), ::toupper);
            bool val = false, malformed = false;
            if(value.compare("TRUE") == 0)
                val = true;
            else if(value.compare("FALSE") != 0)
                malformed = true;
            if(malformed)
            {
                (*out) << "Malformed value for key \"" << PARAM_BOOL[index] << "\": did not ";
                (*out) << "recognize \"true\" or \"false\". Default value will be used." << endl;
                nbErrors++;
                nbLine++;
                continue;
            }
            switch(index)
            {
                case 0:
                    env.setUsingFixedFlowID(val);
                    break;
                case 1:
                    env.setPrescanThirdOpinion(val);
                    break;
                case 2:
                    env.setPrescanExpansion(val);
                    break;
                case 3:
                    env.setARStrictMode(val);
                    break;
                default:
                    break; // Unlikely
            }
            nbLine++;
            continue;
        }
        
        // String parameter
        index = inArray(key, PARAM_STRING, N_PARAM_STRING);
        if(index != - 1)
        {
            // Only one possibility now: attention message
            if(value.length() < 100)
            {
                env.setAttentionMessage(value);
            }
            else
            {
                (*out) << "Malformed value for key \"" << PARAM_STRING[0] << "\": you ";
                (*out) << "provided a string longer than 100 characters, which is too large. ";
                (*out) << "Default attention message will be used instead." << endl;
                nbErrors++;
            }
            nbLine++;
            continue;
        }
        
        // Integer parameters
        index = inArray(key, PARAM_INTEGER, N_PARAM_INTEGER);
        if(index != - 1)
        {
            int asInt = std::atoi(value.c_str());
            switch(index)
            {
                case 0:
                    if(asInt >= 1 && asInt < 5)
                        env.setMaxRetries((unsigned short) asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[0] << "\": ";
                        (*out) << "provided value is outside the range [1, 4]. ";
                        (*out) << "Default value will be used instead." << endl;
                    }
                    break;
                case 1:
                    if(asInt > 1 && asInt <= 32767 && asInt > (env.getARNbIPIDs() + 1))
                        env.setMaxThreads((unsigned short) asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[1] << "\": ";
                        if(asInt <= 1 || asInt > 32767)
                            (*out) << "provided value is outside [2, 32767]. ";
                        else
                        {
                            (*out) << "provided value is smaller than the amount of collected ";
                            (*out) << "IP-IDs per alias candidate. ";
                        }
                        (*out) << "Default value will be used instead." << endl;
                    }
                    break;
                case 2:
                    if(asInt > 0 && asInt <= 64)
                        env.setStartTTL((unsigned char) asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[2] << "\": provided ";
                        (*out) << "value is outside [1, 64], which is unlikely. Default value ";
                        (*out) << "will be used instead." << endl;
                    }
                    break;
                case 3:
                    if(asInt >= 1 && asInt <= 32767)
                        env.setScanningMinTargetsPerThread((unsigned short) asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[3] << "\": ";
                        (*out) << "provided value is outside the range [1, 32767]. ";
                        (*out) << "Default value will be used instead." << endl;
                    }
                    break;
                case 4:
                    if(asInt >= 1 && asInt <= 2048)
                        env.setScanningListSplitThreshold((unsigned short) asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[4] << "\": ";
                        (*out) << "provided value is outside the range [1, 2048]. ";
                        (*out) << "Default value will be used instead." << endl;
                    }
                    break;
                case 5:
                    if(asInt >= 1 && asInt <= 4)
                        env.setScanningNbReprobing((unsigned short) asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[5] << "\": ";
                        (*out) << "provided value is outside the range [1, 4]. ";
                        (*out) << "Default value will be used instead." << endl;
                    }
                    break;
                case 6:
                    if(asInt >= 2 && asInt <= 256)
                        env.setScanningMaxFlickeringDelta((unsigned short) asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[6] << "\": ";
                        (*out) << "provided value is outside the range [1, 256]. ";
                        (*out) << "Default value will be used instead." << endl;
                    }
                    break;
                case 7:
                    if(asInt > 1 && asInt <= 100) // Min. ratio of outliers is 1%
                        env.setOutliersRatioDivisor(asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[7] << "\": provided ";
                        (*out) << "value is outside [1, 100]. Default value will be used ";
                        (*out) << "instead." << endl;
                    }
                    break;
                    break;
                case 8:
                    if(asInt > 1 && asInt <= 4095) // Max. value is #IPs in a full /20 - 1
                        env.setMaxPeerDiscoveryPivots(asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[8] << "\": provided ";
                        (*out) << "value is outside [1, 4095]. Default value will be used ";
                        (*out) << "instead." << endl;
                    }
                    break;
                case 9:
                    if(asInt > 2 && asInt <= 20 && env.getMaxThreads() > (asInt + 1))
                        env.setARNbIPIDs(asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[9] << "\": ";
                        if(asInt <= 2 || asInt > 20)
                            (*out) << "provided value is outside [3, 20]. ";
                        else
                        {
                            (*out) << "provided value is greater than the maximum amount ";
                            (*out) << "of concurrent threads, which is a problem for scheduling. ";
                        }
                        (*out) << "Default value will be used instead." << endl;
                    }
                    break;
                case 10:
                    if(asInt > 0 && asInt <= 32768) // 0.05 * 65356
                        env.setARAllyMaxDiff(asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[10] << "\": provided ";
                        (*out) << "value is outside [1, 32768]. Default value will be used ";
                        (*out) << "instead." << endl;
                    }
                    break;
                case 11:
                    if(asInt > 0 && asInt <= 3277) // 0.5 * 65356
                        env.setARAllyMaxConsecutiveDiff(asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[11] << "\": provided ";
                        (*out) << "value is outside [1, 3277]. Default value will be used ";
                        (*out) << "instead." << endl;
                    }
                    break;
                case 12:
                    if(asInt > 0 && asInt <= 256)
                        env.setARVelocityMaxRollovers(asInt);
                    else
                    {
                        nbErrors++;
                        (*out) << "Warning for key \"" << PARAM_INTEGER[12] << "\": provided ";
                        (*out) << "value is outside [1, 256]. Default value will be used ";
                        (*out) << "instead." << endl;
                    }
                    break;
                default:
                    break; // Unlikely
            }
            nbLine++;
            continue;
        }
        
        // Double parameters
        index = inArray(key, PARAM_DOUBLE, N_PARAM_DOUBLE);
        if(index != - 1)
        {
            double valDouble = StringUtils::string2double(value);
            if(valDouble > 0.0 && valDouble < 1.0)
            {
                if(index == 0)
                    env.setARVelocityBaseTolerance(valDouble);
                else
                    env.setARVelocityMaxError(valDouble);
            }
            else
            {
                nbErrors++;
                (*out) << "Warning for key \"" << PARAM_DOUBLE[index] << "\": provided ";
                (*out) << "value is outside ]0, 1[. Default value will be used instead" << endl;
            }
            nbLine++;
            continue;
        }
        
        // Nothing matches here
        (*out) << "Warning: unrecognized key at line " << (nbLine + 1) << " in ";
        (*out) << inputFileName << ". Key, value pair will be ignored." << endl;
        nbErrors++;
        nbLine++;
    }
    
    if(nbErrors > 0)
    {
        (*out) << "Finished parsing configuration file. ";
        if(nbErrors > 1)
            (*out) << nbErrors << " errors have been detected.\n";
        else
            (*out) << "One error has been detected.\n";
        (*out) << endl;
    }
}
