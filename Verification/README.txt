This open code environment supports two main procedures as follows:

(1) Many-Candidates attribution:
              Searching for the writer of an unknown text among a (possibly huge) set of authors.
			  The author set might or might not contain the actual writer of the unknown text.
			  
(2) Pairs-Verification:
              Verifying whether a given pair of texts were written by the same author or not.
			  
The algorithms used for implementing these two procedures are statistical and based
on random feature set method of (Moshe Koppel, 2007), historically known as LRE.

The software is written in C/C++ and is adjusted to Microsoft platforms.

All the functions and utilities that are supported by this tool-kit are called through LREProgs.cpp.

For operating the tool-kit, follow the two given examples:

 - Unzip the Code.zip.

(1) Many Candidates:
      (i)    Run the next command line (standing on Code root):
	             many-candidates.bat 1k 4 ref
	   (ii)  Compare the results to the correspondent results under reference.
	         Some differences are optional due to the random nature of the algorithm.
(1) Pairs Verification:
      (i)    Run the next command line (standing on Code root):
	             pairs_verification.bat 4 ref Dev 25
	   (ii)  Compare the results to the correspondent results under reference.
	         Some differences are optional due to the random nature of the algorithm.

The configuration/parameters files can be found in the *.bat files.
Parameters can be set for either optimization tuning or for creating further test sets.
