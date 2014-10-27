// See readme & liscense.

// debian provided libs "apt-get install libxml2 libjudy
#include <libxml/xmlreader.h> // xmlTextReader
#include <glib.h>	   // GString functions, g_ascii_isalpha, command line opts
#include <Judy.h>	   // J1 calls, Pvoid_t
// language or compiler libs
#include <stdbool.h>   // ...
#include <sys/stat.h>  // stat
#include <stdlib.h>    // malloc, exit
#include <stdio.h>	   // printf
#include <math.h>      // log2
#include <unistd.h>    // access
// libs added myself
#include "stemmer.h"   // stemmer & stem, see http://tartarus.org/martin/PorterStemmer/
#include "ngramming.h" // ngramming functions

	//-------------------- GLOBALS --------------------//
 
gboolean silent = FALSE;    // print regular status updates or not
gboolean writeFiles = TRUE; // write output files or not
 
	//-------------------- TYPES --------------------//
	
typedef struct {
	GString *title;
	GString *body;
	int articleNumber;
	} article;

	//-------------- HELPER FUNCTIONS ---------------//

/**
 * Prints only if the silent flag was not passed
 * 
 * see parsrCLIopts for where silent is set
 */
void optionalPrint (const char * format, ...) {
	if (!silent) {
		va_list argsList;
		va_start (argsList, format);
		vprintf (format, argsList);
		va_end (argsList);
	}
}

/**
 * Displays a progress bar. Every update does not need to be equal.
 * 
 * current - current amount of data processed
 * total - total amount of data to be processed
 * precent - a persistent value remembered by the caller, initialize with 0
 */
static inline void displayProgressBar (long current, long total, int *percent) {
	
	static char progressBar[50];
	
	int newPercent = (int) ((100 * current) / total);
	
	if (newPercent - *percent) { //only re-draw if new data
		*percent = newPercent;
		progressBar[*percent/2] = '=';
		//displays the progress bar string right padded to 50 chars
		optionalPrint("%3d%% [%-51s%s", *percent, progressBar, "]\r");
		fflush(stdout);
	}
}

/**
 * gets the size of the file to be indexed
 */
long getFileSize(const char *filename) {
    struct stat fileStat;
    if (stat(filename, &fileStat) == 0)
	   return fileStat.st_size;
    fprintf(stderr, "Cannot determine size of %s: \n", filename);
    return -1;
}

	//-------------- MAINLINE ---------------//
 
	//------------- XML PARSING -------------//
/**
 * Gets the title from the parser. Returns true if success, false if EOF
 */
bool getTitle (xmlTextReaderPtr xmlReader, int *moreWiki, GString* articleTitle) {
	
	
	// loops until it finds the title tag or EOF
	// one tree at a time, skips sub trees
    while (1 == *moreWiki) {
		
		// loops until xmlReader is back down to level 2
		// one node at a time
		while (1 == *moreWiki && 2 > xmlTextReaderDepth (xmlReader)) {
			*moreWiki = xmlTextReaderRead (xmlReader);
		}
		// loops until it finds the title tag, end of subtree or EOF
		// one tree at a time, skips sub trees
		while (1 == *moreWiki && 2 <= xmlTextReaderDepth (xmlReader)) {
			if (xmlStrEqual(xmlTextReaderConstName(xmlReader), (const unsigned char *) "title")) {
				*moreWiki = xmlTextReaderRead(xmlReader);
				articleTitle = g_string_assign(articleTitle, (gchar *) xmlTextReaderConstValue(xmlReader));
				return (1 == *moreWiki);
			}
			*moreWiki = xmlTextReaderNext(xmlReader);
		}
	}
	// this should only happen at the end of the document
	return (1 == *moreWiki);
}

/**
 * Determines if the namespace is 0 (namespace of articles)
 * returns true if the NS is 0
 */
bool isNameSpaceZero (xmlTextReaderPtr xmlReader, int *moreWiki) {
	
	
	// loops until it finds the NS tag or EOF
	// one tree at a time, skips sub trees
    while (1 == *moreWiki) {
		*moreWiki = xmlTextReaderNext(xmlReader);
		//printf("loc21\n");
		if (xmlStrEqual(xmlTextReaderConstName(xmlReader), (const unsigned char *) "ns")) {
			break;}
	}
	*moreWiki = xmlTextReaderRead(xmlReader);
	
	return (xmlStrEqual(xmlTextReaderConstValue(xmlReader), (const unsigned char *) "0")
			&& (1 == *moreWiki));
}

/**
 * Determines if the page is a redirect
 * returns true if the page is a redirect
 */
bool isNotRedirect (xmlTextReaderPtr xmlReader, int *moreWiki) {
	
	// loops until it finds the redirect tag, revision tag, or EOF
	// one tree at a time, skips sub trees
    while (1 == *moreWiki) {
		//			printf("loc31\n");
		if (xmlStrEqual(xmlTextReaderConstName(xmlReader), (const unsigned char *) "revision")) {
			return TRUE;} // drop out of loop, assign body
		else if (xmlStrEqual(xmlTextReaderConstName(xmlReader), (const unsigned char *) "redirect")) {
			return FALSE;} //redirect tag, to to next page
		*moreWiki = xmlTextReaderNext(xmlReader);
	}
	// this should never happen
	return FALSE;
}


/**
 * Parses the XML tree until it finds an article body, and sets the article body to what is found in the XML
 */
void getArticleBody (xmlTextReaderPtr xmlReader, int *moreWiki, GString* articleBody) {
	
	*moreWiki = xmlTextReaderRead(xmlReader); // at level 2, need to drop to 3
	
	// loops until it finds the text tag or EOF
	// one tree at a time, skips sub trees
	do {*moreWiki = xmlTextReaderNext(xmlReader);}
	while (1 == *moreWiki && !(xmlStrEqual(xmlTextReaderConstName(xmlReader), (const unsigned char *) "text")));
	*moreWiki = xmlTextReaderRead(xmlReader);
	articleBody = g_string_assign(articleBody, (gchar *) xmlTextReaderConstValue(xmlReader));
}


/**
 * Parses XML tree until it fills the article or end of file
 */
bool getArticle (xmlTextReaderPtr xmlReader, article* currentArticle) {
	
	int moreWiki = 1;
	
	// filters through XML file until a valid article is found
	// XML files are structured like a tree, so is code to filter
	// loops until a valid article is found or EOF
	while (1 == moreWiki) {
		if (getTitle (xmlReader, &moreWiki, currentArticle->title)) { 
			if (isNameSpaceZero (xmlReader, &moreWiki)) {
				if (isNotRedirect (xmlReader, &moreWiki)) {
					getArticleBody (xmlReader, &moreWiki, currentArticle->body);
					return TRUE; // have an article, done!
				}
			}
		}
	}
	return FALSE; // EOF, wiki has ended, loop no more
}

	//-------------- ARTICLE HANDLING ---------------//
	
/**
 * removes any of the wiki markup that would interfere with ngramming
 * 
 * article - gstring of article to remove markup from
 */
void removeMarkup(GString *article) {
	gsize currentExamined = 0;
	
	// removes markup that could cause issues with indexing
	// checks one character per iteration
	while (currentExamined < article->len) {
		
		// removes <..> tags
		if ((unsigned char) article->str[currentExamined] == (unsigned char) '<') {
			//blank current position to end of markup
			do {article->str[currentExamined] = ' '; currentExamined++; }
			while (article->str[currentExamined] != '>' && currentExamined < article->len);
			article->str[currentExamined] = ' ';
		}
		// removes &..; tags
		else if (article->str[currentExamined] == '&') {
			//blank current position to end of markup
			do {article->str[currentExamined] = ' '; currentExamined++; }
			while (article->str[currentExamined] != ';' && currentExamined < article->len);
			article->str[currentExamined] = ' '; 
		}
		//removes [..] and [..|..] tags
		else if ( article->str[currentExamined] == '[' )  {
			int firstBracket = currentExamined;
			int finalPipe 	= currentExamined; 
			int openBrackets = 1;
			
			// blank current position to last pipe in this level of markup
			 do {currentExamined++;
				if (article->str[currentExamined] == '[') {
					openBrackets++;}
				else if (article->str[currentExamined] == ']'){
					openBrackets--;}
				else if ((article->str[currentExamined] == '|') && (openBrackets == 2)) {
					finalPipe = currentExamined;}}
			while (openBrackets > 0 && currentExamined < article->len);

			if (finalPipe != firstBracket) {currentExamined = finalPipe;}
			while (firstBracket < currentExamined) {
				article->str[firstBracket] = ' '; firstBracket++;}
			article->str[currentExamined] = ' '; 
		}
		// removes style==".."
		else if (article->str[currentExamined] == '"' && article->str[currentExamined - 1] == '=' ) {
			//blank the style= tag we just passed
			article->str[currentExamined - 6] = ' '; //s
			article->str[currentExamined - 5] = ' '; //t
			article->str[currentExamined - 4] = ' '; //y
			article->str[currentExamined - 3] = ' '; //l
			article->str[currentExamined - 2] = ' '; //e
			article->str[currentExamined - 1] = ' '; //=
			//blank current position to end of markup
			do {article->str[currentExamined] = ' '; currentExamined++; }
			while (article->str[currentExamined] != '"' && currentExamined < article->len);
			article->str[currentExamined] = ' ';
		}
	currentExamined++;
	}
}

/**
 * builds an ngram bit array out of the current text
 * 
 * text - article body
 * result - bit array index of article
 */
void indexText(GString *text, bool result[lastNgram]) {

	(*text).len = (*text).len - ((*text).len % 3);
	
	//builds an ngram vector for the text
	//adds one ngram per iteration
	for (int index = 0; index < (*text).len; index = index + 3)
		// formula to convert from a 3 digit base 26 number to a base 10 int
		// sets the value at that position to true
		{result[(((int) (*text).str[index])	* 676 + 
						((int) (*text).str[index + 1])	* 26   + 
						((int) (*text).str[index + 2]))   - 68191] = 1;}
}

/**
 * adds the article bit array to the collection bit array / index,
 * zeroes the article index
 * 
 * article index - bit array index of an article
 * wikiIndex - bit / judy array of collection
 */
void addToIndex(bool articleIndex[lastNgram], Pvoid_t *wikiIndex, int articleNumber) {

	int returnCode;
	// adds the current document to the wikiIndex
	// adds document to one ngram per iteration
	// sets articleDictionary to default values
	for (Word_t currentNgram = 0; currentNgram < lastNgram; currentNgram++) {
		if (articleIndex[currentNgram] == 1) {
			J1S(returnCode, wikiIndex[currentNgram], articleNumber);
			articleIndex[currentNgram] = 0;
		}
	}
}

/**
 * filters, transforms, and indexes file using ngrams to the index
 * 
 * file name - name of file to process
 * wikiindex - the judy arrays to store the index of the wiki in
 */
void indexWiki(char* inFileName, Pvoid_t *wikiIndex, int* articleCount) {
	
	//-------------------- initialization --------------------//
	bool articleIndex[lastNgram] = {0}; // boolean array of what trigrams are in an article
	struct stemmer * currentStemmer = create_stemmer();
	
	// file for writing the titles to
	FILE* titleFile = NULL;
	if (writeFiles) {
		titleFile = fopen("title_file", "w");
		if (NULL == titleFile) {
			fprintf(stderr, "Error open title file: %m\n");
			exit(1);
		}
	}
	
	// initializes the libxml library
	LIBXML_TEST_VERSION
	xmlTextReaderPtr wikiReader; //the reader for the document
	wikiReader = xmlReaderForFile(inFileName, NULL, XML_PARSE_RECOVER+XML_PARSE_NOWARNING+XML_PARSE_NOERROR+XML_PARSE_HUGE);
	if (NULL == wikiReader) {
		//fprintf(stderr, "%s %s\n", "Failed to open ", wikiFileName);
		fprintf(stderr, "Error opening XML wiki: %m\n");
		exit(1);
	}

	// for progress bar
	int percent = 0;
	long fileSize = getFileSize(inFileName);
	
	// initialization for currentArticle and its componens 
	article currentArticle;	
	currentArticle.title = g_string_sized_new(256);
	currentArticle.body  = g_string_sized_new(786432); //768*1024

	//-------------------- index the wiki --------------------//
	optionalPrint ("%s", "Adding collection to index.\n");
	optionalPrint ("%d", (int)(fileSize / 1048576));
	optionalPrint (" MB in file\n");
	displayProgressBar (xmlTextReaderByteConsumed(wikiReader), fileSize, &percent);
	 
	//prime the loop
	currentArticle.title->len = 0;
	currentArticle.body->len  = 0;
	xmlTextReaderRead(wikiReader);// at a <page> tag, drop in
	xmlTextReaderRead(wikiReader);// at a <page> tag, drop in
	 
	// reads from xml file until file is finished, adds articles to index, and writes tittles to file
	// processes one article per iteration
	while (getArticle (wikiReader, &currentArticle)) {
		currentArticle.articleNumber = *articleCount;
		*articleCount = *articleCount + 1;
		// filter / transform text
		removeMarkup(currentArticle.body);
		stemText(currentArticle.body, currentStemmer); //ngramming.h
		// index the text
		indexText(currentArticle.body, articleIndex); //ngramming.h
		addToIndex(articleIndex, wikiIndex, currentArticle.articleNumber);
		//adds titles to title file
		if (writeFiles) {fprintf(titleFile, "%s\n", currentArticle.title->str);}
		//re-prime the loop
		currentArticle.title->len = 0;
		currentArticle.body->len  = 0;
		displayProgressBar (xmlTextReaderByteConsumed(wikiReader), fileSize, &percent);
	}
	optionalPrint ("\n%s", "Finished indexing. \n");
	optionalPrint ("%lu", (long)(xmlTextReaderByteConsumed(wikiReader)/1048576));
	optionalPrint ("MB consumed\n");
	
	
	optionalPrint ("%d %s %d %s", *articleCount, "articles found", (int) currentArticle.body->allocated_len, "length allocated for article body\n");
	// clean up of structures needed to process wiki
	if (writeFiles) {fclose (titleFile);}
	free_stemmer(currentStemmer);
	xmlFreeTextReader(wikiReader);
	xmlCleanupParser();
	//g_string_free(currentArticle.title, TRUE);
	//g_string_free(currentArticle.body, TRUE); //malloc fail if this is uncommented ?!
}

/**
 * writes statistics about each ngram
 * calculates the IDF for all ngrams
 * writes the frequency of each ngram and the total number of ngrams ....???.... before it, offset into index file.
 */
void writeNgramStats(Pvoid_t *wikiIndex, int articleCount) {
	Word_t totalIndexes = 0;
	Word_t indexes      = 0;
	
	float highestIDF = 0.0;
	float lowestIDF  = log2f ((float) articleCount);
	
	int highestIDFNgram = 0;
	int lowestIDFNgram = 0;
		
	ngramStats currentStats = {0,0,0.0};

	FILE *ngramStatsFile = NULL;
	if (writeFiles) {
		ngramStatsFile = fopen("ngramStatsFile.bin", "wb");
		if (NULL == ngramStatsFile) {
		//	fprintf(stderr, "%s %s\n", "Failed to open ", statsfilename);
			fprintf(stderr, "Error opening NGram Stats File: %m\n");
			exit(1);
		}
	}

	// calculates the IDF and total cumulative article #s preceeding this ngram's block
	// writes the IDF, count of documents this ngram appears in, and the count of article #s preceeding
	// one ngram per iteration
	for (Word_t currentNgram = 0; currentNgram < lastNgram; currentNgram++) {
		J1C (indexes, wikiIndex[currentNgram], 0, -1); //count indices
		if (indexes) { //avoid a divide by zero
			currentStats.IDF = log2f ((float) articleCount / (float) indexes);
			if (currentStats.IDF > highestIDF) {
				highestIDF = currentStats.IDF;
				highestIDFNgram = currentNgram;
			}
			else if (currentStats.IDF < lowestIDF) {
				lowestIDF = currentStats.IDF;
				lowestIDFNgram = currentNgram;
			}
		}
		else {
			currentStats.IDF = 0.0;}
		currentStats.count    = (int) indexes;
		currentStats.offset = (int) totalIndexes;
		if (writeFiles) {fwrite(&currentStats, sizeof(ngramStats), 1, ngramStatsFile);}
		totalIndexes = totalIndexes + indexes;
	}

	if (writeFiles) {fclose (ngramStatsFile);}
	//intToNgram(lowestIDFNgram, char_ngram);
	//intToNgram(highestIDFNgram, char_ngram);
	optionalPrint ("%d %s", highestIDFNgram, "is the ngram with the highest IDF\n");
	optionalPrint ("%d %s", lowestIDFNgram, "is the ngram with the lowest IDF\n");
	optionalPrint ("%.4f %.3f %s", log2f (1.0), log2f ((float) articleCount), "possible range of IDF\n");
	optionalPrint ("%.4f %.3f %s", lowestIDF, highestIDF, "actual range of IDF\n");
}

/**
 * Builds an array of 0 to documentCount of documentNumbers from a Judy array
 * if input contains 0 ielements behavior is undefined
 */
void judyToArray (Pvoid_t *input, int *result, int *resultLength) {
	
	Word_t articleNumber = 0;
	Word_t moreIndexes = 0;
	*resultLength = 0;
	
	J1F (moreIndexes, *input, articleNumber);
	while (moreIndexes) {
		result[*resultLength] = (int) articleNumber;
		*resultLength = *resultLength + 1;
		J1N (moreIndexes, *input, articleNumber);
	}
}

/**
 * Transforms the index in Judy arrays to a flat binary file. (writeNgramStats writes the offsets into the file)
 */
void writeIndex (Pvoid_t *wikiIndex, int articleCount, char* outFileName) {
	// list of articles an ngram appears in
	int *articleList = (int*)malloc(articleCount*sizeof(int));
	int articleListLength = 0; // number of values in above list
	
	FILE *indexFile;
	indexFile = fopen(outFileName, "wb");
	if (NULL == indexFile) {
		fprintf(stderr, "Error opening index file: %m\n");
		exit(1);
	}
	
	// turns the judy arrays into a regular array, then writes them to a file
	// one Judy array per iteration
	for (Word_t currentNgram = 0; currentNgram < lastNgram; currentNgram++) {
		judyToArray (&wikiIndex[currentNgram], articleList, &articleListLength);
		if (writeFiles) {fwrite(articleList, sizeof(articleList[0]), articleListLength, indexFile);}
	}

	free (articleList);
	fclose (indexFile);
}

/**
 * Frees the Judy arrays that make up the index and gathers some basic information about them.
 */
void freeIndex (Pvoid_t *wikiIndex) {
	Word_t returnCode    = 0;
	Word_t totalIndexes  = 0;
	Word_t indexes       = 0;
	Word_t totalSize     = 0;
	Word_t size          = 0;
	
	// frees the judy arrays in the index and adds the total indexes and the size of each judy array
	// one Judy array per iteration  
	for (int currentNgram = 0; currentNgram < lastNgram; currentNgram++) {
		J1C (indexes, wikiIndex[currentNgram], 0, -1); //count indices
		totalIndexes = totalIndexes + indexes;
		J1MU (size, wikiIndex[currentNgram]); // memory usage
		totalSize = totalSize + size;
		J1FA (returnCode, wikiIndex[currentNgram]); //free
	}
	optionalPrint("%d %s", (int)(totalSize/1048576), "MB of memory used\n");
	optionalPrint("%d", (int)((totalIndexes*4)/1048576));
	optionalPrint(" MB if not compressed\n");	
	optionalPrint("Index freed.\n");
}

/**
 * makes sure the command line options are valid
 * exits if they are not valid, passes them on if they are valid
 */ 
int checkCLIParams (int argc, char **argv, gchar** inFileName, gchar** outFileName) {
		
	writeFiles = FALSE; // YES, THIS IS INTENTIONAL, flops because of how gopt parse works
		
	GOptionEntry entries[] = {
		{ "silent",  's', 0, G_OPTION_ARG_NONE,     &silent,     "No output to standard out", NULL },
		{ "nowrite", 'n', 0, G_OPTION_ARG_NONE,     &writeFiles, "Do not write index or other files", NULL },
		{ "in",      'i', 0, G_OPTION_ARG_FILENAME, inFileName,  "Name of file to read from", "FILE"},
		{ "out",     'o', 0, G_OPTION_ARG_FILENAME, outFileName, "Name of file to write to", "FILE"},
		{ NULL }
	};

	GOptionContext *context;
	context = g_option_context_new ("- infile outfile");
	g_option_context_add_main_entries (context, entries, 0);
	
	if (!g_option_context_parse (context, &argc, &argv, NULL)) {
		g_print (g_option_context_get_help (context, FALSE, NULL));
		return 0;
	}
	
	writeFiles = !writeFiles; // YES, THIS IS INTENTIONAL, flops because of how gopt parse works
	
	if (NULL == *inFileName) {
		// infilename required
		fprintf (stderr, "Input file name required.\n");
		return 0;
	}
	else if (access (*inFileName, R_OK)) {  // <unistd.h>
		//unable to access file for reading
		fprintf (stderr, "Cannot read from specified input file. Aborting. No changes have been made.\n");
		return 0;
	}
	
	// file writing sanity checks
	if (writeFiles) { // required to write files?
		if (NULL == *outFileName) { // is outfilename set?
			fprintf (stderr, "Output filename required to write files. See --help for more options\n");
			return 0;
		}
		else if (access (*outFileName, F_OK) && !access (*outFileName, W_OK)) {  // <unistd.h>
			// file exists and not able to write to
			fprintf (stderr, "Cannot write to specified output file. Aborting. No changes have been made.\n");
			return 0;
		}
		// outfile might be non-existant, but on a non-writeable partition, will fail later
	}
	return 1;
}

/**
 * main... :
 *   sets the CLI parameters
 *   startup operations
 *   kicks off the indexing
 *   writes the results
 *   frees the large data structures in use
 *   done!
 */
int main (int argc, char **argv) {
	gchar* inFileName  = NULL;
	gchar* outFileName = NULL;
	
	if (! checkCLIParams (argc, argv, &inFileName, &outFileName)) {
		//failed to parse the CLI opts
		return (1);
	}
	
	// count of articles found in collection
	int articleCount = 0;
	// array of judy arrays for the index
	Pvoid_t wikiIndex[lastNgram] = {(Pvoid_t) NULL};

	
	indexWiki (inFileName, wikiIndex, &articleCount);
	
	writeNgramStats (wikiIndex, articleCount);
	if (writeFiles) {writeIndex (wikiIndex, articleCount, outFileName);}
	freeIndex (wikiIndex);
		
	optionalPrint ("%s", "Finished!\n");
	return(0);
}
