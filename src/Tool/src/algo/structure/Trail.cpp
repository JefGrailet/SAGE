/*
 * Trail.cpp
 *
 *  Created on: Aug 30, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in Trail.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Trail.h"

void Trail::setInferredInitialTTL(RouteHop lastValidHop)
{
    unsigned short replyTTLAsShort = (unsigned short) lastValidHop.replyTTL;
    if(replyTTLAsShort > 128)
        lastValidIPiTTL = (unsigned char) 255;
    else if(replyTTLAsShort > 64)
        lastValidIPiTTL = (unsigned char) 128;
    // If target is 32+ hops away, replyTTL <= 32 still matches initial TTL of 64
    else if(replyTTLAsShort > 32 || lastValidHop.reqTTL >= 32)
        lastValidIPiTTL = (unsigned char) 64;
    else
        lastValidIPiTTL = (unsigned char) 32;
}

Trail::Trail(RouteHop lastValidHop, unsigned short nbAnomalies)
{
    this->lastValidIP = lastValidHop.ip;
    this->nbAnomalies = nbAnomalies;
    
    direct = true;
    if(nbAnomalies > 0)
        direct = false;
    
    warping = false;
    flickering = false;
    echoing = false;
    
    this->setInferredInitialTTL(lastValidHop);
}

Trail::Trail(RouteHop lastValidHop)
{
    this->lastValidIP = lastValidHop.ip;
    nbAnomalies = 0;
    
    direct = true;
    warping = false;
    flickering = false;
    echoing = false;
    
    this->setInferredInitialTTL(lastValidHop);
}

Trail::Trail(unsigned short routeLength)
{
    lastValidIP = InetAddress(0);
    nbAnomalies = routeLength;
    
    direct = false;
    warping = false;
    flickering = false;
    echoing = false;
    
    lastValidIPiTTL = (unsigned char) 0;
}

Trail::Trail()
{
    lastValidIP = InetAddress(0);
    nbAnomalies = 0;
    
    direct = false;
    warping = false;
    flickering = false;
    echoing = false;
    
    lastValidIPiTTL = (unsigned char) 0;
}

Trail::Trail(const Trail &other)
{
    *this = other;
}

Trail::~Trail()
{
}

Trail& Trail::operator=(const Trail &other)
{
    this->lastValidIP = other.lastValidIP;
    this->nbAnomalies = other.nbAnomalies;
    this->direct = other.direct;
    this->warping = other.warping;
    this->flickering = other.flickering;
    this->echoing = other.echoing;
    this->lastValidIPiTTL = other.lastValidIPiTTL;
    return *this;
}

bool Trail::compare(Trail &t1, Trail &t2)
{
    if(t1.lastValidIP == t2.lastValidIP)
        return t1.nbAnomalies < t2.nbAnomalies;
    return t1.lastValidIP < t2.lastValidIP;
}

string Trail::toString()
{
    stringstream ss;
    ss << (*this);
    return ss.str();
}

bool Trail::operator==(const Trail &other) const
{
    if(lastValidIP == other.getLastValidIP() && nbAnomalies == other.getNbAnomalies())
        return true;
    return false;
}
