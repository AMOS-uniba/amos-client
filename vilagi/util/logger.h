#pragma once
#include <fstream>

using namespace std;

class FileIO {

public:
  FileIO(const char * extn);
  ~FileIO();
  char * timestamp();
  ofstream output;
  void rotate();

private:
  char filename[255];
  char timestring[16];
  char extension[10];
  int year=0;
};

extern FileIO log_0;
extern FileIO log_err;
extern FileIO log_st;
