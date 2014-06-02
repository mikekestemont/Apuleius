// FolderScanner.h
// This object is used for scanning a folder.

#ifndef __FolderScanner__
#define __FolderScanner__

#include <errno.h>
#include <sstream>
#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

using namespace std;

class FolderScanner
{
    private:

        HANDLE hFile;
        WIN32_FIND_DATA findData;
        int iMore;
        std::wstring ws;
        const string stScannedFolder;

        // Avoid copy operations.
        FolderScanner(const FolderScanner&);
        void operator=(const FolderScanner&);

    public:

        FolderScanner(const std::string & stFolderName);
        virtual ~FolderScanner();

        virtual bool bMoreToRead() const;
        virtual bool bIsFolder() const;
        virtual string getNextFile();
};

#endif
