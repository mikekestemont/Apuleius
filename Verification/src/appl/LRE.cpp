#include <FeatureSet.h>
#include <ResourceBox.h>
#include <auxiliary.h>
#include <FileScanner.h>
#include <FolderScanner.h>
#include <math.h>
#include <Timer.h>
#include <LREThreadBuffer.h>
#include <DataFuncs.h>
#include <LRE.h>

using namespace std;

//___________________________________________________________________
int LoadCandidates(vector<FeatureSet*> & vSampleItems,
                   const string        & stItemsFolder,
                   const string        & stItemsFile,
                   const map<string,FeatureInfo,LexComp> * lexicon)
{
    TdhReader items(stItemsFile,512);
    vector<string> vFields(2,"");
    int iFields;
    string stClass;
    int iItemsNumber=0;

    while (items.bMoreToRead())
    {
        iFields = items.iGetLine(vFields);
        stClass = ((iFields == 2)?vFields.at(1):"");

        try
        {
            vSampleItems.at(iItemsNumber) = new StaticSet(lexicon,stItemsFolder,vFields.at(0),stClass);
        }
        catch (...)
        {
            fprintf(stderr,"Failed to alloc FS: index=%d, file=%s\n",iItemsNumber,vFields.at(0).c_str());
            fflush(stderr);
            exit(0);
        }
        
        if (vSampleItems.at(iItemsNumber) == NULL)
        {
            fprintf(stderr,"Alloc FeatureSet failed: %s\n",(stItemsFolder+vFields.at(0)).c_str());
            fflush(stderr);
            exit(0);
        }
        
        iItemsNumber++;
    }

    return iItemsNumber;
}

//___________________________________________________________________
void StaticToRandom(const vector<FeatureSet*> & vStaticSet,
                    vector<FeatureSet*> & vRandomSet,
                    const int iItemsNumber)
{
    int i;
    const StaticSet * p=NULL;

    int j=0;
    for (i=0;i<iItemsNumber;i++)
    {
        p=(StaticSet*)vStaticSet.at(i);
        if (p == NULL) continue;
        vRandomSet.at(j++) = new RandomSet(p);
    }
}

//___________________________________________________________________
void ClearItems(vector<FeatureSet*> & vItems,const int iItemsNumber)
{
    int i;
    FeatureSet * p=NULL;
    
    for (i=0;i<iItemsNumber;i++)
    {
        p=vItems.at(i);
        if (p != NULL)
            delete p;
        vItems.at(i) = NULL;
    }
}

//___________________________________________________________________
void ClearItems(map<string,FeatureSet*,LexComp> & itemsMap)
{
    map<string,FeatureSet*,LexComp>::const_iterator iter;

    for (iter=itemsMap.begin();iter!=itemsMap.end();iter++)
    {
        delete iter->second;
    }
}

//___________________________________________________________________
void SelectRandomFeatures(const int iFeaturesNumber,
                          const int iMaxIndex,
                          vector<bool> & vRandomSet)
{
    int i;
    const double dThreshold = (double)iFeaturesNumber / (double)iMaxIndex;
    
    vRandomSet.assign(iMaxIndex+1,false);
    
    for (i=0;i<iMaxIndex;i++)
    {
        if (get_uniform01_random() > dThreshold) continue;
        vRandomSet.at(i)=true;
    }
}

//___________________________________________________________________
static void SearchAuthor(LREThreadBuffer * buffer,
                         const FeatureSet * fsAnonym,
                         const vector<FeatureSet*> & vAuthors,
                         const int iAuthorsNumber,
                         FILE  * fIRQueryLog,
                         FILE  * debug_log)
{
    int i;
    double dScore;
    const FeatureSet * fsAuthor=NULL;
    vector<double> vRelWeights(fsAnonym->iGetActiveItems(),0.0);
    vector<FeatureSet*> vCandsSet(iAuthorsNumber+1);
    
    const int iCandsNumber = buffer->irAuxBuffer->getCandsSet(fsAnonym->stGetAuthorName(),vAuthors,iAuthorsNumber,vCandsSet);
    buffer->irAuxBuffer->initSearch();

    fprintf(debug_log,"SearchAuthor: Anonym Text = %s, #Features=%d, #Authors=%d, #Cands=%d\n",
            fsAnonym->stGetAuthorName().c_str(),fsAnonym->iGetActiveItems(),iAuthorsNumber,iCandsNumber);
    fflush(debug_log);

    for (i=0;i<iCandsNumber;i++)
    {
        fsAuthor = vCandsSet.at(i);

        buffer->tiCosine.start();
        
        dScore = buffer->simMatch->SimilarityScore(fsAnonym,fsAuthor,vRelWeights,debug_log,buffer->irAuxBuffer->dGetScoreFloor());

        fprintf(debug_log,"Author: %s, #Features=%d, Score=%.5f, i=%d\n",fsAuthor->stGetAuthorName().c_str(),fsAuthor->iGetActiveItems(),dScore,i);
        fflush(debug_log);

        buffer->tiCosine.stop();

        buffer->irAuxBuffer->updateResult(dScore,fsAuthor);
    }
    
    buffer->irAuxBuffer->printResult(fsAnonym,fIRQueryLog);
}

//___________________________________________________________________
static void IRSearch(LREThreadBuffer * buffer,
                     const vector<FeatureSet*> & vAuthors,
                     const vector<FeatureSet*> & vAnonyms,
                     const int iAuthorsNumber,
                     const int iAnonymsNumber,
                     FILE * fIRQueryLog,
                     FILE * debug_log)
{
    const FeatureSet * fsAnonym=NULL;

    buffer->iAnonymIndex=0;
    while (buffer->iAnonymIndex < iAnonymsNumber)
    {
        fsAnonym = vAnonyms.at(buffer->iAnonymIndex++);

        fprintf(debug_log,"IRSearch: unknown=%s, %d\n",fsAnonym->stGetAuthorName().c_str(),buffer->iAnonymIndex);
        fflush(debug_log);

        buffer->tiSrchAht.start();
        SearchAuthor(buffer,
                     fsAnonym,
                     vAuthors,
                     iAuthorsNumber,
                     fIRQueryLog,
                     debug_log);
        buffer->tiSrchAht.stop();
    }
}

//___________________________________________________________________
void * LREProcess(void * arg)
{
    LREThreadBuffer * buffer = (LREThreadBuffer*)arg;
    
    Timer tiThread;
    Timer tiSrchLoop;
    
    tiSrchLoop.reset();
    tiThread.reset();
    
    tiThread.start();
    
    if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stderr,"The thread %s was allocated.\n",buffer->stThreadID.c_str()); fflush(stderr);
    if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stAnonymSuffix = buffer->irAuxBuffer->stGetSubFolder("Anonym");
    vector<FeatureSet*> vAnonyms(buffer->iMaxAnonymsNum+1);
    
    int iIterIndex;
    const double dRandFreq = ResourceBox::Get()->getDoubleValue("RandomSetFreq");
    const int iRandomSetSize = (int)(buffer->lexicon->size() * dRandFreq);
    vector<bool> vRandomSet(buffer->lexicon->size()+1,false);

    if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stderr,"Load data in thread %s. Random Set Size=%d, Freq=%.2f, Lex size=%d\n",buffer->stThreadID.c_str(),iRandomSetSize,dRandFreq,buffer->lexicon->size());
    fprintf(stderr,"#Max Cands=%d, #MaxAnonyms=%d, script=%s\n",buffer->iAuthorsNumber,buffer->iMaxAnonymsNum,buffer->stIRScriptFile.c_str()); fflush(stderr);
    if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

    const int iAnonymsNumber = LoadCandidates(vAnonyms,stSampleFolder+stAnonymSuffix,buffer->stIRScriptFile,buffer->lexicon);
    const int iAuthorsNumber = buffer->iAuthorsNumber;
    const int iIRLoops = ResourceBox::Get()->getIntValue("IRLoops");

    if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stderr,"Transform Static Sets to Random Sets in thread %s.\n",buffer->stThreadID.c_str()); fflush(stderr);
    if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

    vector<FeatureSet*> vRandAuthors(iAuthorsNumber);
    vector<FeatureSet*> vRandAnonyms(iAnonymsNumber);
    StaticToRandom(*(buffer->vAuthors),vRandAuthors,iAuthorsNumber);
    StaticToRandom(vAnonyms,vRandAnonyms,iAnonymsNumber);

    FILE * fIRQueryLog = open_file(buffer->stIRQueryLog,"LREProcess","w");
    FILE * debug_log = open_file(buffer->stDebugLog,"LREProcess","w");

    if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stderr,"LRE Start in thread %s.\n",buffer->stThreadID.c_str()); fflush(stderr);
    if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

    tiSrchLoop.start();
    for (iIterIndex=0;iIterIndex<iIRLoops;iIterIndex++)
    {
        if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
        fprintf(stderr,"%s: start iteration %d\n",buffer->stThreadID.c_str(),iIterIndex); fflush(stderr);
        if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

        buffer->tiSelFtr.start();
        SelectRandomFeatures(iRandomSetSize,buffer->lexicon->size(),vRandomSet);
        buffer->tiSelFtr.stop();
        
        buffer->tiGetFtr.start();
        LREAuxBuffer::SetRandomFeatures(vRandomSet,vRandAuthors,iAuthorsNumber);
        LREAuxBuffer::SetRandomFeatures(vRandomSet,vRandAnonyms,iAnonymsNumber);
        buffer->irAuxBuffer->setRandomFtrs(vRandomSet);
        buffer->tiGetFtr.stop();

        buffer->tiIRSrch.start();
        IRSearch(buffer,
                 vRandAuthors,
                 vRandAnonyms,
                 iAuthorsNumber,
                 iAnonymsNumber,
                 fIRQueryLog,
                 debug_log);
        buffer->tiIRSrch.stop();
    }
    tiSrchLoop.stop();
    
    ClearItems(vAnonyms,iAnonymsNumber);
    ClearItems(vRandAnonyms,iAnonymsNumber);
    ClearItems(vRandAuthors,iAuthorsNumber);
    
    tiThread.stop();

    if (WaitForSingleObject(buffer->mLogMutex,INFINITE) != WAIT_OBJECT_0) abort_err("Failed to lock mutex (Log)");
    fprintf(stderr,"The thread %s has completed:\n",buffer->stThreadID.c_str());
    fprintf(stderr,"Thread Time:     %d\n",tiThread.iGetTime());
    fprintf(stderr,"Srch Loop Time:  %d\n",tiSrchLoop.iGetTime());
    fprintf(stderr,"IR Search Time:  %d\n",buffer->tiIRSrch.iGetTime());
    fprintf(stderr,"Srch Ahtor Time: %d\n",buffer->tiSrchAht.iGetTime());
    fprintf(stderr,"Get Featrs Time: %d\n",buffer->tiGetFtr.iGetTime());
    fprintf(stderr,"Sel Featrs Time: %d\n",buffer->tiSelFtr.iGetTime());
    fprintf(stderr,"Cosine Time:     %d\n",buffer->tiCosine.iGetTime());
    fflush(stderr);
    if (!ReleaseMutex(buffer->mLogMutex)) abort_err("Failed to release a mutex");

    fprintf(debug_log,"Thread Time:     %d\n",tiThread.iGetTime());
    fprintf(debug_log,"Srch Loop Time:  %d\n",tiSrchLoop.iGetTime());
    fprintf(debug_log,"IR Search Time:  %d\n",buffer->tiIRSrch.iGetTime());
    fprintf(debug_log,"Srch Ahtor Time: %d\n",buffer->tiSrchAht.iGetTime());
    fprintf(debug_log,"Get Featrs Time: %d\n",buffer->tiGetFtr.iGetTime());
    fprintf(debug_log,"Sel Featrs Time: %d\n",buffer->tiSelFtr.iGetTime());
    fprintf(debug_log,"Cosine Time:     %d\n",buffer->tiCosine.iGetTime());
    fprintf(debug_log,"LRE - End\n");

    fflush(debug_log);
    fflush(fIRQueryLog);
    fclose(debug_log);
    fclose(fIRQueryLog);
    
    return buffer;
}

//___________________________________________________________________
static LREThreadBuffer * 
            DataSplit(LinesReader  ** lrIRScript,
                      const string & stProgName,
                      HANDLE mutex,
                      const string & stSampleFolder,
                      const string & stThreadID,
                      const int      iMaxAnonymsNum,
                      const vector<FeatureSet*> & vAuthors,
                      const int iAuthorsNumber,
                      const map<string,FeatureInfo,LexComp> & lexicon)
{
    const string stLogsFolder = ResourceBox::Get()->getStringValue("LogsFolder");
    LREThreadBuffer * buffer = NULL;
    int i;
    string stLine(75,'\0');
    
    fprintf(stderr,"Alloc LRE Thread buffer\n");
    fflush(stderr);

    buffer = new LREThreadBuffer(mutex,NULL,iMaxAnonymsNum+1,iAuthorsNumber);
    
    if (buffer == NULL)
    {
        fprintf(stderr,"Alloc Thread Buffer failed\n");
        fflush(stderr);
        return NULL;
    }

    buffer->vAuthors = &vAuthors;
    
    fprintf(stderr,"Alloc LRE IR Aux buffer\n");
    fflush(stderr);

    LREAuxBuffer * irAuxBuffer=NULL;
    if (stProgName.compare("LRE") == 0)
    {
        const bool bIsClosedSet = (ResourceBox::Get()->getIntValue("ClosedAuthorSet") > 0);
        const bool bIsNoAuthorSet = (ResourceBox::Get()->getIntValue("IsNoAuthor") > 0);
        if (bIsClosedSet)
            irAuxBuffer = new LREAuxBuffer();
        else if (!bIsNoAuthorSet)
            irAuxBuffer = new LREOpenSet(&lexicon);
        else
            irAuxBuffer = new LRENoAuthor();
    }
    else if (stProgName.compare("LREClassify") == 0)
    {
        irAuxBuffer = new LREClassify();
    }
    else abort_err(string("Unknown program name in LRE: name=")+stProgName);
    
    if (irAuxBuffer == NULL) abort_err(string("Failed to alloc LREAuxBuffer. name=")+stProgName);
    
    buffer->irAuxBuffer=irAuxBuffer;

    SimilarityMeasure * simMatch = NULL;
    const string stSimFuncType = ResourceBox::Get()->getStringValue("SimilarityMeasure");
    if (stSimFuncType.compare("cos") == 0) simMatch = new CosineMatch;
    else if (stSimFuncType.compare("dist") == 0) simMatch = new DistanceMatch;
    else if (stSimFuncType.compare("mmx") == 0) simMatch = new MinMaxMatch;
    else abort_err(string("Illegal SimilarityMeasure: ")+stSimFuncType);
    if (simMatch == NULL) abort_err(string("Failed to alloc SimilarityMeasure. name=")+stSimFuncType);
    
    buffer->simMatch = simMatch;
    
    buffer->lexicon = &lexicon;
        
    buffer->iAnonymIndex=0;
    
    buffer->stThreadID = stThreadID;
    buffer->stIRScriptFile = stSampleFolder+"IRScript_"+stThreadID+".txt";
    buffer->stIRQueryLog = stLogsFolder+"IR_"+stThreadID+".txt";
    buffer->stDebugLog = stLogsFolder+"debug_"+stThreadID+".txt";
    
    fprintf(stderr,"open the script file: %s\n",buffer->stIRScriptFile.c_str());
    fflush(stderr);

    FILE * fIRScript = open_file(buffer->stIRScriptFile,"DataSplit","w");
    
    i=0;
    
    const string stScriptFile = stSampleFolder+"anonyms.txt";
    fprintf(stderr,"Alloc Lines Reader: %s\n",stScriptFile.c_str());
    fflush(stderr);

    if (lrIRScript == NULL) {fprintf(stderr,"NULL lines reader buffer\n"); fflush(stderr); exit(0);}
    if (*lrIRScript == NULL) *lrIRScript = new LinesReader(stScriptFile,512);
    if (*lrIRScript == NULL) {fprintf(stderr,"Failed to alloc lines reader object\n"); fflush(stderr); exit(0);}
    while ((i++<iMaxAnonymsNum) && (*lrIRScript)->bMoreToRead())
    {
        (*lrIRScript)->voGetLine(stLine);
        fprintf(fIRScript,"%s\n",stLine.c_str());
        fflush(fIRScript);
    }
    fclose(fIRScript);
    
    fprintf(stderr,"Create Thread\n");
    fflush(stderr);

    // Create the batch audio mutex.
    DWORD tiLREThread;
    buffer->tid = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)LREProcess,(void*)buffer,0,&tiLREThread);
    if (buffer->tid == NULL) abort_err("Failed to create LRE thread");
    
    return buffer;
}

//___________________________________________________________________
void ConsolidateLogs(FILE * fLogFile,const string & stLogFile)
{
    LinesReader lrReader(stLogFile,512);
    string stLine(75,'\0');
    while (lrReader.bMoreToRead())
    {
        lrReader.voGetLine(stLine);
        fprintf(fLogFile,"%s\n",stLine.c_str());
        fflush(fLogFile);
    }
}

//___________________________________________________________________
void LRE(const string & stProgName)
{
    Timer tiLRE;
    tiLRE.reset();
    tiLRE.start();
        
    const string stLogsFolder = ResourceBox::Get()->getStringValue("LogsFolder");
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stFeaturePool = stSampleFolder+"FeaturePool.txt";
    const int iIRLoops = ResourceBox::Get()->getIntValue("IRLoops");
    map<string,FeatureInfo,LexComp> lexicon;
    HANDLE mutex;
    
    fprintf(stderr,"Main Thread - Start\n");
    fflush(stderr);

    mutex = CreateMutex(NULL,FALSE,NULL);
    if (mutex == NULL) abort_err("Create mutex failed in LRE");

    fprintf(stderr,"Load Feature Pool\n");
    fflush(stderr);

    LoadFeatureSet(lexicon,NULL,stFeaturePool,NULL);
    LinesReader * lrSamples=NULL;
    
    fprintf(stderr,"Load Authors Set - Start\n");
    fflush(stderr);

    const string stAuthorSuffix = string("Author") + ((stProgName.compare("LREClassify") == 0)?"\\":"\\Ftrs\\");
    const string stAuthorsList = stSampleFolder + "authors.txt";
    const int iMaxCandsNum = ScriptSize(stAuthorsList);
    vector<FeatureSet*> vAuthors(iMaxCandsNum+1);
    const int iAuthorsNumber = LoadCandidates(vAuthors,stSampleFolder+stAuthorSuffix,stAuthorsList,&lexicon);

    fprintf(stderr,"Load Authors Set - End (#Authors=%d)\n",iAuthorsNumber);
    fflush(stderr);

    FILE * fIRQueryLog = open_file(stLogsFolder+"IR_All.txt","LRE","w");

    const int iMaxAnonymsNum = ScriptSize(stSampleFolder+"anonyms.txt");
    const int iMultiProcDegree = ResourceBox::Get()->getIntValue("MultiProcessingDegree");
    const int iAnonymSegment = iMaxAnonymsNum/iMultiProcDegree + iMaxAnonymsNum%iMultiProcDegree;
    int i;
    list<LREThreadBuffer*> threadsList;
    LREThreadBuffer * buffer=NULL;
    char thread_id[50];
    string stLine(75,'\0');
    
    fprintf(stderr,"#MaxAnonyms=%d; Segment: %d + %d = %d\n",iMaxAnonymsNum,iMaxAnonymsNum/iMultiProcDegree,iMaxAnonymsNum%iMultiProcDegree,iAnonymSegment);
    fflush(stderr);

    fprintf(stderr,"Alloc Threads\n");
    fflush(stderr);

    for (i=0;i<iMultiProcDegree;i++)
    {
        sprintf(thread_id,"tid%d",i);
        buffer = DataSplit(&lrSamples,
                           stProgName,
                           mutex,
                           stSampleFolder,
                           thread_id,
                           iAnonymSegment,
                           vAuthors,
                           iAuthorsNumber,
                           lexicon);
        threadsList.push_back(buffer);
    }
    
    fprintf(stderr,"Join Threads\n");
    fflush(stderr);

    list<LREThreadBuffer*>::iterator threadsIter = threadsList.begin();
    int iAnonymsNumber=0;
    while (threadsIter != threadsList.end())
    {
        fprintf(stderr,"Wait for thread end\n");
        fflush(stderr);

        LREThreadBuffer * ltbTemp = *threadsIter;

        fprintf(stderr,"Call Join\n");
        fflush(stderr);

        // Wait for the LRE thread end.
        WaitForSingleObject(ltbTemp->tid,INFINITE);
        CloseHandle(ltbTemp->tid);
        threadsIter++;

        fprintf(stderr,"Consolidate the IR queries log. tid=%s, #Anonyms=%d\nIRLog=%s\n",
                ltbTemp->stThreadID.c_str(),ltbTemp->iAnonymIndex,ltbTemp->stIRQueryLog.c_str());
        fflush(stderr);
        
        ConsolidateLogs(fIRQueryLog,ltbTemp->stIRQueryLog);

        iAnonymsNumber += ltbTemp->iAnonymIndex;
        
        fprintf(stderr,"Del Buffer\n");
        fflush(stderr);

        delete ltbTemp;

        fprintf(stderr,"End Join iteration\n");
        fflush(stderr);
    }
    
    fprintf(stderr,"#Anonyms=%d\n",iAnonymsNumber);
    fflush(stderr);

    ClearItems(vAuthors,iAuthorsNumber);

    tiLRE.stop();

    fprintf(stderr,"Total LRE Time:  %d\n",tiLRE.iGetTime());
    fflush(stderr);
    fprintf(fIRQueryLog,"#Total LRE Time:  %d\n",tiLRE.iGetTime());
    fflush(fIRQueryLog);
    fclose(fIRQueryLog);

    if (CloseHandle(mutex) == 0) abort_err("Destroy mutex failed");
}
