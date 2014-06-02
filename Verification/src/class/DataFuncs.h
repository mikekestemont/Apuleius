#ifndef __DATAFUNCS__
#define __DATAFUNCS__

#include <auxiliary.h>
#include <ResourceBox.h>
#include <math.h>
#include <FileScanner.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <FeatureSet.h>

using namespace std;

//___________________________________________________________________
void LoadFeatureSet(map<string,FeatureInfo,LexComp> & w2fLex,
                    map<int,string>                 * i2fLex,
                    const string & stFeatureSet,
                    FILE * debug_log);

//___________________________________________________________________
void LoadFeatureSet(map<string,FeatureInfo,LexComp> & w2fLex,
                    map<int,string>                 * i2fLex,
                    const vector<string> & vFtrsPool,
                    const int iFeaturesNumber);

//___________________________________________________________________
int PrintFeatureSet(const string & stFileName,
                    const map<string,FeatureInfo,LexComp> & lexicon,
                    const int iWordsNumber,
                    const int iDocsNumber,
                    FILE * debug_log);

//___________________________________________________________________
int PrintFeatureSet(const string & stFileName,
                    const map<string,FeatureInfo,LexComp> & lexicon,
                    FILE * debug_log);

//___________________________________________________________________
void LoadMeanVarData(const string & stFileName,
                     const map<string,FeatureInfo,LexComp> & w2fLex,
                     vector<double> & vMeanVec,
                     vector<double> & vVarVec);
                    

//___________________________________________________________________
void ReadFeaturesVec(FileScanner & fsTrainSet,
                     vector<double> & vFeaturesVec,
                     int & iRightClass,
                     const int iFeatureSetSize,
                     string & stInfo);

//___________________________________________________________________
void PrintFeaturesVec(FILE * fTrainVecs,
                      const vector<double> & vDiff,
                      const string & stTargetVal,
                      const string & stInfo,
                      const int iFeatureSetSize);

#endif
