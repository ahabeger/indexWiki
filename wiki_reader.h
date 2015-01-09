#ifndef WIKI_ARTICLE_PRODUCER
#define WIKI_ARTICLE_PRODUCER

#include <glib.h>    // GString
#include <stdbool.h> // ...
#include <stddef.h>  // size_t

typedef struct {
	GString* title;
	GString* body;
	size_t  article_number;
	} article_t;

struct reader_struct;
typedef struct reader_struct wiki_reader_t;

/**
 * Creates a wiki reader
 * 
 * file_name - the file to be read
 * buffer_max - the size of the buffer internal to the reader (128 or greater, 8192 works best for me)
 * article_id_start - articles are produced with an article_number, this determined their starting
 * 		value
 */
wiki_reader_t* wiki_reader_new (char* file_name, size_t buffer_max, size_t article_id_first);

/**
 * gets the name of the wiki
 * 
 * callable until wiki_reader_free has completed
 */ 
GString* wiki_reader_get_wiki_name (wiki_reader_t* wiki_reader);

/**
 * reads an article from the wiki
 * 
 * article - the latest article from the wiki - needs to have been g_string_new or variation before called
 * 
 * returns true if there is a valid article supplied.
 * returns false if there are no more articles in the wiki and there is no valid article supplied
 */ 
bool wiki_reader_get_article (wiki_reader_t* wiki_reader, article_t* article);

/**
 * provides the current position within the file and the total amount to be read
 */ 
void wiki_reader_progress (wiki_reader_t* wiki_reader, long *position, long *end);

/**
 * has the parser has finished or not
 */ 
bool wiki_reader_finished (wiki_reader_t* wiki_reader);

/**
 * frees article_producer and all the structures contained within
 */ 
void wiki_reader_free (wiki_reader_t* wiki_reader);

#endif
