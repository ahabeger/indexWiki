#include "wiki_reader.h" // ...
#include <stdio.h>	     // printf
#include <glib.h>        // GString

	char* filename = "enwiki-20141208-pages-articles.xml";
	//char* filename = "uncyclo_pages_current.xml";
	
void get_wiki (wiki_reader_t* wiki_reader) {
	
	int articles_processed = 0;
	
	article_t article;
	article.title = g_string_sized_new(256);
	article.body  = g_string_sized_new(786432); //768*1024
	
	while (wiki_reader_get_article (wiki_reader, &article)) {
		// uncomment the following line if you want to see the title and
		// lengh of every article in the wiki
		//fprintf (stderr, "Article Title: %s Body Length: %ld\n", article.title->str, article.body->len);
		articles_processed++;
	}
	
	fprintf (stderr, "Processed %d articles\n", articles_processed);
	
	g_string_free (article.title, TRUE);
	g_string_free (article.body, TRUE);
}

void print_title (wiki_reader_t* wiki_reader) {
	GString* wiki_name = wiki_reader_get_wiki_name (wiki_reader);	
	fprintf (stderr, "The name of the wiki is: %s\n", wiki_name->str);
}

int main () {
	
	wiki_reader_t* wiki_reader = wiki_reader_new (filename, 8192, 0);
		
	print_title (wiki_reader);
	get_wiki (wiki_reader);
	wiki_reader_free (wiki_reader);
	return 0;
}
