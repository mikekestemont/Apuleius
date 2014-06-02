#include <ResourceBox.h>
#include <auxiliary.h>
#include <FileScanner.h>
#include <FolderScanner.h>
#include <stdio.h>
#include <set>
#include <PairClassifier.h>

using namespace std;

//___________________________________________________________________
static double DummyDecoySim(const string & stFileName, const int iFirst, const int iSize)
{
    return 0.0;
}

//___________________________________________________________________
class PairItem
{
    public:
        
        string stAnonym;
        string stAuthor;
        double dDecoyWinSim;
        
        PairItem(){}
        void operator=(const PairItem & item)
        {
            stAnonym=item.stAnonym;
            stAuthor=item.stAuthor;
            dDecoyWinSim=item.dDecoyWinSim;
        }
        PairItem(const PairItem & item) {(*this)=item;}
};

//___________________________________________________________________
struct CompPairItem
{
    bool operator()(const PairItem & src,const PairItem & dst) const
    {
        if (src.stAnonym.compare(dst.stAnonym) < 0) return true;
        if (src.stAnonym.compare(dst.stAnonym) > 0) return false;

        if (src.stAuthor.compare(dst.stAuthor) < 0) return true;
        
        return false;
    }
};

//___________________________________________________________________
static void InitResult(map<PairItem,int,CompPairItem> & result, const string & stScriptFile)
{
    TdhReader trScript(stScriptFile,512);
    vector<string> vFields(2,"");
    int            iFields;
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stSimDecoysFolder = stSampleFolder + ResourceBox::Get()->getStringValue("SimilarDecoyFolder") + "\\";
    const string stWebDecoysFolder = stSampleFolder + ResourceBox::Get()->getStringValue("DecoyFolder") + "\\";
    const string stSortedDecoysFile = ResourceBox::Get()->getStringValue("SortedDecoysFile");
    const int iFirstIndex = ResourceBox::Get()->getIntValue("SelectDecoysFirst");
    const int iSetSize    = ResourceBox::Get()->getIntValue("SelectDecoysSetSize");
    const bool bIsRandom = (ResourceBox::Get()->getStringValue("DecoysType").compare("RandomDecoys") == 0);
    const bool bIsSelDcyByGen = (ResourceBox::Get()->getStringValue("DecoysType").compare("GenreDecoys") == 0);
    const bool bIsWebDecoys = (ResourceBox::Get()->getStringValue("DecoysType").compare("WebDecoys") == 0);
    double (*SimFunc)(const string &, const int, const int) = (bIsRandom?&DummyDecoySim:&DecoyWindowSim);
    int iLine=0;
    string stPairKey;
    string stDecoyFile;
    double dWindowSim;
    PairItem pairItem;
    
    while (trScript.bMoreToRead())
    {
        iFields = trScript.iGetLine(vFields);
        iLine++;
        if (iFields != 2) abort_err(string("Illegal Line: Line=")+stringOf(iLine)+", File="+stScriptFile);
        stPairKey = GetPairKey(vFields.at(0),vFields.at(1));
        
        if (bIsSelDcyByGen)
        {
            stDecoyFile = stSimDecoysFolder + "XY\\" + stPairKey + "_decoy.txt";
            dWindowSim = (*SimFunc)(stDecoyFile,iFirstIndex,iSetSize);

            stDecoyFile = stSimDecoysFolder + "YX\\" + GetPairKey(vFields.at(1),vFields.at(0)) + "_decoy.txt";
            dWindowSim += (*SimFunc)(stDecoyFile,iFirstIndex,iSetSize);
            dWindowSim /= 2.0;
        }
        else if (bIsWebDecoys)
        {
            stDecoyFile = stWebDecoysFolder + stPairKey + "\\" + stSortedDecoysFile;
            dWindowSim = (*SimFunc)(stDecoyFile,iFirstIndex,iSetSize);
        }
        else
        {
            stDecoyFile = stSimDecoysFolder+stPairKey+"_decoy.txt";
            dWindowSim = (*SimFunc)(stDecoyFile,iFirstIndex,iSetSize);
        }

        
        pairItem.stAnonym=vFields.at(0);
        pairItem.stAuthor=vFields.at(1);
        pairItem.dDecoyWinSim = dWindowSim;

        if (result.find(pairItem) != result.end()) abort_err(string("The base Pair <")+vFields.at(0)+","+vFields.at(1)+"> found already");
        result.insert(map<PairItem,int,CompPairItem>::value_type(pairItem,0));
    }
}

//___________________________________________________________________
int ReadClassifyPairRes(map<PairItem,int,CompPairItem> & result,
                        const string & stResFile)
{
    TdhReader trPairs(stResFile,512);
    vector<string> vFields(5,"");
    int            iFields;
    int            iSamplesNumber=0;
    map<PairItem,int,CompPairItem>::iterator iter;
    bool           bTargetFound;
    PairItem       pairItem;
    const int iMinRankToConsider = ResourceBox::Get()->getIntValue("MinRankToConsider");
    int iRank;

    while (trPairs.bMoreToRead())
    {
        iFields = trPairs.iGetLine(vFields);
        iSamplesNumber++;
        if (iFields != 5) abort_err(string("Illegal Line: fields=")+stringOf(iFields)+"<"+vFields.at(0)+","+vFields.at(1)+">");

        pairItem.stAnonym=vFields.at(0);
        pairItem.stAuthor=vFields.at(1);
        iter = result.find(pairItem);
        if (iter == result.end()) abort_err(string("Could not find result for <")+vFields.at(0)+","+vFields.at(1)+">");

        if (!string_as<int>(iRank,vFields.at(2),std::dec)) abort_err(string("Illegal rank field: ")+vFields.at(0)+" "+vFields.at(1)+" "+vFields.at(2));
        bTargetFound = (iRank <= iMinRankToConsider);
        if (bTargetFound) iter->second++;
    }

    return iSamplesNumber;
}

//___________________________________________________________________
void PrintClassifyPairRes(const map<PairItem,int,CompPairItem> & result, const string & stScriptFile,FILE * fSummary)
{
    map<PairItem,int,CompPairItem>::const_iterator iter;
    TdhReader trScript(stScriptFile,512);
    vector<string> vFields(2,"");
    int            iFields;
    int iLine=0;
    bool bPositive;
    PairItem pairItem;
    const int iStartFrom = ResourceBox::Get()->getIntValue("StartNameIndex");
    const int iNameKeyLen = ResourceBox::Get()->getIntValue("NameKeyLength");

    while (trScript.bMoreToRead())
    {
        iFields = trScript.iGetLine(vFields);
        iLine++;
        if (iFields != 2) abort_err(string("Illegal Line: Line=")+stringOf(iLine)+", File="+stScriptFile);

        pairItem.stAnonym=vFields.at(0);
        pairItem.stAuthor=vFields.at(1);
        if ((iter = result.find(pairItem)) == result.end()) abort_err(string("The Pair <")+vFields.at(0)+","+vFields.at(1)+"> not found in res map");

        bPositive = (vFields.at(0).substr(iStartFrom,iNameKeyLen).compare(vFields.at(1).substr(iStartFrom,iNameKeyLen))==0);

        fprintf(fSummary,"%s\t%s\t%s\t%d\t%.5f\n",vFields.at(0).c_str(),vFields.at(1).c_str(),(bPositive?"Yes":"No"),iter->second,iter->first.dDecoyWinSim);
        fflush(fSummary);
    }
}

//___________________________________________________________________
static void UpdateMax(const double dCurrVal,
                      const int iCurrIndex,
                      double & dMaxVal,
                      int & iMaxIndex)
{
    if (dMaxVal >= dCurrVal) return;
    
    dMaxVal = dCurrVal;
    iMaxIndex = iCurrIndex;
}
                 
//___________________________________________________________________
static void PrintAccuracyCurve(const map<PairItem,int,CompPairItem> & result,
                               const int iIRLoops,
                               FILE * fAccCurve,
                               const string & stScriptFile)
{
    map<PairItem,int,CompPairItem>::const_iterator iter;
    vector<int> vTruePositive(iIRLoops+1,0);
    vector<int> vTrueNegative(iIRLoops+1,0);
    vector<int> vFalsePositive(iIRLoops+1,0);
    vector<int> vFalseNegative(iIRLoops+1,0);
    vector<int> * vReject = NULL;
    vector<int> * vAccept = NULL;
    
    int i,k;
    int iPosItems=0;
    int iConfLevel;
    double dGlobalDecoyMean = 0.0;
    const int iSampleSize=ScriptSize(stScriptFile);
    TdhReader trScript(stScriptFile,512);
    vector<string> vFields(2,"");
    PairItem pairItem;
    const int iStartFrom = ResourceBox::Get()->getIntValue("StartNameIndex");
    const int iNameKeyLen = ResourceBox::Get()->getIntValue("NameKeyLength");
    bool bIsSameAuthor;

    int iDebug=0;
    while (trScript.bMoreToRead())
    {
        trScript.iGetLine(vFields);
    
        pairItem.stAnonym=vFields.at(0);
        pairItem.stAuthor=vFields.at(1);
        if ((iter = result.find(pairItem)) == result.end()) abort_err(string("The Pair <")+vFields.at(0)+","+vFields.at(1)+"> not found in res map");

        bIsSameAuthor = (iter->first.stAnonym.substr(iStartFrom,iNameKeyLen).compare(iter->first.stAuthor.substr(iStartFrom,iNameKeyLen))==0);
        iConfLevel = iter->second;
        
        if (bIsSameAuthor) {vAccept=&vTruePositive; vReject=&vFalseNegative; iPosItems++;}
        else {vAccept=&vFalsePositive; vReject=&vTrueNegative;}

        dGlobalDecoyMean += iter->first.dDecoyWinSim;
        
        try
        {
            for (i=0;i<=iConfLevel;i++) vAccept->at(i)++;
            for (i=(iConfLevel+1);i<=iIRLoops;i++) vReject->at(i)++;
        }
        catch (...)
        {
            abort_err(string("Failed to access entry in PrintAccuracyCurve. i=")+stringOf(i)+", pair: <"+iter->first.stAnonym+","+iter->first.stAuthor+">");
        }
    }
    dGlobalDecoyMean /= iSampleSize;
    
    double dPosRec,dPosPrec,dNegRec,dNegPrec,dAcc,dPosF1,dNegF1,dMacroF1,dReqRec,dBestPrec;
    int iBestAccLoop;
    int iBestPosF1Loop;
    int iBestNegF1Loop;
    int iBestMacroF1Loop;
    int iClassifyAsPos;
    double dBestAcc=-1.0;
    double dBestPosF1=-1.0;
    double dBestNegF1=-1.0;
    double dBestMacroF1=-1.0;

    fprintf(fAccCurve,"Conf\tRec\tPrec\tPosF1\tNegF1\tMacroF1\tAcc\tTP\tFN\tFP\tTN\n");
    fflush(fAccCurve);
    for (i=0;i<=iIRLoops;i++)
    {
        iClassifyAsPos = vTruePositive.at(i) + vFalsePositive.at(i);
        
        dPosRec = ((iPosItems>0)?((double)vTruePositive.at(i) / (double)iPosItems):0.0);
        dPosPrec = ((iClassifyAsPos>0)?((double)vTruePositive.at(i) / (double)iClassifyAsPos):0.0);
        dPosF1 = dPosRec+dPosPrec;
        dPosF1 = ((dPosF1>0)?(2*dPosRec*dPosPrec)/(dPosRec+dPosPrec):0.0);

        dNegRec = ((double)vTrueNegative.at(i) / (double)(iSampleSize-iPosItems));
        dNegPrec = (double)vTrueNegative.at(i) / (double)(vTrueNegative.at(i) + vFalseNegative.at(i));       
        dNegF1 = dNegRec+dNegPrec;
        dNegF1 = ((dNegF1>0)?(2*dNegRec*dNegPrec)/(dNegRec+dNegPrec):0.0);
        
        dMacroF1 = (dNegF1+dPosF1)/2.0;
        
        dAcc = (double)(vTruePositive.at(i) + vTrueNegative.at(i)) / (double)iSampleSize;
        fprintf(fAccCurve,"%d\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%d\t%d\t%d\t%d\n",
                i,
                dPosRec,
                dPosPrec,
                dPosF1,
                dNegF1,
                dMacroF1,
                dAcc,
                vTruePositive.at(i),
                vFalseNegative.at(i),
                vFalsePositive.at(i),
                vTrueNegative.at(i));
        fflush(fAccCurve);

        UpdateMax(dAcc,i,dBestAcc,iBestAccLoop);
        UpdateMax(dPosF1,i,dBestPosF1,iBestPosF1Loop);
        UpdateMax(dNegF1,i,dBestNegF1,iBestNegF1Loop);
        UpdateMax(dMacroF1,i,dBestMacroF1,iBestMacroF1Loop);
    }
    fprintf(fAccCurve,"Best   Acc: %.3f (at %d)\n",dBestAcc,iBestAccLoop);
    fprintf(fAccCurve,"Best   Best Pos F1:  %.3f (at %d)\n",dBestPosF1,iBestPosF1Loop);
    fprintf(fAccCurve,"Best   Best Neg F1:  %.3f (at %d)\n",dBestNegF1,iBestNegF1Loop);
    fprintf(fAccCurve,"Best   Best Mac F1:  %.3f (at %d)\n",dBestMacroF1,iBestMacroF1Loop);
    fprintf(fAccCurve,"Decoys Sim: %.5f\n",dGlobalDecoyMean);
    fflush(fAccCurve);

    fprintf(fAccCurve,"\n\nRecall-Prec curve (Same Author):\n\nRecall\tPrecision\t#Loops\n");
    for (k=0;k<=100;k++)
    {
        dReqRec = (double)k/100.0;
        dBestPrec=0.0;
        for (i=iIRLoops;i>0;i--)
        {
            iClassifyAsPos = vTruePositive.at(i) + vFalsePositive.at(i);
        
            dPosRec = ((iPosItems>0)?((double)vTruePositive.at(i) / (double)iPosItems):0.0);
            
            if (dPosRec < dReqRec) continue;

            dPosPrec = (double)vTruePositive.at(i) / (double)iClassifyAsPos;
            UpdateMax(dPosPrec,i,dBestPrec,iBestAccLoop);
        }

        fprintf(fAccCurve,"%.5f\t%.5f\t%d\n",dReqRec,dBestPrec,iBestAccLoop);
        fflush(fAccCurve);
    }

    fprintf(fAccCurve,"\n\nRecall-Prec curve (Diff Author):\n\nRecall\tPrecision\t#Loops\n");
    for (k=0;((k<=100) && ((iSampleSize-iPosItems)>0));k++)
    {
        dReqRec = (double)k/100.0;
        dBestPrec=0.0;
        for (i=0;i<iIRLoops;i++)
        {
            dNegRec = ((double)vTrueNegative.at(i) / (double)(iSampleSize-iPosItems));
            
            if (dNegRec < dReqRec) continue;

            dNegPrec = (double)vTrueNegative.at(i) / (double)(vTrueNegative.at(i) + vFalseNegative.at(i));
            UpdateMax(dNegPrec,i,dBestPrec,iBestAccLoop);
        }

        fprintf(fAccCurve,"%.5f\t%.5f\t%d\n",dReqRec,dBestPrec,iBestAccLoop);
        fflush(fAccCurve);
    }

    fprintf(stderr,"Some Details:\n");
    fprintf(stderr,"Sample Size=%d\n",result.size());
    if (vTruePositive.size() > 15)
    {
        fprintf(stderr,"Always Propose Recall=%d\n",vTruePositive.at(1));
        fprintf(stderr,"At ConfLevel 15: ta=%d, fa=%d, tr=%d, fr=%d\n",vTruePositive.at(15),vFalsePositive.at(15),vTrueNegative.at(15),vFalseNegative.at(15));
    }
    fflush(stderr);
}
                       
//___________________________________________________________________
void ClassifyPairSummary()
{
    map<PairItem,int,CompPairItem> result;
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const bool bIsSelDcyByGen = (ResourceBox::Get()->getStringValue("DecoysType").compare("GenreDecoys") == 0);
    const int iIRLoops = ((bIsSelDcyByGen?2:1) * ResourceBox::Get()->getIntValue("IRLoops"));
    const string stScriptFile = stSampleFolder+ResourceBox::Get()->getStringValue("ScriptFile");
    const string stSummaryFile = ResourceBox::Get()->getStringValue("SummaryFile");
    const string stAccuracyCurve = ResourceBox::Get()->getStringValue("AccuracyCurve");
    
    const string stResFile = ResourceBox::Get()->getStringValue("PairClassRes");

    InitResult(result,stScriptFile);
    
    FILE * fSummary = open_file(stSummaryFile,"ClassifyPairSummary","w");
    FILE * fAccCurve = open_file(stAccuracyCurve,"ClassifyPairSummary","w");
    
    ReadClassifyPairRes(result,stResFile);
    
    PrintClassifyPairRes(result,stScriptFile,fSummary);
    fclose(fSummary);
    
    PrintAccuracyCurve(result,
                       iIRLoops,
                       fAccCurve,
                       stScriptFile);
    fclose(fAccCurve);
}

//___________________________________________________________________
void PlainSimSummary()
{
    map<PairItem,int,CompPairItem> result;
    map<PairItem,int,CompPairItem>::const_iterator iter;
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stScriptFile = stSampleFolder + ResourceBox::Get()->getStringValue("ScriptFile");
    const string stResFile = ResourceBox::Get()->getStringValue("PairClassRes");
    const string stAccuracyCurve = ResourceBox::Get()->getStringValue("AccuracyCurve");
    const string stSummaryFile = ResourceBox::Get()->getStringValue("SummaryFile");
    const double dMinRange = ResourceBox::Get()->getDoubleValue("MinRawSimRange");
    const double dMaxRange = ResourceBox::Get()->getDoubleValue("MaxRawSimRange");
    
    FILE * fSummary = open_file(stSummaryFile,"PlaneSimSummary","w");

    const int iSampleSize = ScriptSize(stResFile);
    TdhReader trScript(stResFile,512);
    vector<string> vFields(3,"");
    PairItem pairItem;
    int iConfLevel;
    double dCurrScore;
    while (trScript.bMoreToRead())
    {
        if (trScript.iGetLine(vFields) != 3) abort_err(string("Illegal #Fields in res file: ")+vFields.at(0));
        if (!string_as<double>(dCurrScore,vFields.at(2),std::dec)) abort_err(string("Illegal score field: ")+vFields.at(2));
        
        // Normalize score (log): (t-x)/(y-x)
        dCurrScore -= dMinRange;
        dCurrScore /= (dMaxRange - dMinRange);
        
        if (dCurrScore > 1.0) dCurrScore=1.0;
        if (dCurrScore < 0.0) dCurrScore=0.0;
        
        dCurrScore *= 100;
        iConfLevel = (int) dCurrScore;
        
        pairItem.stAnonym=vFields.at(0);
        pairItem.stAuthor=vFields.at(1);

        fprintf(fSummary,"%s\t%s\t%d\n",vFields.at(0).c_str(),vFields.at(1).c_str(),iConfLevel); fflush(fSummary);

        if (result.find(pairItem) != result.end()) abort_err(vFields.at(0)+" found already in "+stResFile);
        result.insert( map<PairItem,int,CompPairItem>::value_type(pairItem,iConfLevel));
    }
    fclose(fSummary);

    FILE * fAccCurve = open_file(stAccuracyCurve,"ClassifyPairSummary","w");
    PrintAccuracyCurve(result,
                       100,
                       fAccCurve,
                       stScriptFile);
    fclose(fAccCurve);
}

//___________________________________________________________________
void MergeSVMREs()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stRawLogRes = ResourceBox::Get()->getStringValue("RawLogRes");
    const string stScriptFile = "script_5050.txt";

    FILE * fResFile = open_file(ResourceBox::Get()->getStringValue("PairClassRes"),"MergeSVMREs","w");

    TdhReader tdhScript(stSampleFolder+stScriptFile,512);
    LinesReader lrLog(stRawLogRes,512);
    vector<string> vFields(2,"");
    string stScore;
    int iItems=1;

    while (tdhScript.bMoreToRead() && lrLog.bMoreToRead())
    {
        tdhScript.iGetLine(vFields);
        lrLog.voGetLine(stScore);

        fprintf(fResFile,"%s\t%s\t%s\n",vFields.at(0).c_str(),vFields.at(1).c_str(),stScore.c_str()); fflush(fResFile);
        fprintf(stdout,"Working on pair %d: %s, %s, %s\n",iItems++,vFields.at(0).c_str(),vFields.at(1).c_str(),stScore.c_str()); fflush(stdout);
    }
    if (tdhScript.bMoreToRead() || lrLog.bMoreToRead()) abort_err(string("MergeSVMREs: Non match files"));
    fflush(fResFile); fclose(fResFile);
}
