/*
 * ConfigFileParser.h
 *
 *  Created on: Sept 22, 2017
 *      Author: jefgrailet
 *
 * This class is dedicated to parsing a configuration file (a stream of key/value pairs, with the 
 * "=" symbol being the separator), containing (among others) probing parameters and some other 
 * values to change the behavior of given phases.
 */
 
#ifndef CONFIGFILEPARSER_H_
#define CONFIGFILEPARSER_H_

#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <list>
using std::list;

#include "../Environment.h"
#include "../../common/inet/NetworkAddress.h"

class ConfigFileParser
{
public:
    
    // Arrays containing parameters of each kind, along their size
    static const unsigned short N_PARAM_TIMEVAL;
    static const string PARAM_TIMEVAL[];
    
    static const unsigned short N_PARAM_BOOL;
    static const string PARAM_BOOL[];
    
    static const unsigned short N_PARAM_STRING;
    static const string PARAM_STRING[];
    
    static const unsigned short N_PARAM_INTEGER;
    static const string PARAM_INTEGER[];
    
    static const unsigned short N_PARAM_DOUBLE;
    static const string PARAM_DOUBLE[];
    
    // Constructor, destructor
    ConfigFileParser(Environment &env);
    ~ConfigFileParser();
    
    void parse(string inputFileName);

private:
    
    // Pointer to the Environment singleton (provides access to output stream and to setters)
    Environment &env;
    
    // Private method to "explode" a string (i.e., split it into chunks at a given delimiter)
    static list<string> explode(string input, char delimiter);
    
    // Checks if a string appears in an array. Returns -1 if not found, and the index otherwise.
    static int inArray(string needle, const string haystack[], unsigned short sizeHaystack);
};

#endif /* CONFIGFILEPARSER_H_ */
