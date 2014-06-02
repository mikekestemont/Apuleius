#define _FILE_OFFSET_BITS 64
#define __USE_LARGEFILE64

#include <stdlib.h>
#include <FileScanner.h>
#include <auxiliary.h>

//___________________________________________________________________
FileScanner::FileScanner(const std::string & _stFileName,const int buffer_size):
                stFileName(_stFileName),
                iBufferSize(buffer_size),
                cBufferP(NULL),
                fInputFileP(NULL),
                iNextByteToRead(0),
                iActualReadBytes(0)
{
    cBufferP = new unsigned char[iBufferSize + 1];

	if (cBufferP == NULL) abort_err("Failed to allocate buffer in FileScanner");
    fInputFileP = open_file(stFileName,"FileScanner::FileScanner","r",false);

	// Handle BOM bytes.
    this->SkipBOM();
}

//___________________________________________________________________
void FileScanner::SkipBOM()
{
    this->bMoreToRead();

    if (iActualReadBytes < 3) return; // Short / Dummy file.
    if (cBufferP[0] <= 0x7F) return;  // The first letter is ascii.

    if ((cBufferP[0] == 0xEF) && (cBufferP[1] == 0xBB) && (cBufferP[2] == 0xBF))
    {
        iNextByteToRead=3;
        //fprintf(stdout,"UTF-8's BOM detected: %s\n",stFileName.c_str()); fflush(stdout);
    }
    /*else
    {
        fprintf(stdout,"No BOM detected in %s although it starts by non-ascii sign...probably UTF-8 without BOM\n",stFileName.c_str());
        fflush(stdout);
        abort_err(string("Illegal BOM in the beginning of ")+stFileName+": "+stringOf((int)cBufferP[0])+", "+stringOf((int)cBufferP[1])+", "+stringOf((int)cBufferP[2]));
    }*/
}

//___________________________________________________________________
FileScanner::~FileScanner()
{
    delete [] cBufferP;
    fclose (fInputFileP);
}

//___________________________________________________________________
void FileScanner::restart()
{
    fclose(fInputFileP);
    fInputFileP = open_file(stFileName,"restart","r",false);
    for (int i=0;i<=iBufferSize;i++) cBufferP[i]='\0';
    this->SkipBOM();
}

//___________________________________________________________________
bool FileScanner::bMoreToRead()
{
    if (iNextByteToRead < iActualReadBytes)
        return true;

    iActualReadBytes = fread(cBufferP,1,iBufferSize,fInputFileP);
    iNextByteToRead = 0;

    return (iActualReadBytes > 0);
}

//___________________________________________________________________
char FileScanner::cGetNextByte()
{
    return cBufferP[iNextByteToRead++];
}

//___________________________________________________________________
char FileScanner::cLookAtNext()
{
    if (!this->bMoreToRead()) return '\0';
    return cBufferP[iNextByteToRead];
}

//___________________________________________________________________
const std::string & FileScanner::stGetFileName() const
{
    return stFileName;
}

// LinesReader:

//___________________________________________________________________
void LinesReader::voGetLine(std::string & stLine)
{
    char cReadSign;
    stLine.clear();

    while (this->bMoreToRead())
    {
        cReadSign = this->cGetNextByte();
        if (cReadSign == '\n')
            break;
        stLine += cReadSign;
    }
}

//___________________________________________________________________
int TdhReader::iGetLine(std::vector<string> & vLine)
{
    char cReadSign;
    int iField=0;
    bool bReadMore=true;
    string stToken(75,'\0');

    stToken="";
    while (this->bMoreToRead() && bReadMore)
    {
        cReadSign = this->cGetNextByte();
        if (cReadSign == '\n')
        {
            if (!stToken.empty())
            {
                SetVecEntry(vLine,stToken,iField,"TdhReader::iGetLine");
                iField++;
                stToken="";
            }
            bReadMore=false;
        }
        else if (cReadSign == '\t')
        {
            SetVecEntry(vLine,stToken,iField,"TdhReader::iGetLine");
            stToken="";
            iField++;
        }
        else
            stToken += cReadSign;
    }

    // Check is the last field was handled (in case the line ends by EOF).
    if (!stToken.empty())
    {
        SetVecEntry(vLine,stToken,iField,"TdhReader::iGetLine");
        iField++;
    }

    return iField;
}

//___________________________________________________________________
int TabSepReader::iGetLine(std::vector<string> & vLine)
{
    char cReadSign;
    int iField=0;
    bool bReadMore=true;
    string stToken(75,'\0');

    stToken = "";
    while (this->bMoreToRead() && bReadMore)
    {
        cReadSign = this->cGetNextByte();
        if (cReadSign == '\n')
        {
            if (!stToken.empty()) vLine.at(iField++)=stToken;
            bReadMore=false;
        }
        else if ((cReadSign == ' ') && stToken.empty()) continue;
        else if (cReadSign == ' ')
        {
            vLine.at(iField) = stToken;
            stToken="";
            iField++;
        }
        else
            stToken += cReadSign;
    }
    return iField;
}

//___________________________________________________________________
int TabCommaReader::iGetLine(std::vector<string> & vLine)
{
    char cReadSign;
    int iField=0;
    bool bReadMore=true;
    string stToken(75,'\0');

    stToken = "";
    while (this->bMoreToRead() && bReadMore)
    {
        cReadSign = this->cGetNextByte();
        if (cReadSign == '\n')
        {
            if (!stToken.empty()) vLine.at(iField++)=stToken;
            bReadMore=false;
        }
        else if (cReadSign == ',')
        {
            vLine.at(iField) = stToken;
            stToken="";
            iField++;
        }
        else
            stToken += cReadSign;
    }
    return iField;
}
