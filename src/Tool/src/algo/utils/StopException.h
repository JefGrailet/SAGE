/*
 * StopException.h
 *
 *  Created on: Nov 9, 2016
 *      Author: jefgrailet
 *
 * Exception thrown when the stop mode is entered. Originally from TreeNET v3.0.
 */

#ifndef STOPEXCEPTION_H_
#define STOPEXCEPTION_H_

#include <stdexcept>
using std::runtime_error;
#include <string>
using std::string;

class StopException : public runtime_error
{
public:
	StopException(const string &msg="Program is stopping");
	virtual ~StopException() throw();
};

#endif /* STOPEXCEPTION_H_ */
