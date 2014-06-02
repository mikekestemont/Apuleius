/*
  Definition of the feature hander's functions.
*/
#include <stdlib.h>
#include <FeaturesHandler.h>
#include <auxiliary.h>
#include <ResourceBox.h>
#include <Timer.h>
#include <math.h>
#include <DataFuncs.h>

//___________________________________________________________________
FeaturesHandler::FeaturesHandler(const int _iTypesNumber):
    bCaseSensitive(ResourceBox::Get()->getIntValue("CaseSensitiveFeature")>0),
    bUseContentWords(ResourceBox::Get()->getIntValue("UseContentWords")>0),
    bSortTermByFreq(ResourceBox::Get()->getIntValue("SortTermByFreq")>0),
    bConsiderPunctuations(ResourceBox::Get()->getStringValue("ConsiderPunctuations").compare("puncs")==0),
	bGetFeatureByWindow(ResourceBox::Get()->getStringValue("GetFeatureByWindow").compare("window")==0),
    iNGram(ResourceBox::Get()->getIntValue("NGramFactor")),
    iMinDocsFreq(ResourceBox::Get()->getIntValue("FeatureDocsFloor")),
    iMinFreq2DocDiff(ResourceBox::Get()->getIntValue("MinFreqToDocsDiff")),
	iMinWindowLength(ResourceBox::Get()->getIntValue("MinWindowLength")),
	iMinWindowWords(ResourceBox::Get()->getIntValue("MinWindowWords")),
    stAssocMeasure(ResourceBox::Get()->getStringValue("TermAssocMeasure")),
    iTypesNumber(_iTypesNumber),
    iCorpusSize(0),
    iDocsCount(0),
    iDocSize(0),
    iFeatureIndex(1)
{
}

//___________________________________________________________________
FeaturesHandler::~FeaturesHandler()
{
}

//___________________________________________________________________
int FeaturesHandler::iGetIndex(const string & stFeature) const
{
    map<string,FeatureInfo,LexComp>::const_iterator iter = lexicon.find(stFeature);
    
    if (iter == lexicon.end())
        return -1;
    return iter->second.index;
}
                                     
//___________________________________________________________________
const string & FeaturesHandler::stGetFeature(const int iTermIndex) const
{
    map<int,string>::const_iterator iter = i2fLex.find(iTermIndex);
    
    if (iter == i2fLex.end())
    {
        fprintf(stderr,"Illegal feature index: %d\n",iTermIndex);
        fflush(stderr);
        exit(0);
    }
    
    return iter->second;
}

//___________________________________________________________________
const FeatureInfo & FeaturesHandler::getFeatureInfo(const int index) const
{
    const string stTerm = this->stGetFeature(index);
    map<string,FeatureInfo,LexComp>::const_iterator iter = lexicon.find(stTerm);
    if (iter == lexicon.end())
    {
        fprintf(stderr,"No Feature Item found for (%d,%s)\n",index,stTerm.c_str());
        fflush(stderr);
        exit(0);
    }
    return iter->second;
}

//___________________________________________________________________
void FeaturesHandler::Reset()
{
    iCorpusSize=0;
    iDocSize=0;
    iFeatureIndex=0;
}

//___________________________________________________________________
bool FeaturesHandler::bIsWhiteSpace(const char c) const
{
    if (c == ' ') return true;
    if (c == '\t') return true;
    if (c == '\n') return true;
    if (c == '\r') return true;
    return false;
}

//___________________________________________________________________
bool FeaturesHandler::bIsCWEnd(const char letter) const
{
    const bool bIsLetter = (((letter >= 'a') && (letter <= 'z')) ||
                            ((letter >= 'A') && (letter <= 'Z')) ||
                            (letter == '\'') ||
                            ((letter >= '0') && (letter <= '9')));
    return !bIsLetter;
}

//___________________________________________________________________
bool FeaturesHandler::bIsWordEnd(const char c) const
{
    if (bUseContentWords) return this->bIsCWEnd(c);
    return this->bIsWhiteSpace(c);
}

//___________________________________________________________________
void FeaturesHandler::GetWord(FileScanner & fsReader,
                              string      & stWord)
{
    bool bInsideWord=false;
    bool bWordEnd=false;
    char cByte;
    bool bWhiteSpace;

    stWord = "";
    while (fsReader.bMoreToRead() && !bWordEnd)
    {
        cByte = fsReader.cGetNextByte();
        bWhiteSpace = this->bIsWordEnd(cByte);

        // No word has been encountered yet.
        if (bWhiteSpace && !bInsideWord)
        {
            continue;
        }

        // The word's end.
        else if (bWhiteSpace)
        {
            bWordEnd=true;
        }

        // Inside a word - accumulate
        else
        {
            bInsideWord=true;
            stWord += cByte;
        }
    }
}

//___________________________________________________________________
void FeaturesHandler::HandleToken(const vector<string> & vToken, const unsigned int iLetters, set<string,LexComp> & termsMarker)
{
    string stFeature;
    unsigned int i,j;

    // Count all letter-n-grams contained in the word.
    //fprintf(stdout,"\t\tHandleToken - Start (%d, %d)\n",iLetters,iNGram); fflush(stdout);
    if ((iLetters <= iNGram) || bUseContentWords)
    {
        stFeature = "";
        for (i=0;i<iLetters;i++)
        {
            stFeature+=vToken.at(i);
        }
        //fprintf(stdout,"\t\t\tShort Token: %s\n",stFeature.c_str()); fflush(stdout);
        this->HandleFeature(stFeature,LetterNgram,termsMarker);
    }
    else
    {
        for (i=0;i<=(iLetters-iNGram);i++)
        {
            stFeature="";
            for (j=i;((j-i)<iNGram);j++)
            {
                stFeature+=vToken.at(j);
            }
            //fprintf(stdout,"\t\t\tLong Token (%d): %s\n",i,stFeature.c_str()); fflush(stdout);
            this->HandleFeature(stFeature,LetterNgram,termsMarker);
        }
    }
    //fprintf(stdout,"\t\tHandleToken - End\n"); fflush(stdout);
}

//___________________________________________________________________
void FeaturesHandler::BreakToken(const string & stToken, list<EncodedToken> & tokens, string & stWindow) const
{
    // Convert the word into a letter-vector.
    // This is done in order to support UTF-8 encoding.
    const unsigned int iBytesNumber = stToken.size();
    unsigned int i;
    string stLetter;
    char cByte;
    bool bIsRegularSign;
    bool bRegularLetter;
    bool bPuncSeq=false;
    EncodedToken eToken(stToken.size());

    //fprintf(stdout,"BreakToken - Start (token=%s, %d)\n",stToken.c_str(),stToken.size()); fflush(stdout);

    // Scan the token's bytes and transform them into UTF-8 letters.
    tokens.clear();
    eToken.Reset();
	stWindow = "";
    for (i=0;i<iBytesNumber;i++)
    {
        // Get the current letter.
        GetUTF8Letter(stToken,stLetter,i);
		stWindow += stLetter;

        // Get the letter's type.
        cByte=stLetter.at(0);
        bIsRegularSign = (((cByte>='a') && (cByte<='z')) ||
                          ((cByte>='A') && (cByte<='Z')) ||
                          ((cByte>='0') && (cByte<='9')));
        bRegularLetter = ((stLetter.size() > 1) || bIsRegularSign);

        //fprintf(stdout,"\tCurrent Letter: %s (i=%d)\n",stLetter.c_str(),i); fflush(stdout);

        // Do not consider punctuations.
        if (!bConsiderPunctuations)
        {
            eToken.AddLetter(stLetter);
        }

        // Deal with regular letter - no punctuation sequence.
        else if (bRegularLetter && !bPuncSeq)
        {
            eToken.AddLetter(stLetter);
        }

        // Regular letter - end punctuation sequence.
        else if (bRegularLetter)
        {
            // End the punctuation sequence.
            if (eToken.iLetters > 0) tokens.push_back(eToken);
            eToken.Reset();
            bPuncSeq=false;

            // Start the regular sequence.
            eToken.AddLetter(stLetter);
        }

        // Non Regular Letter - continue constructing the sequence.
        else if (bPuncSeq)
        {
            eToken.AddLetter(stLetter);
        }

        // Non Regular Letter - end regular sequence.
        else
        {
            // End the regular sequence.
            if (eToken.iLetters > 0) tokens.push_back(eToken);
            eToken.Reset();
            bPuncSeq=true;

            // Start non regular sequence.
            eToken.AddLetter(stLetter);
        }
    }

    // Add the last word.
    if (eToken.iLetters > 0) tokens.push_back(eToken);
    //fprintf(stdout,"BreakToken - End (token=%s)\n",stToken.c_str()); fflush(stdout);
}

//___________________________________________________________________
void FeaturesHandler::DocToVec(const string      & stFileName,
                               list<FeatureItem> * featuresList,
                               set<string,LexComp> & termsMarker,
                               FILE * debug_log)
{
    FileScanner fsReader(stFileName,1024);
    string stWindow(1024,'\0');
    string stPrevWindow(1024,'\0');
    string stPrevWord(1024,'\0');
    list<EncodedToken> tokens;
    list<EncodedToken>::const_iterator iter;

    //fprintf(stdout,"\nFeaturesHandler::DocToVec: %s\n",stFileName.c_str()); fflush(stdout);

    this->InitFreq(featuresList);
    iDocsCount++;
	stPrevWindow = "";
	stPrevWord = "";
    while (fsReader.bMoreToRead())
    {
		if (bGetFeatureByWindow)
		{
			this->GetFeatureByWindow(fsReader,stWindow,stPrevWindow,stPrevWord,termsMarker);
		}
		else
		{
			this->GetWord(fsReader,stWindow);
		}

		if (bUseContentWords && bGetFeatureByWindow) continue;
        
        if (!bCaseSensitive) voToLowerCase(stWindow);
        if (stWindow.size() <= 0) continue;

		//fprintf(stdout,"\tCurrent Window: %s\n",stWindow.c_str()); fflush(stdout);

        // Convert the word into a letter-vector.
        // This is done in order to support UTF-8 encoding.
        this->BreakToken(stWindow,tokens,stPrevWindow);

		// Set the previous window.
        //fprintf(stdout,"\tPrevious Window (1): %s\n",stPrevWindow.c_str()); fflush(stdout);
		voTrimEdges(stPrevWindow," ");
		if (stPrevWindow.size() >= iNGram)
		{
			stPrevWindow = stPrevWindow.substr(stPrevWindow.size() - iNGram + 1);
		}
        //fprintf(stdout,"\tPrevious Window (2): %s\n",stPrevWindow.c_str()); fflush(stdout);

        // Scan all words and extract the features of each word.
        for (iter=tokens.begin();iter!=tokens.end();iter++)
        {
            //fprintf(stdout,"\t\tCurrent Token: %s (%d)\n",iter->stToken.c_str(),iter->stToken.size()); fflush(stdout);
            this->HandleToken(iter->vLetters,iter->iLetters,termsMarker);
        }
    }
    this->SetFreq(featuresList,debug_log);
}

//___________________________________________________________________
void FeaturesHandler::GetFeatureByWindow(FileScanner & fsReader, string & stWindow, const string & stPrevWindow, string & stPrevWord, set<string,LexComp> & termsMarker)
{
	int i;
	string stWord(100,'\0');
	string stLastWord = stPrevWord;

	stWindow = stPrevWindow;
	voTrimEdges(stWindow," ");

	for (i=0; (((i<iMinWindowWords) || ((int)stWindow.size()<iMinWindowLength)) && fsReader.bMoreToRead()); i++)
	{
		// Read the next word from the text.
		this->GetWord(fsReader,stWord);

		// Handle content word, if needed.
		if (bUseContentWords)
		{
			this->HandleFeature(stWord,WordNgram,termsMarker);
		}

		// Update the window.
		if (!stWindow.empty()) stWindow += " ";
		stWindow += stWord;

		// Handle the word-2-gram.
		if (!stLastWord.empty())
		{
			stLastWord += " ";
			stLastWord += stWord;
			this->HandleFeature(stLastWord,WordNgram,termsMarker);
		}
		stLastWord = stWord;
	}
	stPrevWord = stLastWord;
}

//___________________________________________________________________
int FeaturesHandler::iPrintFeatureSet(const string & stFileName,
                                      FILE * debug_log) const
{
    if (bSortTermByFreq)
    {
        return PrintFeatureSet(stFileName,lexicon,iCorpusSize,iDocsCount,debug_log);
    }
    else
    {
        return PrintFeatureSet(stFileName,lexicon,debug_log);
    }
}

//___________________________________________________________________
void FeaturesHandler::voLoadFeatureSet(const string & stFileName,
                                       FILE * debug_log)
{
    LoadFeatureSet(lexicon,
                   &i2fLex,
                   stFileName,
                   debug_log);
}

//___________________________________________________________________
void ExtractFeatures::HandleFeature(const string & stFeature,
	                                FeatureType featureType,
                                    set<string,LexComp> & termsMarker)
{
    set<string,LexComp>::const_iterator markedItem = termsMarker.find(stFeature);
    map<string,FeatureInfo,LexComp>::iterator ftrItem = lexicon.find(stFeature);
    FeatureInfo item;
    
    iDocSize++;
    iCorpusSize++;
    
    if (ftrItem == lexicon.end())
    {
        item.index=iFeatureIndex++;
        item.iTotalFreq=1;
        item.iDocsFreq=1;
        item.stTerm=stFeature;
		item.featureType = featureType;
        lexicon.insert(map<string,FeatureInfo,LexComp>::value_type(item.stTerm,item));
        if (markedItem == termsMarker.end()) termsMarker.insert(stFeature);
        else {fprintf(stderr,"The feature %s is unknown but marked?!\n",stFeature.c_str()); fflush(stderr); exit(0);}
    }
    else
    {
        ftrItem->second.iTotalFreq++;
        
        if (markedItem == termsMarker.end())
        {
            ftrItem->second.iDocsFreq++;
            termsMarker.insert(stFeature);
        }
    }
}

//___________________________________________________________________
void CountFeatures::HandleFeature(const string & stFeature,
	                              FeatureType featureType,
                                  set<string,LexComp> & termsMarker)
{
    map<string,FeatureInfo,LexComp>::const_iterator w2fIter = lexicon.find(stFeature);
    if (w2fIter == lexicon.end()) return;

    map<int,int>::iterator i2fIter=featureFreq.find(w2fIter->second.index);
    if (i2fIter==featureFreq.end())
        featureFreq.insert(map<int,int>::value_type(w2fIter->second.index,1));
    else
    {
        i2fIter->second++;
    }

    iDocSize++;
}

//___________________________________________________________________
void CountFeatures::InitFreq(list<FeatureItem> * featuresList)
{
    iDocSize=0;
    featureFreq.clear();
}

//___________________________________________________________________
void CountFeatures::SetFreq(list<FeatureItem> * featuresList,
                            FILE * debug_log) const
{
    map<int,int>::const_iterator freqIter;
    double weight,tf=0,idf=0.0;
    FeatureItem item;
    FeatureInfo info;
    
    /*fprintf(debug_log,"\nSet Freq: (DocSize=%d,#Types=%d)\n",iDocSize,iTypesNumber);
    fflush(debug_log);*/

    featuresList->clear();
    for (freqIter=featureFreq.begin();freqIter!=featureFreq.end();freqIter++)
    {
        info = this->getFeatureInfo(freqIter->first);
        
        tf = (double)(freqIter->second) / (double)iDocSize;
        
        if (((info.iTotalFreq - info.iDocsFreq) < iMinFreq2DocDiff) || (iMinDocsFreq > info.iDocsFreq)) continue;
        
        if (stAssocMeasure.compare("tfidf") == 0)
        {
            idf = log_base2((double)iTypesNumber / (double)info.iDocsFreq);
            idf += 1.0;
            weight = tf * idf;
        }
        else if (stAssocMeasure.compare("termfreq") == 0)
        {
            weight = tf;
        }
        else if (stAssocMeasure.compare("binary") == 0)
        {
            weight = 1.0;
        }
        else
        {
            fprintf(stderr,"Illegal Assoc Measure: %s\n",stAssocMeasure.c_str());
            fflush(stderr);
            exit(0);
        }

        item.index=info.index;
        item.weight=weight;
        item.idf = idf;
        featuresList->push_back(item);
    }
}

//___________________________________________________________________
void CountFeatures::Reset()
{
    FeaturesHandler::Reset();
    featureFreq.clear();
}
