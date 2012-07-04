
////////////////////////////////////////////////////////////////////////////////////////////////
//
// This file is redistributable on the same terms as the PCRE 4.3 license, except copyright
// notice must be attributed to:
//
// (C) 2003 Zachary Hansen xaxxon@slackworks.com
// (C) Hugo Etchegoyen hetchego@hasar.com
//
// Distribution under the GPL or LGPL overrides any other restrictions, as in the PCRE license
//
////////////////////////////////////////////////////////////////////////////////////////////////
	
#include "stdinc.h"
#include "DCPlusPlus.h"

#include "pme.h"
#include "Text.h"
	
unsigned int PME::DeterminePcreOptions ( const std::string & opts ///< perl style character modifiers -- i.e. "gi" is global, case-insensitive
 )
{
	unsigned int return_opts = 0;
	if ( strchr ( opts.c_str ( ), 'i' ) ) {
		return_opts |= PCRE_CASELESS;
	}
	if ( strchr ( opts.c_str ( ), 'm' ) ) {
		return_opts |= PCRE_MULTILINE;
	}
	if ( strchr ( opts.c_str ( ), 's' ) ) {
		return_opts |= PCRE_DOTALL;
	}
	if ( strchr ( opts.c_str ( ), 'x' ) ) {
		return_opts |= PCRE_EXTENDED;
	}
	// not perl compatible
	if ( strchr ( opts.c_str ( ), 'U' ) ) {
		return_opts |= PCRE_UNGREEDY;
	}

	// if 'g' is set, it stores the previous
	//   result end position
	if ( strchr ( opts.c_str ( ), 'g' ) ) {
		m_isglobal = 1;
	}


	return return_opts;
}


unsigned
PME::options()
{
	return _opts;
}

void					
PME::options(unsigned opts ///< PCRE-style option flags to be set on PME object
)
{
	_opts = opts;
}

void 
PME::compile(const std::string & s ///< string to compile into regular expression
)
{
	const char * errorptr;
	int erroroffset;
	
	re = pcre_compile(s.c_str(), _opts, &errorptr, &erroroffset, 0);

	if ( re != NULL ) {
		nValid = 1;
	} else {
		nValid = 0;
	}

}

pcre *
PME::clone_re(pcre * re ///< PME object to clone
)
{
	if ( !re )
		return 0;
	size_t size;
	pcre_fullinfo(re, 0, PCRE_INFO_SIZE, &size);
	pcre * newre = (pcre *) new char[size];

	memcpy(newre, re, size);
	return newre;
}

PME::PME( ) 
{
	reset ( );
	_opts = 9;
	re = NULL;
	extra = NULL;
	lastglobalposition = 0;
	m_isglobal = 0;
	m_nMatches = 0;
}

PME::PME(const std::string & s ///< string to copmile into regular expression
			 , unsigned opts ///< PCRE-style option flags to be set on PME object
)
{
	reset ( );
	m_isglobal = 0;
	_opts = opts;
	compile(s);
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}

PME::PME(const std::wstring & s ///< string to copmile into regular expression
			, unsigned opts ///< PCRE-style option flags to be set on PME object
)
{
	reset ( );
	m_isglobal = 0;
	_opts = opts;
	compile(Text::wideToAcp(s));
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;
}

PME::PME ( const std::string & s, ///< string to compile into regular expression
		   const std::string & opts ///< perl-style character flags to be set on PME object 
)
{
	reset ( );
	m_isglobal = 0;
	_opts = DeterminePcreOptions ( opts );
	compile ( s );
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}

PME::PME ( const std::wstring & s, ///< string to compile into regular expression
		  const std::wstring & opts ///< perl-style character flags to be set on PME object 
		  )
{
	reset ( );
	m_isglobal = 0;
	_opts = DeterminePcreOptions ( Text::wideToAcp(opts) );
	compile ( Text::wideToAcp(s) );
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}

PME::PME(const char * s, ///< string to compile into regular expression
			 unsigned opts ///< PCRE-style option flags to be set on PME object
)
{
	reset ( );
	m_isglobal = 0;
	_opts = opts;
	compile(s);
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}

PME::PME(const wchar_t * s, ///< string to compile into regular expression
		 unsigned opts ///< PCRE-style option flags to be set on PME object
		 )
{
	reset ( );
	m_isglobal = 0;
	_opts = opts;
	compile( Text::wideToAcp(s) );
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}

PME::PME ( const char * s, ///< string to compile into regular expression
			   const std::string & opts ///< perl-style character flags to be set on PME object
)
{
	reset ( );
	m_isglobal = 0;
	_opts = DeterminePcreOptions ( opts );
	compile ( s );
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}

PME::PME ( const wchar_t * s, ///< string to compile into regular expression
		  const std::wstring & opts ///< perl-style character flags to be set on PME object
		  )
{
	reset ( );
	m_isglobal = 0;
	_opts = DeterminePcreOptions ( Text::wideToAcp(opts) );
	compile ( Text::wideToAcp(s) );
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}


PME::PME(const PME & r ///< PME object to make copy of 
)
{
	reset ( );
	m_isglobal = 0;
	_opts = r._opts;
	re = clone_re(r.re);
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;

}


PME::~PME()
{
	if ( re ) {
		pcre_free ( re );
	}
	if ( extra ) {
 	    pcre_free ( extra );
 	}
}


int
PME::match(const std::string & s, ///< s String to match against
			 unsigned offset ///< offset Offset at which to start matching
			 )
{
	size_t msize;
	pcre_fullinfo(re, 0, PCRE_INFO_CAPTURECOUNT, &msize);
	msize = 3*(msize+1);
	int *m = new int[msize];

	// if we got a new string, reset the global position counter
    if ( AddressOfLastString != (void *) &s ) {
//		fprintf ( stderr, "PME RESETTING: new string\n" );
		lastglobalposition = 0;
	}

	if ( m_isglobal ) {
		offset += lastglobalposition;
	}

	m_nMatches = pcre_exec(re, extra, s.c_str(), s.length(), offset, 0, m, msize);

   // clear out the old marks and make room for the new ones
    m_marks.clear ( );
	
    if ( m_nMatches > 0 ) {
		m_marks.reserve ( m_nMatches );
    }

    // go through all the matches and fill out our data structures
    int * p = m;
    for ( int i = 0; i < m_nMatches ; i++, p+=2 ) {
		m_marks.push_back(markers(p[0], p[1]));
	}

	delete[] m;

	// store the last set of results locally, as well as returning them
	laststringmatched = s;
    AddressOfLastString = (void *) &s;

	if ( m_isglobal ) {

		if ( m_nMatches == PCRE_ERROR_NOMATCH ) {
//			fprintf ( stderr, "PME RESETTING: reset for no match\n" );
			lastglobalposition = 0; // reset the position for next match (perl does this)
		} else if ( m_nMatches > 0 ) {
//			fprintf ( stderr, "PME RESETTING: setting to %d\n", marks[0].second );
			lastglobalposition = m_marks[0].second; // increment by the end of the match
		} else {
//			fprintf ( stderr, "PME RESETTING: reset for no unknown\n" );
			lastglobalposition = 0;
		}
	}

	int returnvalue = 0;
    if ( m_nMatches > 0 ) {
	returnvalue = m_nMatches;
    } else {
	returnvalue = 0;
    }

	return returnvalue;
}

int
PME::match(const std::wstring & s, ///< s String to match against
		   unsigned offset ///< offset Offset at which to start matching
		   )
{
	return match( Text::wideToAcp(s), offset );
}

std::string
PME::substr(const std::string & s, const std::vector<PME::markers> & marks,
			  unsigned index)
{

	if ( index >= marks.size() ) {
		return "";
	}

	int begin = marks[index].first;
	if ( begin == -1 )
		return "";
	int end = marks[index].second;
	return s.substr(begin, end-begin);
}


void PME::reset ( )
{

    AddressOfLastString = NULL;
	lastglobalposition = 0;
	m_marks.clear ( );
#ifdef HAVE_STL_CLEAR
	laststringmatched.clear ( );
#else
	laststringmatched = "";
#endif

	nValid = 0; // not valid by default, but assigned on construction

}

/** Splits into at most maxfields.  If maxfields is unspecified or 0, 
 ** trailing empty matches are discarded.  If maxfields is positive,
 ** no more than maxfields fields will be returned and trailing empty
 ** matches are preserved.  If maxfields is empty, all fields (including
 ** trailing empty ones) are returned.  This *should* be the same as the
 ** perl behaviour
 */
int
PME::split(const std::string & s, unsigned maxfields)
{
	/// stores the marks for the split
	std::vector<markers> oMarks;

	// this is a list of current trailing empty matches if maxfields is 
	//   unspecified or 0.  If there is stuff in it and a non-empty match
	//   is found, then everything in here is pushed into oMarks and then 
	//   the new match is pushed on.  If the end of the string is reached
	//   and there are empty matches in here, they are discarded.
	std::vector<markers> oCurrentTrailingEmpties; 

	int nOffset = 0;
	unsigned int nMatchesFound = 0;

	// while we are still finding matches and maxfields is 0 or negative
	//   (meaning we get all matches), or we haven't gotten to the number
	//   of specified matches
	while ( (  match ( s, nOffset ) )  > 0 &&
			( ( maxfields < 1 ) ||
			  nMatchesFound < maxfields ) ) {

		nMatchesFound++;
//		printf ( "nMatchesFound = %d\n", nMatchesFound );
		// check to see if the match is empty
		if ( nOffset != m_marks [ 0 ].first ) {
			//fprintf ( stderr, "Match is not empty\n" );
			// if this one isn't empty, then make sure to push anything from 
			//   oCurrentTrailingEmpties into oMarks
			oMarks.insert ( oMarks.end ( ),
							oCurrentTrailingEmpties.begin ( ),
							oCurrentTrailingEmpties.end ( ) );

			// grab from nOffset to m_marks[0].first and again from m_marks[0].second to 
			//   the end of the string
			oMarks.push_back ( markers ( nOffset, m_marks [ 0 ].first ) );

		} 
		// else the match was empty and we have to do some checking
		//   to see what to do
		else {
			//fprintf ( stderr, "match was empty\n" );
			// if maxfields == 0, discard empty trailing matches
			if ( maxfields == 0 ) {
				//fprintf ( stderr, "putting empty into trailing empties list");
				oCurrentTrailingEmpties.push_back ( markers ( nOffset, m_marks [ 0 ].first ) );

			}
			// else we keep all the matches, empty or no
			else {
				//fprintf ( stderr, "Keeping empty match\n" );
				// grab from nOffset to m_marks[0].first and again from m_marks[0].second to 
				//   the end of the string
				oMarks.push_back ( markers ( nOffset, m_marks [ 0 ].first ) );

			}

		}

		// set nOffset to the beginning of the second part of the split
		nOffset = m_marks [ 0 ].second;
		
		fflush ( stdout );

	} // end while ( match ( ... ) ) 

	//fprintf ( stderr, "***match status = %d offset = %d\n", nMatchStatus, nOffset);


	// if there were no matches found, push the whole thing on
	if ( nMatchesFound == 0 ) {
//		printf ( "Putting the whole thing in..\n" );
		oMarks.push_back ( markers ( 0, s.length ( ) ) );
	} 	
	// if we ran out of matches, then append the rest of the string 
	//   onto the end of the last split field
	else if ( maxfields > 0 && 
		 nMatchesFound >= maxfields ) {
//		printf ( "Something else..\n" );
		oMarks [ oMarks.size ( ) - 1 ].second = s.length ( );
	} 
	// else we have to add another entry for the end of the string
	else {
//		printf ( "Something REALLY else..\n" );
		oMarks.push_back ( markers ( /*marks [ 0 ].second*/nOffset, s.length ( ) ) );
	}

    // set the PME object's marks to the split's marks
	m_marks = oMarks;
    m_nMatches = m_marks.size ( );

	//fprintf ( stderr, "match returning %d\n", m_marks.size ( ) );

	return m_marks.size ( );
}

int
PME::split(const std::wstring & s, unsigned maxfields)
{
	return split( Text::wideToAcp( s ), maxfields );
}


void PME::study()
{

	const char * errorptr = NULL;
	
	extra = pcre_study( re, 
						0, // no options exist
						&errorptr );

	// check for error condition
	if ( errorptr != NULL ) {
		
		if ( extra != NULL ) {

			pcre_free ( extra );

		}

		extra = NULL;

	} 

	return;

}


std::string PME::operator[](int index)
{

	return substr ( laststringmatched, m_marks, index );

}

const PME & PME::operator = (const PME & r){
	re = clone_re(r.re);
	m_isglobal = r.m_isglobal;
	_opts = r._opts;
	nValid = r.nValid;
	extra = NULL;
	
	return *this;
}



std::string PME::sub ( const std::string & s, const std::string & r)
{
	
	string newstring = s;
		
	if ( m_isglobal ) {
			
		while ( match ( s ) ) 
			newstring.replace(m_marks[0].first, m_marks[0].second, r);
	} else {
		if ( match ( s ) > 0 ) 
			newstring.replace(m_marks[0].first, m_marks[0].second, r);
	}

	return newstring;

}

std::wstring PME::sub ( const std::wstring & s, const std::wstring & r )
{
	return Text::acpToWide( sub( Text::wideToAcp( s ), Text::wideToAcp( r ) ) );
}


StringVector PME::GetStringVector ( ) 
{
	
	StringVector oStringVector;

	for ( int nCurrentMatch = 0;
		  nCurrentMatch < m_nMatches;
		  nCurrentMatch++ ) {

		oStringVector.insert ( oStringVector.end ( ), (*this)[nCurrentMatch] );

	}

	return oStringVector;

}

WStringVector PME::GetStringVectorW ( )
{
	WStringVector oStringVector;

	for ( int nCurrentMatch = 0;
		  nCurrentMatch < m_nMatches;
		  nCurrentMatch++ ) {

			  oStringVector.insert( oStringVector.end ( ), Text::utf8ToWide( (*this)[nCurrentMatch] ) );
		  }

	return oStringVector;
}

int PME::GetStartPos( int _backRef )
{
	if( _backRef >= m_nMatches || _backRef < 0 )
		return -1;

	return m_marks[_backRef].first;
}

int PME::GetLength( int _backRef )
{
	if( _backRef >= m_nMatches || _backRef < 0 )
		return -1;

	return m_marks[_backRef].second - m_marks[_backRef].first;
}

void PME::Init(const std::string & s, unsigned opts )
{
	if( re )
		pcre_free( re );
	reset ( );
	m_isglobal = 0;
	_opts = opts;
	compile ( s );
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;
}

void PME::Init(const std::wstring & s, unsigned opts )
{
	Init( Text::wideToAcp( s ), opts );
}

void PME::Init(const std::string & s, const std::string & opts )
{
	if( re )
		pcre_free( re );

	reset ( );
	m_isglobal = 0;
	_opts = DeterminePcreOptions ( opts );
	compile ( s );
	extra = NULL;
	lastglobalposition = 0;
	m_nMatches = 0;
}

void PME::Init(const std::wstring & s, const std::wstring & opts )
{
	Init( Text::wideToAcp( s ), Text::wideToAcp(opts) );
}
