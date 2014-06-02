#ifndef __PAIR_CLASSIFIER__
#define __PAIR_CLASSIFIER__

#include <stdio.h>
#include <stdlib.h>
#include <FeatureSet.h>
#include <vector>
#include <string>
#include <Timer.h>
#include <auxiliary.h>
#include <LREThreadBuffer.h>
#include <SimilarityMeasure.h>

using namespace std;

//___________________________________________________________________
// This function calculates the average similarity of the given
// impostors window to its correspondent pair of snippets.
//___________________________________________________________________
double DecoyWindowSim(const string & stDecoysFile,
                      const int iFirstIndex,
                      const int iWindowSize);

//___________________________________________________________________
// Defintion of the pair classifier objects.
//___________________________________________________________________
class PairClassifier
{
    protected:
        
        // Files & Folders names.
        const string stSampleFolder;
        const string stFeatureSet;
        const string stScriptFile;
        const string stDecoyFile;
        const string stDecoyFolder;
        const string stSimilarDecoyFolder;
        const string stLogsFolder;
        const string stSortedDecoysFile;
        
        // Various data sizes.
        const int iIRLoops;
        const int iPairsNumber;
        const int iReqRandDecoyNum;
        const int iDecoyWindowSize;
        const int iDecoyWindowIndex;
        const int iMultiProcDegree;
        const int iNameKeyLength;
        const int iStartNameIndex;
        
        const double dRandFtrsSetFreq;
        
        // Dynamic buffers/Counters (updates upon pair/session).
        int iRandFtrsSetNum;
        int iSnipsNumber;
        int iDecoysNumber;
        double dRandDecoysRate;
        
        string stPairEntryKey;
        string stPairTargetKey;

        FILE * debug_log;
        FILE * fPairClassRes;
        
        // The Feature Sets buffers.
        vector<FeatureSet*> vBeginSnips;
        vector<FeatureSet*> vEndSnips;
        vector<FeatureSet*> vDecoySnips;
        map<string,FeatureSet*,LexComp> decoysMap;
        
        // For supporting multithreading.
        LREThreadBuffer * buffer;
        
        // The similarity function.
        SimilarityMeasure * simMatch;

        // Declarations of member functions:

        // This function gets a pair pf snippets and a set
        // of impostors and checks, based on this information,
        // whether it's a same-author pair or not.
        virtual void RankPairByDecoys(const FeatureSet * fsX,
                                      const FeatureSet * fsY,
                                      const vector<FeatureSet*> & vRandomDecoys,
                                      const int iRandomDecoys);
        virtual void ClassifyPair(FeatureSet * fsX,FeatureSet * fsY)=0;
        virtual double GetScore(const FeatureSet * fsX,
                                const FeatureSet * fsY,
                                const FeatureSet * fsDecoy,
                                vector<double> & vRelWeights,
                                const double dScoreFloor) const;
        virtual void SetSearchKeys(const FeatureSet * fsX, const FeatureSet * fsY);

        // This function loads all pairs of snippets (begin / end).
        virtual int LoadPairSnips(const string & stLocalScript);
        
        // This function loads the map of impostors.
        virtual void LoadDecoysMap();
        
        // Randomly select a set of impostors for a match iteration.
        virtual int SelectRandomDecoys(vector<FeatureSet*> & vRandomDecoys) const;
                               
        // Assign the random features to all items in the impostors map.
        virtual void SetRandomFeatures(const vector<bool> & vRandomSet);
                               
        // These functions load the set of impostors for a given pair,
        // either from a file or from a map.
        virtual int GetDecoys(const string & stSelDecoyFile);
        virtual int LoadDecoys(const string & stSelDecoyFile,
                               const string & stPairKey,
                               const int iMaxDecoysNumber);
        virtual void LoadPrivateDecoys(vector< vector<string> * > & vFtrsSets,
                                       const vector<int> & vSetSize,
                                       const vector<string> & vFileName,
                                       const int iPrivateDecoysNumber);
        
        // Load a global feature set.
        virtual void LoadFtrsSet(const vector<string> * vFtrsPool=NULL,
                                 const int iFtrsPoolSize=-1);
                               
        virtual int ReadFeatureFile(const string & stFeatureFile,
                                    vector<string> & vFtrsVec,
                                    const int iBlockSize,
                                    const bool bSkipHeader) const;
                                    
        virtual int ReadDecoySet(vector< vector<string> * > & vFtrsSets,
                                 vector<int> & vSetSize,
                                 vector<string> & vFileName,
                                 const string & stWorkFolder,
                                 const string & stWorkFile) const;

        // Classification stages:
                               
        virtual string stGetClassifier() const=0;
        virtual void InitClassify()=0;
        virtual void DoClassify()=0;
        virtual void EndClassify();

    private:
        
        // Avoid copy operations.
        PairClassifier(const PairClassifier&);
        void operator=(const PairClassifier&);
        
    public:
        
        PairClassifier(LREThreadBuffer * _buffer);
        virtual ~PairClassifier(){if (simMatch != NULL) delete simMatch;}

        static int LoadPairSnips(vector<FeatureSet*> & v1,
                                 vector<FeatureSet*> & v2,
                                 const string & stFolder,
                                 const string & stFile,
                                 const map<string,FeatureInfo,LexComp> * lexicon);
        
        virtual void ClassifyPairs();
};

//___________________________________________________________________
class RandomDecoyClassifier: public PairClassifier
{
    protected:
        
        virtual void ClassifyPair(FeatureSet * fsX,FeatureSet * fsY) {abort_err("ClassifyPair(X,Y) not implemented for RandomDecoyClassifier");}
        virtual void InitClassify();
        virtual void DoClassify();
        virtual void EndClassify();
        
    private:
        
        // Avoid copy operations.
        RandomDecoyClassifier(const RandomDecoyClassifier&);
        void operator=(const RandomDecoyClassifier&);
        
    public:
        
        RandomDecoyClassifier(LREThreadBuffer * _buffer):PairClassifier(_buffer){}
        virtual ~RandomDecoyClassifier(){}

        virtual string stGetClassifier() const {return "Random Decoys";}
};

//___________________________________________________________________
class SimilarDecoyClassifier: public PairClassifier
{
    protected:
        
        virtual void ClassifyPair(FeatureSet * fsX,FeatureSet * fsY);
        virtual void InitClassify();
        virtual void DoClassify();
        virtual void EndClassify();
        
    private:
        
        // Avoid copy operations.
        SimilarDecoyClassifier(const SimilarDecoyClassifier&);
        void operator=(const SimilarDecoyClassifier&);
        
    public:
        
        SimilarDecoyClassifier(LREThreadBuffer * _buffer):PairClassifier(_buffer){}
        virtual ~SimilarDecoyClassifier(){}

        virtual string stGetClassifier() const {return "Similar Decoys";}
};

//___________________________________________________________________
class GenreDecoyClassifier: public SimilarDecoyClassifier
{
    protected:
        
        virtual void ClassifyPair(FeatureSet * fsX,FeatureSet * fsY);
        virtual double GetScore(const FeatureSet * fsX,
                                const FeatureSet * fsY,
                                const FeatureSet * fsDecoy,
                                vector<double> & vRelWeights,
                                const double dScoreFloor) const;
        
    private:
        
        // Avoid copy operations.
        GenreDecoyClassifier(const GenreDecoyClassifier&);
        void operator=(const GenreDecoyClassifier&);
        
        virtual void ClassifyPair(FeatureSet * fsX,
                                  FeatureSet * fsY,
                                  const string & stDirection);
        
    public:
        
        GenreDecoyClassifier(LREThreadBuffer * _buffer):SimilarDecoyClassifier(_buffer){}
        virtual ~GenreDecoyClassifier(){}

        virtual string stGetClassifier() const {return "Select Decoys By Genre";}
};

//___________________________________________________________________
class PlainSimilarityClassifier: public PairClassifier
{
    protected:
        
        virtual void ClassifyPair(FeatureSet * fsX,FeatureSet * fsY);
        virtual void InitClassify();
        virtual void DoClassify();
        virtual void EndClassify();
        
    private:
        
        // Avoid copy operations.
        PlainSimilarityClassifier(const PlainSimilarityClassifier&);
        void operator=(const PlainSimilarityClassifier&);
        
    public:
        
        PlainSimilarityClassifier(LREThreadBuffer * _buffer):PairClassifier(_buffer){}
        virtual ~PlainSimilarityClassifier(){}

        virtual string stGetClassifier() const {return "Plain Similarity";}
};

//___________________________________________________________________
class WebDecoyClassifier: public PairClassifier
{
    protected:
        
        virtual void ClassifyPair(FeatureSet * fsX,FeatureSet * fsY);
        virtual void InitClassify(){}
        virtual void DoClassify();
        
    private:
        
        // Avoid copy operations.
        WebDecoyClassifier(const WebDecoyClassifier&);
        void operator=(const WebDecoyClassifier&);
        
    public:
        
        WebDecoyClassifier(LREThreadBuffer * _buffer):PairClassifier(_buffer){}
        virtual ~WebDecoyClassifier(){}
        virtual string stGetClassifier() const {return "Web Decoys";}
};

#endif
