//
// htmerge.cc
//
// Merge the databases into a form usable by htsearch
// Updates databases to remove old documents and 
// ensures the databases are consistent.
//
//
// $Log: htmerge.cc,v $
// Revision 1.12  1999/02/04 00:29:51  ghutchis
// Updated documentation on command-line flags.
//
// Revision 1.11  1999/01/23 18:22:09  ghutchis
// Unlink the db.docs.index file before we build it again. This ensures we have
// a clean copy and don't duplicate URLs.
//
// Revision 1.10  1999/01/21 13:41:22  ghutchis
// Check HtURLCodec for errors.
//
// Revision 1.9  1999/01/09 20:18:07  ghutchis
// Set up merge_config file and add options for mergeDB code.
//
// Revision 1.8  1998/12/05 00:52:04  ghutchis
// Remove previous db.words.db file before doing a word merging. Fixes bug with
// deleted documents keeping entries.
//
// Revision 1.7  1998/12/04 04:13:51  ghutchis
// Use configure check to only include getopt.h when it exists.
// 
// Revision 1.5  1998/10/02 17:07:32  ghutchis
// More configure changes
//
// Revision 1.4  1998/08/03 16:50:43  ghutchis
// Fixed compiler warnings under -Wall
//
// Revision 1.3  1998/01/05 05:43:24  turtle
// format changes
//
// Revision 1.2  1997/06/23 02:24:49  turtle
// Added version info to usage message
//
// Revision 1.1.1.1  1997/02/03 17:11:06  turtle
// Initial CVS
//
//

#include "htmerge.h"

// If we have this, we probably want it.
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

//
// This hash is used to keep track of all the document IDs which have to be
// discarded.
// The wordlist file contains the information on which docs need to go.
//
Dictionary	discard_list;

// This config is used for merging multiple databses
Configuration    merge_config;

int		verbose = 0;
int		stats = 0;

void usage();
void reportError(char *msg);

//*****************************************************************************
// int main(int ac, char **av)
//
int main(int ac, char **av)
{
    int			do_words = 1;
    int			do_docs = 1;
    int			alt_work_area = 0;
    String		configfile = DEFAULT_CONFIG_FILE;
    String              merge_configfile = 0;
    int			c;
    /* Currently unused    extern int		optind; */
    extern char		*optarg;

    while ((c = getopt(ac, av, "svm:c:dwa")) != -1)
    {
	switch (c)
	{
	    case 'd':
		do_docs = 0;
		break;
	    case 'w':
		do_words = 0;
		break;
	    case 'c':
		configfile = optarg;
		break;
	    case 'm':
	      	merge_configfile = optarg;
	      	break;
	    case 'v':
		verbose++;
		break;
	    case 's':
		stats++;
		break;
	    case 'a':
		alt_work_area++;
		break;
	    case '?':
		usage();
		break;
	}
    }

    config.Defaults(&defaults[0]);

    if (access(configfile, R_OK) < 0)
    {
	reportError(form("Unable to find configuration file '%s'",
			 configfile.get()));
    }
	
    config.Read(configfile);
    
    //
    // Check url_part_aliases and common_url_parts for
    // errors.
    String url_part_errors = HtURLCodec::instance()->ErrMsg();

    if (url_part_errors.length() != 0)
      reportError(form("Invalid url_part_aliases or common_url_parts: %s",
                       url_part_errors.get()));

    if (merge_configfile.length())
    {
    	merge_config.Defaults(&defaults[0]);
	if (access(merge_configfile, R_OK) < 0)
    	{
	reportError(form("Unable to find configuration file '%s'",
			 merge_configfile.get()));
    	}
	merge_config.Read(merge_configfile);
    }

    if (alt_work_area != 0)
    {
	String	configValue;

	configValue = config["word_list"];
	if (configValue.length() != 0)
	{
	    configValue << ".work";
	    config.Add("word_list", configValue);
	}

	configValue = config["word_db"];
	if (configValue.length() != 0)
	{
	    configValue << ".work";
	    config.Add("word_db", configValue);
	}

	configValue = config["doc_db"];
	if (configValue.length() != 0)
	{
	    configValue << ".work";
	    config.Add("doc_db", configValue);
	}

	configValue = config["doc_index"];
	if (configValue.length() != 0)
	{
	    configValue << ".work";
	    config.Add("doc_index", configValue);
	}
    }
    
    if (merge_configfile.length())
    {
	// Merge the databases specified in merge_configfile into the current
	// databases. Do this first then update the other databases as usual
	// Note: We don't have to specify anything, it's all in the config vars

	mergeDB();
    }
    
    
    String	file1, file2;
    if (do_words)
    {
	file1 = config["word_list"];
	file2 = config["word_db"];
	// Before we start, get rid of the previous one
	// it doesn't matter if it's .work or not.
	// This ensures we generate it from scratch and prevents duplicates
	unlink(file2);
	mergeWords(file1, file2);
    }
    if (do_docs)
    {
	file1 = config["doc_db"];
	file2 = config["doc_index"];
	// Before we start, get rid of the previous one
	// it doesn't matter if it's .work or not.
	// This ensures we generate it from scratch and prevents duplicates
	unlink(file2);
	convertDocs(file1, file2);
    }
    return 0;
}


//*****************************************************************************
// void usage()
//   Display program usage information
//
void usage()
{
    cout << "usage: htmerge [-v][-d][-w][-c configfile][-m merge_configfile]\n";
    cout << "This program is part of ht://Dig " << VERSION << "\n\n";
    cout << "Options:\n";
    cout << "\t-v\tVerbose mode.  This increases the verbosity of the\n";
    cout << "\t\tprogram.  Using more than 2 is probably only useful\n";
    cout << "\t\tfor debugging purposes.  The default verbose mode\n";
    cout << "\t\tgives a progress on what it is doing and where it is.\n\n";
    cout << "\t-d\tDo NOT merge the document database.\n\n";
    cout << "\t-w\tDo NOT merge the word database.\n\n";
    cout << "\t-m merge_configfile\n";
    cout << "\t\tMerge the databases specified into the databases specified\n";
    cout << "\t\tby -c or the default.\n\n";
    cout << "\t-c configfile\n";
    cout << "\t\tUse the specified configuration file instead on the\n";
    cout << "\t\tdefault.\n\n";
    cout << "\t-a\tUse alternate work files.\n";
    cout << "\t\tTells htmerge to append .work to database files causing\n";
    cout << "\t\ta second copy of the database to be built.  This allows\n";
    cout << "\t\toriginal files to be used by htsearch during the indexing\n";
    cout << "\t\trun.\n\n";
    exit(0);
}


//*****************************************************************************
// Report an error and die
//
void reportError(char *msg)
{
    cout << "htmerge: " << msg << "\n\n";
    exit(1);
}
