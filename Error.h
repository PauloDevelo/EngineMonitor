/*
 * Error.h
 *
 *  Created on: 30/01/2017
 *      Author: paul_
 */

#ifndef LIBRARIES_UTILS_ERROR_H_
#define LIBRARIES_UTILS_ERROR_H_

#include "Utils.h"
#include <Arduino.h>

#define ERROR(msg) 	{\
						Serial.print(F("ERROR["));\
						Serial.print(F(__FILE__));\
						Serial.print(F("][Line "));\
						Serial.print(__LINE__);\
						Serial.print(F("]["));\
						Serial.print(Utils::FreeRam());\
						Serial.print(F("]-->"));\
						Serial.println(F(msg));\
					}

#define ERROR_EXPR(msg)	{Serial.print(F(#msg));Serial.print(F(": "));Serial.println(msg);}



#endif /* LIBRARIES_UTILS_ERROR_H_ */
