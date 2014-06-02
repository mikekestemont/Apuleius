#ifndef __ResourceBox__
#define __ResourceBox__

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <list>
#include <auxiliary.h>

#pragma warning(disable : 4786)

using namespace std;

class ResourceBox;

static ResourceBox * resourceBoxP = NULL;

class ResourceBox
{
    private:

        ResourceBox();
        ResourceBox(const string & st,const map<string,string,LexComp> * paramsMap);
        ResourceBox(const ResourceBox&);
        void operator=(const ResourceBox&);

        ~ResourceBox();

        void load(const string & st,const map<string,string,LexComp> * paramsMap);
        void SetParams(string & val, const map<string,string,LexComp> * paramsMap) const;

        map<string,string,LexComp> box;

    public:

        static ResourceBox * Get(const string & st="", const map<string,string,LexComp> * paramsMap=NULL);
        const string & getStringValue(const string & key) const;
        double getDoubleValue(const string & key) const;
        int getIntValue(const string & key) const;
        void getListValues(const string & stKey,list<string> & valuesList) const;
        bool isExists(const string & key) const;
};

#endif
