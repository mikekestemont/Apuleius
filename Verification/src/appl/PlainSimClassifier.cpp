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
void PlainSimilarityClassifier::InitClassify()
{
    fprintf(stderr,"\nPlainSimilarityClassifier::InitClassify - Start\n"); fflush(stderr);
    this->LoadFtrsSet();
    fprintf(stderr,"Load Candidates - Start\n"); fflush(stderr);
    iSnipsNumber = this->LoadPairSnips(stScriptFile);
    fprintf(stderr,"Load Candidates - End (#Snips=%d)\n",iSnipsNumber); fflush(stderr);
    fprintf(stderr,"PlainSimilarityClassifier::InitClassify - End\n\n"); fflush(stderr);
}

//___________________________________________________________________
void PlainSimilarityClassifier::DoClassify()
{
    fprintf(stderr,"\nPlainSimilarityClassifier::DoClassify - Start\n"); fflush(stderr);
    
    vector<bool> vRandomSet(buffer->lexicon->size()+1,true);
    LREAuxBuffer::SetRandomFeatures(vRandomSet,vBeginSnips,iSnipsNumber);
    LREAuxBuffer::SetRandomFeatures(vRandomSet,vEndSnips,iSnipsNumber);

    for (int i=0;i<iSnipsNumber;i++)
    {
        //fprintf(stderr,"Working on pair: %s -> %s, %d\n",vBeginSnips.at(i)->stGetAuthorName().c_str(),vEndSnips.at(i)->stGetAuthorName().c_str(),i); fflush(stderr);

        buffer->tiSrchAht.start();
        this->ClassifyPair(vBeginSnips.at(i),vEndSnips.at(i));
        buffer->tiSrchAht.stop();
    }
    fprintf(stderr,"PlainSimilarityClassifier::DoClassify - End\n\n"); fflush(stderr);
}

//___________________________________________________________________
void PlainSimilarityClassifier::EndClassify()
{
    fprintf(stderr,"\nPlainSimilarityClassifier::EndClassify - Start\n"); fflush(stderr);

    PairClassifier::EndClassify();
    
    ClearItems(vBeginSnips,iSnipsNumber);
    ClearItems(vEndSnips,iSnipsNumber);
    map<string,FeatureInfo,LexComp> * lexicon = const_cast<map<string,FeatureInfo,LexComp> *>(buffer->lexicon);
    delete lexicon;

    fprintf(stderr,"PlainSimilarityClassifier::EndClassify - End\n\n"); fflush(stderr);
}

//___________________________________________________________________
void PlainSimilarityClassifier::ClassifyPair(FeatureSet * fsX,FeatureSet * fsY)
{
    double dScore;

    // Symmetry
    const int xSize = fsX->iGetActiveItems();
    const int ySize = fsY->iGetActiveItems();
    vector<double> vRelWeights(((xSize>ySize)?xSize:ySize),0.0);
    LREAuxBuffer irAuxBuf;
    
    dScore = simMatch->SimilarityScore(fsX,fsY,vRelWeights,debug_log,irAuxBuf.dGetScoreFloor());

    // Symmetry
    dScore += simMatch->SimilarityScore(fsY,fsX,vRelWeights,debug_log,irAuxBuf.dGetScoreFloor());

    fprintf(fPairClassRes,"%s\t%s\t%.15f\n",fsX->stGetAuthorName().c_str(),fsY->stGetAuthorName().c_str(),dScore);
    fflush(fPairClassRes);
}

