#include <stdlib.h>
#include <ResourceBox.h>
#include <FileScanner.h>
#include <auxiliary.h>

#pragma warning(disable : 4786)

using namespace std;

//___________________________________________________________________
ResourceBox::~ResourceBox()
{
    if (resourceBoxP != NULL)
        delete resourceBoxP;
}

//___________________________________________________________________
ResourceBox::ResourceBox(const string & st, const map<string,string,LexComp> * paramsMap)
{
    this->load(st,paramsMap);
}

//___________________________________________________________________
void ResourceBox::SetParams(string & stVal, const map<string,string,LexComp> * paramsMap) const
{
    if (paramsMap == NULL) return;

    map<string,string,LexComp>::const_iterator iter = paramsMap->begin();

    while (iter != paramsMap->end())
    {
        while (ReplaceStr(stVal,string("<")+iter->first+">",iter->second));
        iter++;
    }
}

//___________________________________________________________________
void ResourceBox::load(const string & st,const map<string,string,LexComp> * paramsMap)
{
    if (st.empty())
    {
        std::cout<<"Empty resource file"<<std::endl;
        exit(1);
    }
    
    LinesReader fileReader(st,1024);
    string line;
    size_t tab;
    string key,val;

    while (fileReader.bMoreToRead())
    {
        fileReader.voGetLine(line);
        
        if (line.empty()) continue;
        
        tab = line.find_first_of('\t');
        if (tab == string::npos) abort_err(string("illegal resource line: ")+line+" in file: "+st+", param="+line);
        key = line.substr(0,tab);
        val = line.substr(tab+1);
        
        if (box.find(key) != box.end()) continue;

        SetParams(val,paramsMap);

        if (key.compare("#include") == 0)
        {
            this->load(val,paramsMap);
            continue;
        }

        box.insert(map<string,string,LexComp>::value_type(key,val));
    }
}

//___________________________________________________________________
ResourceBox * ResourceBox::Get(const string & st,const map<string,string,LexComp> * paramsMap)
{
    if (resourceBoxP != NULL)
        return resourceBoxP;
    resourceBoxP = new ResourceBox(st,paramsMap);
    if (resourceBoxP == NULL)
    {
        std::cout<<"Failed to load the ResourceBox"<<std::endl;
        exit(1);
    }
    return resourceBoxP;
}

//___________________________________________________________________
const string & ResourceBox::getStringValue(const string & key) const
{
    map<string,string,LexComp>::const_iterator item  = box.find(key);

    if (item == box.end())
    {
        std::cout<<"The resource "<<key<<" does not exist"<<std::endl;
        exit(1);
    }
    return item->second;
}

//___________________________________________________________________
double ResourceBox::getDoubleValue(const string & key) const
{
    map<string,string,LexComp>::const_iterator item  = box.find(key);
    double d;

    if (item == box.end())
    {
        std::cout<<"The resource "<<key<<" does not exist"<<std::endl;
        exit(1);
    }

    if (string_as<double>(d,item->second,std::dec))
        return d;
    std::cout<<"The resource "<<key<<" could not be transformed into double\n";
    exit(1);
    return 0.0;
}

//___________________________________________________________________
int ResourceBox::getIntValue(const string & key) const
{
    map<string,string,LexComp>::const_iterator item  = box.find(key);
    int i;

    if (item == box.end())
    {
        std::cout<<"The resource "<<key<<" does not exist"<<std::endl;
        exit(1);
    }

    if (string_as<int>(i,item->second,std::dec))
        return i;
    std::cout<<"The resource "<<key<<" could not be transformed into int\n";
    exit(1);
    return 0;
}

//___________________________________________________________________
void ResourceBox::getListValues(const string & stKey,list<string> & valuesList) const
{
    map<string,string,LexComp>::const_iterator item  = box.find(stKey);

    if (item == box.end())
    {
        fprintf(stderr,"The resource %s does not exist\n",stKey.c_str());
        fflush(stderr);
        exit(1);
    }

    valuesList.clear();
    string stItem;
    string stValues = item->second;
    size_t index = stValues.find_first_of(',');
    while (index != string::npos)
    {
        stItem = stValues.substr(0,index);
        valuesList.push_back(stItem);
        stValues = stValues.substr(index+1);
        index = stValues.find_first_of(',');
    }
    if (!stValues.empty()) valuesList.push_back(stValues);
}

//___________________________________________________________________
bool ResourceBox::isExists(const string & key) const
{
    map<string,string,LexComp>::const_iterator item  = box.find(key);
    return (item != box.end());
}
