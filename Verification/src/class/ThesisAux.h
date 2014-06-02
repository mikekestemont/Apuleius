#ifndef __THESIS_AUX__
#define __THESIS_AUX__

#include <string>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <math.h>
#include <windows.h>
#include <auxiliary.h>
#include <FileScanner.h>

using namespace std;

void AverageScoreOverChunks();
void BreakTexts();
void TextsToPairs();
void CreateIRScript();
void FilterEnglishText();
void GetAllPairs();
void GetNextWord(FileScanner & fsReader,string & stWord);
void TruncatePosts();
void TextsToPairScript();
void TextLengths();
void CreateApulieusIRTest();

class EncodedToken
{
    public:

        vector<string> vLetters;
        int iLetters;
        string stToken;

        EncodedToken(const int iSize);
        EncodedToken(const EncodedToken &);
        void operator=(const EncodedToken &);
        void Reset();
        void AddLetter(const string & stLetter);
};

#endif
