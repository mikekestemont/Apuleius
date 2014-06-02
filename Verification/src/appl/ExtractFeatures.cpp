#include <ResourceBox.h>
#include <auxiliary.h>
#include <FolderScanner.h>
#include <FeaturesHandler.h>
#include <Timer.h>

using namespace std;

//___________________________________________________________________
static void HandleTrainFolder(const string & stTrainFolder,
                              FeaturesHandler * featureSet,
                              Timer & tiGetFtrs,
                              FILE * fTrainScript,
                              FILE * debug_log)
{
    FolderScanner fsTrain(stTrainFolder+"\\*.txt");
    string        stFileName(250,'\0');
    string        stCurrentFile(250,'\0');
    set<string,LexComp> termsMarker;

    fprintf(stdout,"Start folder scanning (%s)\n",stTrainFolder.c_str()); fflush(stdout);
    while (fsTrain.bMoreToRead())
    {
        stFileName = fsTrain.getNextFile();

        if ((stFileName.compare(".") == 0) || (stFileName.compare("..") == 0)) continue;
        if (fsTrain.bIsFolder()) abort_err(string("DIR encountered in ")+stTrainFolder+": "+stFileName);

        stCurrentFile = stTrainFolder;
        stCurrentFile += stFileName;

        if (fTrainScript) {fprintf(fTrainScript,"%s\n",stCurrentFile.c_str()); fflush(fTrainScript);}

        if ((featureSet->iGetDocsCount()%500) == 0) {fprintf(stdout,"#File=%d, %s\n",featureSet->iGetDocsCount(),stFileName.c_str()); fflush(stdout);}

        tiGetFtrs.start();
        termsMarker.clear();
        featureSet->DocToVec(stCurrentFile,NULL,termsMarker,debug_log);
        tiGetFtrs.stop();
    }
    fprintf(stdout,"End folder scanning\n"); fflush(stdout);
}

//___________________________________________________________________
static void HandlePrivateDecoys(const string & stTrainFolder,
                                const string & stExtension,
                                FILE * fTrainScript,
                                FeaturesHandler * featureSet,
                                Timer & tiGetFtrs,
                                FILE * debug_log)
{
    FolderScanner dirScanner(stTrainFolder+"\\*.txt");
    string        stItem(250,'\0');

    fprintf(stdout,"HandlePrivateDecoys - Start (%s)\n",stTrainFolder.c_str()); fflush(stdout);
    int iDirs=1;
    while (dirScanner.bMoreToRead())
    {
        stItem = dirScanner.getNextFile();

        fprintf(stdout,"Working on pair: %s, %d ...\n",stItem.c_str(),iDirs++); fflush(stdout);

        if ((stItem.compare(".") == 0) || (stItem.compare("..") == 0)) continue;
        if (!dirScanner.bIsFolder()) abort_err(string("Non DIR encountered in ")+stTrainFolder+": "+stItem);
        
        HandleTrainFolder(stTrainFolder+stItem+"\\"+stExtension,featureSet,tiGetFtrs,fTrainScript,debug_log);
    }
    fprintf(stdout,"HandlePrivateDecoys - End\n"); fflush(stdout);
}

//___________________________________________________________________
void FeaturesExtraction()
{
    fprintf(stdout,"FeaturesExtraction - Start\n"); fflush(stdout);

    list<string> foldersList;
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stTrainFolder = ResourceBox::Get()->getStringValue("TrainFolder");
    ResourceBox::Get()->getListValues("TrainSubFolders",foldersList);

    ExtractFeatures extractor;

    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"FeaturesExtraction","w");
    FILE * fTrainScript = open_file(stSampleFolder + ResourceBox::Get()->getStringValue("TrainScriptFile"),"FeaturesExtraction","w");
    
    Timer tiMainLoop;
    Timer tiGetFtrs;

    tiMainLoop.start();
    fprintf(stdout,"Scan Sub Folders - Start (Train Folder=%s)\n",stTrainFolder.c_str()); fflush(stdout);
    string stSubFolder;
    string stExtension;
    size_t iPos;
    while (!foldersList.empty())
    {
        stSubFolder = foldersList.front();
        fprintf(stdout,"Working on sub folder: %s\n",stSubFolder.c_str()); fflush(stdout);

        if (stSubFolder.substr(0,9).compare("PAIRDECOY") == 0)
        {
            iPos = stSubFolder.find(";");
            if (iPos == string::npos) {stExtension=""; stSubFolder=stSubFolder.substr(10);}
            else {stExtension=stSubFolder.substr(iPos+1)+"\\"; stSubFolder="";}
            if (!stSubFolder.empty()) stSubFolder+"\\";
            fprintf(stdout,"Call Private Decoys: SubFolder=%s, Extension=%s, pos=%d\n",stSubFolder.c_str(),stExtension.c_str(),iPos); fflush(stdout);
            HandlePrivateDecoys(stTrainFolder + stSubFolder,
                                stExtension,
                                fTrainScript,
                                &extractor,
                                tiGetFtrs,
                                debug_log);
        }
        else
        {
            fprintf(stdout,"Call Regualr Decoys: SubFolder=%s\n",stSubFolder.c_str()); fflush(stdout);
            HandleTrainFolder(stTrainFolder + stSubFolder + "\\",&extractor,tiGetFtrs,fTrainScript,debug_log);
        }
        foldersList.pop_front();
    }
    fprintf(stdout,"Scan Sub Folders - End\n"); fflush(stdout);
    tiMainLoop.stop();

    const string stFeatureSet = ResourceBox::Get()->getStringValue("FeatureSetFile");
    const int iLines = extractor.iPrintFeatureSet(stSampleFolder+stFeatureSet,debug_log);

    fprintf(stdout,"Features Count: %d (out of %d)\n",iLines,extractor.iGetFeaturesCount());
    fprintf(stdout,"Lexicon Size: %d\n",extractor.iGetLexiconSize());
    fprintf(stdout,"Corpus Size: %d\n",extractor.iGetCorpusSize());
    fprintf(stdout,"Main Loop:    %d\n",tiMainLoop.iGetTime());
    fprintf(stdout,"Get Features: %d\n",tiGetFtrs.iGetTime());
    fprintf(stdout,"#Files: %d\n",extractor.iGetDocsCount());
    fprintf(stdout,"Sample Folder: %s\n",stSampleFolder.c_str());
    fflush(stdout);
    
    fflush(debug_log);
    fclose(debug_log);
    fflush(fTrainScript);
    fclose(fTrainScript);

    fprintf(stdout,"FeaturesExtraction - End\n"); fflush(stdout);
}

//___________________________________________________________________
void ExtractPrivateFeatures()
{
    fprintf(stdout,"ExtractPrivateFeatures - Start\n"); fflush(stdout);

    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stFeatureSet = ResourceBox::Get()->getStringValue("FeatureSetFile");
    const string stPairsFolder = stSampleFolder + ResourceBox::Get()->getStringValue("DecoyFolder") + "\\";
    const string stBeginPrefix = ResourceBox::Get()->getStringValue("PrefixCodeBegin");
    const string stEndPrefix = ResourceBox::Get()->getStringValue("PrefixCodeEnd");

    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"ExtractPrivateFeatures","w");
    
    ExtractFeatures * extractor = NULL;
    int iFtrsCount;
    Timer tiGetFtrs;
    
    const string stScriptFile = stSampleFolder + ResourceBox::Get()->getStringValue("ScriptFile");
    TdhReader tdhScript(stScriptFile,512);
    vector<string> vFields(2,"");
    string        stPairKey(250,'\0');
    string        stWorkFolder;
    set<string,LexComp> termsMarker;

    fprintf(stdout,"Scan Pairs - Start\n"); fflush(stdout);
    int iPairs=1;
    while (tdhScript.bMoreToRead())
    {
        tdhScript.iGetLine(vFields);
        stPairKey = GetPairKey(vFields.at(0),vFields.at(1));
        stWorkFolder = stPairsFolder + stPairKey + "\\";

        fprintf(stdout,"Working on pair: %s, %d ...\n",stPairKey.c_str(),iPairs++); fflush(stdout);
        
        extractor = new ExtractFeatures;
        tiGetFtrs.reset();
        HandleTrainFolder(stWorkFolder+"Text\\",extractor,tiGetFtrs,NULL,debug_log);

        /*termsMarker.clear();
        extractor->DocToVec(stWorkFolder+"Text\\"+stBeginPrefix+vFields.at(0),NULL,termsMarker,debug_log);
        termsMarker.clear();
        extractor->DocToVec(stWorkFolder+"Text\\"+stEndPrefix+vFields.at(1),NULL,termsMarker,debug_log);*/

        iFtrsCount = extractor->iPrintFeatureSet(stWorkFolder+stFeatureSet,debug_log);

        fprintf(stdout,"Features Count: %d (out of %d)\n",iFtrsCount,extractor->iGetFeaturesCount());
        fprintf(stdout,"Corpus Size: %d\n",extractor->iGetCorpusSize());
        fprintf(stdout,"Get Features: %d\n",tiGetFtrs.iGetTime());
        fprintf(stdout,"#Files: %d\n",extractor->iGetDocsCount());
        fflush(stdout);
        
        delete extractor;
        extractor=NULL;
    }
    fprintf(stdout,"Scan Pairs - End (#Pairs=%d)\n",iPairs); fflush(stdout);
    
    fflush(debug_log);
    fclose(debug_log);

    fprintf(stdout,"ExtractPrivateFeatures - End\n"); fflush(stdout);
}

//___________________________________________________________________
static void HandleHTMLFolder(const string & stSnipName,
                             FeaturesHandler * featureSet,
                             Timer & tiGetFtrs,
                             FILE * debug_log)
{
    const string stHTMLFolder = ResourceBox::Get()->getStringValue("WebTextFolder");
    string stSubFolderName = stSnipName;
    RemoveFileSuffix(stSubFolderName,4);
    HandleTrainFolder(stHTMLFolder+stSubFolderName+"\\",featureSet,tiGetFtrs,NULL,debug_log);
}

//___________________________________________________________________
void ExtractHTMLFeatures()
{
    fprintf(stdout,"ExtractHTMLFeatures - Start\n"); fflush(stdout);

    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stFeatureSet = ResourceBox::Get()->getStringValue("FeatureSetFile");
    const string stPairsFolder = stSampleFolder + ResourceBox::Get()->getStringValue("DecoyFolder") + "\\";
    const string stBeginPrefix = ResourceBox::Get()->getStringValue("PrefixCodeBegin");
    const string stEndPrefix = ResourceBox::Get()->getStringValue("PrefixCodeEnd");

    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"ExtractHTMLFeatures","w");
    
    ExtractFeatures * extractor = NULL;
    int iFtrsCount;
    Timer tiGetFtrs;
    
    const string stScriptFile = stSampleFolder + ResourceBox::Get()->getStringValue("ScriptFile");
    TdhReader tdhScript(stScriptFile,512);
    vector<string> vFields(2,"");
    string        stPairKey(250,'\0');
    string        stWorkFolder;
    set<string,LexComp> termsMarker;

    fprintf(stdout,"Scan Pairs - Start\n"); fflush(stdout);
    int iPairs=1;
    while (tdhScript.bMoreToRead())
    {
        tdhScript.iGetLine(vFields);
        stPairKey = GetPairKey(vFields.at(0),vFields.at(1));
        stWorkFolder = stPairsFolder + stPairKey + "\\";

        fprintf(stdout,"Working on pair: %s, %d ...\n",stPairKey.c_str(),iPairs++); fflush(stdout);
        
        extractor = new ExtractFeatures;
        tiGetFtrs.reset();
        HandleHTMLFolder(string("Begin\\")+vFields.at(0),extractor,tiGetFtrs,debug_log);
        HandleHTMLFolder(string("End\\")+vFields.at(1),extractor,tiGetFtrs,debug_log);

        termsMarker.clear();
        extractor->DocToVec(stWorkFolder+"Text\\"+stBeginPrefix+vFields.at(0),NULL,termsMarker,debug_log);
        termsMarker.clear();
        extractor->DocToVec(stWorkFolder+"Text\\"+stEndPrefix+vFields.at(1),NULL,termsMarker,debug_log);

        iFtrsCount = extractor->iPrintFeatureSet(stWorkFolder+stFeatureSet,debug_log);

        fprintf(stdout,"Features Count: %d (out of %d)\n",iFtrsCount,extractor->iGetFeaturesCount());
        fprintf(stdout,"Corpus Size: %d\n",extractor->iGetCorpusSize());
        fprintf(stdout,"Get Features: %d\n",tiGetFtrs.iGetTime());
        fprintf(stdout,"#Files: %d\n",extractor->iGetDocsCount());
        fflush(stdout);
        
        delete extractor;
        extractor=NULL;
    }
    fprintf(stdout,"Scan Pairs - End (#Pairs=%d)\n",iPairs); fflush(stdout);
    
    fflush(debug_log);
    fclose(debug_log);

    fprintf(stdout,"ExtractHTMLFeatures - End\n"); fflush(stdout);
}
