#ifndef __AL_AUX_FUNCS__
#define __AL_AUX_FUNCS__

#include <stdlib.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <FeaturesHandler.h>
#include <auxiliary.h>
#include <Timer.h>

using namespace std;

void DocToVec(const string & stFileName,
              const int iFeatureSetSize,
              FeaturesHandler * featureSet,
              vector<double> & vFtrsVec,
              FILE * debug_log);

void RandomTrainSet(const string & stTrainFile,
                    const string & stRandomFile,
                    const int iTrainVecsNumber);

void MultiCatToBinary(const string & stMultiCatSet,
                      const string & stBinarySet,
                      const string & stTarget);

void SummaryResults(const string & stSummaryFile,
                    const map<string,string,LexComp> & catsList);
                       
#endif
