//
// Configuration.cc
//
// Implementation of the Configuration class
// Standard class for storing all user options (and defaults)
//
//
#if RELEASE
static char	RCSid[] = "$Id: Configuration.cc,v 1.9.2.6 2001/07/05 16:26:35 ghutchis Exp $";
#endif

#include "Configuration.h"
#include "htString.h"
#include "ParsedString.h"
#include <fstream.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>


//*********************************************************************
// Configuration::Configuration()
//
Configuration::Configuration()
{
    separators = new String("=:");
    allow_multiple = 0;
}


//*********************************************************************
// Configuration::~Configuration()
//
Configuration::~Configuration()
{
    delete separators;
}


//*********************************************************************
// void Configuration::NameValueSeparators(char *s)
//
void Configuration::NameValueSeparators(char *s)
{
    delete separators;
    separators = new String(s);
}


//*********************************************************************
// void Configuration::Add(char *str)
//   Add an entry to the configuration table.
//
void Configuration::Add(char *str)
{
    String	name, value;
	
    while (str && *str)
    {
        while (isspace(*str))
            str++;
        name = 0;
        if (!isalpha(*str))
            break;
        // Some isalnum() implementations don't allow all the letters that
        // isalpha() does, e.g. accented ones.  They're not POSIX.2 compliant
        // but we won't punish them with an infinite loop...
        if (!isalnum(*str))
            break;
        while (isalnum(*str) || *str == '-' || *str == '_')
            name << *str++;

        name.lowercase();
		
        //
        // We have the name.  Let's see if we will get a value
        //
        while (isspace(*str))
            str++;
        if (!*str)
        {
            //
            // End of string.  We need to store the name as a boolean TRUE
            //
            Add(name, "true");
            return;
        }

        if (!strchr(separators->get(), *str))
        {
            //
            // We are now at a new name.  The previous one needs to be set
            // to boolean TRUE
            //
            Add(name, "true");
            continue;
        }

        //
        // We now need to deal with the value
        //
        str++;			// Skip the separator
        while (isspace(*str))
            str++;
        if (!*str)
        {
            //
            // End of string reached.  The value must be blank
            //
            Add(name, "");
            break;
        }
        value = 0;
        if (*str == '"')
        {
            //
            // Ah!  A quoted value.  This should be easy to deal with...
            // (Just kidding!)
            //
            str++;
            while (*str && *str != '"')
            {
                value << *str++;
            }
            Add(name, value);
            if (*str == '"')
                str++;
            continue;
        }
        else if (*str == '\'')
        {
            // A single quoted value.
            str++;
            while (*str && *str != '\'')
            {
                value << *str++;
            }
            Add(name, value);
            if (*str == '\'')
                str++;
            continue;
        }
        else
        {
            //
            // A non-quoted string.  This string will terminate at the
            // next blank
            //
            while (*str && !isspace(*str))
            {
                value << *str++;
            }
            Add(name, value);
            continue;
        }
    }
}


//*********************************************************************
// void Configuration::Add(char *name, char *value)
//   Add an entry to the configuration table, without allowing variable
//   or file expansion of the value.
//
void Configuration::Add(char *name, char *value)
{
    String	escaped;
    while (*value)
    {
        if (strchr("$`\\", *value))
            escaped << '\\';
        escaped << *value++;
    }
    ParsedString	*ps = new ParsedString(escaped.get());
    dict.Add(name, ps);
}


//*********************************************************************
// void Configuration::AddParsed(char *name, char *value)
//   Add an entry to the configuration table, allowing parsing for variable
//   or file expansion of the value.
//
void Configuration::AddParsed(char *name, char *value)
{
    ParsedString	*ps = new ParsedString(value);
    if (mystrcasecmp(name, "locale") == 0)
    {
        String *str = new String(setlocale(LC_ALL, ps->get(dict)));
        ps->set(str->get());
        delete str;

        //
        // Set time format to standard to avoid sending If-Modified-Since
        // http headers in native format which http servers can't
        // understand
        //
        setlocale(LC_TIME, "C");
    }
    dict.Add(name, ps);
}


//*********************************************************************
// int Configuration::Remove(char *name)
//   Remove an entry from both the hash table and from the list of keys.
//
int Configuration::Remove(char *name)
{
    return dict.Remove(name);
}


//*********************************************************************
// Object *Configuration::Find(char *name)
//   Retrieve a variable from the configuration database.  This variable
//   will be parsed and a new String object will be returned.
//
char *Configuration::Find(char *name)
{
    ParsedString	*ps = (ParsedString *) dict[name];
    if (ps)
    {
        return ps->get(dict);
    }
    else
    {
#ifdef DEBUG
        cerr << "Could not find configuration option " << name << "\n";
#endif
        return 0;
    }
}


//*********************************************************************
// int Configuration::Value(char *name, int default_value)
//
int Configuration::Value(char *name, int default_value)
{
    int		value = default_value;
    char	*s = Find(name);
    if (s && *s)
    {
        value = atoi(s);
    }

    return value;
}


//*********************************************************************
// double Configuration::Double(char *name, double default_value)
//
double Configuration::Double(char *name, double default_value)
{
    double		value = default_value;
    char	*s = Find(name);
    if (s && *s)
    {
        value = atof(s);
    }

    return value;
}


//*********************************************************************
// int Configuration::Boolean(char *name, int default_value)
//
int Configuration::Boolean(char *name, int default_value)
{
    int		value = default_value;
    char	*s = Find(name);
    if (s && *s)
    {
        if (mystrcasecmp(s, "true") == 0 ||
            mystrcasecmp(s, "yes") == 0 ||
            mystrcasecmp(s, "1") == 0)
            value = 1;
        else if (mystrcasecmp(s, "false") == 0 ||
                 mystrcasecmp(s, "no") == 0 ||
                 mystrcasecmp(s, "0") == 0)
            value = 0;
    }

    return value;
}


//*********************************************************************
// char *Configuration::operator[](char *name)
//
char *Configuration::operator[](char *name)
{
    return Find(name);
}


//*********************************************************************
// int Configuration::Read(char *filename)
//
int Configuration::Read(char *filename)
{
    ifstream	in(filename);

    if (in.bad() || in.eof())
        return NOTOK;

    //
    // Make the line buffer large so that we can read long lists of start
    // URLs.
    //
    char	buffer[50000];
    char	*current;
    String	line;
    String	name;
    char	*value;
    int         len;
    while (!in.bad() && !in.eof())
    {
	buffer[0] = '\0';
        in.getline(buffer, sizeof(buffer));
        if (in.eof() && !*buffer)
            break;
        line << buffer;
        line.chop("\r\n");
        if (line.last() == '\\')
        {
            line.chop(1);
            continue;			// Append the next line to this one
        }

        current = line.get();
        if (*current == '#' || *current == '\0')
        {
            line = 0;
            continue;			// Comments and blank lines are skipped
        }

        name = strtok(current, ": =\t");
        value = strtok(0, "\r\n");
        if (!value)
            value = "";			// Blank value

        //
        // Skip any whitespace before the actual text
        //
        while (*value == ' ' || *value == '\t')
            value++;
	len = strlen(value) - 1;
	if (len >= 0)
	  {
	    //
	    // Skip any whitespace after the actual text
	    //
	    while (value[len] == ' ' || value[len] == '\t')
	      {
		value[len] = '\0';
		len--;
	      }
	  }

	if (mystrcasecmp(name, "include") == 0)
	{
	    ParsedString	ps(value);
	    String		str(ps.get(dict));
	    if (str[0] != '/')		// Given file name not fully qualified
	    {
		str = filename;		// so strip dir. name from current one
		len = str.lastIndexOf('/') + 1;
		if (len > 0)
		    str.chop(str.length() - len);
		else
		    str = "";		// No slash in current filename
		str << ps.get(dict);
	    }
	    Read(str.get());
	    line = 0;
	    continue;
	}

        AddParsed(name, value);
        line = 0;
    }
    in.close();
    return OK;
}


//*********************************************************************
// void Configuration::Defaults(ConfigDefaults *array)
//
void Configuration::Defaults(ConfigDefaults *array)
{
    for (int i = 0; array[i].name; i++)
    {
        AddParsed(array[i].name, array[i].value);
    }
}

