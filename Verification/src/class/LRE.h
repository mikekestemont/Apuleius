#ifndef __LRE_H__
#define __LRE_H__

#include <FeatureSet.h>
#include <math.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Timer.h>

using namespace std;

int LoadCandidates(vector<FeatureSet*> & vSampleItems,
                   const string        & stItemsFolder,
                   const string        & stItemsFile,
                   const map<string,FeatureInfo,LexComp> * lexicon);
void ClearItems(vector<FeatureSet*> & vItems,const int iItemsNumber);
void ClearItems(map<string,FeatureSet*,LexComp> & itemsMap);
void SelectRandomFeatures(const int iFeaturesNumber,
                          const int iMaxIndex,
                          vector<bool> & vRandomSet);

void StaticToRandom(const vector<FeatureSet*> & vStaticSet,
                    vector<FeatureSet*> & vRandomSet,
                    const int iItemsNumber);

void ConsolidateLogs(FILE * fLogFile,const string & stLogFile);

#endif
