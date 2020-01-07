/*
 * Trail.h
 *
 *  Created on: Aug 30, 2018
 *      Author: jefgrailet
 *
 * This class models a "trail", i.e., the end of the route towards some IP which consists of the 
 * last valid IP observed before reaching the target plus an amount of anomalies. An anomaly is:
 * -an anonymous hop (there was no reply to the probe), 
 * -or a cycle.
 * A valid IP is any other IP. If there's a cycle, the last valid IP consists of the first 
 * occurrence of the cycling IP.
 * 
 * A trail is used to both identify individual subnets (IPs that are close in the IPv4 address 
 * space, which share the same distance TTL-wise and have the same trail should be on the same 
 * subnet) and gather subnets into neighborhoods. This class is meant to ease handling of such 
 * a concept in the rest of the code.
 */

#ifndef TRAIL_H_
#define TRAIL_H_

#include <iostream>
using std::ostream;

#include "../../common/inet/InetAddress.h"
#include "RouteHop.h"

class Trail
{
public:

    friend ostream &operator<<(ostream &out, const Trail &t)
	{
		out << "[" << t.getLastValidIP();
		unsigned short nbAnomalies = t.getNbAnomalies();
		if(nbAnomalies > 0)
		    out << " | " << nbAnomalies;
		out << "]";
		return out;
	}
    
    Trail(RouteHop lastValidHop, unsigned short nbAnomalies);
    Trail(RouteHop lastValidHop); // No anomaly
    Trail(unsigned short routeLength); // Exceptional situation where there's only anonymous hops
    Trail(); // Empty trail
    Trail(const Trail &other);
    ~Trail();
    
    // Normally added implicitely by compiler; added explicitely to signal its use
    Trail &operator=(const Trail &other);
    
    inline bool isVoid() { return lastValidIP == InetAddress(0) && nbAnomalies == 0; }
    
    inline InetAddress getLastValidIP() const { return lastValidIP; }
    inline unsigned short getNbAnomalies() const { return nbAnomalies; }
    inline unsigned short getLengthInTTL() { return nbAnomalies + 1; }
    inline unsigned char getLastValidIPiTTL() const { return lastValidIPiTTL; }
    
    inline bool isDirect() { return direct; }
    inline bool isWarping() { return warping; }
    inline bool isFlickering() { return flickering; }
    inline bool isEchoing() { return echoing; }
    
    inline void setAsWarping() { warping = true; }
    inline void setAsFlickering() { flickering = true; }
    inline void setAsEchoing() { echoing = true; }
    
    static bool compare(Trail &t1, Trail &t2);
    
    string toString();
    bool operator==(const Trail &other) const;
    
private:
    
    InetAddress lastValidIP;
    unsigned short nbAnomalies;
	
	bool direct, warping, flickering, echoing;
	
	/*
	 * (October 2019) Additional field and private method to compute the inferred initial TTL 
	 * associated to the last valid IP of the trail, based on the "RouteHop" object passed during 
	 * construction. This piece of information is later recorded in the dictionary and re-used 
	 * for alias resolution. Indeed, the initial TTL value of a time exceeded reply is used in an 
	 * extended fingerprinting approach inspired by Vanaubel et al. (see "Network Fingerprinting: 
	 * TTL-Based Router Signatures" published at IMC 2013).
	 */
	
	unsigned char lastValidIPiTTL;
	
	void setInferredInitialTTL(RouteHop lastValidHop);
	
};

#endif /* TRAIL_H_ */
