/*
  Definition of various functions for handling features.
*/
#include <DataFuncs.h>

using namespace std;

// Load a feature set from a set of information vectors.
//___________________________________________________________________
static void LoadFeatureSet(map<string,FeatureInfo,LexComp> & w2fLex,
                           map<int,string>                 * i2fLex,
                           const vector<string> & vTermsVec,
                           const vector<string> & vWeightsVec,
                           const vector<string> & vTotalFreq,
                           const vector<string> & vDocsFreq,
                           const vector<string> & vFeatureTypeVec,
                           const int iFeaturesNumber)
{
    FeatureInfo item;
    int i;
    
    w2fLex.clear();
    if (i2fLex != NULL) i2fLex->clear();
    for (i=0;i<iFeaturesNumber;i++)
    {
        item.stTerm = vTermsVec.at(i);
        if (!string_as<int>(item.iTotalFreq,vTotalFreq.at(i),std::dec)) abort_err(string("#Total: ")+vTermsVec.at(i)+", "+vTotalFreq.at(i));
        if (!string_as<int>(item.iDocsFreq,vDocsFreq.at(i),std::dec)) abort_err(string("#Docs: ")+vTermsVec.at(i)+", "+vDocsFreq.at(i));
        if (!string_as<double>(item.dFreq,vWeightsVec.at(i),std::dec)) abort_err(string("Freq Val: ")+vTermsVec.at(i)+", "+vWeightsVec.at(i));
		item.featureType = strToFeatureType(vFeatureTypeVec.at(i));
        item.index = i+1;
        
        if (w2fLex.find(vTermsVec.at(i)) == w2fLex.end()) w2fLex.insert(map<string,FeatureInfo,LexComp>::value_type(item.stTerm,item));
        else abort_err(string("A feature appears twice in Feature Set: ")+vTermsVec.at(i));
        
        if (i2fLex == NULL) continue;
        if (i2fLex->find(item.index) == i2fLex->end()) i2fLex->insert(map<int,string>::value_type(item.index,item.stTerm));
        else abort_err(string("A feature appears twice in Feature Set: ")+vTermsVec.at(i));
    }
}

// Load a features set from a vector pool.
//___________________________________________________________________
void LoadFeatureSet(map<string,FeatureInfo,LexComp> & w2fLex,
                    map<int,string>                 * i2fLex,
                    const vector<string> & vFtrsPool,
                    const int iFeatureSetSize)
{
    vector<string> vTermsVec(iFeatureSetSize,"");
    vector<string> vTotalFreq(iFeatureSetSize,"");
    vector<string> vDocsFreq(iFeatureSetSize,"");
    vector<string> vWeightsVec(iFeatureSetSize,"");
    vector<string> vFeatureType(iFeatureSetSize,"");
    int i;
    size_t iPos;
    string stLine;
    
    for (i=0;i<iFeatureSetSize;i++)
    {
        stLine = vFtrsPool.at(i);
        
        if ((iPos = stLine.find_first_of('\t')) == string::npos) abort_err(string("Illegal feature line: ")+vFtrsPool.at(i));
        vTermsVec.at(i) = stLine.substr(0,iPos); stLine = stLine.substr(iPos+1);

        if ((iPos = stLine.find_first_of('\t')) == string::npos) abort_err(string("Illegal feature line: ")+vFtrsPool.at(i));
        vWeightsVec.at(i) = stLine.substr(0,iPos); stLine = stLine.substr(iPos+1);

        if ((iPos = stLine.find_first_of('\t')) == string::npos) abort_err(string("Illegal feature line: ")+vFtrsPool.at(i));
        vTotalFreq.at(i) = stLine.substr(0,iPos);  stLine = stLine.substr(iPos+1);

        if ((iPos = stLine.find_first_of('\t')) == string::npos) abort_err(string("Illegal feature line: ")+vFtrsPool.at(i));
        vDocsFreq.at(i) = stLine.substr(0,iPos);
		vFeatureType.at(i) = stLine.substr(iPos+1);
    }
    
    LoadFeatureSet(w2fLex,i2fLex,vTermsVec,vWeightsVec,vTotalFreq,vDocsFreq,vFeatureType,iFeatureSetSize);
}

// Load a featurese set from a features file.
//___________________________________________________________________
void LoadFeatureSet(map<string,FeatureInfo,LexComp> & w2fLex,
                    map<int,string>                 * i2fLex,
                    const string & stFeatureSet,
                    FILE * debug_log)
{
    TdhReader fsReader(stFeatureSet,1024);
    vector<string> vFields(5,"");
    int iFields;
    FeatureInfo item;
    int iFeatureIndex=0;
    const int iFeatureSetSize = ResourceBox::Get()->getIntValue("FeatureSetSize");
    
    if (debug_log != NULL) fprintf(debug_log,"\n\nLoadFeatureSet - Start (fs=%d, file=%s)\n",iFeatureSetSize,stFeatureSet.c_str());
    if (debug_log != NULL) fflush(debug_log);
    
    vector<string> vTermsVec(iFeatureSetSize,"");
    vector<string> vTotalFreq(iFeatureSetSize,"");
    vector<string> vDocsFreq(iFeatureSetSize,"");
    vector<string> vWeightsVec(iFeatureSetSize,"");
    vector<string> vFeatureTypeVec(iFeatureSetSize,"");
    
    // Skip Header
    fsReader.iGetLine(vFields);
    while (fsReader.bMoreToRead() && (iFeatureIndex < iFeatureSetSize))
    {
        if ((iFields=fsReader.iGetLine(vFields)) != 5) abort_err(string("#Fields=")+stringOf(iFields)+" in feature set "+stFeatureSet);
        
        vTermsVec.at(iFeatureIndex) = vFields.at(0);
        vTotalFreq.at(iFeatureIndex) = vFields.at(2);
        vDocsFreq.at(iFeatureIndex) = vFields.at(3);
        vWeightsVec.at(iFeatureIndex) = vFields.at(1);
        vFeatureTypeVec.at(iFeatureIndex) = vFields.at(4);

        iFeatureIndex++;
    }
    
    LoadFeatureSet(w2fLex,i2fLex,vTermsVec,vWeightsVec,vTotalFreq,vDocsFreq,vFeatureTypeVec,iFeatureIndex);

    if (debug_log != NULL) fprintf(debug_log,"LoadFeatureSet - End (fs=%d)\n\n",iFeatureIndex);
    if (debug_log != NULL) fflush(debug_log);
}

// Print a feature set.
//___________________________________________________________________
int PrintFeatureSet(const string & stFeatureSet,
                    const map<string,FeatureInfo,LexComp> & lexicon,
                    const int iWordsNumber,
                    const int iDocsNumber,
                    FILE * debug_log)
{
    fprintf(stderr,"PrintWordsFreq - Start (#Words=%d, #Docs=%d)\n",iWordsNumber,iDocsNumber);
    fflush(stderr);

    const int iDocsFreqFloor = ResourceBox::Get()->getIntValue("FeatureDocsFloor");
	const int iLetterFreqFloor = ResourceBox::Get()->getIntValue("LetterNgramFreqFloor");
	const int iWordFreqFloor = ResourceBox::Get()->getIntValue("WordNgramFreqFloor");
    const int iFeatureSetSize = ResourceBox::Get()->getIntValue("FeatureSetSize");
    const double dLambda = ResourceBox::Get()->getDoubleValue("Lambda");
    const bool bNormByDocs = (ResourceBox::Get()->getIntValue("NormFreqByDocs") > 0);
    const int * iFreqNorm = (bNormByDocs?&iDocsNumber:&iWordsNumber);
    const int * iFreqCounter = NULL;
    set<FeatureInfo,CompFIByFreq> wordsFreq;
    set<FeatureInfo,CompFIByFreq>::iterator wfIter;
    map<string,FeatureInfo,LexComp>::const_iterator wcIter;
    const double dTotalLog = log_base2((double)(lexicon.size()*dLambda + *iFreqNorm));
    double dFreq;
    FeatureInfo item;
    
    fprintf(stderr,"Scan Words Counter - Start (#Words=%d, #Docs=%d)\n",iWordsNumber,iDocsNumber);
    fflush(stderr);
    for (wcIter=lexicon.begin();wcIter!=lexicon.end();wcIter++)
    {
		if (wcIter->second.iDocsFreq < iDocsFreqFloor) continue;
		if (wcIter->second.featureType == LetterNgram)
		{
			if (wcIter->second.iTotalFreq < iLetterFreqFloor) continue;
		}
		else
		{
			if (wcIter->second.iTotalFreq < iWordFreqFloor) continue;
		}
        iFreqCounter = (bNormByDocs?&(wcIter->second.iDocsFreq):&(wcIter->second.iTotalFreq));
        dFreq = log_base2((double)*iFreqCounter + dLambda);
        dFreq -= dTotalLog;
        item=wcIter->second;
        item.dFreq=dFreq;
        wordsFreq.insert(item);
    }
    fprintf(stderr,"Scan Words Counter - End\n");
    fflush(stderr);

    FILE * fWordsFreq = open_file(stFeatureSet,"PrintFeatureSet","w",true);

    fprintf(fWordsFreq,"Term\tFreq\tTotal Freq\tDocs Freq\n");
    fflush(fWordsFreq);
    int iItems=0;
    wfIter = wordsFreq.begin();
    double dTotalFreq = 0;
    while ((wfIter != wordsFreq.end()) && (iItems < iFeatureSetSize))
    {
		fprintf(fWordsFreq,"%s\t%.5f\t%d\t%d\t%s\n",wfIter->stTerm.c_str(),wfIter->dFreq,wfIter->iTotalFreq,wfIter->iDocsFreq,featureTypeToStr(wfIter->featureType));
        fflush(fWordsFreq);
        iItems++;
        wfIter++;
        dTotalFreq += pow(2.0,wfIter->dFreq);
    }
    fflush(fWordsFreq);
    fclose(fWordsFreq);

    fprintf(stderr,"PrintWordsFreq - End (Total Freq=%.5f)\n",dTotalFreq);
    fflush(stderr);
    
    return iItems;
}

// Print a feature set.
//___________________________________________________________________
int PrintFeatureSet(const string & stFeatureSet,
                    const map<string,FeatureInfo,LexComp> & lexicon,
                    FILE * debug_log)
{
    map<string,FeatureInfo,LexComp>::const_iterator iter;
    const int iFreqFloor = ResourceBox::Get()->getIntValue("FeatureDocsFloor");
    int iCount;
    int iLines=0;
    
    FILE * fFeaturesFile = open_file(stFeatureSet,"PrintFeatureSet","w",true);

    fprintf(fFeaturesFile,"Term\tFreq\t#Total\t#Docs\n");
    fflush(fFeaturesFile);
    for (iter=lexicon.begin();iter!=lexicon.end();iter++)
    {
        iCount = iter->second.iDocsFreq;
        if (iCount < iFreqFloor) continue;
		fprintf(fFeaturesFile,"%s\t0\t%d\t%d\n",iter->second.stTerm.c_str(),iter->second.iTotalFreq,iter->second.iDocsFreq,featureTypeToStr(iter->second.featureType));
        fflush(fFeaturesFile);
        iLines++;
    }
    
    fflush(fFeaturesFile);
    fclose(fFeaturesFile);
    
    return iLines;
}

//___________________________________________________________________
void LoadMeanVarData(const string & stFileName,
                     const map<string,FeatureInfo,LexComp> & w2fLex,
                     vector<double> & vMeanVec,
                     vector<double> & vVarVec)
{
    TdhReader trFile(stFileName,512);
    vector<string> vFields(3,"");
    map<string,FeatureInfo,LexComp>::const_iterator iter;
    int i;
    double w;
    
    // Skip Header.
    trFile.iGetLine(vFields);
    while (trFile.bMoreToRead())
    {
        if ((i=trFile.iGetLine(vFields)) != 3) {fprintf(stderr,"#Fields=%d in Mean-Var file (%s)\n",i,stFileName.c_str()); fflush(stderr); exit(0);}
        
        iter=w2fLex.find(vFields.at(0));
        if (iter == w2fLex.end()) {fprintf(stderr,"The word [%s] not found in lexicon\n",vFields.at(0).c_str()); fflush(stderr); exit(0);}
        
        if (!string_as<double>(w,vFields.at(1),std::dec)) {fprintf(stderr,"Freq Val: %s, %s\n",vFields.at(0).c_str(),vFields.at(1).c_str()); fflush(stderr); exit(0);}
        vMeanVec.at(iter->second.index) = w;
        if (!string_as<double>(w,vFields.at(2),std::dec)) {fprintf(stderr,"Freq Val: %s, %s\n",vFields.at(0).c_str(),vFields.at(2).c_str()); fflush(stderr); exit(0);}
        vVarVec.at(iter->second.index) = w;
    }
}
                    

//___________________________________________________________________
void ReadFeaturesVec(FileScanner & fsTrainSet,
                     vector<double> & vFeaturesVec,
                     int & iRightClass,
                     const int iFeatureSetSize,
                     string & stInfo)
{
    char letter;
    string stToken(75,'\0');
    int index=-1;
    size_t pos;
    double dWeight;
    
    for (index=0;index<=iFeatureSetSize;index++) vFeaturesVec.at(index) = 0.0;
    index=-1;
    iRightClass=-1;
    stToken="";
    stInfo="";
    while (fsTrainSet.bMoreToRead())
    {
        letter = fsTrainSet.cGetNextByte();
        
        if (letter == '\n') break;
        
        if (letter == '#')
        {
            while (fsTrainSet.bMoreToRead())
            {
                letter = fsTrainSet.cGetNextByte();
                if (letter == '\n') break;
                stInfo += letter;
            }
            break;
        }
        
        if  ((letter == ' ') && stToken.empty()) continue;
        
        if (letter == ' ')
        {
            if (index < 0)
            {
                index=0;
                if (!string_as<int>(iRightClass,stToken,std::dec)) {fprintf(stderr,"RC Val: %s\n",stToken.c_str()); fflush(stderr); exit(0);}
            }
            else
            {
                pos = stToken.find_first_of(':');
                if (pos == string::npos)
                {
                    fprintf(stderr,"Illegal feature entry in GetTrainVec: %s\n",stToken.c_str());
                    fflush(stderr);
                    exit(0);
                }
                if (!string_as<int>(index,stToken.substr(0,pos),std::dec)) {fprintf(stderr,"Ind Val: %s\n",stToken.substr(0,pos).c_str()); fflush(stderr); exit(0);}
                if (!string_as<double>(dWeight,stToken.substr(pos+1),std::dec)) {fprintf(stderr,"Weight Val: %s\n",stToken.substr(pos+1).c_str()); fflush(stderr); exit(0);}
                vFeaturesVec.at(index) = dWeight;
            }
            
            stToken="";
        }
        else
        {
            stToken+=letter;
        }
    }
}

//___________________________________________________________________
void PrintFeaturesVec(FILE * fTrainVecs,
                      const vector<double> & vFtrsVec,
                      const string & stTargetVal,
                      const string & stInfo,
                      const int iFeatureSetSize)
{
    int i;
    string stWhiteSpace="";
    const double dMinValidWeight = ResourceBox::Get()->getDoubleValue("FeatureMinValidWeight");
    
    if (!stTargetVal.empty())
    {
        fprintf(fTrainVecs,"%s",stTargetVal.c_str());
        fflush(fTrainVecs);
        stWhiteSpace=" ";
    }
    
    for (i=1;i<=iFeatureSetSize;i++)
    {
        if (fabs(vFtrsVec.at(i)) <= dMinValidWeight) continue;
        fprintf(fTrainVecs,"%s%d:%.15f",stWhiteSpace.c_str(),i,vFtrsVec.at(i));
        fflush(fTrainVecs);
        stWhiteSpace = " ";
    }

    if (!stInfo.empty())
    {
        fprintf(fTrainVecs," # %s\n",stInfo.c_str());
        fflush(fTrainVecs);
    }
}
