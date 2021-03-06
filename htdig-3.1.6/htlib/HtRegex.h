//
// HtRegex.h
//
// HtRegex: A simple C++ wrapper class for the system regex routines.
//
// Part of the ht://Dig package   <http://www.htdig.org/>
// Copyright (c) 1999-2002 The ht://Dig Group
// For copyright details, see the file COPYING in your distribution
// or the GNU General Public License version 2 or later 
// <http://www.gnu.org/copyleft/gpl.html>
//
// $Id: HtRegex.h,v 1.6.2.3 2002/01/26 03:43:38 ghutchis Exp $
//
//

#ifndef	_HtRegex_h_
#define	_HtRegex_h_

#include "Object.h"
#include "StringList.h"

// This is an attempt to get around compatibility problems 
// with the included regex
#ifdef USE_RX
#include <rxposix.h>
#else // Use regex
#ifdef HAVE_BROKEN_REGEX
#include <regex.h>
#else // included regex code and header
#include "gregex.h"
#endif
#endif

#include <sys/types.h>
#include <fstream.h>

class HtRegex : public Object
{
public:
    //
    // Construction/Destruction
    //
    HtRegex();
    HtRegex(const char *str, int case_sensitive = 0);
    virtual ~HtRegex();

    //
    // Methods for setting the pattern
    //
    int		set(const String& str, int case_sensitive = 0) { return set(str.get(), case_sensitive); }
    int		set(const char *str, int case_sensitive = 0);
    int		setEscaped(StringList &list, int case_sensitive = 0);

	virtual const String &lastError();	// returns the last error message

    //
    // Methods for checking a match
    //
    int		match(const String& str, int nullmatch, int nullstr) { return match(str.get(), nullmatch, nullstr); }
    int		match(const char *str, int nullmatch, int nullstr);

protected:
    int			compiled;
    regex_t		re;

    String		lastErrorMessage;
};

#endif
