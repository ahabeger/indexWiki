#include "ngramming.h" // ngramming functions
#include "stemmer.h"   // stemmer & stem
#include <glib.h>      // GString functions, g_ascii_isalpha
#include <ctype.h>	   // tolower
#include <string.h>	   // memmove

extern void intToNgram (int intNgram, char charNgram[3]) {
	//int modified = int_ngram;
	//char_ngram[3] = '\0';
	charNgram[2] = 'a' + intNgram % 26;
	intNgram           = intNgram / 26;
	charNgram[1] = 'a' + intNgram % 26;
	intNgram           = intNgram / 26;
	charNgram[0] = 'a' + intNgram;
}

/**
 * stems the words in the text and compresses the text to the begnining of the string
 */
extern void stemText(GString *text, struct stemmer * currentStemmer) {
	int currentExamined = 0; //general index into article string
	int wordBegin 		= 0; //index of where a word begins
	int wordLength		= 0; //lenth of the word that is being handled / stemmed
	int stemmedLen		= 0; //the length of the article after stemmed words compressed to begining
		
	//stems the words in the provided article, lowercases all letters, concentrates text at begining
	//examines one character per iteration
	while (currentExamined < text->len) {
		if g_ascii_isalpha( (unsigned char) text->str[currentExamined] ) {
			text->str[currentExamined] = tolower(text->str[currentExamined]);
			wordBegin = currentExamined;
			
			//find the end of the word
			do {text->str[currentExamined] = tolower(text->str[currentExamined]);
				currentExamined++;}
			while (g_ascii_isalpha((unsigned char) text->str[currentExamined] ));

			wordLength = currentExamined - wordBegin;
			
			//the word has ended, if it is long enough it will be stemmed then moved to begning of article string
			if (wordLength > 2) {
				wordLength = stem(currentStemmer, text->str+wordBegin, wordLength - 1 ) + 1;
				memmove(text->str+stemmedLen, text->str+wordBegin, wordLength);
				stemmedLen = stemmedLen + wordLength;
			}
		}
		currentExamined++;
	}
	text->len = stemmedLen;
}
