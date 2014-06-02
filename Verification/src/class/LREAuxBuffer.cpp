#include <math.h>
#include <LREAuxBuffer.h>
#include <ResourceBox.h>
#include <auxiliary.h>
#include <set>
                                 
//___________________________________________________________________
LREAuxBuffer::LREAuxBuffer():
    dBestScore(0.0),
    stBestCand(""),
    dScoreFloor(exp(-30.0)),
    iStartIndex(ResourceBox::Get()->getIntValue("StartNameIndex")),
    iNameKeyLength(ResourceBox::Get()->getIntValue("NameKeyLength"))
{
}

// Use all candidates authors.
//___________________________________________________________________
int LREAuxBuffer::getCandsSet(const string              & stAnonym,
                              const vector<FeatureSet*> & vAuthors,
                              const int                   iAuthorsNumber,
                              vector<FeatureSet*>       & vCandidates) const
{
    int i;
    for (i=0;i<iAuthorsNumber;i++)
    {
        vCandidates.at(i) = vAuthors.at(i);
    }
    return iAuthorsNumber;
}

//___________________________________________________________________
LRENoAuthor::LRENoAuthor():LREAuxBuffer()
{
    const string stAvoidAuthors = ResourceBox::Get()->getStringValue("AvoidAuthorList");
    TdhReader tdhAuthors(stAvoidAuthors,512);
    vector<string> vFields(2,"");

    avoidAuthors.clear();
    map<string, set<string,LexComp> ,LexComp>::iterator iter;
    while (tdhAuthors.bMoreToRead())
    {
        tdhAuthors.iGetLine(vFields);
        iter = avoidAuthors.find(vFields.at(0));
        if (iter == avoidAuthors.end())
        {
            avoidAuthors.insert(map<string, set<string,LexComp> ,LexComp>::value_type(vFields.at(0),set<string,LexComp>()));
            if ((iter = avoidAuthors.find(vFields.at(0)))==avoidAuthors.end()) abort_err(string("Couldn't add entry for ")+vFields.at(0)+" -> "+vFields.at(1)+" in LRENoAuthor::LRENoAuthor");
        }
        iter->second.insert(vFields.at(1));
    }
}

// Get all candidate authors, excluding the author of the current
// anonimous text.
//___________________________________________________________________
int LRENoAuthor::getCandsSet(const string              & stAnonym,
                             const vector<FeatureSet*> & vAuthors,
                             const int                   iAuthorsNumber,
                             vector<FeatureSet*>       & vCandidates) const
{
    int i;
    int iItems=0;

    //fprintf(stdout,"LRENoAuthor::getCandsSet - Start (%s, sni=%d, nkl=%d)\n",stAnonym.c_str(),iStartIndex,iNameKeyLength); fflush(stdout);
    map<string, set<string,LexComp> ,LexComp>::const_iterator iter;
    for (i=0;i<iAuthorsNumber;i++)
    {
        //fprintf(stdout,"\tCurr Author: %s\n",vAuthors.at(i)->stGetAuthorName().c_str()); fflush(stdout);
        if (vAuthors.at(i)->stGetAuthorName().substr(iStartIndex,iNameKeyLength).compare(stAnonym.substr(iStartIndex,iNameKeyLength)) == 0) continue;

        iter = avoidAuthors.find(stAnonym.substr(iStartIndex,iNameKeyLength));
        if (iter == avoidAuthors.end())
            vCandidates.at(iItems++) = vAuthors.at(i);
        else if (iter->second.find(vAuthors.at(i)->stGetAuthorName().substr(iStartIndex,iNameKeyLength)) == iter->second.end())
            vCandidates.at(iItems++) = vAuthors.at(i);
        else
            continue;
        //fprintf(stdout,"\t\tAdd Author!!\n"); fflush(stdout);
    }
    //fprintf(stdout,"LRENoAuthor::getCandsSet - End (%s)\n",stAnonym.c_str()); fflush(stdout);
    return iItems;
}

//___________________________________________________________________
void LREAuxBuffer::updateResult(const double dScore,const FeatureSet * fsCand)
{
    if (dScore > dBestScore)
    {
        dBestScore=dScore;
        stBestCand=fsCand->stGetAuthorName();
    }
}

//___________________________________________________________________
void LREAuxBuffer::printResult(const FeatureSet * fsAnonym,FILE * resFile)
{
    fprintf(resFile,"%s\t%s\t%.5f\n",fsAnonym->stGetAuthorName().c_str(),stBestCand.c_str(),dBestScore);
    fflush(resFile);
}

//___________________________________________________________________
void LREAuxBuffer::SetRandomFeatures(const vector<bool>  & vRandomSet,
                                     vector<FeatureSet*> & vec,
                                     const int             iVecSize)
{
    FeatureSet * fsItem = NULL;
    int i;
    
	try
	{
		for (i=0;i<iVecSize;i++)
		{
			fsItem = vec.at(i);


			if (fsItem == NULL) abort_err(string("fsItem==NULL at SetRandomFeatures. vec=") + stringOf(iVecSize));

			fsItem->SetRandomFeatures(vRandomSet);
		}
	} 
	catch(...)
	{
		abort_err(string("Failed to set random features. vec=") + stringOf(iVecSize));
	}
}

//___________________________________________________________________
LREOpenSet::LREOpenSet(const map<string,FeatureInfo,LexComp> * _lexicon):
    LREAuxBuffer(),
    lexicon(_lexicon)
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stAuthorsFile = stSampleFolder+"anonyms.txt";
    TdhReader items(stAuthorsFile,512);
    vector<string> vFields(2,"");
    StaticSet * fs = NULL;
    RandomSet * rs = NULL;

    authors.clear();
    while (items.bMoreToRead())
    {
        items.iGetLine(vFields);

        fs = new StaticSet(lexicon,stSampleFolder+"Author\\Ftrs\\",vFields.at(0),"");
        rs = new RandomSet(fs);
        if ((fs==NULL) || (rs == NULL)) abort_err(string("Failed to alloc fs: ")+vFields.at(0));
        if (authors.find(vFields.at(0)) != authors.end()) abort_err(vFields.at(0)+" already exists");
        authors.insert(map<string,FeatureSet*,LexComp>::value_type(vFields.at(0),rs));
    }
}

//___________________________________________________________________
LREOpenSet::~LREOpenSet()
{
    map<string,FeatureSet*,LexComp>::iterator iter;
    StaticSet * ss=NULL;
    RandomSet * rs=NULL;

    for (iter=authors.begin();iter!=authors.end();iter++)
    {
        rs = static_cast<RandomSet*>(iter->second);
        ss = const_cast<StaticSet*>(static_cast<const StaticSet*>(rs->getStaticSet()));
        delete ss;
        delete rs;
    }
}

// Get all candidate authors and add the author of the current
// anonimous text.
//___________________________________________________________________
int LREOpenSet::getCandsSet(const string              & stAnonym,
                            const vector<FeatureSet*> & vAuthors,
                            const int                   iAuthorsNumber,
                            vector<FeatureSet*>       & vCandidates) const
{
    int i;
    map<string,FeatureSet*,LexComp>::const_iterator iter;
    
    for (i=0;i<iAuthorsNumber;i++)
    {
        vCandidates.at(i) = vAuthors.at(i);
    }

    iter = authors.find(stAnonym);
    if (iter == authors.end()) abort_err(string("Can't find author: ")+stAnonym);
    vCandidates.at(iAuthorsNumber) = const_cast<FeatureSet*>(iter->second);
    return (iAuthorsNumber+1);
}

//___________________________________________________________________
void LREOpenSet::setRandomFtrs(const vector<bool> & vRandomSet)
{
    map<string,FeatureSet*,LexComp>::iterator it;
    for (it=authors.begin();it!=authors.end();it++) it->second->SetRandomFeatures(vRandomSet);
}

//___________________________________________________________________
void LREClassify::updateResult(const double dScore,const FeatureSet * fsCand)
{
    if (dScore > dBestScore)
    {
        dBestScore=dScore;
        stBestCand=fsCand->stGetClassName();
        stBestAuthor=fsCand->stGetAuthorName();
    }
}

//___________________________________________________________________
void LREClassify::printResult(const FeatureSet * fsAnonym,
                              FILE * resFile)
{
    fprintf(resFile,"%s\t%s\n",
            fsAnonym->stGetClassName().c_str(),
            fsAnonym->stGetAuthorName().c_str());
    fflush(resFile);
}

