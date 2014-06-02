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
void RandomDecoyClassifier::InitClassify()
{
    fprintf(stdout,"\nRandomDecoyClassifier::InitClassify - Start\n"); fflush(stdout);
    
    this->LoadFtrsSet();
    
    fprintf(stdout,"Load Candidates - Start\n"); fflush(stdout);
    iSnipsNumber = this->LoadPairSnips(stScriptFile);
    fprintf(stdout,"Load Candidates - End (#Snips=%d)\n",iSnipsNumber); fflush(stdout);

    fprintf(stdout,"Load Decoys - Start: file=%s, folder=%s\n",stDecoyFile.c_str(),stDecoyFolder.c_str()); fflush(stdout);
    const int iMaxDecoysNumber = ScriptSize(stSampleFolder + stDecoyFile);
    vDecoySnips.resize(iMaxDecoysNumber);
    iDecoysNumber = LoadCandidates(vDecoySnips,stSampleFolder+stDecoyFolder+"\\Ftrs\\",stSampleFolder+stDecoyFile,buffer->lexicon);
    dRandDecoysRate = (double(iReqRandDecoyNum) / double(iDecoysNumber)) + 0.055;
    fprintf(stdout,"Load Decoys - End: #Decoys=%d, RandRate=%.3f\n",iDecoysNumber,dRandDecoysRate); fflush(stdout);

    fprintf(stdout,"RandomDecoyClassifier::InitClassify - End\n\n"); fflush(stdout);
}

//___________________________________________________________________
void RandomDecoyClassifier::DoClassify()
{
    fprintf(stdout,"\nRandomDecoyClassifier::DoClassify - Start\n"); fflush(stdout);
    fprintf(stdout,"UniformDecoys - Start (IRLoops=%d, Rand Set Sz=%d, #TotalDcy=%d, RandDcyRate=%.3f)\n",iIRLoops,iRandFtrsSetNum,iDecoysNumber,dRandDecoysRate); fflush(stdout);
    
    vector<bool> vRandFtrsSet(buffer->lexicon->size()+1,false);
    vector<FeatureSet*> vRandomDecoys(iDecoysNumber);
    buffer->tiIRSrch.start();
    int iIter,iDecoys,iSnipInd;
    string stPairEntryKey;
    string stPairTargetKey;
    for (iIter=0;iIter<iIRLoops;iIter++)
    {
        fprintf(stdout,"start iteration %d\n",iIter); fflush(stdout);

        buffer->tiSelFtr.start();
        SelectRandomFeatures(iRandFtrsSetNum,buffer->lexicon->size(),vRandFtrsSet);
        buffer->tiSelFtr.stop();

        fprintf(stdout,"\tSelect Decoys\n",iIter); fflush(stdout);
        buffer->tiGetRndDcy.start();
        iDecoys = this->SelectRandomDecoys(vRandomDecoys);
        buffer->tiGetRndDcy.stop();

        fprintf(stdout,"\tSet Features\n",iIter); fflush(stdout);
        buffer->tiGetFtr.start();
        fprintf(stdout,"\t\tBegin Snips\n",iIter); fflush(stdout);
        LREAuxBuffer::SetRandomFeatures(vRandFtrsSet,vBeginSnips,iSnipsNumber);
        fprintf(stdout,"\t\tEnd Snips\n",iIter); fflush(stdout);
        LREAuxBuffer::SetRandomFeatures(vRandFtrsSet,vEndSnips,iSnipsNumber);
        fprintf(stdout,"\t\tDcys Snips\n",iIter); fflush(stdout);
        LREAuxBuffer::SetRandomFeatures(vRandFtrsSet,vRandomDecoys,iDecoys);
        buffer->tiGetFtr.stop();

        fprintf(stdout,"\tClassify\n",iIter); fflush(stdout);
        buffer->tiSrchAht.start();
        for (iSnipInd=0;iSnipInd<iSnipsNumber;iSnipInd++)
        {
            this->SetSearchKeys(vBeginSnips.at(iSnipInd),vEndSnips.at(iSnipInd));
            this->RankPairByDecoys(vBeginSnips.at(iSnipInd),vEndSnips.at(iSnipInd),vRandomDecoys,iDecoys);
        }
        buffer->tiSrchAht.stop();
    }
    buffer->tiIRSrch.stop();
    fprintf(stdout,"RandomDecoyClassifier::DoClassify - End\n\n"); fflush(stdout);
}

//___________________________________________________________________
void RandomDecoyClassifier::EndClassify()
{
    fprintf(stdout,"\nRandomDecoyClassifier::EndClassify - Start\n"); fflush(stdout);

    PairClassifier::EndClassify();
    
    ClearItems(vBeginSnips,iSnipsNumber);
    ClearItems(vEndSnips,iSnipsNumber);
    ClearItems(vDecoySnips,iDecoysNumber);

    map<string,FeatureInfo,LexComp> * lexicon = const_cast<map<string,FeatureInfo,LexComp> *>(buffer->lexicon);
    delete lexicon;

    fprintf(stdout,"RandomDecoyClassifier::EndClassify - End\n\n"); fflush(stdout);
}
