#include <ResourceBox.h>
#include <auxiliary.h>
#include <FileScanner.h>
#include <FolderScanner.h>
#include <FeaturesHandler.h>
#include <PairClassifier.h>
#include <math.h>

using namespace std;

//___________________________________________________________________
static void ProcessFile(const string & stSamplePath,
                        const string & stFileName,
                        FeaturesHandler * featureSet,
                        int & iTotalDocsSize,
                        FILE * debug_log)
{
    list<FeatureItem> lTextProfile;
    list<FeatureItem>::iterator lit;
    set<FeatureItem,FIOrderByWeight> sortedItems;
    set<FeatureItem,FIOrderByWeight>::const_iterator mit;
    set<string,LexComp> termsMarker;
    FILE * featureFile = open_file(stSamplePath+"Ftrs\\"+stFileName,"ProcessFile","w",true);

    termsMarker.clear();
    featureSet->DocToVec(stSamplePath+"Text\\"+stFileName,&lTextProfile,termsMarker,debug_log);

    iTotalDocsSize += featureSet->iGetDocSize();
    
    int iPrevIndex=-1;
    sortedItems.clear();
    for (lit=lTextProfile.begin();lit!=lTextProfile.end();lit++)
    {
        if (iPrevIndex >= lit->index) abort_err(string("Illegal list order: ")+stringOf(iPrevIndex)+stringOf(lit->index));
        iPrevIndex=lit->index;
        sortedItems.insert(*lit);
    }

    for (mit=sortedItems.begin();mit!=sortedItems.end();mit++)
    {
        fprintf(featureFile,"%s\t%.15f\t%.10f\n",featureSet->stGetFeature(mit->index).c_str(),mit->weight,mit->idf);
    }
    fflush(featureFile);
    fclose(featureFile);
}

//___________________________________________________________________
static void ProcessSample(FeaturesHandler * featureSet,
                          const string & stSamplePath,
                          const string & stType,
                          const string & stScriptFile,
                          FILE         * debug_log)
{
    const string stSampleFolder = stSamplePath+stType+"\\";
    int iTotalDocsSize = 0;

    fprintf(stdout,"Working on: %s, %s\n",stType.c_str(),stScriptFile.c_str());
    fflush(stdout);

    LinesReader lrReader(stScriptFile,512);
    string      stCurrentFile;
    string      stFileName;
    
    CleanFolder(stSampleFolder+"Ftrs\\");

    featureSet->Reset();
    int iFiles=0;
    while (lrReader.bMoreToRead())
    {
        lrReader.voGetLine(stFileName);

        if ((++iFiles%500)==0) {fprintf(stdout,"%d files have been read...\n",iFiles); fflush(stdout);}
        
        ProcessFile(stSampleFolder,
                    stFileName,
                    featureSet,
                    iTotalDocsSize,
                    debug_log);
    }

    fprintf(stdout,"Completed Sample: %s\n",stSampleFolder.c_str());
    fprintf(stdout,"Lexicon Size:     %d\n",featureSet->iGetLexiconSize());
    fprintf(stdout,"Avg Doc Size:     %d\n",(iTotalDocsSize / iFiles));
    fprintf(stdout,"#Files:           %d\n\n",iFiles);
}

//___________________________________________________________________
static int CountTrainFiles()
{
    fprintf(stdout,"Count Train Files - Start\n"); fflush(stdout);

    const string stTrainFolder = ResourceBox::Get()->getStringValue("TrainFolder");
    int iCounter=0;
    size_t iPos;
    list<string> foldersList;
    ResourceBox::Get()->getListValues("TrainSubFolders",foldersList);
    string stSubFolder;
    FolderScanner * fsTrainFolders=NULL;
    string stPairKey;
    fprintf(stdout,"Scan sub folders - Start\n"); fflush(stdout);
    while (!foldersList.empty())
    {
        stSubFolder = foldersList.front();

        fprintf(stdout,"\tCurrent Sub Folder: %s\n",stSubFolder.c_str()); fflush(stdout);

        if (stSubFolder.substr(0,9).compare("PAIRDECOY") == 0)
        {
            fprintf(stdout,"\t\tHandle PAIR DECOY\n"); fflush(stdout);
            iPos = stSubFolder.find(";");
            if (iPos == string::npos)
            {
                fsTrainFolders = new FolderScanner(stTrainFolder + stSubFolder.substr(10) + "\\*.txt");

                while (fsTrainFolders->bMoreToRead())
                {
                    stPairKey = fsTrainFolders->getNextFile();
                    if ((stPairKey.compare(".") == 0) || (stPairKey.compare("..") == 0)) continue;
                    if (!fsTrainFolders->bIsFolder()) abort_err(string("Non DIR encountered in ")+stTrainFolder+": "+stPairKey);
                    iCounter += CountFiles(stTrainFolder + stSubFolder.substr(10) + "\\" + stPairKey + "\\Text\\");
                }
                delete fsTrainFolders;
                fsTrainFolders=NULL;
            }
            else
            {
                string stExtension=stSubFolder.substr(iPos+1)+"\\";
                fprintf(stdout,"Call Private Decoys: SubFolder=%s, Extension=%s, pos=%d\n",stSubFolder.c_str(),stExtension.c_str(),iPos); fflush(stdout);

                fsTrainFolders = new FolderScanner(stTrainFolder+"\\*.txt");

                while (fsTrainFolders->bMoreToRead())
                {
                    stSubFolder = fsTrainFolders->getNextFile();
                    if ((stSubFolder.compare(".") == 0) || (stSubFolder.compare("..") == 0)) continue;
                    if (!fsTrainFolders->bIsFolder()) abort_err(string("Non DIR encountered in ")+stTrainFolder);
                    iCounter += CountFiles(stTrainFolder + stSubFolder + "\\" + stExtension + "\\");
                }
                delete fsTrainFolders;
                fsTrainFolders=NULL;
            }
        }
        else
        {
            fprintf(stdout,"\t\tHandle Regular Sub Folder\n"); fflush(stdout);
            iCounter += CountFiles(stTrainFolder + stSubFolder + "\\*.txt");
        }
        foldersList.pop_front();
    }
    fprintf(stdout,"Scan sub folders - End\n"); fflush(stdout);

    fprintf(stdout,"Count Train Files - End (#Files=%d)\n",iCounter); fflush(stdout);

    return iCounter;
}

//___________________________________________________________________
void TextToFeatures()
{
    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"TextToFeatures","w");
    list<string> foldersList;
    const string stTrainFolder = ResourceBox::Get()->getStringValue("TrainFolder");
    ResourceBox::Get()->getListValues("TrainSubFolders",foldersList);
    const int iTrainFiles = CountTrainFiles();
    
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stFeatureSet = ResourceBox::Get()->getStringValue("FeatureSetFile");
    CountFeatures featureSet(iTrainFiles);
    featureSet.voLoadFeatureSet(stSampleFolder+stFeatureSet,debug_log);

    list<string> samplesList;
    list<string> scriptsList;
    ResourceBox::Get()->getListValues("SamplesList",samplesList);
    ResourceBox::Get()->getListValues("ScriptsList",scriptsList);

    fprintf(stdout,"Train Docs: %d\n",iTrainFiles);
    while (!samplesList.empty())
    {
        ProcessSample(&featureSet,stSampleFolder,samplesList.front(),stSampleFolder+scriptsList.front(),debug_log);
        samplesList.pop_front();
        scriptsList.pop_front();
    }

    fflush(debug_log);
    fclose(debug_log);
}

//___________________________________________________________________
static void HandlePair(const string & stDecoysFolder,
                       const string & stBeginSnip,
                       const string & stEndSnip,
                       int & iMinDecoysNumber,
                       int & iTotalDecoys,
                       FILE * debug_log)
{
    const string stPairKey = GetPairKey(stBeginSnip,stEndSnip);
    const string stPairPath = stDecoysFolder + stPairKey + "\\";
    const string stFeatureSet = ResourceBox::Get()->getStringValue("FeatureSetFile");
    const string stDecoyFile = ResourceBox::Get()->getStringValue("DecoyFile");
    const string stBeginPrefix = ResourceBox::Get()->getStringValue("PrefixCodeBegin");
    const string stEndPrefix = ResourceBox::Get()->getStringValue("PrefixCodeEnd");

    const int iTrainFiles = CountFiles(stPairPath+"Text\\");
    CountFeatures featureSet(iTrainFiles);
    featureSet.voLoadFeatureSet(stPairPath+stFeatureSet,debug_log);
    int iDocSize=0;
    
    if (iMinDecoysNumber > iTrainFiles) iMinDecoysNumber=iTrainFiles;
    iTotalDecoys += iTrainFiles;
    
    ProcessSample(&featureSet,stDecoysFolder,stPairKey,stPairPath+stDecoyFile,debug_log);
    ProcessFile(stPairPath,stBeginPrefix+stBeginSnip,&featureSet,iDocSize,debug_log);
    ProcessFile(stPairPath,stEndPrefix+stEndSnip,&featureSet,iDocSize,debug_log);
}

//___________________________________________________________________
void PrivateFSToFeatures()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stPairsFolder = stSampleFolder + ResourceBox::Get()->getStringValue("DecoyFolder") + "\\";
    const string stScriptFile = stSampleFolder + ResourceBox::Get()->getStringValue("ScriptFile");
    TdhReader tdhScript(stScriptFile,512);
    vector<string> vFields(2,"");
    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"TextToFeatures","w");

    fprintf(stdout,"PrivateFSToFeatures - Start (Script File: %s)\n",stScriptFile.c_str());
    fflush(stdout);
    int iCounter=0;
    int iMinDecoysNumber = 10000;
    int iTotalDecoys = 0;
    while (tdhScript.bMoreToRead())
    {
        tdhScript.iGetLine(vFields);

        fprintf(stdout,"Working on %s -> %s, %d...\n",vFields.at(0).c_str(),vFields.at(1).c_str(),++iCounter);
        fflush(stdout);
        
        HandlePair(stPairsFolder,vFields.at(0),vFields.at(1),iMinDecoysNumber,iTotalDecoys,debug_log);
    }
    fprintf(stdout,"PrivateFSToFeatures - End (#Pairs=%d, #MinDecoys=%d, AvgDecoys=%.1f)\n",iCounter,iMinDecoysNumber,(double)(iTotalDecoys - 2*iCounter)/(double)iCounter);
    fflush(stdout);

    fflush(debug_log);
    fclose(debug_log);
}
