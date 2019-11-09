/*
 * MemChuck.h
 *
 *  Created on: 19/04/2017
 *      Author: paul_
 */

#ifndef LIBRARIES_UTILS_MEMCHUNCK_H_
#define LIBRARIES_UTILS_MEMCHUNCK_H_

#include "Error.h"
#include <Arduino.h>

class MemChunck{
private:
	uint8_t* _allocatedMem;
	const size_t _size;
	size_t _currW = 0;
	mutable size_t _currR = 0;

public:
	MemChunck(void* allocatedMem, size_t size, bool initMem):_allocatedMem((uint8_t*)allocatedMem), _size(size){
		if(allocatedMem == 0){
			ERROR("Memory not allocated")
		}

		if(size <= 0){
			ERROR("Size null")
		}

		if(initMem){
			for(size_t i = 0; i < size; i++){
				_allocatedMem[i] = 0;
			}
		}
	}

	void write(const void* memToWrite, size_t size){
		if(_currW + size <= _size){
			memcpy(_allocatedMem + _currW, memToWrite, size);
			_currW += size;
		}
		else{
			ERROR("Out of the memory bound")
		}
	}

	void read(void* mem, size_t size, bool forward = true)const{
		if(_currR + size <= _size){
			memcpy(mem, _allocatedMem + _currR, size);

			if(forward){
				_currR += size;
			}
		}
		else{
			ERROR("Out of the memory bound")
		}
	}

	char readChar()const{
		char c = 0;
		read(&c, 1);
		return c;
	}

	void forward(size_t size = 1)const{
		if(_currR + size <= _size){
			_currR += size;
		}
		else{
			ERROR("Out of the memory bound")
		}
	}

	int compare(const void* mem, size_t size, bool forward = true)const{
		if(_currR + size <= _size){
			int cmpResult = memcmp(mem, _allocatedMem + _currR, size);
			if(forward){
				_currR += size;
			}
			return cmpResult;
		}
		else{
			ERROR("Out of the memory bound")
			return -1;
		}
	}

	const size_t getCurrentSize() const{
		return _currW;
	}

	const size_t getMaxSize() const{
		return _size;
	}

	bool available()const{
		return getCurrentSize() < getMaxSize();
	}

};



#endif /* LIBRARIES_UTILS_MEMCHUNCK_H_ */
