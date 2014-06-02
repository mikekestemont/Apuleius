#include <ThesisAux.h>
#include <ResourceBox.h>
#include <FileScanner.h>
#include <FolderScanner.h>

using namespace std;

//___________________________________________________________________
void CreateApulieusIRTest()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stAnonyms = stSampleFolder+"anonyms.txt";
    const string stAuthors = stSampleFolder+"authors.txt";
    const string stCands = stSampleFolder+"candidates.txt";

    fprintf(stderr,"CreateApulieusIRTest - Start\n");
    fprintf(stderr,"Sample Folder: %s\n",stSampleFolder.c_str());
    fflush(stderr);

    FILE * fAnonyms = open_file(stAnonyms,"CreateApulieusIRTest","w");
    FILE * fAuthors = open_file(stAuthors,"CreateApulieusIRTest","w");
    FILE * fCands = open_file(stCands,"CreateApulieusIRTest","w");

	FolderScanner fsAuthorsScanner(stSampleFolder + "Author\\Text\\*.txt");
	string stFileName;
	int iAnonyms = 0;
	int iAuthors = 0;
	while (fsAuthorsScanner.bMoreToRead())
	{
		stFileName = fsAuthorsScanner.getNextFile();
		fprintf(fAuthors,"%s\n",stFileName.c_str()); fflush(fAuthors);
		fprintf(fCands,"%s\n",stFileName.c_str()); fflush(fAuthors);
		iAuthors++;
	}

	FolderScanner fsAnonymsScanner(stSampleFolder + "Anonym\\Text\\*.txt");
	while (fsAnonymsScanner.bMoreToRead())
	{
		stFileName = fsAnonymsScanner.getNextFile();
		fprintf(fAnonyms,"%s\n",stFileName.c_str()); fflush(fAuthors);
		iAnonyms++;
	}

    fflush(fAnonyms); fclose(fAnonyms);
    fflush(fAuthors); fclose(fAuthors);
    fflush(fCands); fclose(fCands);

    fprintf(stderr,"CreateApulieusIRTest - End (#Anonyms=%d, #Authors=%d)\n",iAnonyms,iAuthors);
    fflush(stderr);
}

//___________________________________________________________________
void TruncatePosts()
{
    const string stSampleFolder = ResourceBox::Get()->getStringValue("SampleFolder");
    const string stFromFolder = stSampleFolder + "Author1\\";
    const string stToFolder = stSampleFolder + "Author\\";
    FILE * fGlobalScript = open_file(stSampleFolder+"authors.txt","TruncatePosts","w");
    FILE * fSubScript = NULL;

    map<string,string,LexComp> catsList;
    map<string,string,LexComp>::const_iterator iter;
    LoadCategoryList(catsList,NULL,NULL,"ClassesList");
    int iCounter=0;
    string stFileName(75,'\0');
    int iSnipIndex;
    string stConstSnipName;
    string stSnipName;
    FILE * fSnipFile=NULL;
    size_t iPos;
    int iWordsNumber;
    int iRequiredLength;
    const int iSnipLength = ResourceBox::Get()->getIntValue("MinBlogLength");

    string stInpFolder;
    string stOutFolder;
    for (iter=catsList.begin();iter!=catsList.end();iter++)
    {
        stInpFolder = stFromFolder+iter->first+"\\Text\\";
        stOutFolder = stToFolder+iter->first+"\\Text\\";
        CleanFolder(stOutFolder);
        FolderScanner * folderScanner = new FolderScanner(stInpFolder+"\\*.txt");
        fSubScript = open_file(stSampleFolder+iter->first+"_authors.txt","TruncatePosts","w");
        fSnipFile=NULL;
        iSnipIndex=0;
        iWordsNumber=0;
        iRequiredLength=iSnipLength;
        while (folderScanner->bMoreToRead())
        {
            stFileName = folderScanner->getNextFile();
            if ((stFileName.compare(".") == 0) || (stFileName.compare("..") == 0))
                continue;
            if (folderScanner->bIsFolder()) abort_err(string("Folder encountered in merge scripts: ")+stFileName);

            FileScanner * fsPost = new FileScanner(stInpFolder+stFileName,512);
            if ((iPos=stFileName.find_first_of('_')) == string::npos) abort_err(string("Illegal file name: ")+stFileName);
            stConstSnipName = stFileName.substr(0,iPos+1);
            while (fsPost->bMoreToRead())
            {
                if (fSnipFile == NULL)
                {
                    iRequiredLength=iSnipLength;
                    stSnipName = stConstSnipName+stringOf(iSnipIndex++)+".txt";
                    fSnipFile = open_file(stOutFolder + stSnipName,"TruncatePosts","w");
                }
                iWordsNumber = PrintSnippet(*fsPost,iRequiredLength,fSnipFile);

                if (iWordsNumber >= iRequiredLength)
                {
                    iWordsNumber=0;
                    iRequiredLength=iSnipLength;
                    fflush(fSnipFile);
                    fclose(fSnipFile);
                    fSnipFile=NULL;
                    fprintf(fSubScript,"%s\n",stSnipName.c_str());
                    fflush(fSubScript);
                    fprintf(fGlobalScript,"%s/Ftrs/%s\t%s\n",iter->first.c_str(),stSnipName.c_str(),iter->first.c_str());
                    fflush(fGlobalScript);
                }
                else
                    iRequiredLength -= iWordsNumber;
            }

            delete fsPost;
        }

        if (fSnipFile != NULL)
        {
            fclose(fSnipFile);
            if (remove((stOutFolder + stSnipName).c_str()) < 0) abort_err(string("Failed to remove ")+stOutFolder+stSnipName);
        }
        
        delete folderScanner;
        fflush(fSubScript);
        fclose(fSubScript);
        fSubScript=NULL;
    }
    fflush(fGlobalScript);
    fclose(fGlobalScript);
}

//___________________________________________________________________
void GetAllPairs()
{
    const string stWorkingDir = ResourceBox::Get()->getStringValue("SampleFolder");
    FolderScanner fsPairs(stWorkingDir+"\\Begin\\Text\\*.txt");
    string stFileName;

    FILE * fAllPairs = open_file(stWorkingDir+"all.txt","GetAllPairs","w");

    while (fsPairs.bMoreToRead())
    {
        stFileName = fsPairs.getNextFile();

        if (fsPairs.bIsFolder()) continue;

        fprintf(fAllPairs,"%s\n",stFileName.c_str()); fflush(fAllPairs);
    }

    fclose(fAllPairs);
}

//___________________________________________________________________
static void GetAuthors(const string & stFolder, const string & stFile)
{
    FolderScanner fsFiles(stFolder+"*.txt");
    string stAuthor;

    FILE * fAuthors = open_file(stFile,"GetAuthors","w",true);
    while (fsFiles.bMoreToRead())
    {
        stAuthor=fsFiles.getNextFile();
        if (fsFiles.bIsFolder()) continue;
        fprintf(fAuthors,"%s\n",stAuthor.c_str());
        fflush(fAuthors);
    }
    fclose(fAuthors);
}

//___________________________________________________________________
void CreateIRScript()
{
    const string stWorkingDir = ResourceBox::Get()->getStringValue("SampleFolder");
    GetAuthors(stWorkingDir+"\\Anonym\\Text\\",stWorkingDir+"anonyms.txt");
    GetAuthors(stWorkingDir+"\\Author\\Text\\",stWorkingDir+"authors.txt");
}
//___________________________________________________________________
void TextsToPairScript()
{
    const string stWorkingDir = ResourceBox::Get()->getStringValue("SampleFolder");
    LinesReader lrTexts(stWorkingDir+"\\all.txt",512);
    string stFileName;
    const int iTextsNumber = ScriptSize(stWorkingDir+"\\all.txt");
    const int iNameKeyLength = ResourceBox::Get()->getIntValue("WriterRecognzerLen");
    const string stSuspectName = ResourceBox::Get()->getStringValue("SuspectName");
    const string stKnownAuthors = ResourceBox::Get()->getStringValue("KnownAuthors");
    const bool bOnlyAgainstSuspect = (ResourceBox::Get()->getIntValue("OnlyAgainstSuspect") > 0);
    set<string,LexComp> knownAuthors;
    vector<string> vTexts(iTextsNumber,"");
    int i,j;
    string stX,stY;

    fprintf(stdout,"TextsToPairScript - Start (nkl=%d, suspect=%s,oas=%s)\n",iNameKeyLength,stSuspectName.c_str(),(bOnlyAgainstSuspect?"Yes":"No")); fflush(stdout);

    LinesReader lrAuthors(stKnownAuthors,512);
    while (lrAuthors.bMoreToRead())
    {
        lrAuthors.voGetLine(stFileName);
        knownAuthors.insert(stFileName);
    }

    FILE * fScript = open_file(stWorkingDir+"script_5050.txt","TextsToPairScript","w");

    fprintf(stdout,"Load texts\n"); fflush(stdout);

    i=0;
    while (lrTexts.bMoreToRead())
    {
        lrTexts.voGetLine(stFileName);
        vTexts.at(i) = stFileName;
        i++;
    }

    fprintf(stdout,"Load Pairs\n"); fflush(stdout);
    for (i=0;i<iTextsNumber;i++)
    {
        fprintf(stdout,"\tWorking on text %d: %s...\n",i,vTexts.at(i).c_str()); fflush(stdout);

        for (j=i+1;j<iTextsNumber;j++)
        {
            if ((vTexts.at(i).substr(0,iNameKeyLength).compare(stSuspectName) != 0) &&
                (vTexts.at(j).substr(0,iNameKeyLength).compare(stSuspectName) != 0) &&
                bOnlyAgainstSuspect)
                continue;

            fprintf(stdout,"\t\tPair %d: %s...\n",j,vTexts.at(j).c_str()); fflush(stdout);
            if (vTexts.at(i).substr(0,iNameKeyLength).compare(vTexts.at(j).substr(0,iNameKeyLength)) == 0)
            {
                stX = vTexts.at(i);
                stY = vTexts.at(j);
            }
            else if (vTexts.at(j).substr(0,iNameKeyLength).compare(stSuspectName) == 0)
            {
                stX = vTexts.at(j);
                stY = vTexts.at(i);
            }
            else if ((knownAuthors.find(vTexts.at(j).substr(0,iNameKeyLength)) != knownAuthors.end()) && (vTexts.at(i).substr(0,iNameKeyLength).compare(stSuspectName) != 0))
            {
                stX = vTexts.at(j);
                stY = vTexts.at(i);
            }
            else
            {
                stX = vTexts.at(i);
                stY = vTexts.at(j);
            }

            fprintf(fScript,"%s\t%s\n",stX.c_str(),stY.c_str());
            fflush(fScript);
        }
    }

    fclose(fScript);
    fprintf(stdout,"TextsToPairScript - End (nkl=%d, suspect=%s,oas=%s)\n",iNameKeyLength,stSuspectName.c_str(),(bOnlyAgainstSuspect?"Yes":"No")); fflush(stdout);
}

//___________________________________________________________________
void BreakTexts()
{
    const string stOriginalTexts = ResourceBox::Get()->getStringValue("OriginalTexts");
    const string stSnippetsFolder = ResourceBox::Get()->getStringValue("SnippetsFolder");
    FolderScanner fsTexts(stOriginalTexts+"*.txt");
    string stFileName;
    string stBaseName;
	string stWriterName;
	string stArticleName;
    const int iSnipLength = ResourceBox::Get()->getIntValue("SnippetLength");
    const int iNameKeyLength = ResourceBox::Get()->getIntValue("WriterRecognzerLen");
    const int iMaxSnippetsPerText = ResourceBox::Get()->getIntValue("MaxSnippetsPerText");
    int iSnipIndex;
    int iDocLength;
    int iSnipsNumber;
    FileScanner * fsDoc = NULL;
	size_t iPos;

    FILE * fSnip = NULL;

    fprintf(stdout,"BreakTexts - Start (inp=%s, out=%s, len=%d)\n",stOriginalTexts.c_str(),stSnippetsFolder.c_str(),iSnipLength); fflush(stdout);

    CleanFolder(stSnippetsFolder);
    while (fsTexts.bMoreToRead())
    {
        stFileName = fsTexts.getNextFile();
        if (fsTexts.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

		// Format name.
		stBaseName = stFileName;
		voToLowerCase(stBaseName);

		if ((iPos = stBaseName.find_first_of('_')) == string::npos)
		{
			abort_err(string("Could not format file name: ") + stBaseName);
		}

		stWriterName = stBaseName.substr(0,iPos);
		stArticleName = stBaseName.substr(iPos+1);

		// Remove '.txt'
		stArticleName = stArticleName.substr(0, stArticleName.size() - 4);

		// Truncate & pad the name.
		truncateAndPadString(stWriterName, iNameKeyLength, 'x');
		truncateAndPadString(stArticleName, iNameKeyLength, 'x');

		stBaseName = stWriterName + "_" + stArticleName;

        fsDoc = new FileScanner(stOriginalTexts+stFileName,512);

        iDocLength = PrintSnippet(*fsDoc,10000000,NULL);
        fsDoc->restart();

		// Check the number of snippets that can be extracted from this document.
        iSnipsNumber = iDocLength / iSnipLength;
		iSnipsNumber = ((iSnipsNumber<iMaxSnippetsPerText)?iSnipsNumber:iMaxSnippetsPerText);
		iSnipsNumber = ((iSnipsNumber<1)?1:iSnipsNumber);

        fprintf(stdout,"\tDocLength=%d, #Snips=%d\n",iDocLength,iSnipsNumber); fflush(stdout);

        for (iSnipIndex=0;iSnipIndex<iSnipsNumber;iSnipIndex++)
        {
			stFileName = stBaseName+"_"+padIntWithZeros(iSnipIndex+1, 3)+".txt";
            fSnip = open_file(stSnippetsFolder+stFileName,"BreakTexts","w",true);
            iDocLength = PrintSnippet(*fsDoc,iSnipLength,fSnip);
            fclose(fSnip);
            fSnip=NULL;

            fprintf(stdout,"\t\t\tCurrent Snip: %s (len=%d)\n",stFileName.c_str(),iDocLength); fflush(stdout);
        }

        delete fsDoc;
        fsDoc=NULL;
    }
    fprintf(stdout,"BreakTexts - End (inp=%s, out=%s, len=%d)\n",stOriginalTexts.c_str(),stSnippetsFolder.c_str(),iSnipLength); fflush(stdout);
}

//___________________________________________________________________
void TextLengths()
{
    const string stOriginalTexts = ResourceBox::Get()->getStringValue("OriginalTexts");
    const string stLengthsFile = ResourceBox::Get()->getStringValue("Lengths");
    FolderScanner fsTexts(stOriginalTexts+"*.txt");
    string stFileName;
	set<PhraseHitCount,PHCComp> sortedTexts;

	fprintf(stdout,"TextLengths - Start (inp=%s, out=%s)\n",stOriginalTexts.c_str(),stLengthsFile.c_str()); fflush(stdout);
	FILE * fLengths = open_file(stLengthsFile,"TextLengths","w");
    while (fsTexts.bMoreToRead())
    {
        stFileName = fsTexts.getNextFile();
        if (fsTexts.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

        FileScanner * fsDoc = new FileScanner(stOriginalTexts+stFileName,512);

        int iDocLength = PrintSnippet(*fsDoc,10000000,NULL);

		sortedTexts.insert(PhraseHitCount(stFileName,iDocLength));
    }

	set<PhraseHitCount,PHCComp>::const_iterator iter;
	for (iter=sortedTexts.begin();iter!=sortedTexts.end();iter++)
	{
		fprintf(fLengths,"%s\t%d\n",iter->stPhrase.c_str(),iter->iHitCount);
		fflush(fLengths);
	}

    fprintf(stdout,"BreakTexts - End (inp=%s, out=%s)\n",stOriginalTexts.c_str(),stLengthsFile.c_str()); fflush(stdout);
	fclose(fLengths);
}

//___________________________________________________________________
void FilterEnglishText()
{
    const string stOriginalTexts = ResourceBox::Get()->getStringValue("OriginalTexts");
    const string stNonEnglishFolder = ResourceBox::Get()->getStringValue("NonEnglishFolder");
    FolderScanner fsTexts(stOriginalTexts+"*.txt");
    string stFileName;

    FILE * fText = NULL;
    FileScanner * fsDoc = NULL;
    string stWord;
    string stLetter;
    unsigned int i;
    unsigned int iWordLen;
    bool bIsFirstWord;
    bool bSkipWord;
    char cByte;
    string stSpace;

    fprintf(stdout,"FilterEnglishText - Start (inp=%s, out=%s)\n",stOriginalTexts.c_str(),stNonEnglishFolder.c_str()); fflush(stdout);

    CleanFolder(stNonEnglishFolder);
    while (fsTexts.bMoreToRead())
    {
        stFileName = fsTexts.getNextFile();
        if (fsTexts.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

        fsDoc = new FileScanner(stOriginalTexts+stFileName,512);
        fText = open_file(stNonEnglishFolder+stFileName,"FilterEnglishText","w",true);
        bIsFirstWord=true;

        while (fsDoc->bMoreToRead())
        {
            GetNextWord(*fsDoc,stWord);
            iWordLen = stWord.size();
            bSkipWord=false;

            fprintf(stdout,"\t\tWord: %s\n",stWord.c_str()); fflush(stdout);
        
            for (i=0;((i<iWordLen) && !bSkipWord);i++)
            {
                GetUTF8Letter(stWord,stLetter,i);
                fprintf(stdout,"\t\t\tLetter: %s\n",stLetter.c_str()); fflush(stdout);
                if (stLetter.size() > 1) continue;
                cByte = stLetter.at(0);
                if (((cByte>='A') && (cByte<='Z')) || ((cByte>='a') && (cByte<='z'))) bSkipWord=true;
            }

            if (bSkipWord)
            {
                fprintf(stdout,"\t\tSkip Word: %s (in %s)\n",stWord.c_str(),stFileName.c_str()); fflush(stdout);
                continue;
            }

            stSpace = (bIsFirstWord?"":" ");
            fprintf(stdout,"\t\tPrint Word!!\n"); fflush(stdout);
            fprintf(fText,"%s%s",stSpace.c_str(),stWord.c_str());
            fflush(fText);
            bIsFirstWord=false;
        }

        delete fsDoc;
        fsDoc=NULL;
        fclose(fText);
        fText=NULL;
    }
    fprintf(stdout,"FilterEnglishText - End (inp=%s, out=%s, len=%d)\n",stOriginalTexts.c_str(),stNonEnglishFolder.c_str()); fflush(stdout);
}

//___________________________________________________________________
void GetNextWord(FileScanner & fsReader,string & stWord)
{
    bool bWordEnd=false;
    char cByte;
    bool bWhiteSpace;

    stWord = "";
    while (fsReader.bMoreToRead() && !bWordEnd)
    {
        cByte = fsReader.cGetNextByte();
        bWhiteSpace = bIsWhiteSpace(cByte);

        // If a word ends by an End-Line - 
        // concatenate the sign to the word
        // and quit.
        if (cByte == '\n')
        {
            stWord += "\n";
            bWordEnd=true;
        }

        // No word has been encountered yet.
        else if (bWhiteSpace && stWord.empty())
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
            stWord += cByte;
        }
    }
}

//___________________________________________________________________
class AuthorResult
{
    public:

        string stAuthorName;
        int iHitCount;
        double dScore;

        AuthorResult(const string & _stAuthor="", const int _HC=0,const double _ds=0.0):stAuthorName(_stAuthor),iHitCount(_HC),dScore(_ds){}
        AuthorResult(const AuthorResult & ar){(*this)=ar;}
        void operator=(const AuthorResult & ar) {stAuthorName=ar.stAuthorName; iHitCount=ar.iHitCount; dScore=ar.dScore;}
};

//___________________________________________________________________
struct SortAuthorRes
{
    bool operator()(const AuthorResult & ar1,const AuthorResult & ar2) const
    {
        if (ar1.dScore > ar2.dScore) return true;
        if (ar1.dScore < ar2.dScore) return false;
        return (ar1.stAuthorName.compare(ar2.stAuthorName) < 0);
    }
};

//___________________________________________________________________
void AverageScoreOverChunks()
{
    const string stSummaryFile = ResourceBox::Get()->getStringValue("SummaryFile");
    const string stAverageScoresReport = ResourceBox::Get()->getStringValue("AverageScoresReport");
    const string stSuspectName = ResourceBox::Get()->getStringValue("SuspectName");
    const int iNameKeyLength = ResourceBox::Get()->getIntValue("NameKeyLength");
    TdhReader tdhSummary(stSummaryFile,512);
    vector<string> vFields(5,"");
    map<string,AuthorResult,LexComp> authorsList;
    set<AuthorResult,SortAuthorRes> authorsSorted;
    map<string,AuthorResult,LexComp>::iterator liter;
    set<AuthorResult,SortAuthorRes>::const_iterator siter;
    int iScore;

    while (tdhSummary.bMoreToRead())
    {
        tdhSummary.iGetLine(vFields);

        if ((vFields.at(0).substr(0,iNameKeyLength).compare(stSuspectName) != 0) &&
            (vFields.at(1).substr(0,iNameKeyLength).compare(stSuspectName) != 0))
            continue;

        if (!string_as(iScore,vFields.at(3),std::dec)) abort_err(string("Couldn't convert to int: <")+vFields.at(0)+","+vFields.at(1)+"> -> "+vFields.at(3));

        liter=authorsList.find(vFields.at(1).substr(0,iNameKeyLength));
        if (liter == authorsList.end())
        {
            authorsList.insert(map<string,AuthorResult,LexComp>::value_type(vFields.at(1).substr(0,iNameKeyLength),AuthorResult(vFields.at(1).substr(0,iNameKeyLength),1,(double)iScore)));
        }
        else
        {
            liter->second.iHitCount++;
            liter->second.dScore += (double)iScore;
        }
    }

    authorsSorted.clear();
    for (liter=authorsList.begin();liter!=authorsList.end();liter++)
    {
        liter->second.dScore /= liter->second.iHitCount;
        authorsSorted.insert(liter->second);
    }

    FILE * fReport = open_file(stAverageScoresReport,"AverageScoreOverChunks","w");
    for (siter=authorsSorted.begin();siter!=authorsSorted.end();siter++)
    {
        fprintf(fReport,"%s\t%.1f\t%d\n",siter->stAuthorName.c_str(),siter->dScore,siter->iHitCount);
        fflush(fReport);
    }
    fclose(fReport);
}

//___________________________________________________________________
EncodedToken::EncodedToken(const int iSize):iLetters(0),stToken(""),vLetters(iSize,""){}

//___________________________________________________________________
EncodedToken::EncodedToken(const EncodedToken & et)
{
    (*this)=et;
}

//___________________________________________________________________
void EncodedToken::operator=(const EncodedToken & et)
{
    iLetters=et.iLetters;
    stToken=et.stToken;
    vLetters=et.vLetters;
}

//___________________________________________________________________
void EncodedToken::Reset()
{
    iLetters=0;
    stToken="";
}

//___________________________________________________________________
void EncodedToken::AddLetter(const string & stLetter)
{
    try
    {
        vLetters.at(iLetters)=stLetter;
        stToken+=stLetter;
        iLetters++;
    }
    catch (...)
    {
        abort_err(string("AddLetter failed: letter=")+stLetter+", #Letters="+stringOf(iLetters));
    }
}

//___________________________________________________________________
void TextsToPairs()
{
	const string stWorkingDir = ResourceBox::Get()->getStringValue("SampleFolder");
    string stFileName;
    string stBaseName;
	string stWriterName;
	string stArticleName;
    const int iArticleKeyLength = ResourceBox::Get()->getIntValue("WriterRecognzerLen");
	const int iSampleSize = CountFiles(stWorkingDir+"Begin\\Text\\"+"*.txt");
	vector<string> vTexts(iSampleSize,"");
	fprintf(stdout,"TextsToPairs - Start (smpl=%s, len_article=%d, #files=%d)\n",stWorkingDir.c_str(),iArticleKeyLength,iSampleSize); fflush(stdout);
    FolderScanner fsTexts(stWorkingDir+"Begin\\Text\\"+"*.txt");
	FILE * fAll = open_file(stWorkingDir+"all.txt","TextsToPairs","w");
	FILE * fDecoys = open_file(stWorkingDir+"decoys.txt","TextsToPairs","w");

	int iFiles = 0;
	int i, j;
    while (fsTexts.bMoreToRead())
    {
        stFileName = fsTexts.getNextFile();
        if (fsTexts.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

		vTexts.at(iFiles++) = stFileName;
		fprintf(fAll,"%s\n",stFileName.c_str()); fflush(fAll);
		fprintf(fDecoys,"%s\n",stFileName.c_str()); fflush(fDecoys);
	}
	fclose(fAll);
	fclose(fDecoys);

	string stBegin;
	string stEnd;
	FILE * fScripts = open_file(stWorkingDir+"scripts.txt","TextsToPairs","w");
	for (i=0;i<iFiles;i++)
	{
		stBegin = vTexts.at(i);
		if (stBegin.substr(iArticleKeyLength+1,3).compare("001") != 0) continue;
		for (j=i+1;j<iFiles;j++)
		{
			stEnd = vTexts.at(j);
			if (stEnd.substr(iArticleKeyLength+1,3).compare("001") != 0) continue;

			if (stBegin.substr(0,iArticleKeyLength).compare(stEnd.substr(0,iArticleKeyLength)) == 0) continue;
			fprintf(fScripts,"%s\t%s\n",stBegin.c_str(),stEnd.c_str());
			fflush(fScripts);
		}
	}
	fclose(fScripts);
	fprintf(stdout,"TextsToPairs - End (smpl=%s, len_article=%d, #files=%d)\n",stWorkingDir.c_str(),iArticleKeyLength,iSampleSize); fflush(stdout);
}

//___________________________________________________________________
void EnvToPairs()
{
	const string stWorkingDir = ResourceBox::Get()->getStringValue("SampleFolder");
    string stFileName;
    string stBaseName;
	string stWriterName;
	string stArticleName;
    const int iArticleKeyLength = ResourceBox::Get()->getIntValue("WriterRecognzerLen");
	const int iBeginFiles = CountFiles(stWorkingDir+"Begin\\Text\\"+"*.txt");
	vector<string> vBegins(iBeginFiles,"");
	const int iEndFiles = CountFiles(stWorkingDir+"End\\Text\\"+"*.txt");
	vector<string> vEnds(iEndFiles,"");

	fprintf(stdout,"TextsToPairs - Start (smpl=%s, len_article=%d, #begins=%d, #ends)\n",stWorkingDir.c_str(),iArticleKeyLength,iBeginFiles,iEndFiles); fflush(stdout);
    FolderScanner fsBegins(stWorkingDir+"Begin\\Text\\"+"*.txt");
    FolderScanner fsEnds(stWorkingDir+"End\\Text\\"+"*.txt");
    FolderScanner fsDecoys(stWorkingDir+"Decoy\\Text\\"+"*.txt");
	FILE * fAll = open_file(stWorkingDir+"all.txt","TextsToPairs","w");
	FILE * fDecoys = open_file(stWorkingDir+"decoys.txt","TextsToPairs","w");

	int iFiles = 0;
	int i, j;
    while (fsBegins.bMoreToRead())
    {
        stFileName = fsBegins.getNextFile();
        if (fsBegins.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

		fprintf(fAll,"%s\n",stFileName.c_str()); fflush(fAll);
		vBegins.at(iFiles++) = stFileName;
	}

	iFiles = 0;
	while (fsEnds.bMoreToRead())
    {
        stFileName = fsEnds.getNextFile();
        if (fsEnds.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

		vEnds.at(iFiles++) = stFileName;
	}

	while (fsDecoys.bMoreToRead())
    {
        stFileName = fsDecoys.getNextFile();
        if (fsEnds.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

		fprintf(fDecoys,"%s\n",stFileName.c_str()); fflush(fDecoys);
	}
	fclose(fAll);
	fclose(fDecoys);

	string stBegin;
	string stEnd;
	set<string,LexComp> pairsMarker;
	string stPairCode;
	FILE * fScripts = open_file(stWorkingDir+"scripts.txt","TextsToPairs","w");
	for (i=0;i<iBeginFiles;i++)
	{
		stBegin = vBegins.at(i);
		if (stBegin.substr(iArticleKeyLength+1,3).compare("001") != 0) continue;
		for (j=0;j<iEndFiles;j++)
		{
			stEnd = vEnds.at(j);
			if (stEnd.substr(iArticleKeyLength+1,3).compare("001") != 0) continue;

			if (stBegin.substr(0,iArticleKeyLength).compare(stEnd.substr(0,iArticleKeyLength)) == 0) continue;

			stPairCode = stEnd + "###" + stBegin;
			if (pairsMarker.find(stPairCode) != pairsMarker.end()) continue;

			stPairCode = stBegin + "###" + stEnd;
			pairsMarker.insert(stPairCode);

			fprintf(fScripts,"%s\t%s\n",stBegin.c_str(),stEnd.c_str());
			fflush(fScripts);
		}
	}
	fclose(fScripts);
	fprintf(stdout,"TextsToPairs - End (smpl=%s, len_article=%d, #begins=%d, #ends)\n",stWorkingDir.c_str(),iArticleKeyLength,iBeginFiles,iEndFiles); fflush(stdout);
}

//___________________________________________________________________
void EnvToIR()
{
	const string stWorkingDir = ResourceBox::Get()->getStringValue("SampleFolder");
    string stFileName;
	
	fprintf(stdout,"EnvToIR - Start (smpl=%s)\n",stWorkingDir.c_str()); fflush(stdout);

    FolderScanner fsAuthors(stWorkingDir+"Author\\Text\\"+"*.txt");
    FolderScanner fsAnonyms(stWorkingDir+"Anonym\\Text\\"+"*.txt");
	FILE * fAnonyms = open_file(stWorkingDir+"anonyms.txt","EnvToIR","w");
	FILE * fAuthors = open_file(stWorkingDir+"authors.txt","EnvToIR","w");
	FILE * fCandidates = open_file(stWorkingDir+"candidates.txt","EnvToIR","w");

    while (fsAuthors.bMoreToRead())
    {
        stFileName = fsAuthors.getNextFile();
        if (fsAuthors.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);

		fprintf(fAuthors,"%s\n",stFileName.c_str()); fflush(fAuthors);
		fprintf(fCandidates,"%s\n",stFileName.c_str()); fflush(fCandidates);
	}

	while (fsAnonyms.bMoreToRead())
    {
        stFileName = fsAnonyms.getNextFile();
        if (fsAnonyms.bIsFolder()) continue;

        fprintf(stdout,"\tCurrent Doc: %s\n",stFileName.c_str()); fflush(stdout);
		fprintf(fAnonyms,"%s\n",stFileName.c_str()); fflush(fAnonyms);
	}
	fclose(fAnonyms);
	fclose(fAuthors);
	fclose(fCandidates);
	fprintf(stdout,"EnvToIR - End (smpl=%s)\n",stWorkingDir.c_str()); fflush(stdout);
}
