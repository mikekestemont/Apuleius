#include <ResourceBox.h>
#include <auxiliary.h>
#include <FileScanner.h>
#include <FolderScanner.h>
#include <MLAuxFuncs.h>
#include <set>
#include <map>

using namespace std;

//___________________________________________________________________
void MakeIRScript()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stAnonyms = stSampleFolder+"anonyms.txt";
    const string stAuthors = stSampleFolder+"authors.txt";
    const string stCands = stSampleFolder+"candidates.txt";
    const int iSampleSize = ResourceBox::Get()->getIntValue("SampleSize");
    const int iCandsNumber = ResourceBox::Get()->getIntValue("CandidatesNumber");
    const int iSnipsNumber = ScriptSize(stSampleFolder+"all.txt");
    const double dAnonymsRate = ((double)iSampleSize / (double)iSnipsNumber) + 0.075;
    const double dAuthorsRate = ((double)iCandsNumber / (double)(iSnipsNumber-iSampleSize + 0.025)) + 0.075;
    const bool bClosedAuthorSet = (ResourceBox::Get()->getIntValue("ClosedAuthorSet") > 0);

    fprintf(stderr,"MakeIRScript - Start\n");
    fprintf(stderr,"Sample Folder: %s\n",stSampleFolder.c_str());
    fprintf(stderr,"Sample=%d, #Cands=%d, #Snips=%d\n",iSampleSize,iCandsNumber,iSnipsNumber);
    fprintf(stderr,"Anonym Rate=%.3f, Author Rate=%.3f\n",dAnonymsRate,dAuthorsRate);
    fflush(stderr);

    FILE * fAnonyms = open_file(stAnonyms,"MakeIRScript","w");
    FILE * fAuthors = open_file(stAuthors,"MakeIRScript","w");
    FILE * fCands = open_file(stCands,"MakeIRScript","w");
    set<string,LexComp> anonyms;

    LinesReader lrList(stSampleFolder+"all.txt",512);
    string stFileName;
    int iAnonyms=0;
    int iAuthors=0;

    while (lrList.bMoreToRead() && (iAnonyms < iSampleSize))
    {
        lrList.voGetLine(stFileName);
        if (get_uniform01_random() > dAnonymsRate) continue;

        iAnonyms++;
        fprintf(fAnonyms,"%s\n",stFileName.c_str()); fflush(fAnonyms);
        fprintf(fCands,"%s\n",stFileName.c_str()); fflush(fCands);
        if (anonyms.find(stFileName) != anonyms.end()) abort_err(stFileName+" already found...");
        anonyms.insert(stFileName);
        if (!bClosedAuthorSet) continue;
        fprintf(fAuthors,"%s\n",stFileName.c_str()); fflush(fAuthors);
        iAuthors++;
    }

    lrList.restart();
    fprintf(stderr,"Start printing authors (last anonyms: %s)\n",stFileName.c_str()); fflush(stderr);
    while (lrList.bMoreToRead() && (iAuthors < iCandsNumber))
    {
        lrList.voGetLine(stFileName);
        if (get_uniform01_random() > dAuthorsRate) continue;
        if (anonyms.find(stFileName) != anonyms.end()) continue;

        iAuthors++;
        fprintf(fAuthors,"%s\n",stFileName.c_str()); fflush(fAuthors);
        fprintf(fCands,"%s\n",stFileName.c_str()); fflush(fCands);
    }

    fflush(fAnonyms); fclose(fAnonyms);
    fflush(fAuthors); fclose(fAuthors);
    fflush(fCands); fclose(fCands);

    fprintf(stderr,"MakeIRScript - End (#Anonyms=%d, #Authors=%d)\n",iAnonyms,iAuthors);
    fflush(stderr);
}

//___________________________________________________________________
static void GetBestCand(const string & stAnonym,
                        const map<string, map<string,int> > & candsBoxes,
                        string & stBestCand,
                        int & iWinCount)
{
    map<string, map<string,int> >::const_iterator boxesIter;
    map<string,int>::const_iterator candsIter;

    iWinCount = -1;
    stBestCand = "";

    if ((boxesIter = candsBoxes.find(stAnonym)) == candsBoxes.end()) abort_err(string("The anonym ")+stAnonym+" not found in cands box");
    for (candsIter = boxesIter->second.begin(); candsIter != boxesIter->second.end(); candsIter++)
    {
        if (candsIter->second <= iWinCount) continue;

        iWinCount = candsIter->second;
        stBestCand = candsIter->first;
    }
}

//___________________________________________________________________
void LRESummary()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stLogFile = ResourceBox::Get()->getStringValue("IRQueryLog");
    const string stIRScript = stSampleFolder+"anonyms.txt";
    const string stLRESummary = ResourceBox::Get()->getStringValue("LRESummary");
    const int iTotalSearches = ScriptSize(stLogFile);;
    const int iAnonymsNum = ScriptSize(stIRScript);;
    const int iIRLoops = ResourceBox::Get()->getIntValue("IRLoops");
    const int iStartFrom = ResourceBox::Get()->getIntValue("StartNameIndex");
    const int iWriterIdLength = ResourceBox::Get()->getIntValue("WriterIdLength");
    const int iNameKeyLen = ResourceBox::Get()->getIntValue("NameKeyLength");
    string stNameKey;

    map<string, map<string,int> > candsBoxes;
    map<string, map<string,int> >::iterator boxesIter;
    map<string,int>::iterator candsIter;
    set<PhraseHitCount,PHCComp> sortedCands;
    set<PhraseHitCount,PHCComp>::const_iterator scIter;
    
    vector<int> vTrueAccept(iIRLoops+1,0);
    vector<int> vFalseAccept(iIRLoops+1,0);
    vector<int> * vActiveVec = NULL;
    
    FILE * fLRESummary = open_file(stLRESummary,"LRESummary","w");
    
    fprintf(stderr,"LRESummary: #Anonyms=%d, #Searches=%d, #Loops=%d\n",iAnonymsNum,iTotalSearches,iIRLoops);
    fflush(stderr);

    LinesReader lrScriptReader(stIRScript,512);
    string stLine;
    fprintf(stderr,"Load Anonyms\n");
    fflush(stderr);
    while (lrScriptReader.bMoreToRead())
    {
        lrScriptReader.voGetLine(stLine);

        //stLine = stLine.substr(iStartFrom,iNameKeyLen);
        if (candsBoxes.find(stLine) != candsBoxes.end()) abort_err(string("Duplicated entries for ")+stLine);
        candsBoxes.insert(map<string, map<string,int> >::value_type(stLine,map<string,int>()));
    }

    TdhReader trLogReader(stLogFile,512);
    vector<string> vFieldsVec(10,"");
    string stBestCand(75,'\0');
    string stDecision(10,'\0');
    
    int i=0;
    int iConfLevel;
    fprintf(stderr,"Read Log File\n");
    fflush(stderr);
    while (trLogReader.bMoreToRead())
    {
        i++;
        trLogReader.iGetLine(vFieldsVec);
        if (vFieldsVec.at(0).find("#Total LRE Time:") != string::npos) break;

        boxesIter = candsBoxes.find(vFieldsVec.at(0)); //.substr(iStartFrom,iNameKeyLen));
        if (boxesIter == candsBoxes.end()) abort_err(string("No entry for ")+vFieldsVec.at(0)+" in cands box");

        stNameKey=vFieldsVec.at(1).substr(iStartFrom,iNameKeyLen);
        candsIter = boxesIter->second.find(stNameKey);
        if (candsIter == boxesIter->second.end()) boxesIter->second.insert(map<string,int>::value_type(stNameKey,1));
        else candsIter->second++;
    }
    fprintf(stderr,"Log File: #Lines=%d\n",i);
    fflush(stderr);
    
    int iCallsNumber=0;
    lrScriptReader.restart();
    fprintf(stderr,"Write Summary\n");
    fflush(stderr);
    while (lrScriptReader.bMoreToRead())
    {
        lrScriptReader.voGetLine(stLine);
        //stLine=stLine.substr(iStartFrom,iNameKeyLen);
        iCallsNumber++;
        GetBestCand(stLine,candsBoxes,stBestCand,iConfLevel);
        
		if (stLine.substr(iStartFrom,iWriterIdLength).compare(stBestCand.substr(iStartFrom,iWriterIdLength)) == 0)
        {
            stDecision="Yes";
            vActiveVec=&vTrueAccept;
        }
        else
        {
            stDecision="No";
            vActiveVec=&vFalseAccept;
        }
        
        fprintf(fLRESummary,"%s\t%s\t%s\t%d\t{",stLine.c_str(),stBestCand.c_str(),stDecision.c_str(),iConfLevel);
        fflush(fLRESummary);

        // Print successors.
        if ((boxesIter=candsBoxes.find(stLine))==candsBoxes.end()) abort_err(string("Couldn't find entry for ")+stLine+"?!?!?");
        candsIter=boxesIter->second.begin();
        int iItems=0;
        sortedCands.clear();
        while ((iItems < 5) && (candsIter != boxesIter->second.end()))
        {
            if (candsIter->first.compare(stBestCand)==0)
            {
                candsIter++;
                continue;
            }

            sortedCands.insert(PhraseHitCount(candsIter->first,candsIter->second));
            iItems++;
            candsIter++;
        }

        for (iItems=0,scIter=sortedCands.begin();scIter!=sortedCands.end();scIter++,iItems++)
        {
            if (iItems>0) fprintf(fLRESummary,", ");
            fprintf(fLRESummary,"(%s, %d)",scIter->stPhrase.c_str(),scIter->iHitCount);
            fflush(fLRESummary);
        }
        fprintf(fLRESummary,"}\n");
        fflush(fLRESummary);
        
        for (i=0;i<=iConfLevel;i++) (*vActiveVec).at(i)++;
    }
    
    fflush(fLRESummary);
    fclose(fLRESummary);

    fprintf(stderr,"Write ROC\n");
    fflush(stderr);
    PrintIRAccuracyCurve(vTrueAccept,vFalseAccept,iIRLoops,iCallsNumber,ResourceBox::Get()->getStringValue("LREAccuracyCurve"));
}

//___________________________________________________________________
void LREClasifySummary()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stLogFile = ResourceBox::Get()->getStringValue("IRQueryLog");
    const string stLRESummary = ResourceBox::Get()->getStringValue("LRESummary");
    const int iMaxAnonymsNum = ScriptSize(stSampleFolder+"anonyms.txt");
    const int iIRLoops = ResourceBox::Get()->getIntValue("IRLoops");

    map<string, map<string,int,LexComp>, LexComp> candsBoxes;
    map<string, map<string,int,LexComp>, LexComp>::iterator cbIter;
    map<string,int,LexComp>::iterator gIter;
    
    string stAnonym;
    string stAuthor;
    string stDecision;
    int iBestScore;
    TdhReader trResFile(stLogFile,512);
    vector<string> vFieldsVec(10,"");
    int iFields,i;
    int iLines=0;

    vector<int> vTrueAccept(iIRLoops+1,0);
    vector<int> vFalseAccept(iIRLoops+1,0);
    vector<int> * vActiveVec = NULL;

    fprintf(stderr,"LREClasifySummary: #Anonyms=%d, #Loops=%d\n",iMaxAnonymsNum,iIRLoops);
    fflush(stderr);

    candsBoxes.clear();
    while (trResFile.bMoreToRead())
    {
        iLines++;
        if ((iFields = trResFile.iGetLine(vFieldsVec)) != 4) abort_err(stringOf(iFields)+" fields (4 required) in line "+stringOf(iLines));

        stAnonym = vFieldsVec.at(0)+"\t"+vFieldsVec.at(1);
        stAuthor = vFieldsVec.at(2);

        cbIter=candsBoxes.find(stAnonym);
        if (cbIter == candsBoxes.end())
        {
            candsBoxes.insert(map<string, map<string,int,LexComp>, LexComp>::value_type(stAnonym,map<string,int,LexComp>()));
            cbIter = candsBoxes.find(stAnonym);
        }

        gIter=cbIter->second.find(stAuthor);
        if (gIter == cbIter->second.end())
            cbIter->second.insert(map<string,int,LexComp>::value_type(stAuthor,1));
        else
        {
            gIter->second++;
        }
    }

    map<string,int,LexComp>::const_iterator candsIter;

    map<string,string,LexComp> catsList;
    map<string,string,LexComp>::const_iterator clIter;
    LoadCategoryList(catsList,NULL,NULL,"CategoryList");

    map<string,int,LexComp> candsFreq;
    map<string,int,LexComp>::const_iterator cfIter;

    int iCallsNumber=0;
    size_t iPos;
    FILE * fResFile = open_file(stLRESummary,"LREClasifySummary","w");
    fprintf(fResFile,"Real Class\t");
    for (clIter=catsList.begin();clIter!=catsList.end();clIter++) fprintf(fResFile,"%s\t",clIter->first.c_str());
    fprintf(fResFile,"Classified As\tFull Key\n");
    fflush(fResFile);
    for (cbIter=candsBoxes.begin();cbIter!=candsBoxes.end();cbIter++)
    {
        iCallsNumber++;
        candsFreq.clear();
        for (candsIter=cbIter->second.begin();candsIter!=cbIter->second.end();candsIter++)
        {
            candsFreq.insert(map<string,int,LexComp>::value_type(candsIter->first,candsIter->second));
        }

        for (clIter=catsList.begin();clIter!=catsList.end();clIter++)
        {
            if (candsFreq.find(clIter->first) == candsFreq.end()) candsFreq.insert(map<string,int,LexComp>::value_type(clIter->first,0));
        }

        if ((iPos=cbIter->first.find('\t')) == string::npos) abort_err(string("Illegal cand key: ")+cbIter->first);
        stAnonym=cbIter->first.substr(0,iPos);

        fprintf(fResFile,"%s\t",stAnonym.c_str());
        iBestScore=-1;
        stAuthor="";
        for (cfIter=candsFreq.begin();cfIter!=candsFreq.end();cfIter++)
        {
            fprintf(fResFile,"%d\t",cfIter->second);

            if (cfIter->second > iBestScore)
            {
                iBestScore=cfIter->second;
                stAuthor=cfIter->first;
            }
        }
        fprintf(fResFile,"%s\t%s\n",stAuthor.c_str(),cbIter->first.c_str());
        fflush(fResFile);

        if (stAnonym.compare(stAuthor) == 0)
        {
            stDecision="Yes";
            vActiveVec=&vTrueAccept;
        }
        else
        {
            stDecision="No";
            vActiveVec=&vFalseAccept;
        }

        for (i=0;i<=iBestScore;i++) (*vActiveVec).at(i)++;
    }

    fflush(fResFile);
    fclose(fResFile);

    PrintIRAccuracyCurve(vTrueAccept,vFalseAccept,iIRLoops,iCallsNumber,ResourceBox::Get()->getStringValue("LREAccuracyCurve"));
    SummaryResults(stLRESummary,catsList);
}

//___________________________________________________________________
void IRSummary()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stLogFile = ResourceBox::Get()->getStringValue("IRQueryLog");
    const string stIRScript = stSampleFolder+"anonyms.txt";
    const int iMaxAnonymsNum = ScriptSize(stSampleFolder+"anonyms.txt");
    const int iIRLoops = ResourceBox::Get()->getIntValue("IRLoops");

    vector<int> vTrueAccept(iIRLoops+1,0);
    vector<int> vFalseAccept(iIRLoops+1,0);
    vector<int> * vActiveVec = NULL;

    fprintf(stderr,"LRESummary: #Anonyms=%d, #Loops=%d\n",iMaxAnonymsNum,iIRLoops);
    fflush(stderr);

    TdhReader trLogReader(stLogFile,512);
    vector<string> vFieldsVec(4,"");

    int iCallsNumber=0;
    int iConfLevel,i;
    int iTrueAccept=0;
    string stDecision;
    while (trLogReader.bMoreToRead())
    {
        if (trLogReader.iGetLine(vFieldsVec) != 4) abort_err(string("Illegal res line: ")+vFieldsVec.at(0));
        iCallsNumber++;

        if (vFieldsVec.at(0).compare(vFieldsVec.at(1)) == 0)
        {
            stDecision="Yes";
            vActiveVec=&vTrueAccept;
            iTrueAccept++;
        }
        else
        {
            stDecision="No";
            vActiveVec=&vFalseAccept;
        }

        if (!string_as<int>(iConfLevel,vFieldsVec.at(2),std::dec)) abort_err(string("Illegal ConfLevel: ")+vFieldsVec.at(2));
        for (i=0;i<=iConfLevel;i++) (*vActiveVec).at(i)++;
    }

    PrintIRAccuracyCurve(vTrueAccept,vFalseAccept,iIRLoops,iCallsNumber,ResourceBox::Get()->getStringValue("LREAccuracyCurve"));
    fprintf(stderr,"Recall: %.1f\n",((double)(iTrueAccept)/(double)iCallsNumber)*100.0);
    fflush(stderr);
}
