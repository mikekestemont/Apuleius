#include <FolderScanner.h>
#include <auxiliary.h>
#include <FileScanner.h>
#include <stdio.h>
#include <map>
#include <set>
#include <string>
#include <list>

//___________________________________________________________________        
FolderScanner::FolderScanner(const std::string & stFolderName):stScannedFolder(stFolderName)
{
    ws.assign (stScannedFolder.begin (), stScannedFolder.end ());
    hFile = FindFirstFile (ws.c_str(), &findData);
    iMore=1;
}

//___________________________________________________________________        
FolderScanner::~FolderScanner()
{
    FindClose(hFile);
}

//___________________________________________________________________        
bool FolderScanner::bMoreToRead() const
{
    return (iMore && (hFile != INVALID_HANDLE_VALUE));
}

//___________________________________________________________________        
string FolderScanner::getNextFile()
{
    string stFileName;
    int i;

    // Scan the folder until a 'legitimate' file is found.
    stFileName="";
    while (stFileName.empty())
    {
        // Get the current file name.
        const unsigned char * cTemp = (unsigned char*)findData.cFileName;
        stFileName="";
        for (i=0;i<1000;i+=2)
        {
            if (cTemp[i]==0) break;
            stFileName+=cTemp[i];
        }

        // Read the next file, for future iteration.
        iMore = FindNextFile (hFile, &findData);
    }
    return stFileName;
}

//___________________________________________________________________        
bool FolderScanner::bIsFolder() const 
{
    return ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}
