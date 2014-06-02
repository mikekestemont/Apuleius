#include <stdio.h>
#include <auxiliary.h>
#include <LREProgs.h>
#include <Timer.h>
#include <ThesisAux.h>

using namespace std;

int main(int argc, char * argv[])
{
    //1: Load the program parameters.
    cout <<"Program Start (argc="<<argc<<")\n";
    if (argc < 3) abort_err(string("Err: argc=")+stringOf(argc));

    // The ResourceBox reads the configuration file and stores
    // all the configuration parameters.
    // It is maintained as a singelton.
    fprintf(stderr,"Load Resource Box (resource file=%s, ProgName=%s)\n",argv[1],argv[2]);
    fflush(stderr);
    map<string,string,LexComp> paramsMap;
    LoadArgs(argc,argv,paramsMap);
    ResourceBox::Get(argv[1],&paramsMap);

    const string stProgramName = argv[2];
    
    Timer tiRunTime;
    tiRunTime.start();

    if (stProgramName.compare("TextToFeatures") == 0)
    {
        TextToFeatures();
    }
    else if (stProgramName.compare("PrivateFSToFeatures") == 0)
    {
        PrivateFSToFeatures();
    }
    else if (stProgramName.compare("LRE") == 0)
    {
        LRE("LRE");
    }
    else if (stProgramName.compare("LREClassify") == 0)
    {
        LRE("LREClassify");
    }
    else if (stProgramName.compare("LRESummary") == 0)
    {
        LRESummary();
    }
    else if (stProgramName.compare("LREClasifySummary") == 0)
    {
        LREClasifySummary();
    }
    else if (stProgramName.compare("ClassifyPairs") == 0)
    {
        ClassifyPairs();
    }
    else if (stProgramName.compare("ClassifyPairSummary") == 0)
    {
        ClassifyPairSummary();
    }
    else if (stProgramName.compare("PlainSimSummary") == 0)
    {
        PlainSimSummary();
    }
    else if (stProgramName.compare("TruncatePosts") == 0)
    {
        TruncatePosts();
    }
    else if (stProgramName.compare("IRSummary") == 0)
    {
        IRSummary();
    }
    else if (stProgramName.compare("FeaturesExtraction") == 0)
    {
        FeaturesExtraction();
    }
    else if (stProgramName.compare("GetAllPairs") == 0)
    {
        GetAllPairs();
    }
    else if (stProgramName.compare("TextsToPairScript") == 0)
    {
        TextsToPairScript();
    }
    else if (stProgramName.compare("BreakTexts") == 0)
    {
        BreakTexts();
    }
    else if (stProgramName.compare("TextLengths") == 0)
    {
        TextLengths();
    }
    else if (stProgramName.compare("FilterEnglishText") == 0)
    {
        FilterEnglishText();
    }
    else if (stProgramName.compare("AverageScoreOverChunks") == 0)
    {
        AverageScoreOverChunks();
    }
    else if (stProgramName.compare("CreateIRScript") == 0)
    {
        CreateIRScript();
    }
	else if (stProgramName.compare("CreateApulieusIRTest") == 0)
	{
		CreateApulieusIRTest();
	}
	else if (stProgramName.compare("TextsToPairs") == 0)
	{
		TextsToPairs();
	}
	else if (stProgramName.compare("EnvToPairs") == 0)
	{
		EnvToPairs();
	}
	else if (stProgramName.compare("EnvToIR") == 0)
	{
		EnvToIR();
	}
    else
    {
        cout <<"Tries to run unknown program: "<<argv[2]<<". Abort\n";
    }
    
    tiRunTime.stop();
    fprintf(stderr,"Total Run Time: %d\n",tiRunTime.iGetTime());
    fflush(stderr);

    return 1;
}
