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
PairClassifier::PairClassifier(LREThreadBuffer * _buffer):
    stSampleFolder(ResourceBox::Get()->getStringValue("SampleFolder")),
    stFeatureSet(ResourceBox::Get()->getStringValue("FeatureSetFile")),
    stScriptFile(ResourceBox::Get()->getStringValue("ScriptFile")),
    stDecoyFile(ResourceBox::Get()->getStringValue("DecoyFile")),
    stDecoyFolder(ResourceBox::Get()->getStringValue("DecoyFolder")),
    stSimilarDecoyFolder(ResourceBox::Get()->getStringValue("SimilarDecoyFolder")),
    stLogsFolder(ResourceBox::Get()->getStringValue("LogsFolder")),
    stSortedDecoysFile(ResourceBox::Get()->getStringValue("SortedDecoysFile")),
    iIRLoops(ResourceBox::Get()->getIntValue("IRLoops")),
    iPairsNumber(ScriptSize(ResourceBox::Get()->getStringValue("SampleFolder")+ResourceBox::Get()->getStringValue("ScriptFile"))),
    iReqRandDecoyNum(ResourceBox::Get()->getIntValue("RandomDecoysNumber")),
    iDecoyWindowIndex(ResourceBox::Get()->getIntValue("SelectDecoysFirst")),
    iDecoyWindowSize(ResourceBox::Get()->getIntValue("SelectDecoysSetSize")),
    iMultiProcDegree(ResourceBox::Get()->getIntValue("MultiProcessingDegree")),
    dRandFtrsSetFreq(ResourceBox::Get()->getDoubleValue("RandomSetFreq")),
    vBeginSnips(ScriptSize(ResourceBox::Get()->getStringValue("SampleFolder")+ResourceBox::Get()->getStringValue("ScriptFile"))),
    vEndSnips(ScriptSize(ResourceBox::Get()->getStringValue("SampleFolder")+ResourceBox::Get()->getStringValue("ScriptFile"))),
    iStartNameIndex(ResourceBox::Get()->getIntValue("StartNameIndex")),
    iNameKeyLength(ResourceBox::Get()->getIntValue("NameKeyLength")),
    buffer(_buffer),
    debug_log(NULL),
    fPairClassRes(NULL),
    iSnipsNumber(0),
    simMatch(NULL)
{
    debug_log = open_file(buffer->stDebugLog,"PairClassifier","w");
    fPairClassRes = open_file(buffer->stIRQueryLog,"PairClassifier","w");
    
    for (int i=0;i<iPairsNumber;i++)
    {
        vBeginSnips.at(i) = NULL;
        vEndSnips.at(i) = NULL;
    }

    const string stSimMatchType = ResourceBox::Get()->getStringValue("SimilarityMeasure");
    if (stSimMatchType.compare("cos") == 0) simMatch = new CosineMatch;
    else if (stSimMatchType.compare("dist") == 0) simMatch = new DistanceMatch;
    else if (stSimMatchType.compare("mmx") == 0) simMatch = new MinMaxMatch;
    if (simMatch == NULL) abort_err(string("Failed to alloc SimilarityMeasure. name=")+stSimMatchType);
}

//___________________________________________________________________
double PairClassifier::GetScore(const FeatureSet * fsX,
                                const FeatureSet * fsY,
                                const FeatureSet * fsDecoy,
                                vector<double> & vRelWeights,
                                const double dScoreFloor) const
{
    double dScore=0.0;
    
    if (fsDecoy == NULL)
    {
        dScore = simMatch->SimilarityScore(fsX,fsY,vRelWeights,debug_log,dScoreFloor);
        dScore += simMatch->SimilarityScore(fsY,fsX,vRelWeights,debug_log,dScoreFloor);
    }
    else
    {
        dScore = simMatch->SimilarityScore(fsX,fsDecoy,vRelWeights,debug_log,dScoreFloor);
        dScore += simMatch->SimilarityScore(fsY,fsDecoy,vRelWeights,debug_log,dScoreFloor);
    }
    
    return dScore;
}

//___________________________________________________________________
void PairClassifier::RankPairByDecoys(const FeatureSet * fsX,
                                      const FeatureSet * fsY,
                                      const vector<FeatureSet*> & vRandomDecoys,
                                      const int iRandomDecoys)
{
    // Symmetry
    const int xSize = fsX->iGetActiveItems();
    const int ySize = fsY->iGetActiveItems();
    vector<double> vRelWeights(((xSize>ySize)?xSize:ySize),0.0);
    const FeatureSet * fsDecoy = NULL;
    int i;
    LREAuxBuffer irAuxBuf;

    set<CandRes,CandsOrder> candsOrder;
    set<CandRes,CandsOrder>::const_iterator iter;
    CandRes candidate;
    
    // Match the pair.
    buffer->tiCosine.start();
    
    candidate.dScore = this->GetScore(fsX,fsY,NULL,vRelWeights,irAuxBuf.dGetScoreFloor());
    candidate.stName = stPairTargetKey;
    candsOrder.insert(candidate);

    /*fprintf(debug_log,"RankPairByDecoys: X=%s (%d), Y=%s (%d), #RandDecpys=%d, Pair Score=%.5f\n",
            fsX->stGetAuthorName().c_str(),fsX->iGetActiveItems(),fsY->stGetAuthorName().c_str(),fsY->iGetActiveItems(),iRandomDecoys,candidate.dScore);
    fflush(debug_log);*/

    buffer->tiCosine.stop();

    // Match the first snippts against all decoys.
    for (i=0;i<iRandomDecoys;i++)
    {
        fsDecoy = vRandomDecoys.at(i);

        // Skip this decoy if it's one of the pair's items.
        if ((fsDecoy->stGetAuthorName().substr(iStartNameIndex,iNameKeyLength).compare(fsX->stGetAuthorName().substr(iStartNameIndex,iNameKeyLength)) == 0) ||
            (fsDecoy->stGetAuthorName().substr(iStartNameIndex,iNameKeyLength).compare(fsY->stGetAuthorName().substr(iStartNameIndex,iNameKeyLength)) == 0))
            continue;
        
        buffer->tiCosine.start();

        candidate.dScore = this->GetScore(fsX,fsY,fsDecoy,vRelWeights,irAuxBuf.dGetScoreFloor());
        candidate.stName = fsDecoy->stGetAuthorName();
        candsOrder.insert(candidate);

        /*fprintf(debug_log,"Decoy: %s (%d), score=%.5f, best=%.5f\n",
                fsDecoy->stGetAuthorName().c_str(),fsDecoy->iGetActiveItems(),candidate.dScore,candsOrder.begin()->dScore);
        fflush(debug_log);*/
    }

    i=0;
    iter=candsOrder.begin();
    bool bPairFound=false;
    while ((iter != candsOrder.end()) && !bPairFound)
    {
        if (iter->stName.compare(stPairTargetKey) == 0) bPairFound=true;
        i++;
        iter++;
    }

    iter=candsOrder.begin();
    fprintf(fPairClassRes,"%s\t%d\t%s\t%.5f\n",stPairEntryKey.c_str(),i,iter->stName.c_str(),iter->dScore);
    fflush(fPairClassRes);
}

//___________________________________________________________________
int PairClassifier::LoadPairSnips(const string & stLocalScript)
{
    return PairClassifier::LoadPairSnips(vBeginSnips,vEndSnips,stSampleFolder,stLocalScript,buffer->lexicon);
}

//___________________________________________________________________
int PairClassifier::LoadPairSnips(vector<FeatureSet*> & v1,
                                  vector<FeatureSet*> & v2,
                                  const string & stFolder,
                                  const string & stFile,
                                  const map<string,FeatureInfo,LexComp> * lexicon)
{
    TdhReader pairs(stFolder + stFile,512);
    vector<string> vFields(2,"");
    int iPairIndex=0;
    const string stBeginFolder = stFolder+"Begin\\Ftrs\\";
    const string stEndFolder = stFolder+"End\\Ftrs\\";
    
	fprintf(stdout,"PairClassifier::LoadPairSnips - Start: begin=%s, end=%s\n",stBeginFolder.c_str(),stEndFolder.c_str()); fflush(stdout);

	while (pairs.bMoreToRead())
    {
        if (pairs.iGetLine(vFields) != 2) abort_err(string("Illegal line found in ")+stFolder+stFile+": "+stringOf(iPairIndex)+", "+vFields.at(0));
        
		if (iPairIndex%5==0) {fprintf(stdout,"\tLoad Pair: %d\n",iPairIndex); fflush(stdout);}

        v1.at(iPairIndex) =  new StaticSet(lexicon,stBeginFolder,vFields.at(0));
        v2.at(iPairIndex) = new StaticSet(lexicon,stEndFolder,vFields.at(1));
        
        if ((v1.at(iPairIndex) == NULL) || (v2.at(iPairIndex) == NULL))
            abort_err(string("Alloc FeatureSet failed: ")+vFields.at(0));

        iPairIndex++;
    }
    
	fprintf(stdout,"PairClassifier::LoadPairSnips - End: #Pairs=%d\n",iPairIndex); fflush(stdout);

	return iPairIndex;
}
        
//___________________________________________________________________
void PairClassifier::LoadFtrsSet(const vector<string> * vFtrsPool,
                                 const int iFtrsPoolSize)
{
    if (buffer->lexicon != NULL) abort_err("Non NULL lexicon in LoadFtrsSet");
    
    map<string,FeatureInfo,LexComp> * lexicon = new map<string,FeatureInfo,LexComp>();

    if (vFtrsPool == NULL)
    {
        fprintf(stderr,"Load Global Feature Pool: %s%s\n",stSampleFolder.c_str(),stFeatureSet.c_str()); fflush(stderr);
        LoadFeatureSet(*lexicon,NULL,stSampleFolder+stFeatureSet,debug_log);
    }
    else
    {
        LoadFeatureSet(*lexicon,NULL,*vFtrsPool,iFtrsPoolSize);
    }

    buffer->lexicon = lexicon;
    iRandFtrsSetNum = (int)(lexicon->size() * dRandFtrsSetFreq);
}

//___________________________________________________________________
void PairClassifier::LoadDecoysMap()
{
    const string stDecoysFtrsFolder = stSampleFolder+stDecoyFolder+"\\Ftrs\\";
    vector<string> vFields(2,"");
    string stCurrDecoy;
    FeatureSet * fSet = NULL;
    LinesReader lrDecoys(stSampleFolder+stDecoyFile,1024);

    decoysMap.clear();
    while (lrDecoys.bMoreToRead())
    {
        lrDecoys.voGetLine(stCurrDecoy);

        if (decoysMap.find(stCurrDecoy) != decoysMap.end()) continue;
        if ((fSet = new StaticSet(buffer->lexicon,stDecoysFtrsFolder,stCurrDecoy)) == NULL) abort_err(string("Failed to alloc ftr set: ")+stCurrDecoy);
        decoysMap.insert(map<string,FeatureSet*,LexComp>::value_type(stCurrDecoy,fSet));
    }
}
        
//___________________________________________________________________
int PairClassifier::SelectRandomDecoys(vector<FeatureSet*> & vRandomDecoys) const
{
    double dRand;
    int i;
    int k=0;

    for (i=0;i<iDecoysNumber;i++) vRandomDecoys.at(i) = NULL;
    for (k=0,i=0;((i<iDecoysNumber) && (k < iReqRandDecoyNum));i++)
    {
        dRand = get_uniform01_random();
        if (dRand > dRandDecoysRate) continue;
        vRandomDecoys.at(k++) = vDecoySnips.at(i);
    }

    return k;
}
                               
//___________________________________________________________________
void PairClassifier::SetRandomFeatures(const vector<bool> & vRandomSet)
{
    map<string,FeatureSet*,LexComp>::iterator iter;
    for (iter=decoysMap.begin();iter!=decoysMap.end();iter++) iter->second->SetRandomFeatures(vRandomSet);
}
                               
//___________________________________________________________________
int PairClassifier::GetDecoys(const string & stSelDecoyFile)
{
    map<string,FeatureSet*,LexComp>::const_iterator iter;
    TdhReader tdhDecoys(stSelDecoyFile,512);
    vector<string> vFields(2,"");

    int j=-1;
    int k=0;
    while (tdhDecoys.bMoreToRead() && (k<iDecoyWindowSize))
    {
        tdhDecoys.iGetLine(vFields);
        j++;

        if (j < iDecoyWindowIndex) continue;
        
        iter = decoysMap.find(vFields.at(0));
        if (iter == decoysMap.end()) abort_err(string("The decoy ")+vFields.at(0)+", is not found");
        vDecoySnips.at(k) = const_cast<FeatureSet*>(iter->second);
        k++;
    }

    return k;
}

//___________________________________________________________________
int PairClassifier::LoadDecoys(const string & stSelDecoyFile,
                               const string & stPairKey,
                               const int iMaxDecoysNumber)
{
    TdhReader tdhDecoys(stSelDecoyFile,4096);
    vector<string> vFields(2,"");

    const string stDecoysFolder = stSampleFolder+stDecoyFolder+"\\"+stPairKey+"\\Ftrs\\";
    const int iTopWindow = (iMaxDecoysNumber - iDecoyWindowSize);
    const int iFirstIndex = ((iDecoyWindowIndex<=iTopWindow)?iDecoyWindowIndex:((iTopWindow<0)?0:iTopWindow));

    vector<string> vFileNames(iDecoyWindowSize,"");

    int j=-1;
    int k=0;
    while (tdhDecoys.bMoreToRead() && (k<iDecoyWindowSize))
    {
        tdhDecoys.iGetLine(vFields);
        j++;

        if (j < iDecoyWindowIndex) continue;

        vFileNames.at(k) = vFields.at(0);
        k++;
    }

    for (j=0;j<k;j++) vDecoySnips.at(j) = new StaticSet(buffer->lexicon,stDecoysFolder,vFileNames.at(j));

    return k;
}
                               
//___________________________________________________________________
void PairClassifier::EndClassify()
{
    if (buffer->mLogMutex != NULL) if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stderr,"\nRunning Times:\n");
    fprintf(stderr,"Total Srch Time: %d\n",buffer->tiSrchAht.iGetTime());
    fprintf(stderr,"IR Loops Time:   %d\n",buffer->tiIRSrch.iGetTime());
    fprintf(stderr,"Get Decoys Time: %d\n",buffer->tiGetRndDcy.iGetTime());
    fprintf(stderr,"Get Featrs Time: %d\n",buffer->tiGetFtr.iGetTime());
    fprintf(stderr,"Sel Featrs Time: %d\n",buffer->tiSelFtr.iGetTime());
    fprintf(stderr,"Cosine Time:     %d\n",buffer->tiCosine.iGetTime());
    fprintf(stderr,"Read Decoy Time: %d\n",buffer->tiReadDcys.iGetTime());
    fprintf(stderr,"Load Decoy Time: %d\n",buffer->tiLoadDcys.iGetTime());
    fprintf(stderr,"Read FS Time:    %d\n",buffer->tiReadFS.iGetTime());
    fprintf(stderr,"Load FS Time:    %d\n",buffer->tiLoadFS.iGetTime());
    fprintf(stderr,"End Classifier: %s\n\n",this->stGetClassifier().c_str());
    fflush(stderr);
    if (buffer->mLogMutex != NULL) if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

    fflush(debug_log);
    fflush(fPairClassRes);
    fclose(debug_log);
    fclose(fPairClassRes);
}

//___________________________________________________________________
int PairClassifier::ReadFeatureFile(const string & stFeatureFile,
                                    vector<string> & vFtrsVec,
                                    const int iBlockSize,
                                    const bool bSkipHeader) const
{
    LinesReader lrFtrsSet(stFeatureFile,512);
    string stLine(150,'\0');
        
    int n=0;
    int s=1;

    if (bSkipHeader) lrFtrsSet.voGetLine(stLine);
    while (lrFtrsSet.bMoreToRead())
    {
        lrFtrsSet.voGetLine(stLine);
        
        if (n >= s*iBlockSize)
        {
            s++;
            vFtrsVec.resize(s*iBlockSize,"");
        }
            
        vFtrsVec.at(n) = stLine;
        n++;
    }
    
    return n;
}

//___________________________________________________________________
int PairClassifier::ReadDecoySet(vector< vector<string> * > & vFtrsSets,
                                 vector<int> & vSetSize,
                                 vector<string> & vFileName,
                                 const string & stWorkFolder,
                                 const string & stWorkFile) const
{
    const string stCurrDecoyFolder = stWorkFolder + "\\Ftrs\\";
    const int iMaxDecoysNumber = ScriptSize(stWorkFile);
    const int iTopWindow = (iMaxDecoysNumber - iDecoyWindowSize);
    const int iFirstIndex = ((iDecoyWindowIndex<=iTopWindow)?iDecoyWindowIndex:((iTopWindow<0)?0:iTopWindow));
    int j,k,n;
    const int iBlockSize = 950;
    
    TdhReader tdhDecoys(stWorkFile,512);
    vector<string> vFields(2,"");
    
    j=-1;
    k=0;
    while (tdhDecoys.bMoreToRead() && (k<iDecoyWindowSize))
    {
        tdhDecoys.iGetLine(vFields);
        j++;

        if (j < iFirstIndex) continue;
        
        if (vFtrsSets.at(k) != NULL) abort_err("Non NULL FS in ReadDecoysSet");
        vFtrsSets.at(k) = new vector<string>(iBlockSize,"");
        
        n = ReadFeatureFile(stCurrDecoyFolder+vFields.at(0),*vFtrsSets.at(k),iBlockSize,false);
        
        vSetSize.at(k) = n;
        vFileName.at(k) = vFields.at(0);
        
        k++;
    }
    
    return k;
}

//___________________________________________________________________
void PairClassifier::ClassifyPairs()
{
    this->InitClassify();
    this->DoClassify();
    this->EndClassify();
}

//___________________________________________________________________
double DecoyWindowSim(const string & stDecoysFile,
                      const int iFirstIndex,
                      const int iWindowSize)
{
    TdhReader tdhDecoys(stDecoysFile,512);
    int j=-1;
    int k=0;
    double dPairMean=0.0;
    double dCurrScore = 1.0;
    vector<string> vFields(2,"");
    while (tdhDecoys.bMoreToRead() && (k<iWindowSize))
    {
        tdhDecoys.iGetLine(vFields);
        j++;

        if (j < iFirstIndex) continue;

        if (!string_as<double>(dCurrScore,vFields.at(1),std::dec)) abort_err(string("Illegal Pair Score: pair=")+stDecoysFile+", decoy="+vFields.at(0)+", score="+vFields.at(1));
        dPairMean += dCurrScore;
        k++;
    }

    dPairMean /= k;
    return dPairMean;
}

//___________________________________________________________________
void PairClassifier::LoadPrivateDecoys(vector< vector<string> * > & vFtrsSets,
                                       const vector<int> & vSetSize,
                                       const vector<string> & vFileName,
                                       const int iPrivateDecoysNumber)
{
    for (int i=0;i<iPrivateDecoysNumber;i++)
    {
        if (vDecoySnips.at(i) != NULL) abort_err("Non NULL decoy in LoadPrivateDecoys");
        vDecoySnips.at(i) = new StaticSet(buffer->lexicon,*(vFtrsSets.at(i)),vSetSize.at(i),vFileName.at(i));
        delete vFtrsSets.at(i);
        vFtrsSets.at(i) = NULL;
    }
}

//___________________________________________________________________
void PairClassifier::SetSearchKeys(const FeatureSet * fsX, const FeatureSet * fsY)
{
    stPairEntryKey = fsX->stGetAuthorName()+"\t"+fsY->stGetAuthorName();
    stPairTargetKey = fsX->stGetAuthorName()+"_PAIRKEY_"+fsY->stGetAuthorName();
}
