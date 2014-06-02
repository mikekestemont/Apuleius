// FileScanner.h
// This object is used for reading a file
// as a vector.

#ifndef __FileScanner__
#define __FileScanner__

#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

using namespace std;

class FileScanner
{
    private:

        std::string     stFileName;

        unsigned char *cBufferP;
        const int  iBufferSize;
        int        iActualReadBytes;
        int        iNextByteToRead;
        FILE       *fInputFileP;

        // Avoid copy operations.
        FileScanner(const FileScanner&);
        void operator=(const FileScanner&);

		virtual void SkipBOM();

    public:

        FileScanner(const std::string & _stFileName,const int buffer_size);
        virtual ~FileScanner();

        virtual bool bMoreToRead();
        virtual char cGetNextByte();
        virtual char cLookAtNext();

        const std::string & stGetFileName() const;
        
        FILE * fGetInputFile() const {return fInputFileP;}
        
        void restart();
};

//___________________________________________________________________
class LinesReader: public FileScanner
//___________________________________________________________________
{
    private:

        LinesReader(const LinesReader&);
        void operator=(const LinesReader&);

    public:

        LinesReader(const std::string & _stFileName,const int buffer_size):
          FileScanner(_stFileName,buffer_size){}

        virtual ~LinesReader(){}

        virtual void voGetLine(std::string & stLine);
};

//___________________________________________________________________
class TdhReader: public FileScanner
//___________________________________________________________________
{
    private:

        TdhReader(const TdhReader&);
        void operator=(const TdhReader&);

    public:

        TdhReader(const std::string & _stFileName,const int buffer_size):
          FileScanner(_stFileName,buffer_size){}

        virtual ~TdhReader(){}

        virtual int iGetLine(vector<string> & vLine);
};

//___________________________________________________________________
class TabSepReader: public TdhReader
//___________________________________________________________________
{
    private:

        TabSepReader(const TabSepReader&);
        void operator=(const TabSepReader&);

    public:

        TabSepReader(const std::string & _stFileName,const int buffer_size):
          TdhReader(_stFileName,buffer_size){}

        virtual ~TabSepReader(){}

        virtual int iGetLine(vector<string> & vLine);
};

//___________________________________________________________________
class TabCommaReader: public TdhReader
//___________________________________________________________________
{
    private:

        TabCommaReader(const TabCommaReader&);
        void operator=(const TabCommaReader&);

    public:

        TabCommaReader(const std::string & _stFileName,const int buffer_size):
          TdhReader(_stFileName,buffer_size){}

        virtual ~TabCommaReader(){}

        virtual int iGetLine(vector<string> & vLine);
};

#endif
