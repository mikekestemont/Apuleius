#include <ResourceBox.h>
#include <auxiliary.h>
#include <FileScanner.h>
#include <FolderScanner.h>
#include <math.h>
#include <FeaturesHandler.h>
#include <LRE.h>
#include <PairClassifier.h>
#include <DataFuncs.h>

using namespace std;

//___________________________________________________________________
UINT WebDecoys(void * arg)
{
    LREThreadBuffer * buffer = (LREThreadBuffer*)arg;
    
    PairClassifier * classifier = NULL;

    classifier = new WebDecoyClassifier(buffer);
    
    classifier->ClassifyPairs();
    delete classifier;
    
    return 0;
}

//___________________________________________________________________
static LREThreadBuffer *
                PairsSplit(LinesReader  ** lrIRScript,
                           HANDLE mLogMutex,
                           HANDLE mReadMutex,
                           const string & stThreadID,
                           const int iPairsSegment,
                           const map<string,FeatureInfo,LexComp> * lexicon)
{
    const string stLogsFolder = ResourceBox::Get()->getStringValue("LogsFolder");
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    LREThreadBuffer * buffer = NULL;
    int i;
    string stLine(75,'\0');

    fprintf(stderr,"Alloc Pairs Thread buffer\n");
    fflush(stderr);

    buffer = new LREThreadBuffer(mLogMutex,mReadMutex,0,iPairsSegment+1);
    if (buffer == NULL) abort_err("Alloc Thread Buffer failed");

    buffer->lexicon = lexicon;
    buffer->iAnonymIndex=0;

    buffer->stThreadID = stThreadID;
    buffer->stIRScriptFile = "pair_script_"+stThreadID+".txt";
    buffer->stIRQueryLog = stLogsFolder+stThreadID+".txt";
    buffer->stDebugLog = stLogsFolder+"debug_"+stThreadID+".txt";

    fprintf(stderr,"open the script file: %s\n",buffer->stIRScriptFile.c_str());
    fflush(stderr);

    FILE * fIRScript = open_file(stSampleFolder + buffer->stIRScriptFile,"PairsSplit","w");

    const string stScriptFile = stSampleFolder+ResourceBox::Get()->getStringValue("ScriptFile");
    fprintf(stderr,"Alloc Lines Reader: %s\n",stScriptFile.c_str());
    fflush(stderr);

    if (lrIRScript == NULL) abort_err("NULL lines reader buffer");
    if (*lrIRScript == NULL) *lrIRScript = new LinesReader(stScriptFile,512);
    if (*lrIRScript == NULL) abort_err("Failed to alloc lines reader object");
    i=0;
    while ((i++<iPairsSegment) && (*lrIRScript)->bMoreToRead())
    {
        (*lrIRScript)->voGetLine(stLine);
        fprintf(fIRScript,"%s\n",stLine.c_str());
        fflush(fIRScript);
    }
    fclose(fIRScript);

    fprintf(stderr,"Create Thread\n");
    fflush(stderr);

    DWORD tiPairThread;
    buffer->tid = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)WebDecoys,(void*)buffer,0,&tiPairThread);
    if (buffer->tid == NULL) abort_err("Failed to create LRE thread");

    return buffer;
}

//___________________________________________________________________
static void WebDecoysProcess()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stFeatureSet = ResourceBox::Get()->getStringValue("FeatureSetFile");
    const string stScriptFile = stSampleFolder+ResourceBox::Get()->getStringValue("ScriptFile");
    const int iSampleSize = ScriptSize(stScriptFile);
    map<string,FeatureInfo,LexComp> * lexicon = NULL;

    fprintf(stderr,"Main Thread (PrivateDecoys) - Start\n");
    fflush(stderr);
    HANDLE mLogMutex;
    HANDLE mReadMutex;

    mLogMutex = CreateMutex(NULL,FALSE,NULL);
    mReadMutex = CreateMutex(NULL,FALSE,NULL);
    if (mLogMutex == NULL) abort_err("Create mutex failed (Log)");
    if (mReadMutex == NULL) abort_err("Create mutex failed (Read)");

    // Allocate the threads.
    const string stLogsFolder = ResourceBox::Get()->getStringValue("LogsFolder");
    FILE * fPairClassRes = open_file(ResourceBox::Get()->getStringValue("PairClassRes"),"PairClassifier","w");
    LinesReader * lrSamples=NULL;

    const int iTotalPairsNumber = ScriptSize(stScriptFile);
    const int iMultiProcDegree = ResourceBox::Get()->getIntValue("MultiProcessingDegree");
    const int iPairsSegment = iTotalPairsNumber/iMultiProcDegree + iTotalPairsNumber%iMultiProcDegree;
    list<LREThreadBuffer*> threadsList;
    LREThreadBuffer * buffer=NULL;
    char thread_id[50];
    int i;

    fprintf(stderr,"#Total Pairs=%d; Segment: %d + %d = %d\n",iTotalPairsNumber,iTotalPairsNumber/iMultiProcDegree,iTotalPairsNumber%iMultiProcDegree,iPairsSegment);
    fflush(stderr);

    fprintf(stderr,"Alloc Threads\n");
    fflush(stderr);

    for (i=0;i<iMultiProcDegree;i++)
    {
        sprintf(thread_id,"tid%d",i);
        buffer = PairsSplit(&lrSamples,
                            mLogMutex,
                            mReadMutex,
                            thread_id,
                            iPairsSegment,
                            lexicon);
        threadsList.push_back(buffer);
    }

    fprintf(stderr,"Join Threads\n");
    fflush(stderr);

    list<LREThreadBuffer*>::iterator threadsIter = threadsList.begin();
    int iProcessedPairs=0;
    while (threadsIter != threadsList.end())
    {
        fprintf(stderr,"Wait for thread end\n");
        fflush(stderr);

        LREThreadBuffer * ltbTemp = *threadsIter;

        fprintf(stderr,"Call Join\n");
        fflush(stderr);

        WaitForSingleObject(ltbTemp->tid,INFINITE);
        if (CloseHandle(ltbTemp->tid) == 0) abort_err("Abort thread failed");
        threadsIter++;

        fprintf(stderr,"Consolidate the Pairs queries log. tid=%s, #Pairs=%d\nIRLog=%s\n",
                buffer->stThreadID.c_str(),buffer->iAnonymIndex,buffer->stIRQueryLog.c_str());
        fflush(stderr);

        ConsolidateLogs(fPairClassRes,buffer->stIRQueryLog);

        iProcessedPairs += buffer->iAnonymIndex;

        fprintf(stderr,"Del Buffer\n");
        fflush(stderr);

        delete buffer;

        fprintf(stderr,"End Join iteration\n");
        fflush(stderr);
    }

    fflush(fPairClassRes);
    fclose(fPairClassRes);

    if (CloseHandle(mLogMutex) == 0) abort_err("Destroy mutex failed (Log)");
    if (CloseHandle(mReadMutex) == 0) abort_err("Destroy mutex failed (Read)");
    
    if (lexicon != NULL) delete lexicon;

    fprintf(stderr,"#Processed Pairs=%d\n",iProcessedPairs);
    fprintf(stderr,"Main Thread (PrivateDecoys) - End\n");
    fflush(stderr);
}

//___________________________________________________________________
void ClassifyPairs()
{
    const string stDecoysType = ResourceBox::Get()->getStringValue("DecoysType");
    PairClassifier * classifier = NULL;
    LREThreadBuffer buffer;
    
    fprintf(stderr,"ClassifyPairs - Start (%s)\n",stDecoysType.c_str()); fflush(stderr);
    
    buffer.stDebugLog = ResourceBox::Get()->getStringValue("Debug_Log");
    buffer.stIRQueryLog = ResourceBox::Get()->getStringValue("PairClassRes");
    
    if (stDecoysType.compare("RandomDecoys") == 0) classifier = new RandomDecoyClassifier(&buffer);
    else if (stDecoysType.compare("SimilarDecoys") == 0) classifier = new SimilarDecoyClassifier(&buffer);
    else if (stDecoysType.compare("GenreDecoys") == 0) classifier = new GenreDecoyClassifier(&buffer);
    else if (stDecoysType.compare("PlainSimilarity") == 0) classifier = new PlainSimilarityClassifier(&buffer);
    
    if (classifier != NULL)
    {
        classifier->ClassifyPairs();
        delete classifier;
    }
    else if (stDecoysType.compare("WebDecoys") == 0)
    {
        WebDecoysProcess();
    }
    else abort_err(string("Illegal decoys type in pairs classification: ")+stDecoysType);
}
