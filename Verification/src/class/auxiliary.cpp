/*
  Various auxiliary functions for manipulating data, texts, files, etc.
  It includes:
    - Open files for reading/writing.
	- Transforming strings to upper/lower case.
	- Scanning folders.
	- etc.
*/
#include <string>
#include <list>
#include <sstream>
#include <iostream>
#include <auxiliary.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <FileScanner.h>
#include <FeatureSet.h>
#include <FolderScanner.h>
#include <ResourceBox.h>

using namespace std;

//___________________________________________________________________
void abort_err(const string & stErrMsg, FILE * debug_log)
{
    fprintf(stderr,"\n\nErr: %s\n\n",stErrMsg.c_str()); fflush(stderr);
    if (debug_log != NULL) {fprintf(debug_log,"%s\n",stErrMsg.c_str()); fflush(debug_log);}
    exit(0);
}

//___________________________________________________________________
FILE * open_file(const string & file_name,
                 const string & func_name,
                 const string & mode,
                 const bool bAddBOM)
{
    FILE * file = fopen(file_name.c_str(),mode.c_str());
    if (file == NULL) abort_err(string("Could not open the file ")+file_name+" in function "+func_name+". err: "+strerror(errno));

    if ((mode.at(0) == 'w') && bAddBOM)
    {
        fprintf(file,"%c%c%c",0xEF,0xBB,0xBF);
        fflush(file);
    }

    return file;
}

/* ================================================================ */
double get_uniform01_random ( void )
/* ================================================================ */
{
   /*  this routine return a uniforn randon variable in the
       interval [ 0 , 1 ]                                       */
    static time_t itemp , idum  ;
    static unsigned long jflone  = 0x3f800000 ;
    static unsigned long jflmsk  = 0x007fffff ;
    static int first_time =  1 ;
    static  double xrand ;

    if (first_time == 1)
    {
        idum = 1217271733;
        idum = time( NULL);
        //printf( "\n %% seed for random %d\n ",idum);
        first_time = 0 ; 
    }
    idum = 1664525L * idum + 1013904223L  ;

    itemp = jflone | ( jflmsk & idum ) ;
    xrand = (*(float *)&itemp) - 1.0 ;

   return xrand ;
}

//___________________________________________________________________
double log_base2(const double dNum)
{
    if (dNum <= 0)
        return -1;
    double dLogNum = log(dNum);
    dLogNum /= log(2.0);
    return dLogNum;
}

//___________________________________________________________________
char cToLower(const char cLetter)
{
    if ((cLetter >= 'A') && (cLetter <= 'Z')) return vToLower.at(cLetter-'A');
    return cLetter;
}

//___________________________________________________________________
char cToUpper(const char cLetter)
{
    if ((cLetter >= 'a') && (cLetter <= 'z')) return vToUpper.at(cLetter-'a');
    return cLetter;
}

//___________________________________________________________________
void voToLowerCase(string & word)
{
    const int len = word.length();
    int i;
    
    for (i=0;i<len;i++)
        word.at(i)=cToLower(word[i]);
}

//___________________________________________________________________
void voToUpperCase(string & word)
{
    const int len = word.length();
    int i;
    
    for (i=0;i<len;i++)
        word.at(i)=cToUpper(word[i]);
}

//___________________________________________________________________
bool bIsUpperCase(const string & stWord)
{
    const int len = stWord.length();
    int i;
    char cLetter;
    
    for (i=0;i<len;i++)
    {
        cLetter=stWord.at(i);
        if ((cLetter >= 'a') && (cLetter <= 'z')) return false;
    }
    return true;
}

//___________________________________________________________________
void LoadArgs(int argc, char * argv[], map<string,string,LexComp> & paramsMap)
{
    int i;
    string stKey;
    string stVal;
    size_t iPos;
    
    paramsMap.clear();

    for (i=3;i<argc;i++)
    {
        stKey = argv[i];
        if ((iPos = stKey.find("=")) == string::npos) continue;

        stVal = stKey.substr(iPos+1);
        stKey = stKey.substr(0,iPos);
        paramsMap.insert(map<string,string,LexComp>::value_type(stKey,stVal));
    }
}

//___________________________________________________________________
bool ReplaceStr(string & stBuffer, const string & stSrc, const string & stDst, size_t iStart)
{
    const size_t iBegin = stBuffer.find(stSrc,iStart);
    const size_t iEnd = iBegin + stSrc.size();

    if (iBegin == string::npos) return false;

    if (iEnd >= stBuffer.size()) stBuffer = stBuffer.substr(0,iBegin) + stDst;
    else stBuffer = stBuffer.substr(0,iBegin) + stDst + stBuffer.substr(iEnd);

    return true;
}

//___________________________________________________________________
string stringOf(const int iNum)
{
    char cBuf[BUF_LEN];
    _snprintf(cBuf,BUF_LEN,"%d",iNum);
    return cBuf;
}

//___________________________________________________________________
string stringOf(const unsigned int iNum)
{
    char cBuf[BUF_LEN];
    _snprintf(cBuf,BUF_LEN,"%u",iNum);
    return cBuf;
}

//___________________________________________________________________
string stringOf(const double dNum)
{
    char cBuf[BUF_LEN];
    _snprintf(cBuf,BUF_LEN,"%.5f",dNum);
    return cBuf;
}

//___________________________________________________________________
string padIntWithZeros(const int iNum, const int iMinLen)
{
    const int iNumLen = (int)floor((double)log10((double)iNum)+1);
    string stPad;
    const int iPadLen = iMinLen - iNumLen;
    int i;
    for (i=0;i<iPadLen;i++)
    {
        stPad += "0";
    }

    char cBuf[BUF_LEN];
    _snprintf(cBuf,BUF_LEN,"%d",iNum);
    
    return (stPad+cBuf);
}

//___________________________________________________________________
void truncateAndPadString(string & str, const int iDesiredLength, const char cPad)
{
	const int len = str.size();

	if (len > iDesiredLength)
	{
		str = str.substr(0, iDesiredLength);
	}
	else if (len < iDesiredLength)
	{
		for (int i = 0; i < (iDesiredLength - len); i++)
		{
			str += cPad;
		}
	}
}

//___________________________________________________________________
void voTrimEdges(string & stBuffer, const string & stTrimmers)
{
    size_t iPos;

    iPos = stBuffer.find_first_not_of(stTrimmers);
    if (iPos==string::npos) stBuffer="";
    else stBuffer=stBuffer.substr(iPos);

    iPos = stBuffer.find_last_not_of(stTrimmers);
    if (iPos==string::npos) stBuffer="";
    else stBuffer=stBuffer.substr(0,iPos+1);
}

//___________________________________________________________________
string GetNextToken(string & stBuffer, const string & stDelimiter)
{
    size_t iPos;
    string stToken;

    voTrimEdges(stBuffer,stDelimiter);
    iPos=stBuffer.find_first_of(stDelimiter);
    if (iPos == string ::npos)
    {
        stToken=stBuffer;
        stBuffer="";
    }
    else
    {
        stToken=stBuffer.substr(0,iPos);
        stBuffer=stBuffer.substr(iPos+1);
    }
    return stToken;
}

//___________________________________________________________________
void StringToList(const string & stBuffer, const string & stDelimiter, list<string> & lVals)
{
    string stTempBuf = stBuffer;
    string stToken;

    lVals.clear();
    while (!stTempBuf.empty())
    {
        stToken = GetNextToken(stTempBuf,stDelimiter);
        lVals.push_back(stToken);
    }
}

//___________________________________________________________________
int StringToVec(const string & stBuffer, const string & stDelimiter, vector<string> & vBuffer)
{
    list<string> lVals;
    StringToList(stBuffer,stDelimiter,lVals);
    vBuffer.reserve(lVals.size()+1);
    int i=0;
    while (!lVals.empty())
    {
        vBuffer.at(i++)=lVals.front();
        lVals.pop_front();
    }
    
    return i;
}

//___________________________________________________________________
void StringToSet(const string & stBuffer, const string & stDelimiter, set<string,LexComp> & sVals)
{
    list<string> lVals;
    StringToList(stBuffer,stDelimiter,lVals);

    sVals.clear();
    while (!lVals.empty())
    {
        sVals.insert(lVals.front());
        lVals.pop_front();
    }
}

//___________________________________________________________________
int LinesNumber(const string & stFileName)
{
    LinesReader lrReader(stFileName,512);
    string stLine;
    int iLines=0;
    while (lrReader.bMoreToRead())
    {
        lrReader.voGetLine(stLine);
        iLines++;
    }
    return iLines;
}

//___________________________________________________________________
void CountItem(map<string,int,LexComp> & buffer, const string & stItemKey)
{
    map<string,int,LexComp>::iterator iter=buffer.find(stItemKey);

    if (iter==buffer.end())
    {
        buffer.insert(map<string,int,LexComp>::value_type(stItemKey,1));
    }
    else
    {
        iter->second++;
    }
}

//___________________________________________________________________
void SetVecEntry(vector<string> & vBuffer, const string & stItem, const int index, const string & stFuncName)
{
    try
    {
        vBuffer.at(index) = stItem;
    }
    catch (...)
    {
        abort_err(string("Failed to access vec in "+stFuncName+". i=")+stringOf(index)+", token="+stItem);
    }
}

//___________________________________________________________________
void GetUTF8Letter(const string & stBuffer, string & stLetter, unsigned int & iLoc)
{
    unsigned char cByte;
    unsigned int iLetterLen=1;

    stLetter="";
    stLetter += stBuffer.at(iLoc);

    while ((iLoc + iLetterLen) < stBuffer.size())
    {
        cByte = stBuffer.at(iLoc+iLetterLen);
        if ((cByte & 0xC0) == 0x80)
        {
            stLetter+=cByte;
            iLetterLen++;
        }
        else
        {
            break;
        }
    }
    iLoc+=(iLetterLen-1);
}

//___________________________________________________________________
bool bIsWhiteSpace(const char c)
{
    if (c == ' ') return true;
    if (c == '\t') return true;
    if (c == '\n') return true;
    if (c == '\r') return true;
    return false;
}

//___________________________________________________________________
int PrintSnippet(FileScanner & fsBlog,
                 const int iWordsToPrint,
                 FILE * fSnippet)
{
    int iWordsNumber=0;
    char cByte;
    const bool bPrint = (fSnippet != NULL);
    string stWord(512,0);

    stWord="";
    while (fsBlog.bMoreToRead())
    {
        cByte = fsBlog.cGetNextByte();
        if (bPrint) {stWord+=cByte;}

        if (bIsWhiteSpace(cByte))
        {
            iWordsNumber++;
            if (iWordsNumber > iWordsToPrint) break;
            while (bIsWhiteSpace(cByte) && fsBlog.bMoreToRead())
            {
                cByte = fsBlog.cGetNextByte();
                if (bPrint) {stWord+=cByte;}
            }
            if (bPrint) {fprintf(fSnippet,"%s",stWord.c_str()); fflush(fSnippet); stWord="";}
        }
    }

	if (bPrint && (stWord.size()>0)) {fprintf(fSnippet,"%s",stWord.c_str()); fflush(fSnippet); stWord="";}
    return iWordsNumber;
}

//___________________________________________________________________
void PrintIRAccuracyCurve(const vector<int> & vTrueAccept,
                          const vector<int> & vFalseAccept,
                          const int iIRLoops,
                          const int iTrueItems,
                          const string & stROCTable)
{
    FILE * fROCTable = open_file(stROCTable,"PrintIRRes","w");
    
    fprintf(fROCTable,"Raw ROC Curve:\n\nPrecision\tRecall\t#Loops\tTA\tFA\n");
    double dPrec,dRec,dReqRec,dBestPrec;
    int k,i,iBestLoop;
    for (i=iIRLoops;i>0;i--)
    {
        if ((vTrueAccept.at(i) <= 0) && (vFalseAccept.at(i) <= 0)) continue;
        dRec = (double)vTrueAccept.at(i) / (double)iTrueItems;
        dPrec = (double)vTrueAccept.at(i) / (double)(vTrueAccept.at(i) + vFalseAccept.at(i));
        fprintf(fROCTable,"%.5f\t%.5f\t%d\t%d\t%d\n",dPrec,dRec,i,vTrueAccept.at(i),vFalseAccept.at(i));
        fflush(fROCTable);
    }
    
    fprintf(fROCTable,"\n\nNorm ROC Curve:\n\nRecall\tPrecision\t#Loops\n");
    for (k=0;k<=100;k++)
    {
        dReqRec = (double)k/100.0;
        dBestPrec=0.0;
        for (i=iIRLoops;i>0;i--)
        {
            dRec = (double)vTrueAccept.at(i) / (double)iTrueItems;
            
            if (dRec < dReqRec) continue;

            dPrec = (double)vTrueAccept.at(i) / (double)(vTrueAccept.at(i) + vFalseAccept.at(i));
            if (dPrec > dBestPrec)
            {
                dBestPrec=dPrec;
                iBestLoop=i;
            }
        }

        fprintf(fROCTable,"%.5f\t%.5f\t%d\n",dReqRec,dBestPrec,iBestLoop);
        fflush(fROCTable);
    }
    
    fflush(fROCTable);
    fclose(fROCTable);
}

//___________________________________________________________________
int CountFiles(const string & stPath)
{
    FolderScanner    folderScanner(stPath);
    string           stFileName;
    int              iFiles=0;
    
    while (folderScanner.bMoreToRead())
    {
        stFileName = folderScanner.getNextFile();
        if ((stFileName.compare(".") == 0) || (stFileName.compare("..") == 0))
            continue;
        iFiles++;
    }
    
    return iFiles;
}

//___________________________________________________________________
int ScriptSize(const string & stScriptFile)
{
    LinesReader lrReader(stScriptFile,512);
    string stItem(75,'\0');
    int iItems=0;
    while (lrReader.bMoreToRead())
    {
        lrReader.voGetLine(stItem);
        iItems++;
    }
    return iItems;
}

//___________________________________________________________________
void LoadCategoryList(map<string,string,LexComp> & catsList,
                      map<int,string> * indToCat,
                      map<string,int,LexComp> * catToInd,
                      const string & stParamName)
{
    list<string> paramsList;
    list<string>::const_iterator iter;
    ResourceBox::Get()->getListValues(stParamName,paramsList);
    string stCategory;
    size_t iPos;
    int index;

    catsList.clear();
    while (!paramsList.empty())
    {
        stCategory=paramsList.front();
        paramsList.pop_front();
        iPos = stCategory.find(':');
        if (iPos == string::npos) {fprintf(stderr,"Bad Cat Ent: %s\n",stCategory.c_str()); fflush(stderr); exit(0);}
        catsList.insert(map<string,string,LexComp>::value_type(stCategory.substr(0,iPos),stCategory.substr(iPos+1)));

        if ((indToCat == NULL) && (catToInd == NULL)) continue;

        if (!string_as<int>(index,stCategory.substr(iPos+1),std::dec)) abort_err(string("Illegal Categoty: ")+stCategory);
        if (indToCat != NULL) indToCat->insert(map<int,string>::value_type(index,stCategory.substr(0,iPos)));
        if (catToInd != NULL) catToInd->insert(map<string,int,LexComp>::value_type(stCategory.substr(0,iPos),index));
    }
}

//___________________________________________________________________
string GetPairKey(const FeatureSet * fsX, const FeatureSet * fsY)
{
    return GetPairKey(fsX->stGetAuthorName(),fsY->stGetAuthorName());
}

//___________________________________________________________________
string GetPairKey(const string & stX, const string & stY)
{
    string stPairKey(75,'\0');

    stPairKey = stX;

    RemoveFileSuffix(stPairKey,4);

    stPairKey += "_";
    stPairKey += stY;

    RemoveFileSuffix(stPairKey,4);

    return stPairKey;
}

//___________________________________________________________________
void CleanFolder(const string & stPath)
{
    FolderScanner    folderScanner(stPath);
    string           stCurrentFile;
    string           stFileName;
    
    while (folderScanner.bMoreToRead())
    {
        stFileName = folderScanner.getNextFile();
        if ((stFileName.compare(".") == 0) || (stFileName.compare("..") == 0))
            continue;
        stCurrentFile = stPath;
        stCurrentFile += stFileName;
        
        if (remove(stCurrentFile.c_str()) < 0)
        {
            fprintf(stderr,"Failed to remove the file %s. err: %s\n",stCurrentFile.c_str(),strerror(errno));
            fflush(stderr);
            exit(0);
        }
    }
}

//___________________________________________________________________
void RemoveFileSuffix(string & stFileName, const int iLen)
{
    stFileName = stFileName.substr(0,stFileName.length()-iLen);
}
