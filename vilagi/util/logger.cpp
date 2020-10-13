#include <ctime>
#include <direct.h>
#include "logger.h"

FileIO log_0("log");

FileIO::FileIO(const char * extn) {
	strcpy(extension, extn);
	rotate();
}

FileIO::~FileIO() {
	output.close();
}

char * FileIO::timestamp() {
	time_t st = time(NULL);
	tm * sts = gmtime(&st);

	sprintf(timestring, "%02u/%02u %02u:%02u:%02u ", sts->tm_mday, 1 + sts->tm_mon, sts->tm_hour, sts->tm_min, sts->tm_sec);
	return(timestring);
}

void FileIO::rotate() {
	time_t st = time(NULL);
	tm * sts = gmtime(&st);

	if (sts->tm_year == year)
		return;

	if (output.is_open())
		output.close();

	_mkdir("log");

	year = sts->tm_year;

	sprintf(filename, "log/%04u.%s", 1900 + year, extension);
	output.open(filename, ios::out | ios::app);
	output.setf(std::ios_base::unitbuf);
}