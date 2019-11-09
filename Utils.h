/*
 *
 * This file is a part of my arduino development.
 * If you make any modifications or improvements to the code, I would
 * appreciate that you share the code with me so that I might include
 * it in the next release. I can be contacted through paul.torruella@gmail.com

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU GENERAL PUBLIC LICENSE 3.0 license.
 * Please see the included documents for further information.

 * Commercial use of this file and/or every others file from this library requires
 * you to buy a license that will allow commercial use. This includes using the code,
 * modified or not, as a tool to sell products.

 *  The license applies to this file and its documentation.
 *
 *  Created on: 23 f�vr. 2016
 *  Author: Paul TORRUELLA
 */

#ifndef utils_h
#define utils_h

#include <Arduino.h>

extern char *__brkval;
extern char __bss_end;

namespace Utils{
int FreeRam();


class Semaphore{
	volatile bool isAvailable;
public:
	Semaphore():isAvailable(true){}

	bool take(){
		if(isAvailable)
			return !(isAvailable = false);
		return false;
	}

	void release(){
		isAvailable = true;
	}
};
}
#endif
