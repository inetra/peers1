/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Automatic Directory Listing Search
 * Henrik Engstr�m, henrikengstrom at home se
 */

#if !defined(ADL_SEARCH_H)
#define ADL_SEARCH_H

#include "Util.h"

#include "SettingsManager.h"
#include "ResourceManager.h"

#include "StringSearch.h"
#include "StringTokenizer.h"
#include "Singleton.h"
#include "DirectoryListing.h"

///////////////////////////////////////////////////////////////////////////////
//
//	Class that represent an ADL search
//
///////////////////////////////////////////////////////////////////////////////
class ADLSearch
{
public:

	// Constructor
	ADLSearch() : searchString("<Enter string>"), isActive(true), isAutoQueue(false), sourceType(OnlyFile), 
		minFileSize(-1), maxFileSize(-1), typeFileSize(SizeBytes), destDir("ADLSearch"), ddIndex(0)
		, isForbidden(false), raw(0) {}

	// Prepare search
	void Prepare(StringMap& params) {
		// Prepare quick search of substrings
		stringSearchList.clear();

		// Replace parameters such as %[nick]
		string stringParams = Util::formatParams(searchString, params, false);

		// Split into substrings
		StringTokenizer<string> st(stringParams, ' ');
		for(StringList::iterator i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
			if(i->size() > 0) {
				// Add substring search
				stringSearchList.push_back(StringSearch(*i));
			}
		}
	}

	// The search string
	string searchString;									 

	// Active search
	bool isActive;
	// Forbidden file
	bool isForbidden;
	int raw;

	// Auto Queue Results
	bool isAutoQueue;

	// Search source type
	enum SourceType {
		TypeFirst = 0,
		OnlyFile = TypeFirst,
		OnlyDirectory,
		FullPath,
		TypeLast
	} sourceType;

	SourceType StringToSourceType(const string& s) {
		if(Util::stricmp(s.c_str(), "Filename") == 0) {
			return OnlyFile;
		} else if(Util::stricmp(s.c_str(), "Directory") == 0) {
			return OnlyDirectory;
		} else if(Util::stricmp(s.c_str(), "Full Path") == 0) {
			return FullPath;
		} else {
			return OnlyFile;
		}
	}

	string SourceTypeToString(SourceType t) {
		switch(t) {
		default:
		case OnlyFile:		return "Filename";
		case OnlyDirectory:	return "Directory";
		case FullPath:		return "Full Path";
		}
	}

	tstring SourceTypeToDisplayString(SourceType t) {
		switch(t) {
		default:
		case OnlyFile:		return TSTRING(FILENAME);
		case OnlyDirectory:	return TSTRING(DIRECTORY);
		case FullPath:		return TSTRING(ADLS_FULL_PATH);
		}
	}

	// Maximum & minimum file sizes (in bytes). 
	// Negative values means do not check.
	int64_t minFileSize;
	int64_t maxFileSize;
	enum SizeType {
		SizeBytes     = TypeFirst,
		SizeKiloBytes,
		SizeMegaBytes,
		SizeGigaBytes
	};
	SizeType typeFileSize;
	SizeType StringToSizeType(const string& s) {
		if(Util::stricmp(s.c_str(), "B") == 0) {
			return SizeBytes;
		} else if(Util::stricmp(s.c_str(), "kB") == 0) {
			return SizeKiloBytes;
		} else if(Util::stricmp(s.c_str(), "MB") == 0) {
			return SizeMegaBytes;
		} else if(Util::stricmp(s.c_str(), "GB") == 0) {
			return SizeGigaBytes;
		} else {
			return SizeBytes;
		}
	}
	string SizeTypeToString(SizeType t) {
		switch(t) {
		default:
		case SizeBytes:		return "B";
		case SizeKiloBytes:	return "kB";
		case SizeMegaBytes:	return "MB";
		case SizeGigaBytes:	return "GB";
		}
	}
	tstring SizeTypeToDisplayString(SizeType t) {
		switch(t) {
		default:
		case SizeBytes:		return CTSTRING(B);
		case SizeKiloBytes:	return CTSTRING(KB);
		case SizeMegaBytes:	return CTSTRING(MB);
		case SizeGigaBytes:	return CTSTRING(GB);
		}
	}
	int64_t GetSizeBase() {
		switch(typeFileSize) {
		default:
		case SizeBytes:		return (int64_t)1;
		case SizeKiloBytes:	return (int64_t)1024;
		case SizeMegaBytes:	return (int64_t)1024 * (int64_t)1024;
		case SizeGigaBytes:	return (int64_t)1024 * (int64_t)1024 * (int64_t)1024;
		}
	}

	// Name of the destination directory (empty = 'ADLSearch') and its index
	string destDir;
	unsigned long ddIndex;

	// Search for file match 
	bool MatchesFile(const string& f, const string& fp, int64_t size) {
		// Check status
		if(!isActive) {
			return false;
		}

		// Check size for files
		if(size >= 0 && (sourceType == OnlyFile || sourceType == FullPath)) {
			if(minFileSize >= 0 && size < minFileSize * GetSizeBase()) {
				// Too small
				return false;
			}
			if(maxFileSize >= 0 && size > maxFileSize * GetSizeBase()) {
				// Too large
				return false;
			}
		}

		// Do search
		switch(sourceType) {
		default:
		case OnlyDirectory:	return false;
		case OnlyFile:		return SearchAll(f);
		case FullPath:		return SearchAll(fp);
		}
	}

	// Search for directory match 
	bool MatchesDirectory(const string& d) {
		// Check status
		if(!isActive) {
			return false;
		}
		if(sourceType != OnlyDirectory) {
			return false;
		}

		// Do search
		return SearchAll(d);
	}

private:

	// Substring searches
	StringSearch::List stringSearchList;
	bool SearchAll(const string& s) {
		// Match all substrings
		for(StringSearch::List::const_iterator i = stringSearchList.begin(); i != stringSearchList.end(); ++i) {
			if(!i->match(s)) {
				return false;
			}
		}
		return (stringSearchList.size() != 0);
	}
};

///////////////////////////////////////////////////////////////////////////////
//
//	Class that holds all active searches
//
///////////////////////////////////////////////////////////////////////////////
class ADLSearchManager : public Singleton<ADLSearchManager>
{
public:
	// Destination directory indexing
	struct DestDir {
		string name;
		DirectoryListing::Directory* dir;
		DirectoryListing::Directory* subdir;
		bool fileAdded;
		DestDir() : dir(NULL), subdir(NULL), fileAdded(false) {}
	};
	typedef vector<DestDir> DestDirList;

	// Constructor/destructor
	ADLSearchManager() { Load(); }
	~ADLSearchManager() { Save(); }

	// Search collection
	typedef vector<ADLSearch> SearchCollection;
	SearchCollection collection;

	// Load/save search collection to XML file
	void Load();
	void Save();

	// Settings
	GETSET(UserPtr, user, User)
	GETSET(bool, breakOnFirst, BreakOnFirst)
	GETSET(bool, sentRaw, SentRaw);

	// @remarks Used to add ADLSearch directories to an existing DirectoryListing
	void matchListing(DirectoryListing& /*aDirList*/) throw();

private:
	// @internal
	void matchRecurse(DestDirList& /*aDestList*/, DirectoryListing::Directory* /*aDir*/, string& /*aPath*/);
	// Search for file match
	void MatchesFile(DestDirList& destDirVector, DirectoryListing::File *currentFile, string& fullPath);
	// Search for directory match
	void MatchesDirectory(DestDirList& destDirVector, DirectoryListing::Directory* currentDir, string& fullPath);
	// Step up directory
	void StepUpDirectory(DestDirList& destDirVector) {
		for(DestDirList::iterator id = destDirVector.begin(); id != destDirVector.end(); ++id) {
			if(id->subdir != NULL) {
				id->subdir = id->subdir->getParent();
				if(id->subdir == id->dir) {
					id->subdir = NULL;
				}
			}
		}
	}

	// Prepare destination directory indexing
	void PrepareDestinationDirectories(DestDirList& destDirVector, DirectoryListing::Directory* root, StringMap& params);
	// Finalize destination directories
	void FinalizeDestinationDirectories(DestDirList& destDirVector, DirectoryListing::Directory* root) {
		string szDiscard = "<<<" + STRING(ADLS_DISCARD) + ">>>";

		// Add non-empty destination directories to the top level
		for(vector<DestDir>::iterator id = destDirVector.begin(); id != destDirVector.end(); ++id) {
			if(id->dir->files.size() == 0 && id->dir->directories.size() == 0) {
				delete (id->dir);
			} else if(Util::stricmp(id->dir->getName(), szDiscard) == 0) {
				delete (id->dir);
			} else {
				root->directories.push_back(id->dir);
			}
		}
	}

	string getConfigFile() { return Util::getConfigPath() + "ADLSearch.xml"; }
};

#endif // !defined(ADL_SEARCH_H)

/**
 * @file
 * $Id: ADLSearch.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
  */
