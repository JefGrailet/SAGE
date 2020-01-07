/*
 * RouteHop.cpp
 *
 *  Created on: Sept 6, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in RouteHop.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "RouteHop.h"

RouteHop::RouteHop()
{
    ip = InetAddress(0);
    state = NOT_MEASURED;
    reqTTL = 0;
    replyTTL = 0;
}

RouteHop::RouteHop(ProbeRecord &record, bool peer)
{
    ip = record.getRplyAddress();
    reqTTL = record.getReqTTL(); // Regardless of having a proper reply or a timeout
    if(ip != InetAddress(0))
    {
        if(peer)
            state = PEERING_POINT;
        else
            state = VIA_TRACEROUTE;
        replyTTL = record.getRplyTTL();
    }   
    else
    {
        state = ANONYMOUS;
        replyTTL = 0;
    }
}

RouteHop::RouteHop(const RouteHop &other)
{
    *this = other;
}

RouteHop::~RouteHop()
{
}

RouteHop &RouteHop::operator=(const RouteHop &other)
{
    this->ip = other.ip;
    this->state = other.state;
    this->reqTTL = other.reqTTL;
    this->replyTTL = other.replyTTL;
    return *this;
}
