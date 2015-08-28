indexWiki
=========
Given a wikipedia .xml database will create an index based off that .xml file utilizing n-grams as the indexing terms.
Search is halfway implemented, need to add features to the indexer first.
Creates two files:
n-gram stats: the IDF, # of docs that ngram appears in, and the appropriate offset into the larger file
index: a binary file that is a list of the document IDs that each ngram appears in.


For databases see:
http://en.wikipedia.org/wiki/Wikipedia:Database_download
and others... uncyclopedias *was* available at one point.

To build:
Depends on Judy Arrays, lfds611, and glib (as in gnome lib)
LibLFDS available here: https://github.com/liblfds
```
"apt-get install libjudy"
gcc -Wall -pipe `pkg-config --cflags --libs glib-2.0` -std=c11 -lJudy -l pthread -L. wiki_reader.c spmc_producer.c stemmer.c -o "%e" "%f" -llfds611
```

Works with -oFast if you are so inclined.

# Done:
- write titles - as recieved or as one big chunk?
- write idf (ngram and freq also for analysis)
- write # of documents an ngram is in next to IDF
- some sort of progress bar?
- write index - csv x = ngram, y = list of articles that value appears in
- move ngram procedures to .h file
- move stemmer to procedure parameter
- move article count to procedure parameter
- file opening safety checks
- usage statement, -s silent, -n no output file
- command line opts refinement
- filenames from globals to parameters
- profile - the I/O shouldn't be *that* slow - 70% of all time is xmltextreaderread
- moreWiki removed from being a variable to a returned value
- fix free (gstrting) - no more malloc fail - probably memcp - create additional buffer - no longer utlizing memcp
- multi thread -- probably not worth much, I/O bound
 - g_str_to_ascii () - after threading
 
# Not going to do:
- interactive shell? find all .XML files in the current directory for input
- ncurses? -- rest of program needs to be more robust first - little added value
- additional index select file - moving to SQL instead of expanding binary files
- -p output file name prefix
- check if infile is valid XML

# To do:
- improve accuracy of markup remover
- move to porter2 -  after threading
- move progress bar to .h / .c file, add features (ETA and rate to progress bar)
- add auto -human readable to filesize / progress bar
- SQLite - replace flat file  - replace judy too?
 - check if outfile is valid SQL at CLI parse
- indexs to word_t types -- i think this is done
- rename elements of ngramStats
- change formatting filtering from 4x ifs to a case statement
- change to lighter weight / faster XML parser - 75% of time is in xmltextreaderread
- change from xmltextreaderread to xmltextreadernext - will need logic & flow changes
- combine -n (nowritefile) and -o options, if you provide both you're being redundant
