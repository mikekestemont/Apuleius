#include <auxiliary.h>
#include <ResourceBox.h>
#include <SimilarityMeasure.h>
#include <math.h>

using namespace std;

//___________________________________________________________________
void DirectionalMatch::UpdateRelevance(vector<double> & vRelWeights,
                                       const int index,
                                       const double relevance) const
{
    try
    {
        vRelWeights.at(index)=relevance;
    }
    catch (...)
    {
        abort_err(string("Illegal Vec access in DirectionalMatch::UpdateRelevance: index=")+stringOf(index));
    }
}

//___________________________________________________________________
double DirectionalMatch::DirectionalWeight(const FeatureSet * u,
                                           const vector<double> & vRelWeights) const
{
    return u->getAveragePrec(vRelWeights);
}

//___________________________________________________________________
SimilarityMeasure::SimilarityMeasure():directional(NULL)
{
    const string stDirectionType = ResourceBox::Get()->getStringValue("DirectionalMatch");
    
    if (stDirectionType.compare("sym") == 0) directional = new SymmetricMatch;
    else if (stDirectionType.compare("drc") == 0) directional = new DirectionalMatch;
    else abort_err(string("Illegal direction type in SimilarityMeasure: ")+stDirectionType);
}

//___________________________________________________________________
double SimilarityMeasure::SJ(const FeatureSet * u,
                             const FeatureSet * v,
                             vector<double> & vRelWeights,
                             FILE * debug_log) const
{
    double dScore = 0.0;
    int iv=0;
    int iu=0;
    const ActiveItem * aiv=NULL;
    const ActiveItem * aiu=NULL;
    const vector<ActiveItem> & uVec = u->vGetActiveSet();
    const vector<ActiveItem> & vVec = v->vGetActiveSet();
    const int uSize = u->iGetActiveItems();
    const int vSize = v->iGetActiveItems();

    while ((iv<vSize) && (iu<uSize))
    {
        try
        {
            aiu=&uVec.at(iu);
            aiv=&vVec.at(iv);
        }
        catch (...)
        {
            abort_err(string("Illegal Vec access in SJ: iu=")+stringOf(iu)+", iv="+stringOf(iv));
        }
        
        if (aiu->index == aiv->index)
        {
            iu++;
            iv++;
            
            dScore += this->Joint(aiu->weight,aiv->weight);
            directional->UpdateRelevance(vRelWeights,aiu->rank,aiv->relevance);
        }
        else if (aiu->index < aiv->index)
        {
            while ((iu < uSize) && (aiu->index < aiv->index))
            {
                directional->UpdateRelevance(vRelWeights,aiu->rank,0.0);

                try
                {
                    iu++;
                    aiu=&uVec.at(iu);
                }
                catch (...)
                {
                    abort_err(string("Illegal Vec access in SJ: iu=")+stringOf(iu));
                }
            }
        }
        else
        {
            try
            {
                iv++;
                while ((iv < vSize) && (vVec.at(iv).index < aiu->index)) iv++;
            }
            catch (...)
            {
                abort_err(string("Illegal Vec access in SJ: iv=")+stringOf(iv));
            }
            
        }
        //fprintf(debug_log,"v=%d, u=%d, x=%.7f, y=%.7f, dEntry=%.7f, Score=%.7f\n",aiv->index,aiu->index,x,y,dEntry,dScore); fflush(debug_log);
    }
    
    return dScore;
}

//___________________________________________________________________
double CosineMatch::SimilarityScore(const FeatureSet * u,
                                    const FeatureSet * v,
                                    vector<double>   & vRelWeights,
                                    FILE * debug_log,
                                    const double dNormFloor) const
{
    double dScalar,dVal;
    double dNorm = u->dGetCosineNorm();
    dNorm *= v->dGetCosineNorm();

    if (dNorm <= 0) return 0.0;

    dNorm = sqrt(dNorm);
    
    dVal = this->SJ(u,v,vRelWeights,debug_log);

    dVal /= dNorm;

    if (dVal < dNormFloor) dVal = dNormFloor;
    dScalar = log(dVal);
    dVal = directional->DirectionalWeight(u,vRelWeights);
    if (dVal < dNormFloor) dVal = dNormFloor;
    dScalar += log(dVal);
    
    return dScalar;
}

//___________________________________________________________________
double MinMaxMatch::SimilarityScore(const FeatureSet * u,
                                    const FeatureSet * v,
                                    vector<double>   & vRelWeights,
                                    FILE * debug_log,
                                    const double dNormFloor) const
{
    double dVal,dScore;

    dVal = this->SJ(u,v,vRelWeights,debug_log);

    if (dVal < dNormFloor) dVal = dNormFloor;
    dScore = log(dVal);
    dVal = directional->DirectionalWeight(u,vRelWeights);
    if (dVal < dNormFloor) dVal = dNormFloor;
    dScore += log(dVal);

    return dScore;
}

//___________________________________________________________________
double DistanceMatch::SJ(const FeatureSet * u,
                         const FeatureSet * v,
                         vector<double> & vRelWeights,
                         FILE * debug_log) const
{
    double dScore = 0.0;
    int iItems=0;
    double x,y;
    int iv=0;
    int iu=0;
    const ActiveItem * aiv=NULL;
    const ActiveItem * aiu=NULL;
    const vector<ActiveItem> & uVec = u->vGetActiveSet();
    const vector<ActiveItem> & vVec = v->vGetActiveSet();
    const int uSize = u->iGetActiveItems();
    const int vSize = v->iGetActiveItems();

    while ((iv<vSize) && (iu<uSize))
    {
        try
        {
            aiu=&uVec.at(iu);
            aiv=&vVec.at(iv);
        }
        catch (...)
        {
            abort_err(string("Illegal Vec access in SJ: iu=")+stringOf(iu)+", iv="+stringOf(iv));
        }
        
        if (aiu->index == aiv->index)
        {
            x = aiu->weight;
            y = aiv->weight;
            
            iu++;
            iv++;
            
            directional->UpdateRelevance(vRelWeights,aiu->rank,aiv->relevance);
        }
        else if (aiu->index < aiv->index)
        {
            x = aiu->weight;
            y = 0.0;
            
            iu++;

            directional->UpdateRelevance(vRelWeights,aiu->rank,0.0);
        }
        else
        {
            x = 0.0;
            y = aiv->weight;
            
            iv++;
        }
        
        dScore += this->Joint(x,y);
        iItems++;

        //fprintf(debug_log,"v=%d, u=%d, x=%.7f, y=%.7f, dEntry=%.7f, Score=%.7f\n",aiv->index,aiu->index,x,y,dEntry,dScore); fflush(debug_log);
    }

    this->NormalizeProduct(dScore,iItems);
    
    return dScore;
}

//___________________________________________________________________
double DistanceMatch::SimilarityScore(const FeatureSet * u,
                                      const FeatureSet * v,
                                      vector<double>   & vRelWeights,
                                      FILE * debug_log,
                                      const double dNormFloor) const
{
    double dScore;
    
    //fprintf(debug_log,"DistScore: %s -> %s\n",u->stGetAuthorName().c_str(),v->stGetAuthorName().c_str()); fflush(debug_log);
    
    dScore = (1.0 - this->SJ(u,v,vRelWeights,debug_log));
    
    //fprintf(debug_log,"Final Score = %.7f\n",dScore); fflush(debug_log);
    
    dScore *= directional->DirectionalWeight(u,vRelWeights);

    //fprintf(debug_log,"Drc: %.7f => %.7f\n",dDrc,dScore); fflush(debug_log);

    return log(dScore);
}
