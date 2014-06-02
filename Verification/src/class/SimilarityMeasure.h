#ifndef __SIMILARITY_MEASURE__
#define __SIMILARITY_MEASURE__

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <FeatureSet.h>

using namespace std;

//___________________________________________________________________
// These objects are used for implementing the directional match.
//___________________________________________________________________
class SimDirectionType
{
    public:
        
        SimDirectionType() {}
        virtual ~SimDirectionType() {}
        
        virtual void UpdateRelevance(vector<double> & vRelWeights, const int index, const double dRelevance) const=0;
        virtual double DirectionalWeight(const FeatureSet * u, const vector<double> & vRelWeights) const=0;
};

//___________________________________________________________________
class SymmetricMatch: public SimDirectionType
{
    public:
        
        SymmetricMatch():SimDirectionType() {}
        virtual ~SymmetricMatch() {}
        
        virtual void UpdateRelevance(vector<double> & vRelWeights, const int index, const double dRelevance) const {return;}
        virtual double DirectionalWeight(const FeatureSet * u, const vector<double> & vRelWeights) const {return 1.0;}
};

//___________________________________________________________________
class DirectionalMatch: public SimDirectionType
{
    public:
        
        DirectionalMatch():SimDirectionType() {}
        virtual ~DirectionalMatch() {}
        
        virtual void UpdateRelevance(vector<double> & vRelWeights, const int index, const double dRelevance) const;
        virtual double DirectionalWeight(const FeatureSet * u, const vector<double> & vRelWeights) const;
};


//*******************************************************************
// The various similarity measure objects:

//___________________________________________________________________
class SimilarityMeasure
{
    protected:
        
        SimDirectionType * directional;
        
        virtual double SJ(const FeatureSet * u, const FeatureSet * v, vector<double> & vRelWeights, FILE * debug_log) const;
        virtual void NormalizeProduct(double & dScore, const int iItems) const {return;}
        virtual double Joint(const double x, const double y) const=0;
        
    public:
        
        SimilarityMeasure();
        virtual ~SimilarityMeasure() {if (directional != NULL) delete directional;}

        virtual double SimilarityScore(const FeatureSet * u,
                                       const FeatureSet * v,
                                       vector<double>   & vRelWeights,
                                       FILE * debug_log,
                                       const double dNormFloor) const=0;
};

//___________________________________________________________________
class CosineMatch: public SimilarityMeasure
{
    private:
        
        CosineMatch(const CosineMatch&);
        void operator=(const CosineMatch&);

    protected:
        
        virtual double Joint(const double x, const double y) const {return x*y;}

    public:
        
        CosineMatch():SimilarityMeasure() {}
        virtual ~CosineMatch() {}
        
        virtual double SimilarityScore(const FeatureSet * u,
                                       const FeatureSet * v,
                                       vector<double>   & vRelWeights,
                                       FILE * debug_log,
                                       const double dNormFloor) const;
};

//___________________________________________________________________
class MinMaxMatch: public SimilarityMeasure
{
    private:
        
        MinMaxMatch(const MinMaxMatch&);
        void operator=(const MinMaxMatch&);
        
    protected:

        virtual double Joint(const double x, const double y) const {return ((x<y)?x:y);}

    public:
        
        MinMaxMatch():SimilarityMeasure() {}
        virtual ~MinMaxMatch() {}
        
        virtual double SimilarityScore(const FeatureSet * u,
                                       const FeatureSet * v,
                                       vector<double>   & vRelWeights,
                                       FILE * debug_log,
                                       const double dNormFloor) const;
};

//___________________________________________________________________
class DistanceMatch: public SimilarityMeasure
{
    private:
        
        DistanceMatch(const DistanceMatch&);
        void operator=(const DistanceMatch&);
        
    protected:
        
        virtual double SJ(const FeatureSet * u, const FeatureSet * v, vector<double> & vRelWeights, FILE * debug_log) const;
        virtual void NormalizeProduct(double & dScore, const int iItems) const {dScore/=iItems;}
        virtual double Joint(const double x, const double y) const {return pow( ( (x-y) / (x+y) ) , 2.0 );}

    public:
        
        DistanceMatch():SimilarityMeasure() {}
        virtual ~DistanceMatch() {}
        
        virtual double SimilarityScore(const FeatureSet * u,
                                       const FeatureSet * v,
                                       vector<double>   & vRelWeights,
                                       FILE * debug_log,
                                       const double dNormFloor) const;
};

#endif
