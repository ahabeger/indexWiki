#include "wiki_reader.h" // ...
#include <sys/stat.h>    // stat
#include <stdio.h>	     // printf
#include <stdbool.h>     // ...
#include <stdlib.h>      // malloc, exit
#include <string.h>      // strncpy, strstr...


struct reader_struct {
		FILE*	file;			// the file corresponding to this slice
		char*	buffer;			// buffer that gets searched regularly for the slice
		char*	buffer_first; 	// a pointer within buffer of the last character examined
		size_t	buffer_length;	// position of last valid character in the buffer (set by fread usually)
		size_t	buffer_max;		// the maximum size of the buffer
		size_t	article_count;	// the number of articles processed (initial value set in init)
		long	end;			// the number of bytes in the file
		long	position;		// the number of bytes read in the file
		GString* wiki_name;		// the title of the file
};

/**
 * gets the size of the file to be indexed
 */
long get_file_size (const char* file_name) {
    struct stat file_stat;
    if (stat(file_name, &file_stat) == 0)
	   return (long int) file_stat.st_size;
    fprintf (stderr, "Cannot determine size of %s: \n", file_name);
    return -1;
}

/*
 * returns true if item found and slice.buffer_first points at the element searched for
 * needle_len = minimum # of items in buffer after buffer_first 
 */
bool find_next (wiki_reader_t* wiki_reader, char* needle, int needle_len) {

	do {wiki_reader->buffer_first = strstr (wiki_reader->buffer_first, needle);
		if (NULL == wiki_reader->buffer_first || (wiki_reader->buffer_first - wiki_reader->buffer) + needle_len > wiki_reader->buffer_length) {
			fseek (wiki_reader->file, (needle_len - 1) * -1, SEEK_CUR); // in case the all but one character of the needle was in the buffer
			wiki_reader->buffer_length = fread (wiki_reader->buffer, 1, wiki_reader->buffer_max, wiki_reader->file); 
			wiki_reader->buffer_first = wiki_reader->buffer;
		} else {
			return TRUE;
		}
	} while (! (feof (wiki_reader->file)));
	
	// special case
	// the portion of the buffer [0 .. buffer_length] is still valid searchable area.
	wiki_reader->buffer_first = strstr (wiki_reader->buffer, needle);
	if (NULL == wiki_reader->buffer_first) {
		// search string not found in FILE
		return FALSE;
	} else {
		// needle found in remainder of buffer
		return wiki_reader->buffer_length > (size_t) (wiki_reader->buffer_first - wiki_reader->buffer);
	}
}

void get_title (wiki_reader_t* wiki_reader, GString* title) {
	title = g_string_set_size (title, 0);
		
	find_next (wiki_reader, "<title>", 7);
	wiki_reader->buffer_first = wiki_reader->buffer_first + 7;
	
	char* title_end = strchr (wiki_reader->buffer_first, '<');
	// only gauranteed to have <title> in the buffer, nothing about the rest of the title
	while (NULL == title_end) {
	    title = g_string_append_len (title, wiki_reader->buffer_first, (wiki_reader->buffer - wiki_reader->buffer_first) + wiki_reader->buffer_length);
		wiki_reader->buffer_length = fread (wiki_reader->buffer, 1, wiki_reader->buffer_max, wiki_reader->file); 
		wiki_reader->buffer_first = wiki_reader->buffer;
	    title_end = strchr (wiki_reader->buffer_first, '<');
	}
	title = g_string_append_len (title, wiki_reader->buffer_first, title_end - wiki_reader->buffer_first);
}

bool is_ns_zero (wiki_reader_t* wiki_reader) {
	find_next (wiki_reader, "<ns>", 5); // 5 to ensure that there are a minimum of 5 characters in the buffer
	return '0' == *(wiki_reader->buffer_first + 4);
}

bool is_revision (wiki_reader_t* wiki_reader) {
	
	find_next (wiki_reader, "<re", 4);
	
	// is it a restroction tag?
	if ('s' == wiki_reader->buffer_first[3]) {
		wiki_reader->buffer_first = wiki_reader->buffer_first + 1;
		find_next (wiki_reader, "<re", 4);
	}

	// is it a revision?
	if ('v' == wiki_reader->buffer_first[3]) {
		//revision_count = revision_count + 1; 
		return TRUE;
	}
	// is it a redirect?
	else if ('d' == wiki_reader->buffer_first[3]) {
		return FALSE;
	}
	
	return FALSE;
}

void get_body (wiki_reader_t* wiki_reader, GString* body) {
	body = g_string_set_size (body, 0);
	
	find_next (wiki_reader, "<text xml", 9); // not all wikimedia files have the same tag for text
	find_next (wiki_reader, ">", 2);
	wiki_reader->buffer_first = wiki_reader->buffer_first + 1;
		
	char* body_end = strchr (wiki_reader->buffer_first, '<');
	while (NULL == body_end) {
	    body = g_string_append_len (body, wiki_reader->buffer_first, (wiki_reader->buffer - wiki_reader->buffer_first) + wiki_reader->buffer_length);
		wiki_reader->buffer_length = fread (wiki_reader->buffer, 1, wiki_reader->buffer_max, wiki_reader->file); 
		wiki_reader->buffer_first = wiki_reader->buffer;
	    body_end = strchr (wiki_reader->buffer_first, '<');
	}
	
	body = g_string_append_len (body, wiki_reader->buffer_first, body_end - wiki_reader->buffer_first);
}

/**
 * assumes sizename is 64 characters or less or fits into buffer
 */ 
void get_wiki_name (wiki_reader_t* wiki_reader) {

	find_next (wiki_reader, "<sitename>", 75);

	wiki_reader->buffer_first = wiki_reader->buffer_first + 10; // 10 == length of the needle, first now points to site name

	wiki_reader->wiki_name = g_string_new (NULL);
	wiki_reader->wiki_name = g_string_append_len (wiki_reader->wiki_name, wiki_reader->buffer_first, strchr (wiki_reader->buffer_first, '<') - wiki_reader->buffer_first);
}

	//-------------- EXTERNAL FUNCTIONS ---------------//
	
// see .h for documentation
wiki_reader_t* wiki_reader_new (char* file_name, size_t buffer_max, size_t article_id_first) {

	wiki_reader_t* result = (wiki_reader_t*) malloc (sizeof (wiki_reader_t));
	
	result->file = fopen (file_name , "r");
	result->buffer_max  = buffer_max;
	result->buffer = (char*) malloc (result->buffer_max * sizeof (char));
	result->buffer_first = result->buffer;
	result->buffer_length = fread (result->buffer, 1, result->buffer_max, result->file);
	result->article_count = article_id_first;
	result->position = 0;
	result->end = get_file_size (file_name);
	
	get_wiki_name (result);
	
	return result;
}

// see .h for documentation
bool wiki_reader_get_article (wiki_reader_t* wiki_reader, article_t* article) {
	
	// searches file for tags that identify an article
	// returns one article per iteration
	while (! (feof (wiki_reader->file))) {
		if (find_next (wiki_reader, "<page>", 6)) {
			// every page has a title
			get_title (wiki_reader, article->title);
			if (is_ns_zero (wiki_reader)) {
				// only want NS 0 items
				if (is_revision (wiki_reader)) {
					// only want revisions, not redirects
					get_body (wiki_reader, article->body);
				    article->article_number = wiki_reader->article_count;
				    wiki_reader->article_count++;
				    wiki_reader->position = ftell (wiki_reader->file);
					return TRUE; // have an article, done!
				}
			}
		}
	}
	// EOF reached, close the file and portions of the struct that are private
	wiki_reader->position = wiki_reader->end;
	fclose (wiki_reader->file);
	free (wiki_reader->buffer);
	return FALSE;
}

GString* wiki_reader_get_wiki_name (wiki_reader_t* wiki_reader) {
	return wiki_reader->wiki_name;
}

void wiki_reader_progress (wiki_reader_t* wiki_reader, long *position, long *end) {
	// might not be thread safe
	*position = wiki_reader->position;
	*end = wiki_reader->end;
}

bool wiki_reader_finished (wiki_reader_t* wiki_reader) {
	return wiki_reader->position == wiki_reader->end;
}

void wiki_reader_free (wiki_reader_t* wiki_reader) {
	g_string_free (wiki_reader->wiki_name, TRUE);
	free (wiki_reader);
}
