#include <math.h>
#include <auxiliary.h>
#include <ResourceBox.h>
#include <FileScanner.h>
#include <FeaturesHandler.h>
#include <DataFuncs.h>
#include <set>
#include <stack>

using namespace std;

//___________________________________________________________________
void MergeFtrsVecs()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const int iFeatureSetSize = ResourceBox::Get()->getIntValue("FeatureSetSize");
    const int iStartIndex = ResourceBox::Get()->getIntValue("SecondSetStartIndex");
    int i;

    const string stSetVecs1 = stSampleFolder + ResourceBox::Get()->getStringValue("VecsFileSet1");
    const string stSetVecs2 = stSampleFolder + ResourceBox::Get()->getStringValue("VecsFileSet2");
    const string stTrainFile = stSampleFolder + ResourceBox::Get()->getStringValue("TrainVecsFile");
    FileScanner fsTrainSet1(stSetVecs1,1024);
    FileScanner fsTrainSet2(stSetVecs2,1024);

    FILE * fMergedVecs = open_file(stTrainFile,"MergeFtrsVecs","w");
    FILE * debug_log = open_file(ResourceBox::Get()->getStringValue("Debug_Log"),"MergeFtrsVecs","w");

    vector<double> vFeaturesVec1(iFeatureSetSize+1,0.0);
    vector<double> vFeaturesVec2(iFeatureSetSize+1,0.0);
    vector<double> vMergedVec(iFeatureSetSize+1,0.0);
    int iTrainVecsNumber = 0;
    string stInfo1(75,'\0');
    string stInfo2(75,'\0');
    int iRightClass1;
    int iRightClass2;

    fprintf(stderr,"Scan Training Sets: %s, %s\n",stSetVecs1.c_str(),stSetVecs2.c_str());
    fflush(stderr);
    while (fsTrainSet1.bMoreToRead() && fsTrainSet2.bMoreToRead())
    {
        ReadFeaturesVec(fsTrainSet1,vFeaturesVec1,iRightClass1,iFeatureSetSize,stInfo1);
        ReadFeaturesVec(fsTrainSet2,vFeaturesVec2,iRightClass2,iFeatureSetSize,stInfo2);

        if ((iTrainVecsNumber%500) == 0)
        {
            fprintf(stderr,"Reading vector %d...\n",iTrainVecsNumber);
            fflush(stderr);
        }

        if ((stInfo1.compare(stInfo2) != 0) || (iRightClass1 != iRightClass2))
        {
            fprintf(stderr,"Non match vectors: file1: (%s, %s), file2: (%s, %s), right=(%d,%d)\n",
                    stSetVecs1.c_str(),stInfo1.c_str(),stSetVecs2.c_str(),stInfo2.c_str(),iRightClass1,iRightClass2);
            fflush(stderr);
            exit(0);
        }

        for (i=1;i<iStartIndex;i++) vMergedVec.at(i) = vFeaturesVec1.at(i);
        for (i=iStartIndex;i<=iFeatureSetSize;i++) vMergedVec.at(i) = vFeaturesVec2.at(i-iStartIndex+1);
        
        PrintFeaturesVec(fMergedVecs,vMergedVec,stringOf(iRightClass1),stInfo1,iFeatureSetSize);
        iTrainVecsNumber++;
    }

    fflush(debug_log);
    fclose(debug_log);
    fflush(fMergedVecs);
    fclose(fMergedVecs);

    fprintf(stderr,"MergeFtrsVecs - End\n");
    fflush(stderr);
}
