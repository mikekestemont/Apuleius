#ifndef __LREThreadBuffer__
#define __LREThreadBuffer__

#include <string>
#include <vector>
#include <Timer.h>
#include <LREAuxBuffer.h>
#include <SimilarityMeasure.h>

using namespace std;

//___________________________________________________________________
class LREThreadBuffer
//___________________________________________________________________
{
    public:

        HANDLE tid;
        HANDLE mLogMutex;
        HANDLE mReadMutex;

        const map<string,FeatureInfo,LexComp> * lexicon;
        
        LREAuxBuffer * irAuxBuffer;
        SimilarityMeasure * simMatch;

        const vector<FeatureSet*> * vAuthors;
        const int iAuthorsNumber;
        
        string stIRScriptFile;
        string stIRQueryLog;
        string stDebugLog;
        string stThreadID;
        
        const int iMaxAnonymsNum;
        
        int iAnonymIndex;
                
        Timer tiSrchAht;
        Timer tiGetRndDcy;
        Timer tiGetFtr;
        Timer tiSelFtr;
        Timer tiIRSrch;
        Timer tiCosine;
        Timer tiLoadDcys;
        Timer tiLoadFS;
        Timer tiReadDcys;
        Timer tiReadFS;
        
        LREThreadBuffer(HANDLE _mLogMutex=NULL,
                        HANDLE _mReadMutex=NULL,
                        const int _iMaxAnonymsNum=0,
                        const int _iAuthorsNumber=0):
                        mLogMutex(_mLogMutex),
                        mReadMutex(_mReadMutex),
                        iMaxAnonymsNum(_iMaxAnonymsNum),
                        lexicon(NULL),
                        irAuxBuffer(NULL),
                        simMatch(NULL),
                        iAuthorsNumber(_iAuthorsNumber)
        {
            tiSelFtr.reset();
            tiSrchAht.reset();
            tiGetFtr.reset();
            tiGetRndDcy.reset();
            tiIRSrch.reset();
            tiCosine.reset();
            tiLoadDcys.reset();
            tiLoadFS.reset();
            tiReadDcys.reset();
            tiReadFS.reset();
        }
        
        virtual ~LREThreadBuffer()
        {
            if (irAuxBuffer != NULL) delete irAuxBuffer;
            if (simMatch != NULL) delete simMatch;
        }
    
    private:
        
        LREThreadBuffer(const LREThreadBuffer&);
        void operator=(const LREThreadBuffer&);
};

#endif
