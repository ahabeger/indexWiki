indexWiki
=========
Given a wikipedia .xml database will create an index based off that .xml file utilizing n-grams as the indexing terms.
Search is halfway implemented, need to add features to the indexer first.
Creates an n-gram indexed binary file with offsets into the larger index file.


For databases see:
http://en.wikipedia.org/wiki/Wikipedia:Database_download
and others... uncyclopedias *was* available at one point.

To build:
Depends on Judy Arrays, libxml2, and glib (as in gnome lib)
"apt-get install libxml2 libjudy"
gcc -Wall `pkg-config --cflags --libs glib-2.0 libxml-2.0` -lJudy -std=c11 -march=native -pipe stemmer.c ngramming.c indexWiki.c -o indexWiki

Works with -oFast if you are so inclined.

 Done:
 -- write titles - as recieved or as one big chunk?
 -- write idf (ngram and freq also for analysis)
 -- write # of documents an ngram is in next to IDF
 -- some sort of progress bar?
 -- write index - csv x = ngram, y = list of articles that value appears in
 -- move ngram procedures to .h file
 -- move stemmer to procedure parameter
 -- move article count to procedure parameter
 -- file opening safety checks
 -- usage statement, -s silent, -n no output file
 -- command line opts refinement
 -- filenames from globals to parameters
 -- profile - the I/O shouldn't be *that* slow - 70% of all time is xmltextreaderread

 Not going to do:
 interactive shell? find all .XML files in the current directory for input
 ncurses? -- rest of program needs to be more robust first - little added value
 additional index select file - moving to SQL instead of expanding binary files
 -p output file name prefix
 check if infile is valid XML

 To do:
 
 move progress bar to .h / .c file, add features
 ETA and rate to progress bar
 add auto -human readable to filesize / progress bar
 multi thread -- probably not worth much, I/O bound
 	g_str_to_ascii () - after threading
 	move to porter2 -  after threading
 SQLite - replace flat file  - replace judy too?
	check if outfile is valid SQL at ClI parse
 moreWiki removed from being a variable to a returned value
 indexs to word_t types -- i think this is done
 rename elements of ngramStats
 fix free (gstrting) - no more malloc fail
 change formatting filtering from 4x ifs to a case statement
 change to lighter weight / faster XML parser - 75% of time is in xmltextreaderread
 change from xmltextreaderread to xmltextreadernext - will need logic & flow changes
