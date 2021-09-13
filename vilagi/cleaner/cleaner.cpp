#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <conio.h>
#include <windows.h>
#include <util\logger.h>

using namespace std;

const USHORT MAXCHARS = 255;
const char * AUX = "files.txt";

char source[MAXCHARS], dest[MAXCHARS];
int older_than = 7;

bool fileExists(const char* filePath) {
	DWORD dwAttrib = GetFileAttributes(filePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool fileTime(const char* filePath, LPFILETIME last_mod) {
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != NULL) {
		bool success = GetFileTime(hFile, NULL, NULL, last_mod);
		CloseHandle(hFile);
		return success;
	}
	return false;
}

bool fileSize(const char* filePath, PLARGE_INTEGER size) {
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != NULL) {
		bool success = GetFileSizeEx(hFile, size);
		CloseHandle(hFile);
		return success;
	}
	return false;
}

void readCfg(char * filename) {//station initialization
	char input[MAXCHARS];

	if (!fileExists(filename)) {
		log_0.output << log_0.timestamp() << filename << " sa nepodarilo najst/nacitat" << endl;
		exit(1);
	}

	ifstream fin(filename);

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	strcpy(source, strtok(NULL, " "));

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	strcpy(dest, strtok(NULL, " "));

	fin.getline(input, MAXCHARS);
	strtok(input, "=");
	older_than = atoi(strtok(NULL, " "));
}


int main() {
	LARGE_INTEGER src_size, dst_size;
	FILETIME t_sys, t_file;
	ULARGE_INTEGER t0, t1;

	readCfg("cleaner.ini");
	GetSystemTimeAsFileTime(&t_sys);

	cout << "Starting cleanup of " << source << endl << "press any key to abort..." << endl;
	for (size_t i = 10; i > 0; i--) {
		if (_kbhit())
			return(1);
		cout << setw(2) << left << i;
		Sleep(1000);
		cout << "\r";
	}

	USHORT dlength = (USHORT)strlen(dest), slength = (USHORT)strlen(source);
	USHORT soffset = dlength > slength ? dlength - slength : 0;
	USHORT doffset = abs(dlength - slength) - soffset;
	char path[MAXCHARS], cmd[MAXCHARS];
	char * 	src_path = path + soffset;
	char * 	dst_path = path + doffset;

	sprintf(cmd, "dir %s /S/B/O:D/T:W/A:-H-D > %s", source, AUX); system(cmd); //find files, non-hidden, non-directories, recursively sorted from oldest modification time

	ifstream fin("files.txt");
	while (!fin.eof()) {
		fin.getline(src_path, MAXCHARS);

		//do not delete if cannot get source size and time info
		if (!fileSize(src_path, &src_size) || !fileTime(src_path, &t_file))
			continue;

		//do not delete detlogs
		if (strstr(src_path, "detlog.csv") != NULL)
			continue;

		//compare file time and do not delete if not older than limit
		//memcpy(&t0, &t_file, sizeof(t1));
		//memcpy(&t1, &t_sys, sizeof(t1));
		t0.HighPart = t_file.dwHighDateTime;
		t0.LowPart = t_file.dwLowDateTime;
		t1.HighPart = t_sys.dwHighDateTime;
		t1.LowPart = t_sys.dwLowDateTime;

		double diff =  (t1.QuadPart - t0.QuadPart)/864000000000.0;
		if (diff < older_than)
			continue;

		//do not delete if file is not on destination and same size
		memcpy(dst_path, dest, dlength);
		if (!fileExists(dst_path) || !fileSize(dst_path, &dst_size) || src_size.QuadPart != dst_size.QuadPart)
			continue;

		memcpy(src_path, source, slength);
		cout << src_path << " deleted" << endl;
		if (DeleteFile(src_path)) {
			cout << src_path << " deleted" << endl;
			log_0.output << log_0.timestamp() << src_path << " deleted" << endl;
		}
	}
	fin.close();

	return(0);
}
