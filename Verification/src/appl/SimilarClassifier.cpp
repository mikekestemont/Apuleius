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
void SimilarDecoyClassifier::InitClassify()
{
    fprintf(stderr,"\nSimilarDecoyClassifier::InitClassify - Start\n"); fflush(stderr);
    
    this->LoadFtrsSet();
    
    fprintf(stderr,"Load Candidates - Start\n"); fflush(stderr);
    iSnipsNumber = this->LoadPairSnips(stScriptFile);
    fprintf(stderr,"Load Candidates - End (#Snips=%d)\n",iSnipsNumber); fflush(stderr);

    fprintf(stderr,"Load Decoys - Start\n"); fflush(stderr);
    this->LoadDecoysMap();
    fprintf(stderr,"Load Decoys - End (#Total Decoys=%d)\n",decoysMap.size()); fflush(stderr);
    
    vDecoySnips.resize(iDecoyWindowSize);

    fprintf(stderr,"SimilarDecoyClassifier::InitClassify - End\n\n"); fflush(stderr);
}

//___________________________________________________________________
void SimilarDecoyClassifier::ClassifyPair(FeatureSet * fsX,FeatureSet * fsY)
{
    const string stSelDecoyFile = stSampleFolder + stSimilarDecoyFolder + "\\" + GetPairKey(fsX,fsY) + "_decoy.txt";
    
    iDecoysNumber = this->GetDecoys(stSelDecoyFile);
    dRandDecoysRate = ((double)iReqRandDecoyNum / (double)iDecoysNumber) + 0.035;
    vector<FeatureSet*> vRandomDecoys(iDecoysNumber);

    buffer->tiGetRndDcy.start();
    const int iActualDecoysNumber = this->SelectRandomDecoys(vRandomDecoys);
    buffer->tiGetRndDcy.stop();
    
    this->SetSearchKeys(fsX,fsY);
    PairClassifier::RankPairByDecoys(fsX,
                                     fsY,
                                     vRandomDecoys,
                                     iActualDecoysNumber);
}

//___________________________________________________________________
void SimilarDecoyClassifier::DoClassify()
{
    fprintf(stderr,"\nSimilarDecoyClassifier::DoClassify - Start\n"); fflush(stderr);

    vector<bool> vRandFtrsSet(buffer->lexicon->size()+1,false);
    int i,j;
    buffer->tiIRSrch.start();
    for (j=0;j<iIRLoops;j++)
    {
        fprintf(stderr,"Working on IR loop: %d...\n",j); fflush(stderr);
        
        buffer->tiSelFtr.start();
        SelectRandomFeatures(iRandFtrsSetNum,buffer->lexicon->size(),vRandFtrsSet);
        buffer->tiSelFtr.stop();

        buffer->tiGetFtr.start();
        LREAuxBuffer::SetRandomFeatures(vRandFtrsSet,vBeginSnips,iSnipsNumber);
        LREAuxBuffer::SetRandomFeatures(vRandFtrsSet,vEndSnips,iSnipsNumber);
        this->SetRandomFeatures(vRandFtrsSet);
        buffer->tiGetFtr.stop();

        buffer->tiSrchAht.start();
        for (i=0;i<iSnipsNumber;i++)
        {
            this->ClassifyPair(vBeginSnips.at(i),vEndSnips.at(i));
        }
        buffer->tiSrchAht.stop();
    }
    buffer->tiIRSrch.stop();

    fprintf(stderr,"SimilarDecoyClassifier::DoClassify - End\n\n"); fflush(stderr);
}

//___________________________________________________________________
void SimilarDecoyClassifier::EndClassify()
{
    fprintf(stderr,"\nSimilarDecoyClassifier::EndClassify - Start\n"); fflush(stderr);

    PairClassifier::EndClassify();
    
    ClearItems(vBeginSnips,iSnipsNumber);
    ClearItems(vEndSnips,iSnipsNumber);
    ClearItems(decoysMap);

    map<string,FeatureInfo,LexComp> * lexicon = const_cast<map<string,FeatureInfo,LexComp> *>(buffer->lexicon);
    delete lexicon;

    fprintf(stderr,"SimilarDecoyClassifier::EndClassify - End\n\n"); fflush(stderr);
}

//___________________________________________________________________
void GenreDecoyClassifier::ClassifyPair(FeatureSet * fsX,FeatureSet * fsY)
{
    this->SetSearchKeys(fsX,fsY);
    this->ClassifyPair(fsX,fsY,"XY");
    this->ClassifyPair(fsY,fsX,"YX");
}

//___________________________________________________________________
void GenreDecoyClassifier::ClassifyPair(FeatureSet * fsX,
                                        FeatureSet * fsY,
                                        const string & stDirection)
{
    const string stSelDecoyFile = stSampleFolder + stSimilarDecoyFolder + "\\" + stDirection + "\\" + GetPairKey(fsX,fsY) + "_decoy.txt";
    
    iDecoysNumber = this->GetDecoys(stSelDecoyFile);
    dRandDecoysRate = ((double)iReqRandDecoyNum / (double)iDecoysNumber) + 0.035;
    vector<FeatureSet*> vRandomDecoys(iDecoysNumber);

    buffer->tiGetRndDcy.start();
    const int iActualDecoysNumber = this->SelectRandomDecoys(vRandomDecoys);
    buffer->tiGetRndDcy.stop();
    
    this->RankPairByDecoys(fsX,fsY,vRandomDecoys,iActualDecoysNumber);
}

//___________________________________________________________________
double GenreDecoyClassifier::GetScore(const FeatureSet * fsX,
                                      const FeatureSet * fsY,
                                      const FeatureSet * fsDecoy,
                                      vector<double> & vRelWeights,
                                      const double dScoreFloor) const
{
    double dScore=0.0;
    
    if (fsDecoy == NULL)
    {
        dScore = simMatch->SimilarityScore(fsX,fsY,vRelWeights,debug_log,dScoreFloor);
    }
    else
    {
        dScore = simMatch->SimilarityScore(fsX,fsDecoy,vRelWeights,debug_log,dScoreFloor);
    }
    
    return dScore;
}
