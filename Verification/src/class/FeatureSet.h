/*
  Declaration of the structures constituting the feature set.
*/
#ifndef __FeatureSet__
#define __FeatureSet__

#include <list>
#include <set>
#include <map>
#include <FileScanner.h>
#include <auxiliary.h>

using namespace std;

// Definition of a single feature.
// This object contains the static information of a feature.
//___________________________________________________________________
class FeatureItem
{
    public:

        // The index of the feature in the feature set.
        int index;

		// The feature's weight.
		// Its actual content depends on the configuration parameters
		// (e.g. tf-idf, term-frequency, binary, etc.)
        double weight;

		// The idf value of this feature.
        double idf;
        
        FeatureItem(){}
        
        void operator=(const FeatureItem & item)
        {
            index=item.index;
            weight=item.weight;
            idf=item.idf;
        }
        
        FeatureItem(const FeatureItem & item){*this=item;}
};

// An active item representing a feature item during the attribution procedures.
// While a feature consists of various static elements (e.g. its weight, index, etc.),
// it might be manipulated during the attribution.
//___________________________________________________________________
class ActiveItem
{
    public:

        // Points to the static item.
        int index;

		// The manipulated weight of the feature.
        double weight;

        // Random/Variable feaure's info (change upon each iteration).
        int rank;
        double relevance;

        ActiveItem(){}

        void operator=(const ActiveItem & item)
        {
            index=item.index;
            weight=item.weight;
            rank=item.rank;
            relevance=item.relevance;
        }

        ActiveItem(const ActiveItem & item){*this=item;}
};

// The feature type.
enum FeatureType
{
	UndefinedFT,
	LetterNgram,
	WordNgram
};

// Definition of an object that represents a feature during the
// feature extraction procedure.
// The object, thus, contains various counters that are updated
// during the feature extraction, and then can be used for computing
// the feauture weight.
//___________________________________________________________________
class FeatureInfo
{
    public:
        
		// A unique serial index of the feature.
        int index;

		// Total occurrences number of the feature in the training corpus.
        int iTotalFreq;

		// The number of documents containing the feature in the training corpus.
        int iDocsFreq;

		// The feature frequency (computed in the end of the training).
        double dFreq;

		// The term and its type.
        string stTerm;
		FeatureType featureType;
        
        virtual ~FeatureInfo() {}
        
        FeatureInfo(){}
                       
        void operator=(const FeatureInfo & item)
        {
            index=item.index;
            iTotalFreq=item.iTotalFreq;
            iDocsFreq=item.iDocsFreq;
            dFreq=item.dFreq;
            stTerm=item.stTerm;
			featureType=item.featureType;
        }
        
        FeatureInfo(const FeatureInfo & item) {(*this)=item;}
};

// Converting a feature type into string and vice-versa.
FeatureType strToFeatureType(const string & stFeatureType);
const char * featureTypeToStr(FeatureType featureType);

/*
  Ordering structures for the various feature representation functions.
*/

// Sort an active item by its index.
//___________________________________________________________________
struct AIOrderByIndex
{
    bool operator()(const ActiveItem & ai1,const ActiveItem & ai2) const
    {
        return (ai1.index < ai2.index);
    }
};

// Sort a feature item by its index.
//___________________________________________________________________
struct FIOrderByIndex
{
    bool operator()(const FeatureItem & fi1,const FeatureItem & fi2) const
    {
        return (fi1.index < fi2.index);
    }
};

// Sort a feature item by its weight.
//___________________________________________________________________
struct FIOrderByWeight
{
    bool operator()(const FeatureItem & fi1,const FeatureItem & fi2) const
    {
        if (fi1.weight > fi2.weight) return true;
        if (fi1.weight < fi2.weight) return false;

        if (fi1.idf > fi2.idf) return true;
        if (fi1.idf < fi2.idf) return false;

        return (fi1.index < fi2.index);
    }
};

// Sort a feature info by its frequency.
//___________________________________________________________________
struct CompFIByFreq
{
    bool operator()(const FeatureInfo & fi1,const FeatureInfo & fi2) const
    {
        if (fi1.dFreq > fi2.dFreq) return true;
        if (fi1.dFreq < fi2.dFreq) return false;
        return (fi1.stTerm.compare(fi2.stTerm) < 0);
    }
};

// Declaration of the feature set.
// During the various training and attribution procedures, each
// document is represented by its correspondent feature set.
//___________________________________________________________________
class FeatureSet
{
    protected:

		// The feaures vector.
        vector<ActiveItem> vActiveSet;
        int iActiveItems;

		// The relevance of each feature.
		// Used for computing the directional component.
        vector<double> vMyRelWeights;

		// The total weight of this feature set, according to types.
		// Used for normalizing scores.
        double dMinMaxWeight;
        double dCosineWeight;

    public:

        FeatureSet():vActiveSet(1),iActiveItems(0){}

        virtual ~FeatureSet() {}

        virtual const string & stGetAuthorName() const=0;
        virtual const string & stGetClassName() const=0;
        virtual void SetRandomFeatures(const vector<bool> & vRandomSet);
        const vector<ActiveItem> & vGetActiveSet() const {return vActiveSet;}
        int iGetActiveItems() const {return iActiveItems;}
        virtual int iGetTopFtrs(vector<FeatureItem> & vFtrs) const {return -1;}
        const double & dGetMinMaxNorm() const {return dMinMaxWeight;}
        const double & dGetCosineNorm() const {return dCosineWeight;}
        virtual double getAveragePrec(const vector<double> & vRelWeight) const;
        virtual const FeatureSet * getStaticSet() const=0;
        virtual const vector<FeatureItem> & vGetStaticSet() const=0;
        virtual int iGetStaticSetSize() const=0;
};

// A declaration of the static feature set.
// Stores the static information of the features.
//___________________________________________________________________
class StaticSet: public FeatureSet
{
    private:
        
		// The feature vector.
        vector<FeatureItem> vStaticSet;
        int iStaticItems;
        
		// The document name associated with this feature set.
        const string stAuthorName;
        const string stClassName;
        
        void SetData(const map<string,FeatureInfo,LexComp> * lexicon,
                     const vector<string> & vTermVec,
                     const vector<string> & vWeightVec,
                     const vector<string> & vIdfVec,
                     const int iFtrsNumber);

        // Avoid copy operations.
        StaticSet(const StaticSet&);
        void operator=(const StaticSet&);

    public:

        StaticSet(const map<string,FeatureInfo,LexComp> * lexicon=NULL,
                  const string & stFeaturePath="",
                  const string & _stAuthorName="",
                  const string & _stClassName="");
        
        StaticSet(const map<string,FeatureInfo,LexComp> * lexicon,
                  const vector<string> & vFtrsVec,
                  const int iFtrsNumber,
                  const string & _stAuthorName="",
                  const string & _stClassName="");
        
        virtual ~StaticSet() {}
                                 
        virtual const string & stGetAuthorName() const {return stAuthorName;}
        virtual const string & stGetClassName() const {return stClassName;}
        virtual int iGetTopFtrs(vector<FeatureItem> & vFtrs) const;
        virtual const FeatureSet * getStaticSet() const {abort_err("getStaticSet called from StaticSet"); return NULL;}
        virtual const vector<FeatureItem> & vGetStaticSet() const {return vStaticSet;}
        virtual int iGetStaticSetSize() const {return iStaticItems;}
};

// A declaration of the random feature set.
// Used for representing the random features reprtesenting
// a document in each iteration.
//___________________________________________________________________
class RandomSet: public FeatureSet
{
    private:

		// Points to the associated static feature set.
        const StaticSet * const featureSet;

        // Avoid copy operations.
        RandomSet(const RandomSet&);
        void operator=(const RandomSet&);

    public:

        RandomSet(const StaticSet * _featureSet=NULL):featureSet(_featureSet){}

        virtual ~RandomSet() {}

        virtual const string & stGetAuthorName() const {return featureSet->stGetAuthorName();}
        virtual const string & stGetClassName() const {return featureSet->stGetClassName();}
        virtual const FeatureSet * getStaticSet() const {return featureSet;}
        virtual const vector<FeatureItem> & vGetStaticSet() const {return featureSet->vGetStaticSet();}
        virtual int iGetStaticSetSize() const {return featureSet->iGetStaticSetSize();}
};

#endif
