/*
  Declaration of various auxiliary functions that are used for preparing
  and manipulating the data.
*/

#ifndef __AUXILIARY__
#define __AUXILIARY__

#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <math.h>
#include <windows.h>

using namespace std;

class FileScanner;
class FeatureSet;

// Used for converting string to lower/upper case.
const string vToLower="abcdefghijklmnopqrstuvwxyz";
const string vToUpper="ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define BUF_LEN 100

// For lexical comparison of strings.
struct LexComp
{
    bool operator()(const string & s1,const string & s2) const
    {
        return (s1.compare(s2) < 0);
    }
};

// Transforms a string to a builtin type (e.g. int, float, etc).
// Returns true on success, otherwise returns false.
template <class T>
bool string_as(T & t, 
               const std::string& s, 
               std::ios_base& (*f)(std::ios_base&))
{
    std::istringstream iss(s);
    return !(iss >> f >> t).fail();
}

// Stores the information about the freuency of a feature in the corpus.
//___________________________________________________________________
class PhraseHitCount
{
    public:

        string stPhrase;
        int iHitCount;

        PhraseHitCount(const string & _stPhrase="", const int _iRuleHC=0):stPhrase(_stPhrase),iHitCount(_iRuleHC){}
        PhraseHitCount(const PhraseHitCount & rf){(*this)=rf;}
        void operator=(const PhraseHitCount & rf) {stPhrase=rf.stPhrase; iHitCount=rf.iHitCount;}
};

// Sort features according to their frequencies, and then alphabetically.
//___________________________________________________________________
struct PHCComp
{
    bool operator()(const PhraseHitCount & phc1,const PhraseHitCount & phc2) const
    {
        if (phc1.iHitCount > phc2.iHitCount) return true;
        if (phc1.iHitCount < phc2.iHitCount) return false;
        return (phc1.stPhrase.compare(phc2.stPhrase) < 0);
    }
};

// Open a file and returns a pointer to it.
// - file_name - the full path of the file.
// - fucn_name - the name of the calling function (used for error messages).
// - mode - "w"/"r"/"a" etc. the mode of fopen.
// - bAddBOM - indicates whether to add the utf-8 code at the beginning of a printed file.
FILE * open_file(const string & file_name,
                 const string & func_name,
                 const string & mode,
                 const bool bAddBOM=false);

// Load the command line arguments to a <key,value> map.
// Used for configuring each run comfortably.
void LoadArgs(int argc, char * argv[], map<string,string,LexComp> & paramsMap);

// Exit with a proper error message.
void abort_err(const string & stErrMsg, FILE * debug_log=NULL);

// Generate a uniformly-distributed random number [0,1].
double get_uniform01_random ( void );

// Returns the log2 of the given number.
double log_base2(const double dNum);

// Lower/Upper case transformations.
void voToLowerCase(string & word);
void voToUpperCase(string & word);
bool bIsUpperCase(const string & stWord);

// Add an item to the lexicon and maintain the counters accordingly.
void CountItem(map<string,int,LexComp> & buffer, const string & stItemKey);

// Conversion of builtin types into string.
string stringOf(const int iNum);
string stringOf(const unsigned int iNum);
string stringOf(const double dNum);

// e.g. file.txt ==> file
void RemoveFileSuffix(string & stFileName, const int iLen);

// e.g 3 ==> 003
string padIntWithZeros(const int iNum, const int iMinLen);

//e.g. auxiliary ==> aux, or aux ==> auxzzz
void truncateAndPadString(string & str, const int iDesiredLength, const char cPad);

// e.g. <auxiliary, iliary, zzz) ==> auxzzz
bool ReplaceStr(string & stBuffer, const string & stSrc, const string & stDst, size_t iStart=0);

// e.g. " aux " ==> "aux"
void voTrimEdges(string & stInp, const string & stTrimmers);

// Fuctions used for transforming a string to its particles, given a seperator.
void StringToList(const string & stBuffer, const string & stDelimiter, list<string> & lVals);
int StringToVec(const string & stBuffer, const string & stDelimiter, vector<string> & vBuffer);
void StringToSet(const string & stBuffer, const string & stDelimiter, set<string,LexComp> & sVals);
string GetNextToken(string & stBuffer, const string & stDelimiter);
void SetVecEntry(vector<string> & vBuffer, const string & stItem, const int index, const string & stFuncName);

// Count the number of lines in the given file.
int LinesNumber(const string & stFileName);

// Read the utf-8 letter starting at iLoc in string stBuffer.
// The letter is retrieved by stLetter and iLoc is updated accordingly.
void GetUTF8Letter(const string & stBuffer, string & stLetter, unsigned int & iLoc);

// Print a section from the input file into the output file.
// - fsBlog - scanner/reader of the input file.
// - iWordsToPrint - the length of the section to print (by words).
// - fSnippet - the output file.
// Notice - the section starts by the current location in the input file.
//          if the section is longer than what's left in the input file -
//          just the actual content is printed.
//          Return the number of printed words.
int PrintSnippet(FileScanner & fsBlog,
                 const int iWordsToPrint,
                 FILE * fSnippet);

// Print ROC table of the many-candidates method.
void PrintIRAccuracyCurve(const vector<int> & vTrueAccept,
                          const vector<int> & vFalseAccept,
                          const int iIRLoops,
                          const int iTrueItems,
                          const string & stROCTable);

// 
int CountFiles(const string & stPath);
int ScriptSize(const string & stScriptFile);
void LoadCategoryList(map<string,string,LexComp> & catsList,
                      map<int,string> * indToCat,
                      map<string,int,LexComp> * catToInd,
                      const string & stParamName);
string GetPairKey(const FeatureSet * fsX, const FeatureSet * fsY);
string GetPairKey(const string & stX, const string & stY);
void CleanFolder(const string & stPath);
bool bIsWhiteSpace(const char cByte);

#endif
