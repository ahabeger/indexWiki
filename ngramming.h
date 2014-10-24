#ifndef NGRAMMING_H
#define NGRAMMING_H
// helpers and common code for indexWiki & and search wiki

#include <glib.h> // GString
#include <stdbool.h>
#include "stemmer.h"   // stemmer & stem

// 26^3
#define lastNgram 17576

typedef struct {
	int count;  // # of articles the ngram appears in
	int offset; // offset into binary file
	float IDF;
	} ngramStats;

void intToNgram (int intNgram, char charNgram[3]);

/**
 * stems the words in the text and compresses the text to the begnining of the string
 */
void stemText(GString *articleBody, struct stemmer * currentStemmer);

#endif
