/*
  Declaration of the feature handler.
  This class is used for extracting features from documents.
*/
#ifndef __FeaturesHandler__
#define __FeaturesHandler__

#include <FileScanner.h>
#include <set>
#include <map>
#include <list>
#include <FeatureSet.h>
#include <ThesisAux.h>

using namespace std;

/*
  This class is used for reading text files and extracting
  the features.
*/
class FeaturesHandler
{
    protected:

		// Map each term to its accumulated information, during the training.
        map<string,FeatureInfo,LexComp> lexicon;

		// Map an index to its associated term.
        map<int,string> i2fLex;

		// Indicates whether to transform the text to lower-case.
        const bool bCaseSensitive;

		// Indicates whether to use words as feautures.
        const bool bUseContentWords;

		// Indicates whether to print the lexicon in a frequency-sorted order.
        const bool bSortTermByFreq;

		// Indicates whether to use cross-words features.
		const bool bGetFeatureByWindow;

		// Indicates whether to omit punctuation marks or not.
        const bool bConsiderPunctuations;

		// The length of each character-n-gram.
        const unsigned int iNGram;

		// The number of documents in the training corpus.
        const int iTypesNumber;

        // A feature that appears in the less documents than this
        // parameter will be ignored.
        const int iMinDocsFreq;

        // Defines the minimal difference between the total occurences of a
        // feature to the number of documents in which it appears.
        // E.g. For avoiding features that appears only once in each document.
        const int iMinFreq2DocDiff;

        // Defines the minimal length of a document that will be considered during
        // the feature extraction process.
		const int iMinWindowLength;

        // When cross-words features are used, this parameters defines the size of
        // a window (in words).
		const int iMinWindowWords;

        // Define the association measure in use (e.g. tf-idf, term-frequency, etc.)
        const string stAssocMeasure;

        // Various corpus info counters.
        int iCorpusSize;
        int iDocsCount;
        int iDocSize;
        int iFeatureIndex;

        // Read a (space-free) word.
        virtual void GetWord(FileScanner & fsReader,
                             string      & stWord);

        // Load and handle a feature.
        virtual void HandleFeature(const string & stFeature,
			                       FeatureType featureType,
                                   set<string,LexComp> & termsMarker)=0;

        // Init/End fucntions when handling a document.
        virtual void InitFreq(list<FeatureItem> * featuresList)=0;
        virtual void SetFreq(list<FeatureItem> * featuresList,FILE * debug_log) const=0;
        
        // Special characters classification.
        virtual bool bIsWhiteSpace(const char c) const;
        virtual bool bIsCWEnd(const char c) const;
        virtual bool bIsWordEnd(const char c) const;

        // Functions that process the features.
		virtual void GetFeatureByWindow(FileScanner & fsReader, string & stWindow, const string & stPrevWindow, string & stPrevWord, set<string,LexComp> & termsMarker);
        virtual void BreakToken(const string & stToken, list<EncodedToken> & tokens, string & stWindow) const;
        virtual void HandleToken(const vector<string> & vToken, const unsigned int iLetters, set<string,LexComp> & termsMarker);

    private:

        // Avoid copy operations.
        FeaturesHandler(const FeaturesHandler&);
        void operator=(const FeaturesHandler&);

    public:

        FeaturesHandler(const int _iTypesNumber);
        virtual ~FeaturesHandler();

        // This fucntion gets a document, extracts its features,
        // and transforms it into vector.
        virtual void DocToVec(const string      & stFileName,
                              list<FeatureItem> * featuresList,
                              set<string,LexComp> & termsMarker,
                              FILE * debug_log);
                              
        // Print the feature set / lexicon.
        virtual int iPrintFeatureSet(const string & stFileName,
                                     FILE * debug_log) const;

        // Load the feature set / lexicon.
        virtual void voLoadFeatureSet(const string & stFileName,
                                      FILE * debug_log);
                                 
        const map<string,FeatureInfo,LexComp> * getLexicon() const {return &lexicon;}
        virtual int iGetLexiconSize() const {return lexicon.size();}
        int iGetIndex(const string & stFeature) const;
        const string & stGetFeature(const int iTermIndex) const;
        int iGetCorpusSize() const {return iCorpusSize;}
        int iGetDocsCount() const {return iDocsCount;}
        int iGetDocSize() const {return iDocSize;}
        int iGetFeaturesCount() const {return (iFeatureIndex-1);}
        
        const FeatureInfo & getFeatureInfo(const int iFeatureIndex) const;
        virtual void Reset();
};

/*
  This class is used for extracting features from a training corpus.
  The outpus of this class is a global lexicon, which store all features
  and their relevant information.
*/
//___________________________________________________________________
class ExtractFeatures: public FeaturesHandler
//___________________________________________________________________
{
    private:

        ExtractFeatures(const ExtractFeatures&);
        void operator=(const ExtractFeatures&);

    protected:

        virtual void HandleFeature(const string & stFeature,
			                       FeatureType featureType,
                                   set<string,LexComp> & termsMarker);
        virtual void InitFreq(list<FeatureItem> * featuresList) {iDocSize=0;}
        virtual void SetFreq(list<FeatureItem> * featuresList,FILE * debug_log) const {}

    public:

        ExtractFeatures(const int _iTypesNumber=-1):FeaturesHandler(_iTypesNumber){}
        virtual ~ExtractFeatures(){}
};

/*
  This class is used for transforming a document into feature vector.
*/
//___________________________________________________________________
class CountFeatures: public FeaturesHandler
//___________________________________________________________________
{
    private:

        map<int,int> featureFreq;
        
    protected:

        virtual void HandleFeature(const string & stFeature,
			                       FeatureType featureType,
                                   set<string,LexComp> & termsMarker);
        virtual void InitFreq(list<FeatureItem> * featuresList);
        virtual void SetFreq(list<FeatureItem> * featuresList,FILE * debug_log) const;

    private:

        CountFeatures(const CountFeatures&);
        void operator=(const CountFeatures&);

    public:

        CountFeatures(const int _iTypesNumber=-1):FeaturesHandler(_iTypesNumber){}
        virtual ~CountFeatures(){}
        virtual void Reset();
};

#endif
