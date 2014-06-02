/*
  Declaration of an auxiliary buffer that is used
  by the various attribution procedures.
*/
#ifndef __LRE_AUX_BUFFER__
#define __LRE_AUX_BUFFER__

#include <map>
#include <set>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <FeatureSet.h>

using namespace std;

#define MIN_SCORE -1000000.0

// An attributed candidate.
//___________________________________________________________________
class CandRes
{
    public:

        double dScore;
        string stName;
        
        virtual ~CandRes() {}
        
        CandRes() {}
                       
        void operator=(const CandRes & item)
        {
            dScore=item.dScore;
            stName=item.stName;
        }
        
        CandRes(const CandRes & item){(*this)=item;}
};

//___________________________________________________________________
struct CandsOrder
{
    bool operator()(const CandRes & c1,const CandRes & c2) const
    {
        if (c1.dScore > c2.dScore) return true;
        if (c1.dScore < c2.dScore) return false;
        return (c1.stName.compare(c2.stName) < 0);
    }
};

// Attribution auxiliary buffer - base class.
//___________________________________________________________________
class LREAuxBuffer
{
    private:

        // Avoid copy operations.
        LREAuxBuffer(const LREAuxBuffer&);
        void operator=(const LREAuxBuffer&);

        const double dScoreFloor;
        
    protected:
        
        // Maintain the best candidate during the attribution.
        double dBestScore;
        string stBestCand;

        // Formatting of the candidate name.
        // For computing the attribution results.
        const int iStartIndex;
        const int iNameKeyLength;

    public:

        LREAuxBuffer();
        virtual ~LREAuxBuffer() {}

        double dGetScoreFloor() const { return dScoreFloor; }

        virtual string stGetSubFolder(const string & stPrefix) const {return (stPrefix+"\\Ftrs\\");}
        virtual void initSearch() {dBestScore=MIN_SCORE;stBestCand="XXXXX";}
        virtual void setRandomFtrs(const vector<bool> & vRandomSet) {return;}
        virtual int getGroup(const string & stAnonym,vector<FeatureSet*> & vGroupItems) const {return -1;}

        // Load the set of candidate writers for an attribution iteration.
        virtual int getCandsSet(const string              & stAnonym,
                                const vector<FeatureSet*> & vAuthors,
                                const int                   iAuthorsNumber,
                                vector<FeatureSet*>       & vCandidates) const;

        // Updathe the best candidate found in an attribution iteration.
        virtual void updateResult(const double dScore,const FeatureSet * fsCand);

        // Print the results of this iteration.
        virtual void printResult(const FeatureSet * fsAnonym,FILE * resFile);

        // Extract the random feature sets of each document, per iteration.
        static void SetRandomFeatures(const vector<bool>  & vRandomSet,
                                      vector<FeatureSet*> & vec,
                                      const int             iVecSize);
};

// Attribution auxiliary buffer - omit the actual writer of the anonimous
// document, for computing the true-negative performance in the worst case
// scenario.
//___________________________________________________________________
class LRENoAuthor: public LREAuxBuffer
{
    private:

        // Avoid copy operations.
        LRENoAuthor(const LRENoAuthor&);
        void operator=(const LRENoAuthor&);

        map<string, set<string,LexComp> ,LexComp> avoidAuthors;

    public:

        LRENoAuthor();
        virtual ~LRENoAuthor(){}

        virtual int getCandsSet(const string              & stAnonym,
                                const vector<FeatureSet*> & vAuthors,
                                const int                   iAuthorsNumber,
                                vector<FeatureSet*>       & vCandidates) const;
};

// Attribution auxiliary buffer - the candidate authors may not contain
// the actual writer.
//___________________________________________________________________
class LREOpenSet: public LREAuxBuffer
{
    private:

        map<string,FeatureSet*,LexComp> authors;
        const map<string,FeatureInfo,LexComp> * const lexicon;

        LREOpenSet(const LREOpenSet&);
        void operator=(const LREOpenSet&);

    public:

        LREOpenSet(const map<string,FeatureInfo,LexComp> * _lexicon);
        virtual ~LREOpenSet();

        virtual int getCandsSet(const string              & stAnonym,
                                const vector<FeatureSet*> & vAuthors,
                                const int                   iAuthorsNumber,
                                vector<FeatureSet*>       & vCandidates) const;
        virtual void setRandomFtrs(const vector<bool> & vRandomSet);
};

// Attribution auxiliary buffer - used for regular IR procedure (with no iterations).
//___________________________________________________________________
class LREClassify: public LREAuxBuffer
{
    private:

        string stBestAuthor;

        LREClassify(const LREClassify&);
        void operator=(const LREClassify&);

    public:

        LREClassify():LREAuxBuffer() {}
        virtual ~LREClassify() {}
        
        virtual string stGetSubFolder(const string & stPrefix) const {return (stPrefix+"\\");}
        virtual void updateResult(const double dScore,const FeatureSet * fsCand);
        virtual void printResult(const FeatureSet * fsAnonym,FILE * resFile);
};

#endif
