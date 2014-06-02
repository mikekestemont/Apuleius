/*
  Definition of the various feature set functionalities.
*/
#include <FeatureSet.h>
#include <stdlib.h>
#include <auxiliary.h>

// Constructor of a feature set from a feature vector.
//___________________________________________________________________
StaticSet::StaticSet(const map<string,FeatureInfo,LexComp> * lexicon, // The features lexicon (all features).
                     const vector<string> & vFtrsVec,                 // The features appearing in the correspondent document.
                     const int iFtrsNumber,                           // Number of (unique) features in the document.
                     const string & _stAuthorName,                    // The document name.
                     const string & _stClassName):                    // The document class (optional).
                     stAuthorName(_stAuthorName),
                     stClassName(_stClassName),
                     vStaticSet(1),
                     FeatureSet()
{
    vector<string> vTermVec(iFtrsNumber,"");
    vector<string> vWeightVec(iFtrsNumber,"");
    vector<string> vIdfVec(iFtrsNumber,"");
    int i;
    size_t iPos;
    string stLine;
    
    if (lexicon == NULL)
    {
        fprintf(stderr,"lexicon == NULL in FeatureSet\n");
        fflush(stderr);
        exit(0);
    }
    
	// Scan the features vector and extract the information corresponded with each feature.
	// The format of each line representing a feature is as follows: <term\tweight\tidf>
    for (i=0;i<iFtrsNumber;i++)
    {
        stLine = vFtrsVec.at(i);
        
		// Get the term's content.
        if ((iPos = stLine.find_first_of('\t')) == string::npos) abort_err(string("Illegal feature line: ")+vFtrsVec.at(i));
        vTermVec.at(i) = stLine.substr(0,iPos); stLine = stLine.substr(iPos+1);

		// Get the feature's weight.
        if ((iPos = stLine.find_first_of('\t')) == string::npos) abort_err(string("Illegal feature line: ")+vFtrsVec.at(i));
        vWeightVec.at(i) = stLine.substr(0,iPos);

		// Get the feature idf.
        vIdfVec.at(i) = stLine.substr(iPos+1);
    }
    
	// Set the data.
    this->SetData(lexicon,vTermVec,vWeightVec,vIdfVec,iFtrsNumber);
}

// Constructor of a feature set from a feature file associated with a document.
//___________________________________________________________________
StaticSet::StaticSet(const map<string,FeatureInfo,LexComp> * lexicon,
                     const string       & stFeaturePath,
                     const string       & _stAuthorName,
                     const string       & _stClassName):
                     stAuthorName(_stAuthorName),
                     stClassName(_stClassName),
                     vStaticSet(1),
                     FeatureSet()
{
    const int iFtrsNum = ScriptSize(stFeaturePath+stAuthorName);
    TdhReader tdhReader(stFeaturePath+stAuthorName,4096);
    vector<string> vTermVec(iFtrsNum,"");
    vector<string> vWeightVec(iFtrsNum,"");
    vector<string> vIdfVec(iFtrsNum,"");
    vector<string> vFields(3,"");
    int i;
    
    if (lexicon == NULL)
    {
        fprintf(stderr,"lexicon == NULL in FeatureSet\n");
        fflush(stderr);
        exit(0);
    }

	// Scan the features file and extract the information corresponded with each feature.
	// The format of each line representing a feature is as follows: <term\tweight\tidf>
    i=0;
    while (tdhReader.bMoreToRead())
    {
        tdhReader.iGetLine(vFields);
        vTermVec.at(i) = vFields.at(0);
        vWeightVec.at(i) = vFields.at(1);
        vIdfVec.at(i) = vFields.at(2);
        i++;
    }
    if (i != iFtrsNum) abort_err(stringOf(i)+" obs fields while "+stringOf(iFtrsNum)+" exp fields in "+stAuthorName);
    
    this->SetData(lexicon,vTermVec,vWeightVec,vIdfVec,iFtrsNum);
}

// Set this feature set data.
//___________________________________________________________________
void StaticSet::SetData(const map<string,FeatureInfo,LexComp> * lexicon,
                        const vector<string> & vTermVec,
                        const vector<string> & vWeightVec,
                        const vector<string> & vIdfVec,
                        const int iFtrsNumber)
{
    set<FeatureItem,FIOrderByIndex> featureSet;
    set<FeatureItem,FIOrderByIndex>::const_iterator mit;
    map<string,FeatureInfo,LexComp>::const_iterator fcIter;
    FeatureItem item;
    int i;
    
	// Scan the vectors containing the various features' content
	// and set them the object.
    for (i=0;i<iFtrsNumber;i++)
    {
        fcIter = lexicon->find(vTermVec.at(i));
        if ((fcIter == lexicon->end()) || (fcIter->second.index < 0))
        {
            fprintf(stderr,"Unknown feature: file=%s, feature=%s\n",stAuthorName.c_str(),vTermVec.at(i).c_str());
            fflush(stderr);
            exit(0);
        }
        
        item.index = fcIter->second.index;

        if (!string_as<double>(item.weight,vWeightVec.at(i),std::dec))
        {
            fprintf(stderr,"Illegal weight value: %s, %s\n",vTermVec.at(i).c_str(),vWeightVec.at(i).c_str());
            fflush(stderr);
            exit(0);
        }

        if (!string_as<double>(item.idf,vIdfVec.at(i),std::dec))
        {
            fprintf(stderr,"Illegal idf value: %s, %s\n",vTermVec.at(i).c_str(),vIdfVec.at(i).c_str());
            fflush(stderr);
            exit(0);
        }

        featureSet.insert(item);
    }

    vStaticSet.resize(featureSet.size()+1);
    iStaticItems=0;
    mit=featureSet.begin();
    while (mit != featureSet.end())
    {
        vStaticSet.at(iStaticItems++) = *mit;
        mit++;
    }
}

// Sort the features by their relevance.
// Used for implementing the directional-weight factor.
//___________________________________________________________________
int StaticSet::iGetTopFtrs(vector<FeatureItem> & vFtrs) const
{
    set<FeatureItem,FIOrderByWeight> relevanceOrder;
    set<FeatureItem,FIOrderByWeight>::const_iterator iter;
    int i;

    // Extract the selected random sub-set
    // and sort its items by their relevance.
    for (i=0;i<iStaticItems;i++) relevanceOrder.insert(vStaticSet.at(i));
    i=0;
    iter=relevanceOrder.begin();
    while (iter != relevanceOrder.end())
    {
        try
        {
            vFtrs.at(i++) = *iter;
            iter++;
        }
        catch (...)
        {
            fprintf(stderr,"Failed to access ftrs vec in iGetTopFtrs. i=%d\n",i);
            fflush(stderr);
            exit(0);
        }
    }

    return i;
}

// Extract the random feature set.
// The fuction gets the random features used in this iteration,
// and creates an active feature set comprising of selected
// features only.
//___________________________________________________________________
void FeatureSet::SetRandomFeatures(const vector<bool> & vRandomSet)
{
    set<FeatureItem,FIOrderByWeight> relevanceOrder;
    set<ActiveItem,AIOrderByIndex> randSubSet;
    int i;
    double relevance;
    const int iFeaturesCount = this->iGetStaticSetSize();
    const vector<FeatureItem> & vStaticSet = this->vGetStaticSet();

    // Extract the selected random sub-set
    // and sort its items by their relevance.
	try
	{
		for (i=0;i<iFeaturesCount;i++)
		{
			if (vRandomSet.at(vStaticSet.at(i).index))
			{
				relevanceOrder.insert(vStaticSet.at(i));
			}
		}
	}
	catch(...)
	{
		abort_err(string("Failed to order features by relevance: author=")+this->stGetAuthorName());
	}

    // Add the to each item its relevance and
    // rank (by relevance).
    // Sort the modified items by their (global) index.
	try
	{
		iActiveItems = relevanceOrder.size();
		vMyRelWeights.resize(iActiveItems+1);
		set<FeatureItem,FIOrderByWeight>::const_iterator order;
		i=0;
		ActiveItem aItem;
		for (order=relevanceOrder.begin();order!=relevanceOrder.end();order++)
		{
			relevance = (double)i / (double)(iActiveItems+1);
			relevance = 1.0 - relevance;

			aItem.index=order->index;
			aItem.weight=order->weight;
			aItem.rank=i;
			aItem.relevance=relevance;

			vMyRelWeights.at(i)=relevance;

			randSubSet.insert(aItem);
			i++;
		}
	}
	catch(...)
	{
		abort_err(string("Failed to create the random subset: author=")+this->stGetAuthorName());
	}

    // Set the random sub-set vector.
    // Notice that the items are sorted according
    // to their global indexes.
	try
	{
		try
		{
			vActiveSet.resize(iActiveItems+1);
		}
		catch(...)
		{
			abort_err(string("Resize subset vector failed: author=")+this->stGetAuthorName() + ", #Items=" + stringOf(iActiveItems));
		}

		set<ActiveItem,AIOrderByIndex>::const_iterator aiIter;
		dCosineWeight=0.0;
		dMinMaxWeight=0.0;
		iActiveItems=0;
		try
		{
			for (aiIter=randSubSet.begin();aiIter!=randSubSet.end();aiIter++)
			{
				try
				{
					vActiveSet.at(iActiveItems++)=*aiIter;
					dMinMaxWeight += aiIter->weight;
					dCosineWeight += (aiIter->weight * aiIter->weight);
				}
				catch(...)
				{
					abort_err(string("Assign item to subset vector failed: author=")+this->stGetAuthorName() + ", i=" + stringOf(iActiveItems));
				}
			}
		}
		catch(...)
		{
			abort_err(string("Setting loop of the subset vector failed: author=")+this->stGetAuthorName());
		}
	}
	catch(...)
	{
		abort_err(string("Failed to set the subset vector: author=")+this->stGetAuthorName());
	}
}

// Compute the average precision of this document.
// Used for computing the directional-weight factor.
//___________________________________________________________________
double FeatureSet::getAveragePrec(const vector<double> & vRelWeight) const
{
    int rank;
    double dAccumPrec=0.0;
    double prec,rel;
    double ap=0.0;

    for (rank=0;rank<iActiveItems;rank++)
    {
        rel = vRelWeight.at(rank);

        if (rel == 0.0) continue;

        dAccumPrec += vMyRelWeights.at(rank);
        prec = dAccumPrec / (double)(rank+1);
        ap += (prec*rel);
    }

    ap /= (double)iActiveItems;

    return ap;
}

//___________________________________________________________________
FeatureType strToFeatureType(const string & stFeatureType)
{
	if (stFeatureType.compare("LetterNgram") == 0) return LetterNgram;
	if (stFeatureType.compare("WordNgram") == 0) return WordNgram;
	return UndefinedFT;
}

//___________________________________________________________________
const char * featureTypeToStr(FeatureType featureType)
{
	if (featureType == LetterNgram) return "LetterNgram";
	if (featureType == WordNgram) return "WordNgram";
	return "UndefinedFT";
}
