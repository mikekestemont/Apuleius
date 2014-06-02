#include <MLAuxFuncs.h>
#include <ResourceBox.h>

using namespace std;

//___________________________________________________________________
void DocToVec(const string & stFileName,
              const int iFeatureSetSize,
              FeaturesHandler * featureSet,
              vector<double> & vFtrsVec,
              FILE * debug_log)
{
    int i;
    list<FeatureItem> lTextProfile;
    set<string,LexComp> dummyMarker;
    int iPrevIndex=-1;
    const FeatureItem * item=NULL;
    
    for (i=1;i<=iFeatureSetSize;i++) vFtrsVec.at(i)=0.0;

    featureSet->DocToVec(stFileName,&lTextProfile,dummyMarker,debug_log);
    
    while (!lTextProfile.empty())
    {
        item = &lTextProfile.front();
        
        if (iPrevIndex >= item->index)
        {
            fprintf(stderr,"Illegal list order: %d -> %d\n",iPrevIndex,item->index);
            fflush(stderr);
            exit(0);
        }
        
        vFtrsVec.at(item->index)=item->weight;
        lTextProfile.pop_front();
    }
}

//___________________________________________________________________
void RandomTrainSet(const string & stTrainFile,
                    const string & stRandomFile,
                    const int iTrainVecsNumber)
{
    LinesReader fileReader(stTrainFile,1024);
    FILE * fRandomFile = open_file(stRandomFile,"RandomTrainSet","w");
    int iSamplesNumber=0;
    vector<string> vLinesBuffer(iTrainVecsNumber+1,"");
    string stLine;
    int i;

    while (fileReader.bMoreToRead())
    {
        fileReader.voGetLine(stLine);
        vLinesBuffer.at(iSamplesNumber++) = stLine;
    }

    vector<bool> vMarker(iSamplesNumber,false);
    int iSetLines=0;
    int iLoopsCount=0;
    double dRandom;
    int iLineIndex;

    while ((iLoopsCount < 3*iSamplesNumber) && (iSetLines < iSamplesNumber))
    {
        iLoopsCount++;
        dRandom = get_uniform01_random();
        iLineIndex = (int)(dRandom*(iSamplesNumber-1));
        if (vMarker.at(iLineIndex))
            continue;
        vMarker.at(iLineIndex) = true;
        iSetLines++;

        fprintf(fRandomFile,"%s\n",vLinesBuffer.at(iLineIndex).c_str());
        fflush(fRandomFile);
    }

    for (i=0;i<iSamplesNumber;i++)
    {
        if (vMarker.at(i)) continue;
        fprintf(fRandomFile,"%s\n",vLinesBuffer.at(i).c_str());
        fflush(fRandomFile);
    }

    fflush(fRandomFile);
    fclose(fRandomFile);
}

//___________________________________________________________________
void MultiCatToBinary(const int argc,char * argv[])
{
    if (argc < 6) abort_err(string("argc=")+stringOf(argc)+" in MultiCatToBinary");

    fprintf(stderr,"\nMultiCatToBinary - Start (%s, %s, %s)\n",argv[3],argv[4], argv[5]);
    fflush(stderr);

    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stWorkFolder = stSampleFolder + argv[3] + "\\";
    const string stRightClass=argv[4];

    const string stTrainFile = stWorkFolder + ResourceBox::Get()->getStringValue("TrainVecsFile");
    LinesReader lrTrainSet(stTrainFile,1024);
    string stLine;
    string stActulaClass;
    size_t iPos;

    const string stVsAll = ResourceBox::Get()->getStringValue("OneVsAllGenName");
    FILE * fOneVsAll = open_file(stWorkFolder+argv[5]+"_"+stVsAll,"MultiCatToBinary","w");
    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"MultiCatToBinary","w");

    fprintf(stderr,"Scan Training Sets: %s\n",stTrainFile.c_str());
    fflush(stderr);
    while (lrTrainSet.bMoreToRead())
    {
        lrTrainSet.voGetLine(stLine);

        if ((iPos=stLine.find_first_of(' '))==string::npos) abort_err(string("Invalid vec: ")+stLine.substr(0,25));

        stActulaClass = stLine.substr(0,iPos);

        if (stActulaClass.compare(stRightClass) == 0)
            fprintf(fOneVsAll,"1 %s\n",stLine.substr(iPos+1).c_str());
        else
            fprintf(fOneVsAll,"-1 %s\n",stLine.substr(iPos+1).c_str());
        fflush(fOneVsAll);
    }

    fflush(debug_log);
    fclose(debug_log);
    fflush(fOneVsAll);
    fclose(fOneVsAll);

    fprintf(stderr,"MultiCatToBinary - End\n");
    fflush(stderr);
}

//___________________________________________________________________
void OneVsAllResults(const int argc,char * argv[])
{
    if (argc < 4) abort_err(string("argc=")+stringOf(argc)+" in OneVsAllResults");

    fprintf(stderr,"\nOneVsAllResults - Start\n");
    fflush(stderr);

    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stResultsFolder = ResourceBox::Get()->getStringValue("LogsFolder");
    const string stResFile = ResourceBox::Get()->getStringValue("SvmResFile");
    const string stWorkFolder = stSampleFolder + argv[3] + "\\";

    const string stTrainFile = stWorkFolder + ResourceBox::Get()->getStringValue("TrainVecsFile");
    LinesReader lrTrainSet(stTrainFile,1024);
    LinesReader * lrCurrFile=NULL;
    string stLine;
    size_t iPos;

    map<int,string> indToCat;
    map<int,string>::const_iterator itcIter;
    map<string,string,LexComp> catsList;
    map<string,string,LexComp>::const_iterator clIter;
    LoadCategoryList(catsList,&indToCat,NULL,"CategoryList");

    const int iClassesNumber=catsList.size();
    map< string, LinesReader* > oneVsAllRes;
    map< string, LinesReader* >::iterator ovaIter;
    double dCurrScore,dBestScore;
    string stBestClass;
    string stRightClass;
    int iRightClass;
    int i=0;
    for (clIter=catsList.begin();clIter!=catsList.end();clIter++)
    {
        oneVsAllRes.insert(map< string, LinesReader* >::value_type(clIter->first,new LinesReader(stResultsFolder+stResFile+"_"+clIter->first+".txt",512)));
    }

    const string stSummaryFile = ResourceBox::Get()->getStringValue("SummaryFile");
    FILE * fResFile = open_file(stResultsFolder+stSummaryFile,"OneVsAllResults","w");
    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"OneVsAllResults","w");

    fprintf(fResFile,"Real Class\t");
    for (clIter=catsList.begin();clIter!=catsList.end();clIter++) fprintf(fResFile,"%s\t",clIter->first.c_str());
    fprintf(fResFile,"Classified As\n");
    fflush(fResFile);

    fprintf(stderr,"Scan Vectors Set: %s\n",stTrainFile.c_str());
    fflush(stderr);
    while (lrTrainSet.bMoreToRead())
    {
        lrTrainSet.voGetLine(stLine);

        if ((iPos=stLine.find_first_of(' '))==string::npos) abort_err(string("Invalid vec: ")+stLine.substr(0,25).c_str());

        stRightClass = stLine.substr(0,iPos);
        if (!string_as<int>(iRightClass,stRightClass,std::dec)) abort_err(string("Non index: ")+stRightClass.c_str());
        if ((itcIter = indToCat.find(iRightClass)) == indToCat.end()) abort_err(string("No class for: ")+stringOf(iRightClass).c_str());

        fprintf(fResFile,"%s\t",itcIter->second.c_str());
        dBestScore=-1000000.0;
        stBestClass="";
        for (ovaIter=oneVsAllRes.begin();ovaIter!=oneVsAllRes.end();ovaIter++)
        {
            lrCurrFile = ovaIter->second;
            if (!lrCurrFile->bMoreToRead()) abort_err(string("Premature res file end: ")+lrCurrFile->stGetFileName());
            lrCurrFile->voGetLine(stLine);
            if (!string_as<double>(dCurrScore,stLine,std::dec)) abort_err(string("Illegal SVM Res Line: ")+stLine+", "+ovaIter->first);
            fprintf(fResFile,"%.5f\t",dCurrScore);

            if (dCurrScore > dBestScore)
            {
                dBestScore=dCurrScore;
                stBestClass=ovaIter->first;
            }
        }
        fprintf(fResFile,"%s\n",stBestClass.c_str());
        fflush(fResFile);
    }

    fflush(debug_log);
    fclose(debug_log);
    fflush(fResFile);
    fclose(fResFile);

    SummaryResults(stResultsFolder+stSummaryFile,catsList);

    for (ovaIter=oneVsAllRes.begin();ovaIter!=oneVsAllRes.end();ovaIter++)
    {
        lrCurrFile = ovaIter->second;
        if (lrCurrFile->bMoreToRead()) abort_err(string("Res file hasn't been compeletly read: ")+lrCurrFile->stGetFileName());
        delete lrCurrFile;
    }

    fprintf(stderr,"\n\nOneVsAllResults - End\n");
    fflush(stderr);
}

//___________________________________________________________________
void SummaryResults(const string & stSummaryFile,
                    const map<string,string,LexComp> & catsList)
{
    fprintf(stderr,"\nSummaryResults - Start(#Classes=%d)\n",catsList.size());
    fflush(stderr);

    const int iClassesNumber = catsList.size();
    TdhReader trSummary(stSummaryFile,512);
    vector<string> vFieldsVec(iClassesNumber+10,"");
    vector<int> vConfMat((iClassesNumber+1)*(iClassesNumber+1),0);

    map<string,int,LexComp>::const_iterator it1;
    map<string,int,LexComp>::const_iterator it2;
    map<string,string,LexComp>::const_iterator iter;

    map<string,int,LexComp> indexMap;

    int iRightClass,iBestClass,i;

    i=1;
    for (iter=catsList.begin();iter!=catsList.end();iter++) indexMap.insert(map<string,int,LexComp>::value_type(iter->first,i++));

    int iTotalSamples=0;
    int iCorrectClassify=0;

    // Skip Header
    trSummary.iGetLine(vFieldsVec);
    while (trSummary.bMoreToRead())
    {
        if ((i=trSummary.iGetLine(vFieldsVec)) < (iClassesNumber+2)) abort_err(string("Illegal #Fields (")+stringOf(i)+") in "+stSummaryFile);
        iTotalSamples++;

        if ((it1=indexMap.find(vFieldsVec.at(0))) == indexMap.end()) abort_err(string("Class [")+vFieldsVec.at(0)+"] not found");
        iRightClass = it1->second;

        if ((it1=indexMap.find(vFieldsVec.at(iClassesNumber+1))) == indexMap.end()) abort_err(string("Class [")+vFieldsVec.at(iClassesNumber+1)+"] not found");
        iBestClass = it1->second;

        if (iRightClass == iBestClass) iCorrectClassify++;

        vConfMat.at(iRightClass*iClassesNumber+iBestClass)++;
    }

    fprintf(stderr,"\n\n#Total Samples: %d\n",iTotalSamples);
    fprintf(stderr,"#Correct Class: %d (%.3f)\n",iCorrectClassify,(double)iCorrectClassify/(double)iTotalSamples);
    fprintf(stderr,"\nConfusion Matrix:\nRight\\Actual");
    for (it1=indexMap.begin();it1!=indexMap.end();it1++)
    {
        fprintf(stderr,"\t%s",it1->first.c_str());
    }
    fprintf(stderr,"\n");
    for (it1=indexMap.begin();it1!=indexMap.end();it1++)
    {
        fprintf(stderr,"%s",it1->first.c_str());
        for (it2=indexMap.begin();it2!=indexMap.end();it2++)
        {
            fprintf(stderr,"\t%d",vConfMat.at(iClassesNumber*it1->second+it2->second));
        }
        fprintf(stderr,"\n");
    }
    fprintf(stderr,"SummaryResults - End\n");
    fflush(stderr);
}
