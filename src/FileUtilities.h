#include <SD.h>

namespace FileUtilities{
	long readlong(const char *filename, int pos);
	byte writelong(const char *filename, int pos, long val);
}