#include "wiki_reader.h" // ...
#include "spmc_producer.h" // ...
#include "stemmer.h"	// for stemmer & stem
#include <stdlib.h>      // malloc, exit
#include <glib.h>        // GString
#include <stdio.h>	     // printf
#include <string.h>      //strlen
#include <Judy.h>	   // J1 calls, Pvoid_t
#include <pthread.h> // pthread_t, mutex
#include <ctype.h> // tolower


#define last_ngram 17576
	
	//char* filename = "enwiki-20141208-pages-articles.xml";
	char* filename = "uncyclo_pages_current.xml";

	//-------------------- TYPES --------------------//
	/*
typedef struct {
	GString *title;
	GString *body;
	int articleNumber;
	} article_t;
	*/
typedef struct {
	pthread_mutex_t lock; // locks for the current element in the index
	Pvoid_t articles_with_ngram;// = {(Pvoid_t) NULL};
	float inter_doc_freq;
	} index_element_t;
	
typedef struct {
	producer_t* producer;
	index_element_t* index;
	int thread_ID;
	int num_threads;
	int article_count;
	} consumer_synchro_t;
	
/**
 * removes any of the wiki markup that would interfere with ngramming
 * 
 * article - gstring of article to remove markup from
 */
void remove_markup (GString *body) {
	gsize currentExamined = 0;
	
	// removes markup that could cause issues with indexing
	// checks one character per iteration
	while (currentExamined < body->len) {
		
		// removes <..> tags
		if ((unsigned char) body->str[currentExamined] == (unsigned char) '<') {
			//blank current position to end of markup
			do {body->str[currentExamined] = ' '; currentExamined++; }
			while (body->str[currentExamined] != '>' && currentExamined < body->len);
			body->str[currentExamined] = ' ';
		}
		// removes &..; tags
		else if (body->str[currentExamined] == '&') {
			//blank current position to end of markup
			do {body->str[currentExamined] = ' '; currentExamined++; }
			while (body->str[currentExamined] != ';' && currentExamined < body->len);
			body->str[currentExamined] = ' '; 
		}
		//removes [..] and [..|..] tags
		else if (body->str[currentExamined] == '[' )  {
			int firstBracket = currentExamined;
			int finalPipe 	= currentExamined; 
			int openBrackets = 1;
			
			// blank current position to last pipe in this level of markup
			 do {currentExamined++;
				if (body->str[currentExamined] == '[') {
					openBrackets++;}
				else if (body->str[currentExamined] == ']'){
					openBrackets--;}
				else if ((body->str[currentExamined] == '|') && (openBrackets == 2)) {
					finalPipe = currentExamined;}}
			while (openBrackets > 0 && currentExamined < body->len);

			if (finalPipe != firstBracket) {currentExamined = finalPipe;}
			while (firstBracket < currentExamined) {
				body->str[firstBracket] = ' '; firstBracket++;}
			body->str[currentExamined] = ' '; 
		}
		// removes style==".."
		else if (body->str[currentExamined] == '"' && body->str[currentExamined - 1] == '=' ) {
			//blank the style= tag we just passed
			body->str[currentExamined - 6] = ' '; //s
			body->str[currentExamined - 5] = ' '; //t
			body->str[currentExamined - 4] = ' '; //y
			body->str[currentExamined - 3] = ' '; //l
			body->str[currentExamined - 2] = ' '; //e
			body->str[currentExamined - 1] = ' '; //=
			//blank current position to end of markup
			do {body->str[currentExamined] = ' '; currentExamined++; }
			while (body->str[currentExamined] != '"' && currentExamined < body->len);
			body->str[currentExamined] = ' ';
		}
	currentExamined++;
	}
}

void index_text (GString *text, struct stemmer *currentStemmer, bool result[17576]) { //struct stemmer * currentStemmer)
	int currentExamined = 0; //general index into article string
	int wordBegin 		= 0; //index of where a word begins
	int wordEnd			= 0; // duuuuuh
		
	int current_ngram	= 0;
	int ngram_position  = 0;
	
	//stems the words in the provided article, lowercases all letters, indexes as it goes along
	//examines one character per iteration
	while (currentExamined < text->len) {
		if g_ascii_isalpha ((unsigned char) text->str[currentExamined]) {
			ngram_position = 0;
			wordBegin = currentExamined;
			
			//find the end of the word
			do {text->str[currentExamined] = tolower (text->str[currentExamined]);
				currentExamined++;}
			while (g_ascii_isalpha((unsigned char) text->str[currentExamined]));

			//wordLength = currentExamined - wordBegin;
			//wordEnd = currentExamined;
			
			//the word has ended, if it is long enough it will be stemmed then converted to an ngram
			if (2 < (currentExamined - wordBegin)) {
				
				wordEnd = wordBegin + stem (currentStemmer, text->str+wordBegin, currentExamined - wordBegin - 1 ) + 1;
				
				// where does word length point? word length++ needed?
				// convert the letters of the word to portions of an ngram
				ngram_position = 0;
				// converts one letter per iteration
				while (wordBegin < wordEnd) {
					switch (ngram_position) {
						case 0:
							current_ngram = text->str [wordBegin] * 676;
							ngram_position = 1;
							break;
						case 1:
							current_ngram = text->str [wordBegin] * 26;
							ngram_position = 2;
							break;
						case 2:
							current_ngram = text->str [wordBegin];
							result [current_ngram - 68191] = 1;
							ngram_position = 0;
							break;
						}
					wordBegin++;
					}
				}
			}
	currentExamined++;
	}

}

/**
 * calculate the IDFs
 *
void calculate_IDFs (

	
	int ngramArticleCount = 0; // number of articles an ngram appears in, # of elements in above array
	double currentIDF = 0.0;
	for (Word_t current_ngram = 0; currentNgram < slice_length; current_ngram++) {
		//judyToArray (&wikiIndex[currentNgram], articleList, &ngramArticleCount);
		if (ngramArticleCount) { //avoid a divide by zero
			currentIDF = log2f ((double) articleCount / (double) ngramArticleCount);}
		else {
			currentIDF = 0.0;}
	}
*/

/**
 * adds the article bit array to the collection bit array / index,
 * zeroes the article index
 * 
 * article index - bit array index of an article
 * wikiIndex - bit / judy array of collection
 */
void add_to_index (bool article_index[last_ngram], index_element_t *working_index, int article_number) {

	int return_code;
	// adds the current document to the wikiIndex
	// adds document to one ngram per iteration
	// sets articleDictionary to default values
	for (Word_t current_ngram = 0; current_ngram < last_ngram; current_ngram++) {
		if (1 == article_index[current_ngram]) {
			pthread_mutex_lock (&working_index[current_ngram].lock);
			J1S (return_code, working_index[current_ngram].articles_with_ngram, article_number);
			pthread_mutex_unlock (&working_index[current_ngram].lock);
			article_index[current_ngram] = 0;
		}
	}
}

/**
 * what is called by the consumer threads
 */
void index_articles (consumer_synchro_t* synchro) {
	
	int articles_consumed = 0;
	int thread_ID = __atomic_fetch_add (&synchro->thread_ID, 1, __ATOMIC_RELAXED);
	
	struct stemmer * currentStemmer;
	currentStemmer = create_stemmer();
	bool article_index[last_ngram] = {0}; // boolean array of what trigrams are in an article
	
	article_t* article = NULL;
	char* original_body = NULL; // g_str_to_ascii copies article to this, and does not free the original
	// bool articleIndex[lastNgram] = {0}; // boolean array of what trigrams are in an article
	
	spmc_producer_consumer_check_in (synchro->producer);
	
	while (spmc_producer_get_item (synchro->producer, (void**) &article)) {
		original_body = article->body->str;
		article->body->str = g_str_to_ascii (article->body->str, NULL);
		g_free (original_body); // or can i just re-assign it?
		article->body->len = strlen (article->body->str);
		
		/*
		if (1000 == article->article_number) {
			fprintf (stdout, article->body->str);
		}
		* */
		remove_markup (article->body);
		/*
		if (1000 == article->article_number) {
			fprintf (stdout, article->body->str);
		}
		* */
		index_text(article->body, currentStemmer, article_index); //ngramming.h
		//ngram text
		// uncomment the following line if you want to see the title and
		// lengh of every article in the wiki
		g_string_free (article->title, TRUE);
		g_string_free (article->body, TRUE);
		free (article);
		articles_consumed++;
	}
	spmc_producer_consumer_check_out (synchro->producer);
	
	free (currentStemmer);
	
	__atomic_add_fetch (&synchro->article_count, articles_consumed, __ATOMIC_RELAXED);
		
	fprintf (stderr, "%d left %d right\n", ((last_ngram / 8) * thread_ID), ((last_ngram / 8) * thread_ID) + (last_ngram / 8));
	fprintf (stderr, "Thread %d consumed %d articles.\n", thread_ID, articles_consumed,);
}

void consume_articles  (producer_t* producer, int num_threads) {
	consumer_synchro_t synchro;
	synchro.num_threads = num_threads;
	synchro.producer = producer;
	synchro.thread_ID = 0;
	synchro.index = malloc ((sizeof (index_element_t)) * last_ngram);
	pthread_t* article_indexers = malloc (sizeof (pthread_t) * num_threads);
	
	for (int current_ngram = 0; current_ngram < last_ngram; current_ngram++) {
		pthread_mutex_init (&(synchro.index[current_ngram].lock), NULL);
		synchro.index[current_ngram].articles_with_ngram = (Pvoid_t) NULL;
	}
	
	for (int current_thread = 0; current_thread < num_threads; current_thread++) {
		pthread_create (&article_indexers [current_thread], NULL, (void *) index_articles, (void *) &synchro);
	}
	
	for (int current_thread = 0; current_thread < num_threads; current_thread++) {
		pthread_join (article_indexers [current_thread], NULL);
	}
}

int main () {
	
	int num_threads = 8;
	
	wiki_reader_t* wiki_reader = wiki_reader_new (filename, 8192, 0);
	producer_t* producer = spmc_producer_new (1024, (bool (*)(void *, void*)) wiki_reader_get_article, wiki_reader);
	
	// allocate synchronizing structure here
	
	spmc_producer_start (producer);
	consume_articles (producer, (num_threads)); // 1 thread for producer, remainder for consumers
	spmc_producer_wait (producer);
	spmc_producer_free (producer);
	wiki_reader_free (wiki_reader);
	
	// synchrnonizing structure to stored database transformation
	return 0;
}
