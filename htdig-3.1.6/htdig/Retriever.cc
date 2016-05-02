//
// Retriever.cc
//
// Implementation of Retriever
//
// $Id: Retriever.cc,v 1.36.2.28 2002/01/25 04:44:33 ghutchis Exp $
//

#include "Retriever.h"
#include "htdig.h"
#include "WordList.h"
#include "URLRef.h"
#include "Server.h"
#include "Parsable.h"
#include "Document.h"
#include "StringList.h"
#include <pwd.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include "HtWordType.h"

static WordList	words;
static int noSignal;


//*****************************************************************************
// Retriever::Retriever()
//
Retriever::Retriever(RetrieverLog flags)
{
    FILE	*urls_parsed;

    currenthopcount = 0;
    max_hop_count = config.Value("max_hop_count", 999999);
		
    //
    // Initialize the weight factors for words in the different
    // HTML headers
    //
    factor[0] = config.Double("text_factor"); // Normal words
    factor[1] = config.Double("title_factor");
    factor[2] = config.Double("heading_factor_1");
    factor[3] = config.Double("heading_factor_2");
    factor[4] = config.Double("heading_factor_3");
    factor[5] = config.Double("heading_factor_4");
    factor[6] = config.Double("heading_factor_5");
    factor[7] = config.Double("heading_factor_6");
    factor[8] = 0;
    factor[9] = 0;
    factor[10] = config.Double("keywords_factor");
    factor[11] = config.Double("meta_description_factor");
	
    //
    // Open the file to which we will append words.
    //
    String	filename = config["word_list"];
    words.WordTempFile(filename);
    words.BadWordFile(config["bad_word_list"]);

    doc = new Document();
    minimumWordLength = config.Value("minimum_word_length", 3);

    log = flags;
    // if in restart mode
    if (Retriever_noLog != log ) 
    {
    String	filelog = config["url_log"];
    char buffer[1000];	// FIXME
    int  l;

	urls_parsed = fopen(filelog,   "r" );
	if (0 != urls_parsed)
        {
  	    // read all url discovered but not fetched before 
	    while (fgets(buffer, sizeof(buffer), urls_parsed))
      	    {
	       l = strlen(buffer);
	       assert(l && buffer[l -1] == '\n');
	       buffer[l -1] = 0;
	       Initial(buffer,2);
            }
            fclose(urls_parsed);
	}
        unlink(filelog);
    }
}


//*****************************************************************************
// Retriever::~Retriever()
//
Retriever::~Retriever()
{
    delete doc;
}


//*****************************************************************************
// void Retriever::setUsernamePassword(char *credentials)
//
void
Retriever::setUsernamePassword(char *credentials)
{
    doc->setUsernamePassword(credentials);
}


//*****************************************************************************
// void Retriever::Initial(char *list, int from)
//   Add a single URL to the list of URLs to visit.
//   Since URLs are stored on a per server basis, we first need to find the
//   the correct server to add the URL's path to.
//
//   from == 0 urls in db.docs and no db.log
//   from == 1 urls in start_url add url only if not already in the list 
//   from == 2 add url from db.log 
//   from == 3 urls in db.docs and there was a db.log 
//
void
Retriever::Initial(char *list, int from)
{
    //
    // Split the list of urls up into individual urls.
    //
    StringList	tokens(list, " \t");
    String	sig;
    String      url;
    Server	*server;

    for (int i = 0; i < tokens.Count(); i++)
    {
	URL	u(tokens[i]);
	server = (Server *) servers[u.signature()];
	url = u.get();
	if (debug > 2)
           cout << "\t" << from << ":" << (int) log << ":" << url;
	if (!server)
	{
	    String robotsURL = "http://";
	    robotsURL << u.host() << "/robots.txt";
	    StringList *localRobotsFiles = GetLocal(robotsURL.get());
	    server = new Server(u.host(), u.port(), localRobotsFiles);
	    servers.Add(u.signature(), server);
	    delete localRobotsFiles;
	}
	else if (from && visited.Exists(url)) 
	{
	    if (debug > 2)
                cout << " skipped" << endl;
	    continue;
	}
        if (Retriever_noLog == log || from != 3) 
        {
	    if (debug > 2)
                cout << " pushed";
	    server->push(u.get(), 0, 0, IsLocalURL(u.get()));
        }
	if (debug > 2)
           cout << endl;
	visited.Add(url, 0);
    }
}


//*****************************************************************************
// void Retriever::Initial(List &list, int from)
//
void
Retriever::Initial(List &list,int from)
{
    list.Start_Get();
    String	*str;
    // from == 0 is an optimisation for pushing url in update mode
    //  assuming that 
    // 1) there's many more urls in docdb 
    // 2) they're pushed first
    // 3) there's no duplicate url in docdb
    // then they don't need to be check against already pushed urls
    // But 2) can be false with -l option
    //
    // FIXME it's nasty, what have to be test is :
    // we have urls to push from db.docs but do we already have them in
    // db.log? For this it's using a side effect with 'visited' and that
    // urls in db.docs are only pushed via this method, and that db.log are pushed
    // first, db.docs second, start_urls third!
    //  
    if (!from && visited.Count())  
    {
       from = 3;
    }
    while ((str = (String *) list.Get_Next()))
    {
	Initial(str->get(),from);
    }
}

//*****************************************************************************
//
static void sigexit(int)
{
 noSignal=0;
}

//*****************************************************************************
// static void sig_handlers
//	initialise signal handlers
//
static void
sig_handlers(void)
{
struct sigaction action;

 /* SIGINT, SIGQUIT, SIGTERM */
 action.sa_handler = sigexit;
 sigemptyset(&action.sa_mask);
 action.sa_flags = 0;
 if (sigaction(SIGINT, &action, NULL) != 0)
	reportError("Cannot install SIGINT handler\n");
 if(sigaction(SIGQUIT, &action, NULL) != 0)
	reportError("Cannot install SIGQUIT handler\n");
 if(sigaction(SIGTERM, &action, NULL) != 0)
	reportError("Cannot install SIGTERM handler\n");
 if(sigaction(SIGHUP, &action, NULL) != 0)
	reportError("Cannot install SIGHUP handler\n");
}


//*****************************************************************************
// void Retriever::Start()
//   This is the main loop of the retriever.  We will go through the
//   list of paths stored for each server.  While parsing the
//   retrieved documents, new paths will be added to the servers.  We
//   return if no more paths need to be retrieved.
//
void
Retriever::Start()
{
    //
    // Main digger loop.  The todo list should initialy have the start
    // URL and all the URLs which were seen in a previous dig.  The
    // loop will continue as long as there are more URLs to visit.
    //
    int		more = 1;
    Server	*server;
    URLRef	*ref;
    
    //  
    // Always sig . The delay bother me but a bad db is worst
    // 
    if ( Retriever_noLog != log ) 
    {
	sig_handlers();
    }
    noSignal = 1;

    while (more && noSignal)
    {
	more = 0;
		
	//
	// Go through all the current servers in sequence.  We take only one
	// URL from each server during this loop.  This ensures that the load
	// on the servers is distributed evenly.
	//
	servers.Start_Get();
	while ( (server = (Server *)servers.Get_NextElement()) && noSignal)
	{
	    if (debug > 1)
		cout << "pick: " << server->host() << ", # servers = " <<
		    servers.Count() << endl;
	    
	    ref = server->pop();
	    if (!ref)
		continue;		      // Nothing on this server
	    // There may be no more documents, or the server
	    // has passed the server_max_docs limit

	    //
	    // We have a URL to index, now.  We need to register the
	    // fact that we are not done yet by setting the 'more'
	    // variable.
	    //
	    more = 1;

	    //
	    // Deal with the actual URL.
	    // We'll check with the server to see if we need to sleep()
	    // before parsing it.
	    //
	    server->delay();   // This will pause if needed and reset the time
	    parse_url(*ref);
            delete ref;
	}
    }
    // if we exited on signal 
    if (Retriever_noLog != log && !noSignal) 
    {
    FILE	*urls_parsed;
    String	filelog = config["url_log"];
        // save url seen but not fetched
	urls_parsed = fopen(filelog,   "w" );
	if (0 == urls_parsed)
	{
	    reportError(form("Unable to create URL log file '%s'",
			     filelog.get()));
	}
        else {
  	   servers.Start_Get();
	   while ((server = (Server *)servers.Get_NextElement()))
	   {
   	      while (NULL != (ref = server->pop())) 
              {
      	          fprintf(urls_parsed, "%s\n", ref->URL());
 		  delete ref;
              }
           }
           fclose(urls_parsed);
        }
    }
}


//*****************************************************************************
// void Retriever::parse_url(URL &url)
//
void
Retriever::parse_url(URLRef &urlRef)
{
    URL			url;
    DocumentRef	*ref;
    int			old_document;
    time_t		date;
    static int	index = 0;
    Server		*server;
    static int		local_urls_only = config.Boolean("local_urls_only");
    static int		mark_dead_servers = config.Boolean("ignore_dead_servers");

//	cout << "**** urlRef URL = '" << urlRef.URL() << "', referer = '" <<
//		urlRef.Referer() << "'\n";
    url.parse(urlRef.URL());
	
    currenthopcount = urlRef.HopCount();
    ref = GetRef(url.get());
    if (ref)
    {
	//
	// We already have an entry for this document in our database.
	// This means we can get the document ID and last modification
	// time from there.
	//
	current_id = ref->DocID();
	date = ref->DocTime();
	if (ref->DocAccessed())
	  old_document = 1;
	else // we haven't retrieved it yet, so we only have the first link
	  old_document = 0;
	ref->DocBackLinks(ref->DocBackLinks() + 1); // we had a new link
	ref->DocAccessed(time(0));
	ref->DocState(Reference_normal);
        currenthopcount=ref->DocHopCount();
    }
    else
    {
	//
	// Never seen this document before.  We need to create an
	// entry for it.  This implies that it gets a new document ID.
	//
	date = 0;
	current_id = docs.NextDocID();
	ref = new DocumentRef;
	ref->DocID(current_id);
	ref->DocURL(url.get());
	ref->DocState(Reference_normal);
	ref->DocAccessed(time(0));
        ref->DocHopCount(currenthopcount);
	ref->DocBackLinks(1); // We had to have a link to get here!
	old_document = 0;
    }

    words.DocumentID(ref->DocID());

    if (debug > 0)
    {
	//
	// Display progress
	//
	cout << index++ <<
	    ':' << current_id <<
	    ':' << currenthopcount <<
	    ':' << url.get() << ": ";
	cout.flush();
    }
    
    // Reset the document to clean out any old data
    doc->Reset();

    doc->Url(url.get());
    doc->Referer(urlRef.Referer());

    base = doc->Url();

    // Retrive document, first trying local file access if possible.
    Document::DocStatus status;
    server = (Server *) servers[url.signature()];
    StringList *local_filenames = GetLocal(url.get());
    if (local_filenames)
    {  
        if (debug > 1)
	    cout << "Trying local files" << endl;
        status = doc->RetrieveLocal(date, local_filenames);
        if (status == Document::Document_not_local)
        {
	    if (local_urls_only)
		status = Document::Document_not_found;
	    else if (server && !server->IsDead())
	    {
        	if (debug > 1)
		    cout << "Local retrieval failed, trying HTTP" << endl;
		status = doc->RetrieveHTTP(date);
	    }
	    else
		status = Document::Document_no_server;
        }
        delete local_filenames;
    }
    else if (server && !server->IsDead())
        status = doc->RetrieveHTTP(date);
    else
	status = Document::Document_no_server;

    current_ref = ref;
	
    //
    // Determine what to do by looking at the status code returned by
    // the Document retrieval process.
    //
    switch (status)
    {
	case Document::Document_ok:
	    trackWords = 1;
	    if (old_document)
	    {
	      if (doc->ModTime() == ref->DocTime())
		{
		  if (debug)
		    cout << " retrieved but not changed" << endl;
		  words.MarkScanned();
		  break;
		}
		//
		// Since we already had a record of this document and
		// we were able to retrieve it, it must have changed
		// since the last time we scanned it.  This means that
		// we need to assign a new document ID to it and mark
		// the old one as obsolete.
		//
		words.MarkModified();
	        int backlinks = ref->DocBackLinks();
		delete ref;
		current_id = docs.NextDocID();
		words.DocumentID(current_id);
		ref = new DocumentRef;
		ref->DocID(current_id);
		ref->DocURL(url.get());
		ref->DocState(Reference_normal);
		ref->DocAccessed(time(0));
		ref->DocHopCount(currenthopcount);
		ref->DocBackLinks(backlinks);
		if (debug)
		    cout << " (changed) ";
	    }
	    RetrievedDocument(*doc, url.get(), ref);
	    // Hey! If this document is marked noindex, don't even bother
	    // adding new words. Mark this as gone and get rid of it!
	    if (ref->DocState() == Reference_noindex)
	      words.MarkModified();
	    else
	      words.Flush();
	    if (debug)
		cout << " size = " << doc->Length() << endl;
	    break;

	case Document::Document_not_changed:
	    if (debug)
		cout << " not changed" << endl;
	    words.MarkScanned();
	    break;

	case Document::Document_not_found:
	    ref->DocState(Reference_not_found);
	    if (debug)
		cout << " not found" << endl;
	    recordNotFound(url.get(),
			   urlRef.Referer(),
			   Document::Document_not_found);
	    words.MarkGone();
	    break;

	case Document::Document_no_host:
	    ref->DocState(Reference_not_found);
	    if (debug)
		cout << " host not found" << endl;
	    recordNotFound(url.get(),
			   urlRef.Referer(),
			   Document::Document_no_host);
	    words.MarkGone();
	    if (server && mark_dead_servers)
		server->IsDead(1);
	    break;

	case Document::Document_no_server:
	    ref->DocState(Reference_not_found);
	    if (debug)
		cout << " no server running" << endl;
	    recordNotFound(url.get(),
			   urlRef.Referer(),
			   Document::Document_no_server);
	    words.MarkGone();
	    if (server && mark_dead_servers)
		server->IsDead(1);
	    break;

	case Document::Document_not_html:
	    if (debug)
		cout << " not HTML" << endl;
	    words.MarkGone();
	    break;

	case Document::Document_redirect:
	    if (debug)
		cout << " redirect" << endl;
	    words.MarkGone();
	    got_redirect(doc->Redirected(), ref);
	    break;
	    
       case Document::Document_not_authorized:
	    if (debug)
	      cout << " not authorized" << endl;
	    break;

      case Document::Document_not_local:
	   if (debug)
	     cout << " not local" << endl;
	   break;
    }
    docs.Add(*ref);
    delete ref;
}


//*****************************************************************************
// void Retriever::RetrievedDocument(Document &doc, char *url, DocumentRef *ref)
//   We found a document that needs to be parsed.  Since we don't know the
//   document type, we'll let the Document itself return an appropriate
//   Parsable object which we can call upon to parse the document contents.
//
void
Retriever::RetrievedDocument(Document &doc, char *, DocumentRef *ref)
{
    n_links = 0;
    current_ref = ref;
    current_anchor_number = 0;
    current_title = 0;
    current_time = 0;
    current_head = 0;
    current_meta_dsc = 0;

    //
    // Create a parser object and let it have a go at the document.
    // We will pass ourselves as a callback object for all the got_*()
    // routines.
    // This will generate the Parsable object as a specific parser
    //
    Parsable	*parsable = doc.getParsable();
    if (parsable)
      parsable->parse(*this, *base);
    else
      { // If we didn't get a parser, then we should get rid of this!
	ref->DocState(Reference_noindex);
	return;
      }

    //
    // We don't need to dispose of the parsable object since it will
    // automatically be reused.
    //

    //
    // Update the document reference
    //
    ref->DocHead(current_head);
    ref->DocMetaDsc(current_meta_dsc);
    if (current_time == 0)
      ref->DocTime(doc.ModTime());
    else
      ref->DocTime(current_time);
    ref->DocTitle(current_title);
    ref->DocSize(doc.Length());
    ref->DocAccessed(time(0));
    ref->DocLinks(n_links);
    ref->DocImageSize(ref->DocImageSize() + doc.Length());
}


//*****************************************************************************
// int Retriever::Need2Get(char *u)
//   Return TRUE if we need to retrieve the given url.  This will
//   check the list of urls we have already visited.
//
int
Retriever::Need2Get(char *u)
{
    static String	url;
    url = u;

    return !visited.Exists(url);
}



//*****************************************************************************
// int Retriever::IsValidURL(char *u)
//   Return TRUE if we need to retrieve the given url.  We will check
//   for limits here.
//
int
Retriever::IsValidURL(char *u)
{
    static Dictionary	*invalids = 0;
    static Dictionary	*valids = 0;

    //
    // Invalid extensions will be kept in a dictionary for quick
    // lookup.  Since the dictionary is static to this function, we
    // need to initialize it the first time we get here.
    //
    if (!invalids)
    {
	// A list of bad extensions, separated by spaces or tabs
	String	t = config["bad_extensions"];
	String	lowerp;
	char	*p = strtok(t, " \t");
	invalids = new Dictionary;
	while (p)
	{
	    // Extensions are case insensitive
	    lowerp = p;
	    lowerp.lowercase();
	    invalids->Add(lowerp, 0);
	    p = strtok(0, " \t");
	}
    }

    //
    // Valid extensions are performed similarly
    //
    if (!valids)
    {
	// A list of allowed extensions, separated by spaces or tabs
	String	t = config["valid_extensions"];
	String	lowerp;
	char	*p = strtok(t, " \t");
	valids = new Dictionary;
	while (p)
	{
	    // Extensions are case insensitive
	    lowerp = p;
	    lowerp.lowercase();
	    valids->Add(lowerp, 0);
	    p = strtok(0, " \t");
	}
    }

    static String	url;
    url = u;

    //
    // Currently, we only deal with HTTP URLs.  Gopher and ftp will
    // come later...  ***FIX***
    //
    if (strstr(u, "/../") || strncmp(u, "http://", 7) != 0)
      {
	if (debug > 2)
	  cout << endl <<"   Rejected: Not an http or relative link!";
	return FALSE;
      }

    //
    // If the URL contains any of the patterns in the exclude list,
    // mark it as invalid
    //
    if (excludes.hasPattern()) // Make sure there's an exclude list!
      {
	int retValue;     // Returned value of findFirst
	int myWhich = 0;    // Item # that matched [0 .. n]
	int myLength = 0;   // Length of the matching value
	retValue = excludes.FindFirst(url, myWhich, myLength);
	if (retValue >= 0)
	  {
	    if (debug > 2)
	      {
		myWhich++;         // [0 .. n] --> [1 .. n+1]
		cout << endl <<"  Rejected: Item in the exclude list: item # ";
		cout << myWhich << " length: " << myLength << endl;
	      }
	    return FALSE;
	  }
      }

    //
    // See if the path extension is in the list of invalid ones
    //
    char	*ext = strrchr(url, '.');
    String	lowerext;
    if (ext && strchr(ext, '/'))	// Ignore a dot if it's not in the
      ext = NULL;			// final component of the path.
    if (ext)
      {
	lowerext = ext;
	int parm = lowerext.indexOf('?');	// chop off URL parameter
	if (parm >= 0)
	    lowerext.chop(lowerext.length() - parm);
	lowerext.lowercase();
	if (invalids->Exists(lowerext))
	  {
	    if (debug > 2)
	      cout << endl <<"   Rejected: Extension is invalid!";
	    return FALSE;
	  }
      }

    //
    // Or NOT in the list of valid ones
    //
    if (ext && valids->Count() > 0 && !valids->Exists(lowerext))
      {
	if (debug > 2)
	  cout << endl <<"   Rejected: Extension is not valid!";
	return FALSE;
      }

    ext = strrchr(url, '?');
    if (ext && badquerystr.hasPattern() &&
       (badquerystr.FindFirst(ext) >= 0))
    {
      if (debug > 2)
	  cout << endl <<"   Rejected: Invalid Querystring!";
       return FALSE;
    }

    //
    // If any of the limits are met, we allow the URL
    //
    if (limits.FindFirst(url) >= 0)
	return TRUE;

    if (debug > 2)
      cout << endl <<"   Rejected: URL not in the limits!";

    return FALSE;
}


//*****************************************************************************
// StringList* Retriever::GetLocal(char *url)
//   Returns a list of strings containing the (possible) local filenames
//   of the given url, or 0 if it's definitely not local.
//   THE CALLER MUST FREE THE STRINGLIST AFTER USE!
//
StringList*
Retriever::GetLocal(char *url)
{
    static StringList *prefixes = 0;
    static StringList *paths = 0;
    static StringList *defaultdocs = 0;

    //
    // Initialize prefix/path list if this is the first time.
    // The list is given in format "prefix1=path1 prefix2=path2 ..."
    //
    if (!prefixes)
    {
    	prefixes = new StringList();
	paths = new StringList();
	defaultdocs = new StringList();

	String t = config["local_urls"];
	char *p = strtok(t, " \t");
	while (p)	
	{
   	    char *path = strchr(p, '=');
   	    if (!path)
	    {
		p = strtok(0, " \t");
   		continue;
	    }
   	    *path++ = '\0';
	    String *pre = new String(p);
	    decodeURL(*pre);
	    prefixes->Add(pre);
	    String *pat = new String(path);
	    decodeURL(*pat);
	    paths->Add(pat);
	    p = strtok(0, " \t");
	}
	t = config["local_default_doc"];
	p = strtok(t, " \t");
	while (p)	
	{
	    String *def = new String(p);
	    decodeURL(*def);
	    defaultdocs->Add(def);
	    p = strtok(0, " \t");
	}
	if (defaultdocs->Count() == 0)
	    delete defaultdocs;
    }

    // Begin by hex-decoding URL...
    String hexurl = url;
    decodeURL(hexurl);
    url = hexurl.get();

    // Check first for local user...
    if (strchr(url, '~'))
    {
	StringList *local = GetLocalUser(url, defaultdocs);
	if (local)
	    return local;
    }

    // This shouldn't happen, but check anyway...
    if (strstr(url, ".."))
        return 0;
    
    String *prefix, *path, *defaultdoc;
    StringList *local_names = new StringList();
    prefixes->Start_Get();
    paths->Start_Get();
    while ((prefix = (String*) prefixes->Get_Next()))
    {
	path = (String*) paths->Get_Next();
        if (mystrncasecmp(*prefix, url, prefix->length()) == 0)
	{
	    int l = strlen(url)-prefix->length()+path->length()+4;
	    String *local = new String(*path, l);
	    *local += &url[prefix->length()];
	    if (local->last() == '/' && defaultdocs) {
	      defaultdocs->Start_Get();
	      while (defaultdoc = (String *)defaultdocs->Get_Next()) {
		String *localdefault = new String(*local, local->length()+defaultdoc->length()+1);
		localdefault->append(*defaultdoc);
		local_names->Add(localdefault);
	      }
	      delete local;
	    }
	    else
	      local_names->Add(local);
	}	
    }
    if (local_names->Count() > 0)
        return local_names;

    delete local_names;
    return 0;
}


//*****************************************************************************
// StringList* Retriever::GetLocalUser(char *url, StringList *defaultdocs)
//   If the URL has ~user part, return a list of strings containing the
//   (possible) local filenames of the given url, or 0 if it's
//   definitely not local.
//   THE CALLER MUST FREE THE STRINGLIST AFTER USE!
//
StringList*
Retriever::GetLocalUser(char *url, StringList *defaultdocs)
{
    static StringList *prefixes = 0, *paths = 0, *dirs = 0;
    static Dictionary home_cache;
    String *defaultdoc;

    //
    // Initialize prefix/path list if this is the first time.
    // The list is given in format "prefix1=path1,dir1 ..."
    // If path is zero-length, user's home directory is looked up. 
    //
    if (!prefixes)
    {
        prefixes = new StringList();
	paths = new StringList();
	dirs = new StringList();
	String t = config["local_user_urls"];
	char *p = strtok(t, " \t");
	while (p)
	{
	    char *path = strchr(p, '=');
	    if (!path)
	    {
		p = strtok(0, " \t");
	        continue;
	    }
	    *path++ = '\0';
	    char *dir = strchr(path, ',');
	    if (!dir)
	    {
		p = strtok(0, " \t");
	        continue;
	    }
	    *dir++ = '\0';
	    String *pre = new String(p);
	    decodeURL(*pre);
	    prefixes->Add(pre);
	    String *pat = new String(path);
	    decodeURL(*pat);
	    paths->Add(pat);
	    String *ptd = new String(dir);
	    decodeURL(*ptd);
	    dirs->Add(ptd);
	    p = strtok(0, " \t");
	}
    }

    // Can we do anything about this?
    if (!strchr(url, '~') || !prefixes->Count() || strstr(url, ".."))
        return 0;

    // Split the URL to components
    String tmp = url;
    char *name = strchr(tmp, '~');
    *name++ = '\0';
    char *rest = strchr(name, '/');
    if (!rest || (rest-name <= 1) || (rest-name > 32))
        return 0;
    *rest++ = '\0';

    // Look it up in the prefix/path/dir table
    prefixes->Start_Get();
    paths->Start_Get();
    dirs->Start_Get();
    String *prefix, *path, *dir;
    StringList *local_names = new StringList();
    while ((prefix = (String*) prefixes->Get_Next()))
    {
        path = (String*) paths->Get_Next();
	dir = (String*) dirs->Get_Next();
        if (mystrcasecmp(*prefix, tmp) != 0)
  	    continue;

	String *local = new String;
	// No path, look up home directory
	if (path->length() == 0)
	{
	    String *home = (String*) home_cache[name];
	    if (!home)
	    {
	        struct passwd *passwd = getpwnam(name);
		if (passwd)
		{
		    home = new String(passwd->pw_dir);
		    home_cache.Add(name, home);
		}
	    }
	    if (home)
	        *local += *home;
	    else
	        continue;
	}
	else
	{
	    *local += *path;
	    *local += name;
	}
	*local += *dir;
	*local += rest;
	if (local->last() == '/' && defaultdocs) {
	  defaultdocs->Start_Get();
	  while (defaultdoc = (String *)defaultdocs->Get_Next()) {
	    String *localdefault = new String(*local, local->length()+defaultdoc->length()+1);
	    localdefault->append(*defaultdoc);
	    local_names->Add(localdefault);
	  }
	  delete local;
	}
	else
	  local_names->Add(local);
    }

    if (local_names->Count() > 0)
        return local_names;

    delete local_names;
    return 0;
}


//*****************************************************************************
// int Retriever::IsLocalURL(char *url)
//   Returns 1 if the given url has a (possible) local filename
//   or 0 if it's definitely not local.
//
int
Retriever::IsLocalURL(char *url)
{
    int ret;

    StringList *local_filename = GetLocal(url);
    ret = (local_filename != 0);
    delete local_filename;

    return ret;
}

 
//*****************************************************************************
// DocumentRef *Retriever::GetRef(char *u)
//   Extract the date field from the given url.  This will require a
//   lookup in the current document database.
//
DocumentRef*
Retriever::GetRef(char *u)
{
    static String	url;
    url = u;

    return docs[url];
}


//*****************************************************************************
// void Retriever::got_word(char *word, int location, int heading)
//   The location is normalized to be in the range 0 - 1000.
//
void
Retriever::got_word(char *word, int location, int heading)
{
    if (debug > 3)
	cout << "word: " << word << '@' << location << endl;
    if (heading > 11 || heading < 0) // Current limits for headings
      heading = 0;  // Assume it's just normal text
    if (trackWords)
    {
      String w = word;
      HtStripPunctuation(w);
      if (w.length() >= minimumWordLength)
	words.Word(w, location, current_anchor_number, factor[heading]);
      if (strcmp(word, w.get()) != 0)	// have punctuation that was stripped
      {
	// Check for compound words...
	String parts = word;
	int added;
	int nparts = 1;
	do
	{
	    added = 0;
	    char *start = parts.get();
	    char *punctp, *nextp, *p;
	    char  punct;
	    int   n;
	    while (*start)
	    {
		p = start;
		for (n = 0; n < nparts; n++)
		{
		    while (HtIsStrictWordChar((unsigned char)*p))
			p++;
		    punctp = p;
		    if (!*punctp && n+1 < nparts)
			break;
		    while (*p && !HtIsStrictWordChar((unsigned char)*p))
			p++;
		    if (n == 0)
			nextp = p;
		}
		if (n < nparts)
		    break;
		punct = *punctp;
		*punctp = '\0';
		if (*start && (*p || start > parts.get()))
		{
		    w = start;
		    HtStripPunctuation(w);
		    if (w.length() >= minimumWordLength)
		    {
			words.Word(w, location, current_anchor_number, factor[heading]);
			if (debug > 3)
			    cout << "word part: " << start << '@' << location << endl;
		    }
		    added++;
		}
		start = nextp;
		*punctp = punct;
	    }
	    nparts++;
	} while (added > 2);
      }
    }
}


//*****************************************************************************
// void Retriever::got_title(char *title)
//
void
Retriever::got_title(char *title)
{
    if (debug > 1)
	cout << "\ntitle: " << title << endl;
    current_title = title;
}


#define EPOCH	1970

//
// time_t parsedcdate(char *date)
//   - converts ISO 8601 (YYYY-MM-DD) date string into a time value
//
time_t
parsedcdate(char *date)
{
    char	*s;
    int		day, month, year, hour, minute, second;

    s = date;
    while (isspace(*s))
	s++;

    // get year...
    if (!isdigit(*s))
	return 0;
    year = 0;
    while (isdigit(*s))
	year = year * 10 + (*s++ - '0');
    if (year < 69)
	year += 2000;
    else if (year < 1900)
	year += 1900;
    else if (year >= 19100)	// seen some programs do it, why not check?
	year -= (19100-2000);
    while (isspace(*s))
	s++;

    // get month...
    if (!isdigit(*s))
	return 0;
    month = 0;
    while (isdigit(*s))
	month = month * 10 + (*s++ - '0');
    if (month < 1 || month > 12)
	return 0;
    while (*s == '-' || isspace(*s))
	s++;

    // get day...
    if (!isdigit(*s))
	return 0;
    day = 0;
    while (isdigit(*s))
	day = day * 10 + (*s++ - '0');
    if (day < 1 || day > 31)
	return 0;
    while (*s == '-' || isspace(*s))
	s++;

    // optionally get hour...
    hour = 0;
    while (isdigit(*s))
	hour = hour * 10 + (*s++ - '0');
    if (hour > 23)
	return 0;
    while (*s == ':' || isspace(*s))
	s++;

    // optionally get minute...
    minute = 0;
    while (isdigit(*s))
	minute = minute * 10 + (*s++ - '0');
    if (minute > 59)
	return 0;
    while (*s == ':' || isspace(*s))
	s++;

    // optionally get second...
    second = 0;
    while (isdigit(*s))
	second = second * 10 + (*s++ - '0');
    if (second > 59)
	return 0;
    while (*s == ':' || isspace(*s))
	s++;

    //
    // Calculate date as seconds since 01 Jan 1970 00:00:00 GMT
    // This is based somewhat on the date calculation code in NetBSD's
    // cd9660_node.c code, for which I was unable to find a reference.
    // It works, though!
    //
    return (time_t) (((((367L*year - 7L*(year+(month+9)/12)/4
				   - 3L*(((year)+((month)+9)/12-1)/100+1)/4
				   + 275L*(month)/9 + day) -
			(367L*EPOCH - 7L*(EPOCH+(1+9)/12)/4
				   - 3L*((EPOCH+(1+9)/12-1)/100+1)/4
				   + 275L*1/9 + 1))
		       * 24 + hour) * 60 + minute) * 60 + second);
}

//*****************************************************************************
// void Retriever::got_time(char *time)
//
void
Retriever::got_time(char *time)
{
    time_t   new_time;
    if (debug > 1)
      cout << "\ntime: " << time << endl;
    //
    // As defined by the Dublin Core, this should be YYYY-MM-DD
    // In the future, we'll need to deal with the scheme portion
    //  in case someone picks a different format.
    //
    new_time = parsedcdate(time);
    if (new_time)
	current_time = new_time;
    // If we can't convert it, current_time stays the same and we get
    // the default--the date returned by the server...
}

//*****************************************************************************
// void Retriever::got_anchor(char *anchor)
//
void
Retriever::got_anchor(char *anchor)
{
    if (debug > 2)
	cout << "anchor: " << anchor << endl;
    current_ref->AddAnchor(anchor);
    current_anchor_number++;
}


//*****************************************************************************
// void Retriever::got_image(char *src)
//
void
Retriever::got_image(char *src)
{
    URL	url(src, *base);
    char	*image = url.get();
	
    if (debug > 2)
	cout << "image: " << image << endl;

    if (images_seen)
	fprintf(images_seen, "%s\n", image);
//	current_ref->DocImageSize(current_ref->DocImageSize() + images.Sizeof(image));
}


//*****************************************************************************
// void Retriever::got_href(char *href, char *description)
//
void
Retriever::got_href(URL &url, char *description)
{
    DocumentRef		*ref;
    Server		*server;

    if (debug > 2)
	cout << "href: " << url.get() << " (" << description << ')' << endl;

    n_links++;

    if (urls_seen)
	fprintf(urls_seen, "%s\n", url.get());

    //
    // Check if this URL falls within the valid range of URLs.
    //
    if (IsValidURL(url.get()))
    {
	String	oldurl;
	//
	// It is valid.  Normalize it (resolve cnames for the server)
	// and check again...
	//
	if (debug > 2)
	{
	    cout << "resolving '" << url.get() << "'\n";
	    cout.flush();
	    oldurl = url.get();
	}

	url.normalize();
	url.rewrite();
	if (debug > 2 && strcmp(oldurl.get(), url.get()) != 0)
	{
	    cout << "normalized/rewritten as '" << url.get() << "'\n";
	    cout.flush();
	}

	// If it is a backlink from the current document,
	// just update that field.  Writing to the database
	// is meaningless, as it will be overwritten.
	// Adding it as a new document may even be harmful, as
	// that will be a duplicate.  This can happen if the
	// current document is never referenced before, as in a
	// start_url.

	if (strcmp(url.get(), current_ref->DocURL()) == 0)
	{
	    current_ref->DocBackLinks(current_ref->DocBackLinks() + 1);
	    current_ref->AddDescription(description);
	}
	else if (limitsn.FindFirst(url.get()) >= 0)
	{
	    //
	    // First add it to the document database
	    //
	    ref = docs[url.get()];
	    // if ref exists we have to call AddDescription even
            // if max_hop_count is reached
    	    if (!ref && currenthopcount + 1 > max_hop_count)
		return;

	    if (!ref)
	    {
		//
		// Didn't see this one, yet.  Create a new reference
		// for it with a unique document ID
		//
		ref = new DocumentRef;
		ref->DocID(docs.NextDocID());
		ref->DocHopCount(currenthopcount + 1);
	    }
	    ref->DocBackLinks(ref->DocBackLinks() + 1); // This one!
	    ref->DocURL(url.get());
	    ref->AddDescription(description);

	    //
    	    // If the dig is restricting by hop count, perform the check here 
	    // too
    	    if (currenthopcount + 1 > max_hop_count)
	    {
		delete ref;
		return;
	    }

	    if (ref->DocHopCount() > currenthopcount + 1)
		ref->DocHopCount(currenthopcount + 1);

	    docs.Add(*ref);

	    //
	    // Now put it in the list of URLs to still visit.
	    //
	    if (Need2Get(url.get()))
	    {
		if (debug > 1)
		    cout << "\n   pushing " << url.get() << endl;
		server = (Server *) servers[url.signature()];
		if (!server)
		{
		    //
		    // Hadn't seen this server, yet.  Register it
		    //
		    String robotsURL = "http://";
		    robotsURL << url.host() << "/robots.txt";
		    StringList *localRobotsFile = GetLocal(robotsURL.get());
		    server = new Server(url.host(), url.port(), localRobotsFile);
		    servers.Add(url.signature(), server);
		    delete localRobotsFile;
		}
		//
		// Let's just be sure we're not pushing an empty URL
		//
		if (strlen(url.get()))
		  server->push(url.get(), ref->DocHopCount(), base->get(),
				IsLocalURL(url.get()));

		String	temp = url.get();
		visited.Add(temp, 0);
		if (debug)
		    cout << '+';
	    }
	    else if (debug)
		cout << '*';
	    delete ref;
	}
	else
	{
	    //
	    // Not a valid URL
	    //
	    if (debug > 1)
		cout << "\nurl rejected: (level 2)" << url.get() << endl;
	    if (debug == 1)
		cout << '-';
	}
    }
    else
    {
	//
	// Not a valid URL
	//
	if (debug > 1)
	    cout << "\nurl rejected: (level 1)" << url.get() << endl;
	if (debug == 1)
	    cout << '-';
    }
    if (debug)
	cout.flush();
}


//*****************************************************************************
// void Retriever::got_redirect(char *new_url, DocumentRef *old_ref)
//
void
Retriever::got_redirect(char *new_url, DocumentRef *old_ref)
{
    // First we must piece together the new URL, which may be relative
    URL parent(old_ref->DocURL());
    URL	url(new_url, parent);

    if (debug > 2)
	cout << "redirect: " << url.get() << endl;

    n_links++;

    if (urls_seen)
	fprintf(urls_seen, "%s\n", url.get());

    //
    // Check if this URL falls within the valid range of URLs.
    //
    if (IsValidURL(url.get()))
    {
	String	oldurl;
	//
	// It is valid.  Normalize it (resolve cnames for the server)
	// and check again...
	//
	if (debug > 2)
	{
	    cout << "resolving '" << url.get() << "'\n";
	    cout.flush();
	    oldurl = url.get();
	}

	url.normalize();
	url.rewrite();
	if (debug > 2 && strcmp(oldurl.get(), url.get()) != 0)
	{
	    cout << "normalized/rewritten as '" << url.get() << "'\n";
	    cout.flush();
	}

	if (limitsn.FindFirst(url.get()) >= 0)
	{
	    //
	    // First add it to the document database
	    //
	    DocumentRef	*ref = docs[url.get()];
	    if (!ref)
	    {
		//
		// Didn't see this one, yet.  Create a new reference
		// for it with a unique document ID
		//
		ref = new DocumentRef;
		ref->DocID(docs.NextDocID());
		ref->DocHopCount(currenthopcount);
	    }
	    ref->DocURL(url.get());
			
	    //
	    // Copy the descriptions of the old DocRef to this one
	    //
	    List	*d = old_ref->Descriptions();
	    if (d)
	    {
		d->Start_Get();
		String	*str;
		while ((str = (String *) d->Get_Next()))
		{
		    ref->AddDescription(str->get());
		}
	    }
	    if (ref->DocHopCount() > old_ref->DocHopCount())
		ref->DocHopCount(old_ref->DocHopCount());

	    // Copy the number of backlinks
	    ref->DocBackLinks(old_ref->DocBackLinks());

	    docs.Add(*ref);

	    //
	    // Now put it in the list of URLs to still visit.
	    //
	    if (Need2Get(url.get()))
	    {
		if (debug > 1)
		    cout << "   pushing " << url.get() << endl;
		Server	*server = (Server *) servers[url.signature()];
		if (!server)
		{
		    //
		    // Hadn't seen this server, yet.  Register it
		    //
		    String robotsURL = "http://";
		    robotsURL << url.host() << "/robots.txt";
		    StringList *localRobotsFile = GetLocal(robotsURL.get());
		    server = new Server(url.host(), url.port(), localRobotsFile);
		    servers.Add(url.signature(), server);
		    delete localRobotsFile;
		}
		server->push(url.get(), ref->DocHopCount(), base->get(),
				IsLocalURL(url.get()));

		String	temp = url.get();
		visited.Add(temp, 0);
	    }

	    delete ref;
	}
    }
}


//*****************************************************************************
// void Retriever::got_head(char *head)
//
void
Retriever::got_head(char *head)
{
    if (debug > 4)
	cout << "head: " << head << endl;
    current_head = head;
}

//*****************************************************************************
// void Retriever::got_meta_dsc(char *md)
//
void
Retriever::got_meta_dsc(char *md)
{
    if (debug > 4)
	cout << "meta description: " << md << endl;
    current_meta_dsc = md;
}


//*****************************************************************************
// void Retriever::got_meta_email(char *e)
//
void
Retriever::got_meta_email(char *e)
{
    if (debug > 1)
	cout << "\nmeta email: " << e << endl;
    current_ref->DocEmail(e);
}


//*****************************************************************************
// void Retriever::got_meta_notification(char *e)
//
void
Retriever::got_meta_notification(char *e)
{
    if (debug > 1)
	cout << "\nmeta notification date: " << e << endl;
    current_ref->DocNotification(e);
}


//*****************************************************************************
// void Retriever::got_meta_subject(char *e)
//
void
Retriever::got_meta_subject(char *e)
{
    if (debug > 1)
	cout << "\nmeta subect: " << e << endl;
    current_ref->DocSubject(e);
}


//*****************************************************************************
// void Retriever::got_noindex()
//
void
Retriever::got_noindex()
{
    if (debug > 1)
      cout << "\nMETA ROBOT: Noindex " << endl;
    current_ref->DocState(Reference_noindex);
}


//*****************************************************************************
//
void
Retriever::recordNotFound(char *url, char *referer, int reason)
{
    char	*message = "";
    
    switch (reason)
    {
	case Document::Document_not_found:
	    message = "Not found";
	    break;
	
	case Document::Document_no_host:
	    message = "Unknown host";
	    break;
	
	case Document::Document_no_server:
	    message = "Unable to contact server";
	    break;
    }

    notFound << message << ": " << url << " Ref: " << referer << '\n';
}

//*****************************************************************************
// void Retriever::ReportStatistics(char *name)
//
void
Retriever::ReportStatistics(char *name)
{
    cout << name << ": Run complete\n";
    cout << name << ": " << servers.Count() << " server";
    if (servers.Count() > 1)
	cout << "s";
    cout << " seen:\n";

    Server		*server;
    String		buffer;
    StringList	results;
    String		newname = name;

    newname << ":    ";
	
    servers.Start_Get();
    while ((server = (Server *) servers.Get_NextElement()))
    {
	buffer = 0;
	server->reportStatistics(buffer, newname);
	results.Add(buffer);
    }
    results.Sort();

    for (int i = 0; i < results.Count(); i++)
    {
	cout << results[i] << "\n";
    }

    if (notFound.length() > 0)
    {
	cout << "\n" << name << ": Errors to take note of:\n";
	cout << notFound;
    }
}

