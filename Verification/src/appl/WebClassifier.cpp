#include <ResourceBox.h>
#include <auxiliary.h>
#include <FileScanner.h>
#include <FolderScanner.h>
#include <math.h>
#include <FeaturesHandler.h>
#include <LRE.h>
#include <LREAuxBuffer.h>
#include <LREThreadBuffer.h>
#include <DataFuncs.h>
#include <PairClassifier.h>

using namespace std;

//___________________________________________________________________
void WebDecoyClassifier::ClassifyPair(FeatureSet * fsX,FeatureSet * fsY)
{
    vector<bool> vRandFtrsSet(buffer->lexicon->size()+1,false);
    vector<FeatureSet*> vRandomDecoys(iDecoysNumber);
    buffer->tiIRSrch.start();
    int i,j;
    const int iReqRandFtrs = (int)(buffer->lexicon->size()*dRandFtrsSetFreq);
    dRandDecoysRate = (double(iReqRandDecoyNum) / double(iDecoysNumber)) + 0.055;
    for (i=0;i<iIRLoops;i++)
    {
        buffer->tiSelFtr.start();
        SelectRandomFeatures(iReqRandFtrs,buffer->lexicon->size(),vRandFtrsSet);
        buffer->tiSelFtr.stop();

        buffer->tiGetRndDcy.start();
        j = this->SelectRandomDecoys(vRandomDecoys);
        buffer->tiGetRndDcy.stop();

        buffer->tiGetFtr.start();
        fsX->SetRandomFeatures(vRandFtrsSet);
        fsY->SetRandomFeatures(vRandFtrsSet);
        LREAuxBuffer::SetRandomFeatures(vRandFtrsSet,vRandomDecoys,j);
        buffer->tiGetFtr.stop();

        this->SetSearchKeys(fsX,fsY);
        this->RankPairByDecoys(fsX,fsY,vRandomDecoys,j);
    }
    buffer->tiIRSrch.stop();
}


//___________________________________________________________________
void WebDecoyClassifier::DoClassify()
{
    int i;
    
    vector< vector<string> * > vFtrsSets(iDecoyWindowSize);
    vector<int> vSetSize(iDecoyWindowSize,0);
    vector<string> vFileName(iDecoyWindowSize,"");

    const int iFtrsPoolBlockSize=5000;
    const int iSnipBlockSize=950;
    int iFtrsPoolSize;
    int iBeginSnipSize;
    int iEndSnipSize;
    vector<string> * vFtrsPool = NULL;
    vector<string> * vBeginSnip = NULL;
    vector<string> * vEndSnip = NULL;
    const string stBeginPrefix = ResourceBox::Get()->getStringValue("PrefixCodeBegin");
    const string stEndPrefix = ResourceBox::Get()->getStringValue("PrefixCodeEnd");

    vDecoySnips.resize(iDecoyWindowSize);
    for (i=0;i<iDecoyWindowSize;i++)
    {
        vDecoySnips.at(i) = NULL;
        vFtrsSets.at(i) = NULL;
    }

    if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stdout,"%s (%s) - Start\n",this->stGetClassifier().c_str(),buffer->stThreadID.c_str()); fflush(stdout);
    if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

    buffer->tiSrchAht.start();
    FeatureSet * fsX = NULL;
    FeatureSet * fsY = NULL;
    string stPairKey;
    string stWorkFolder;
    TdhReader tdhScript(stSampleFolder+buffer->stIRScriptFile,512);
    vector<string> vFields(2,"");
    i=0;
    while (tdhScript.bMoreToRead())
    {
        // Start Reading
        if (WaitForSingleObject(buffer->mReadMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Read)");
        
        tdhScript.iGetLine(vFields);
        i++;

        stPairKey = GetPairKey(vFields.at(0),vFields.at(1));
        stWorkFolder = stSampleFolder + stDecoyFolder + "\\" + stPairKey + "\\";

        buffer->tiReadDcys.start();
        iDecoysNumber = this->ReadDecoySet(vFtrsSets,vSetSize,vFileName,stWorkFolder,stWorkFolder+stSortedDecoysFile);
        buffer->tiReadDcys.stop();
        
        if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
        fprintf(stdout,"Working on pair (tid=%s): %d, #Decoys=%d\n",buffer->stThreadID.c_str(),i,iDecoysNumber); fflush(stdout);
        if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

        buffer->tiReadFS.start();
        vFtrsPool = new vector<string>(iFtrsPoolBlockSize);
        iFtrsPoolSize = ReadFeatureFile(stWorkFolder+stFeatureSet,*vFtrsPool,iFtrsPoolBlockSize,true);
        vBeginSnip = new vector<string>(iSnipBlockSize);
        iBeginSnipSize = ReadFeatureFile(stWorkFolder+"Ftrs\\"+stBeginPrefix+vFields.at(0),*vBeginSnip,iSnipBlockSize,false);
        vEndSnip = new vector<string>(iSnipBlockSize);
        iEndSnipSize = ReadFeatureFile(stWorkFolder+"Ftrs\\"+stEndPrefix+vFields.at(1),*vEndSnip,iSnipBlockSize,false);
        buffer->tiReadFS.stop();
        
        if (!ReleaseMutex(buffer->mReadMutex)) abort_err("Failed to release a mutex");
        // End Reading

        buffer->tiLoadFS.start();
        this->LoadFtrsSet(vFtrsPool,iFtrsPoolSize);
        fsX = new StaticSet(buffer->lexicon,*vBeginSnip,iBeginSnipSize,vFields.at(0));
        fsY = new StaticSet(buffer->lexicon,*vEndSnip,iEndSnipSize,vFields.at(1));
        buffer->tiLoadFS.stop();
        
        buffer->tiLoadDcys.start();
        this->LoadPrivateDecoys(vFtrsSets,vSetSize,vFileName,iDecoysNumber);
        buffer->tiLoadDcys.stop();

        this->ClassifyPair(fsX,fsY);

        ClearItems(vDecoySnips,iDecoysNumber);
        delete fsX;
        delete fsY;
        delete vFtrsPool;
        delete vBeginSnip;
        delete vEndSnip;
        map<string,FeatureInfo,LexComp> * w2fLex = const_cast<map<string,FeatureInfo,LexComp> *>(buffer->lexicon);
        delete w2fLex;
        buffer->lexicon=NULL;
    }
    buffer->iAnonymIndex = i;
    buffer->tiSrchAht.stop();
    
    if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stdout,"%s (%s) - End\n",this->stGetClassifier().c_str(),buffer->stThreadID.c_str()); fflush(stdout);
    if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");
}
