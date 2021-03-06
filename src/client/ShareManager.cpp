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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "ShareManager.h"
#include "ResourceManager.h"

#include "CryptoManager.h"
#include "ClientManager.h"
#include "LogManager.h"
//[-]PPA [Doxygen 1.5.1] #include "HashManager.h"

#include "SimpleXML.h"
//[-]PPA [Doxygen 1.5.1] #include "StringTokenizer.h"
//[-]PPA #include "File.h"
#include "FilteredFile.h"
#include "BZUtils.h"
#include "Wildcards.h"

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>
#endif

#include <limits>

ShareManager::ShareManager() : hits(0), xmlListLen(0), bzXmlListLen(0),
	xmlDirty(true), refreshDirs(false), update(false), initial(true), listN(0), refreshing(0),
	lastXmlUpdate(0), lastFullUpdate(GET_TICK()), bloom(1<<20), sharedSize(0)
{ 
	SettingsManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	HashManager::getInstance()->addListener(this);
}

ShareManager::~ShareManager() {
	SettingsManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);
	HashManager::getInstance()->removeListener(this);

	join();

	StringList lists = File::findFiles(Util::getConfigPath(), "files?*.xml.bz2");
	for_each(lists.begin(), lists.end(), File::deleteFile);

	for(Directory::MapIter j = directories.begin(); j != directories.end(); ++j) {
		delete j->second;
	}
}

ShareManager::Directory::~Directory() {
	for(MapIter i = directories.begin(); i != directories.end(); ++i)
		delete i->second;
}

string ShareManager::Directory::getADCPath() const throw() {
	if(!getParent())
		return '/' + name + '/';
	return getParent()->getADCPath() + name + '/';
}

string ShareManager::Directory::getFullName() const throw() {
	if(!getParent())
		return getName() + '\\';
	return getParent()->getFullName() + getName() + '\\';
}

void ShareManager::Directory::addType(uint32_t type) throw() {
	if(!hasType(type)) {
		fileTypes |= (1 << type);
		if(getParent())
			getParent()->addType(type);
	}
}

string ShareManager::Directory::getRealPath() const throw() {
	if(getParent()) {
		return getParent()->getRealPath() + getName() + PATH_SEPARATOR_STR;
	} else {
		dcassert(ShareManager::getInstance()->getByVirtual(getName()) != ShareManager::getInstance()->directories.end());
		return ShareManager::getInstance()->getByVirtual(getName())->first;
	}
}

int64_t ShareManager::Directory::getSize() const throw() {
	int64_t tmp = size;
	for(Map::const_iterator i = directories.begin(); i != directories.end(); ++i)
		tmp+=i->second->getSize();
	return tmp;
}

size_t ShareManager::Directory::countFiles() const throw() {
	size_t tmp = files.size();
	for(Map::const_iterator i = directories.begin(); i != directories.end(); ++i)
		tmp+=i->second->countFiles();
	return tmp;
}

// !PPA!
string ShareManager::toRealPath(const TTHValue& tth) const {
        Lock l(cs);
        HashFileMap::const_iterator i = tthIndex.find(tth);
        if(i != tthIndex.end())
                return i->second->getRealPath();
        else
                return Util::emptyString;
}

#ifdef PPA_INCLUDE_DEAD_CODE
string ShareManager::toVirtual(const TTHValue& tth) const throw(ShareException) {
	Lock l(cs);
	if(tth == bzXmlRoot) {
		return Transfer::USER_LIST_NAME_BZ;
	} else if(tth == xmlRoot) {
		return Transfer::USER_LIST_NAME;
	}

	HashFileMap::const_iterator i = tthIndex.find(tth);
	if(i != tthIndex.end()) {
		return i->second->getADCPath();
	} else {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}
}
#endif
string ShareManager::toReal(const string& virtualFile, bool isInSharingHub) throw(ShareException) {
	if(virtualFile == "MyList.DcLst") {
		throw ShareException("NMDC-style lists no longer supported, please upgrade your client");
	} else if(virtualFile == Transfer::USER_LIST_NAME_BZ || virtualFile == Transfer::USER_LIST_NAME) {
		generateXmlList();
			if (!isInSharingHub) { //Hide Share Mod
				return (Util::getConfigPath() + "Emptyfiles.xml.bz2");
			}
		return getBZXmlFile();
	} else {
		string realFile;
		Lock l(cs);

		return findFile(virtualFile)->getRealPath();
	}
}

TTHValue ShareManager::getTTH(const string& virtualFile) const throw(ShareException) {
	Lock l(cs);
	if(virtualFile == Transfer::USER_LIST_NAME_BZ) {
		return bzXmlRoot;
	} else if(virtualFile == Transfer::USER_LIST_NAME) {
		return xmlRoot;
	}

	return findFile(virtualFile)->getTTH();
}

MemoryInputStream* ShareManager::getTree(const string& virtualFile) const {
	TigerTree tree;
	if(virtualFile.compare(0, 4, "TTH/") == 0) {
		if(!HashManager::getInstance()->getTree(TTHValue(virtualFile.substr(4)), tree))
			return 0;
	} else {
		try {
			TTHValue tth = getTTH(virtualFile);
			HashManager::getInstance()->getTree(tth, tree);
		} catch(const Exception&) {
			return 0;
		}
	}

	ByteVector buf;
	tree.getLeafData(buf);
	return new MemoryInputStream(&buf[0], buf.size());
}

AdcCommand ShareManager::getFileInfo(const string& aFile) throw(ShareException) {
	if(aFile == Transfer::USER_LIST_NAME) {
		generateXmlList();
		AdcCommand cmd(AdcCommand::CMD_RES);
		cmd.addParam("FN", aFile);
		cmd.addParam("SI", Util::toString(xmlListLen));
		cmd.addParam("TR", xmlRoot.toBase32());
		return cmd;
	} else if(aFile == Transfer::USER_LIST_NAME_BZ) {
		generateXmlList();

		AdcCommand cmd(AdcCommand::CMD_RES);
		cmd.addParam("FN", aFile);
		cmd.addParam("SI", Util::toString(bzXmlListLen));
		cmd.addParam("TR", bzXmlRoot.toBase32());
		return cmd;
	}

	if(aFile.compare(0, 4, "TTH/") != 0)
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);

	Lock l(cs);
	TTHValue val(aFile.substr(4));
	HashFileIter i = tthIndex.find(val);
	if(i == tthIndex.end()) {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}

	const Directory::File& f = *i->second;
	AdcCommand cmd(AdcCommand::CMD_RES);
	cmd.addParam("FN", f.getADCPath());
	cmd.addParam("SI", Util::toString(f.getSize()));
	cmd.addParam("TR", f.getTTH().toBase32());
	return cmd;
}

ShareManager::Directory::File::Set::const_iterator ShareManager::findFile(const string& virtualFile) const throw(ShareException) {
	if(virtualFile.compare(0, 4, "TTH/") == 0) {
		HashFileMap::const_iterator i = tthIndex.find(TTHValue(virtualFile.substr(4)));
		if(i == tthIndex.end()) {
			throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
		}
		return i->second;
	} else if(virtualFile.empty() || virtualFile[0] != '/') {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}

	string::size_type i = virtualFile.find('/', 1);
	if(i == string::npos || i == 1) {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}

	string virtualName = virtualFile.substr(1, i-1);
	Directory::Map::const_iterator dmi = getByVirtual(virtualName);
	if(dmi == directories.end()) {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}
	Directory* d = dmi->second;

	string file = virtualFile.substr(i + 1);
	
	string::size_type j = 0;
	while( (i = file.find('/', j)) != string::npos) {
		Directory::MapIter mi = d->directories.find(file.substr(j, i-j));
		j = i + 1;
		if(mi == d->directories.end())
			throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
		d = mi->second;
	}
	
	Directory::File::Set::const_iterator it = find_if(d->files.begin(), d->files.end(), Directory::File::StringComp(file.substr(j)));
	if(it == d->files.end())
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	return it;
}

string ShareManager::validateVirtual(const string& aVirt) const throw() {
	string tmp = aVirt;
	string::size_type idx = 0;

	while( (idx = tmp.find_first_of("$|:\\/"), idx) != string::npos) {
		tmp[idx] = '_';
	}
	return tmp;
}

bool ShareManager::hasVirtual(const string& virtualName) const throw() {
	return getByVirtual(virtualName) != directories.end();
}

void ShareManager::load(SimpleXML& aXml) {
	Lock l(cs);

	if(aXml.findChild("Share")) {
		aXml.stepIn();
		while(aXml.findChild("Directory")) {
			const string& realPath = aXml.getChildData();
			if(realPath.empty()) {
				continue;
			}
			if(!Util::fileExists(realPath))
				continue;

			const string& virtualName = aXml.getChildAttrib("Virtual");
			string vName = validateVirtual(virtualName.empty() ? Util::getLastDir(realPath) : virtualName);

			// add only unique directories
			if(!hasVirtual(vName)) {
				directories[realPath] = new Directory(virtualName, 0);
			}
		}
		aXml.stepOut();
	}
	if(aXml.findChild("NoShare")) {
		aXml.stepIn();
		while(aXml.findChild("Directory"))
			notShared.push_back(aXml.getChildData());
	
		aXml.stepOut();
	}
}

static const string SDIRECTORY = "Directory";
static const string SFILE = "File";
static const string SNAME = "Name";
static const string SSIZE = "Size";
static const string STTH = "TTH";

struct ShareLoader : public SimpleXMLReader::CallBack {
	ShareLoader(ShareManager::Directory::Map& aDirs) : dirs(aDirs), cur(0), depth(0) { }
	virtual void startTag(const string& p_name, StringPairList& attribs, bool simple) {
		if(p_name == SDIRECTORY) {
			const string& name = getAttrib(attribs, SNAME, 0);
			if(!name.empty()) {
				if(depth == 0) {
					for(ShareManager::Directory::MapIter i = dirs.begin(); i != dirs.end(); ++i) {
						if(Util::stricmp(i->second->getName(), name) == 0) {
							cur = i->second;
							break;
						}
					}
				} else if(cur) {
					cur = new ShareManager::Directory(name, cur);
					cur->addType(SearchManager::TYPE_DIRECTORY); // needed since we match our own name in directory searches
					cur->getParent()->directories[cur->getName()] = cur;
				}
			}

			if(simple) {
				if(cur) {
				cur = cur->getParent();
				}
			} else {
				depth++;
			}
		} else if(cur && p_name == SFILE) {
			const string& fname = getAttrib(attribs, SNAME, 0);
			const string& size = getAttrib(attribs, SSIZE, 1);
			const string& root = getAttrib(attribs, STTH, 2);
			if(fname.empty() || size.empty() || (root.size() != 39)) {
				dcdebug("Invalid file found: %s\n", fname.c_str());
				return;
			}
			cur->files.insert(ShareManager::Directory::File(fname, Util::toInt64(size), cur, TTHValue(root)));
		}
	}
	virtual void endTag(const string& name, const string&) {
		if(name == SDIRECTORY) {
			depth--;
			if(cur) {
				cur = cur->getParent();
			}
		}
	}

private:
	ShareManager::Directory::Map& dirs;

	ShareManager::Directory* cur;
	size_t depth;
};

bool ShareManager::loadCache() throw() {
	try {
		ShareLoader loader(directories);
		string txt;
		::File l_ff(Util::getConfigPath() + "files.xml.bz2", ::File::READ, ::File::OPEN);
		FilteredInputStream<UnBZFilter, false> f(&l_ff);
		const size_t BUF_SIZE = 64*1024;
		AutoArray<char> buf(BUF_SIZE);
		size_t len;
		for(;;) {
			size_t n = BUF_SIZE;
			len = f.read(buf, n);
			txt.append(buf, len);
			if(len < BUF_SIZE)
				break;
		}

		SimpleXMLReader(&loader).fromXML(txt);

		for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
			addTree(*i->second);
		}

		return true;
	} catch(const Exception& e) {
		dcdebug("%s\n", e.getError().c_str());
	}
	return false;
}

void ShareManager::save(SimpleXML& aXml) {
	Lock l(cs);
	
	aXml.addTag("Share");
	aXml.stepIn();
	for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
		aXml.addTag("Directory", i->first);
		aXml.addChildAttrib("Virtual", i->second->getName());
	}
	aXml.stepOut();
	aXml.addTag("NoShare");
	aXml.stepIn();
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j) {
		aXml.addTag("Directory", *j);
	}
	aXml.stepOut();
}

void ShareManager::addDirectory(const string& realPath, const string& virtualName) throw(ShareException) {
	if(!Util::fileExists(realPath))
		return;

	if(realPath.empty() || virtualName.empty()) {
		throw ShareException(STRING(NO_DIRECTORY_SPECIFIED));
	}

	if(Util::stricmp(SETTING(TEMP_DOWNLOAD_DIRECTORY), realPath) == 0) {
		throw ShareException(STRING(DONT_SHARE_TEMP_DIRECTORY));
	}

	string vName = validateVirtual(virtualName);

	slist<string> removeMap;
	{
		Lock l(cs);
		
		Directory::Map a = directories;
		for(Directory::MapIter i = a.begin(); i != a.end(); ++i) {
			if(Util::strnicmp(realPath, i->first, i->first.length()) == 0) {
				// Trying to share an already shared directory
				removeMap.push_front(i->first);
			} else if(Util::strnicmp(realPath, i->first, realPath.length()) == 0) {
				// Trying to share a parent directory
				removeMap.push_front(i->first);	
			}
		}

		if(hasVirtual(vName)) {
			throw ShareException(STRING(VIRTUAL_NAME_EXISTS));
		}
	}

	for(slist<string>::const_iterator i = removeMap.begin(); i != removeMap.end(); ++i) {
		removeDirectory(*i);
	}
	
	Directory* dp = buildTree(realPath, 0);
	dp->setName(vName);

	{
		Lock l(cs);
		addTree(*dp);

		directories[realPath] = dp;
		setDirty();
	}
}

void ShareManager::removeDirectory(const string& realPath) {
	if(realPath.empty())
		return;

	{
		Lock l(cs);

		Directory::Map::iterator i = directories.find(realPath);
		if(i != directories.end()) {
			delete i->second;
			directories.erase(i);
		}

		rebuildIndices();
		setDirty();
	}

	HashManager::getInstance()->stopHashing(realPath);
}

void ShareManager::renameDirectory(const string& realPath, const string& virtualName) throw(ShareException) {
	string vName = validateVirtual(virtualName);

	Lock l(cs);
	//Find the virtual name
	if (hasVirtual(vName)) {
		throw ShareException(STRING(VIRTUAL_NAME_EXISTS));
	}

	Directory::MapIter j = directories.find(realPath);
	if(j == directories.end())
		return;

	j->second->setName(vName);
}

ShareManager::Directory::Map::const_iterator ShareManager::getByVirtual(const string& virtualName) const throw() {
	for(Directory::Map::const_iterator i = directories.begin(); i != directories.end(); ++i) {
		if(Util::stricmp(i->second->getName(), virtualName) == 0) {
			return i;
		}
	}
	return directories.end();
}

int64_t ShareManager::getShareSize(const string& realPath) const throw() {
	Lock l(cs);
	dcassert(realPath.size()>0);
	Directory::Map::const_iterator i = directories.find(realPath);

	if(i != directories.end()) {
		return i->second->getSize();
	}

	return -1;
}

int64_t ShareManager::getShareSize() const throw() {
	Lock l(cs);
	int64_t tmp = 0;
	for(Directory::Map::const_iterator i = directories.begin(); i != directories.end(); ++i) {
		tmp += i->second->getSize();
	}
	return tmp;
}

size_t ShareManager::getSharedFiles() const throw() {
	Lock l(cs);
	return tthIndex.size();
}

class FileFindIter {
#ifdef _WIN32
public:
	/** End iterator constructor */
	FileFindIter() : handle(INVALID_HANDLE_VALUE) { }
	/** Begin iterator constructor, path in utf-8 */
	FileFindIter(const string& path) : handle(INVALID_HANDLE_VALUE) { 
		handle = ::FindFirstFile(Text::toT(path).c_str(), &data);
	}

	~FileFindIter() {
		if(handle != INVALID_HANDLE_VALUE) {
			::FindClose(handle);
		}
	}

	FileFindIter& operator++() {
		if(!::FindNextFile(handle, &data)) {
			::FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
		return *this;
	}
	bool operator!=(const FileFindIter& rhs) const { return handle != rhs.handle; }

	struct DirData : public WIN32_FIND_DATA {
		string getFileName() const {
			return Text::fromT(cFileName);
		}

		bool isDirectory() const  {
			return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
		}

		bool isHidden() const {
			return (dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) > 0;
//[-]PPA				|| (cFileName[0] == L'.'));
						}

		int64_t getSize() const  {
			return (int64_t)nFileSizeLow | ((int64_t)nFileSizeHigh)<<32;
		}

		uint32_t getLastWriteTime() {
			return File::convertTime(&ftLastWriteTime);
		}
	};


private:
	HANDLE handle;
#else
// This code has been cleaned up/fixed a little.
public:
	FileFindIter() {
		dir = NULL;
		data.ent = NULL;
	}
	
	~FileFindIter() {
		if (dir) closedir(dir);
	}

	FileFindIter(const string& name) {
		string filename = Text::fromUtf8(name);
		dir = opendir(filename.c_str());
		if (!dir)
			return;
		data.base = filename;
		data.ent = readdir(dir);
		if (!data.ent) {
			closedir(dir);
			dir = NULL;
		}
	}
	
	FileFindIter& operator++() {
		if (!dir) 
			return *this;
		data.ent = readdir(dir);
		if (!data.ent) {
			closedir(dir);
			dir = NULL;
		}
		return *this;
	}
	
	// good enough to say if it's null
	bool operator !=(const FileFindIter& rhs) const {
		return dir != rhs.dir;
	}

	struct DirData {
		DirData() : ent(NULL) {}
		string getFileName() {
			if (!ent) return Util::emptyString;
			return Text::toUtf8(ent->d_name);
		}
		bool isDirectory() {
			struct stat inode;
			if (!ent) return false;
			if (stat((base + PATH_SEPARATOR + ent->d_name).c_str(), &inode) == -1) return false;
			return S_ISDIR(inode.st_mode);
		}
		bool isHidden() {
			if (!ent) return false;
			return ent->d_name[0] == '.';
		}
		int64_t getSize() {
			struct stat inode;
			if (!ent) return false;
			if (stat((base + PATH_SEPARATOR + ent->d_name).c_str(), &inode) == -1) return 0;
			return inode.st_size;
		}
		uint32_t getLastWriteTime() {
			struct stat inode;
			if (!ent) return false;
			if (stat((base + PATH_SEPARATOR + ent->d_name).c_str(), &inode) == -1) return 0;
			return inode.st_mtime;
		}
		struct dirent* ent;
		string base;
	};
private:
	DIR* dir;
#endif

public:

	DirData& operator*() { return data; }
	DirData* operator->() { return &data; }

private:
	DirData data;

};
ShareManager::Directory* ShareManager::buildTree(const string& aName, Directory* aParent) {
	Directory* dir = new Directory(Util::getLastDir(aName), aParent);
	dir->addType(SearchManager::TYPE_DIRECTORY); // needed since we match our own name in directory searches

	Directory::File::Set::iterator lastFileIter = dir->files.begin();

	FileFindIter end;
	const string l_skip_list = SETTING(SKIPLIST_SHARE); 
#ifdef _WIN32
		for(FileFindIter i(aName + "*"); i != end; ++i) {
#else
	//the fileiter just searches directorys for now, not sure if more 
	//will be needed later
	//for(FileFindIter i(aName + "*"); i != end; ++i) {
	for(FileFindIter i(aName); i != end; ++i) {
#endif
		const string name = i->getFileName();
		if(name == Util::m_dot || name == Util::m_dot_dot)
			continue;
		if(l_skip_list.size())
		  if(Wildcard::patternMatch(name, l_skip_list, ';')) {
                        LogManager::getInstance()->message("User has choosen not to share file: " + name
							 + " (" + STRING(SIZE) + ": " + Util::toString(File::getSize(name)) + " " 
							 + STRING(B) + ") (" + STRING(DIRECTORY) + ": \"" + aName + "\")");
			continue;
		}

		if(BOOLSETTING(REMOVE_FORBIDDEN)) {
		//check for forbidden file patterns
			string::size_type nameLen = name.size();
			string fileExt = Util::getFileExt(name);
			if((Util::stricmp(fileExt.c_str(),  ".jc!") == 0) ||
				(Util::stricmp(fileExt.c_str(), ".dmf") == 0) ||
				(Util::stricmp(fileExt.c_str(), ".!ut") == 0) ||
				(Util::stricmp(fileExt.c_str(), ".bc!") == 0) ||
                (Util::stricmp(fileExt.c_str(), ".tdc") == 0) ||
				(Util::stricmp(fileExt.c_str(), ".GetRight") == 0) ||
				(Util::stricmp(fileExt.c_str(), ".temp") == 0) ||
				(nameLen > 9 && name.rfind("part.met") == nameLen - 8) ||				
				(name.find("__INCOMPLETE__") == 0) ||		//winmx
				(name.find("__incomplete__") == 0) ||		//winmx
				(nameLen >= 28 && name.find("download") == 0 && 
				  name.rfind(".dat") == nameLen - 4 && 
				  Util::toInt64(name.substr(8, 16)))) {		//kazaa temps
					LogManager::getInstance()->message("Forbidden file will not be shared: " +
						name + " (" + STRING(SIZE) + ": " 
						+ Util::toString(File::getSize(name)) + " " 
						+ STRING(B) + ") (" + STRING(DIRECTORY) + ": \"" + aName + "\")");
					continue;
			}
		}
		if(i->isHidden() && !BOOLSETTING(SHARE_HIDDEN))
			continue;

		if(i->isDirectory()) {
			const string newName = aName + name + PATH_SEPARATOR;
			if((Util::stricmp(newName + PATH_SEPARATOR, SETTING(TEMP_DOWNLOAD_DIRECTORY)) != 0) && shareFolder(newName)) {
				dir->directories[name] = buildTree(newName, dir);
			}
		} else {
			// Not a directory, assume it's a file...make sure we're not sharing the settings file...
			const string l_ext = Util::getFileExt(name);
			if( (Util::stricmp(name.c_str(), "DCPlusPlus.xml") != 0) && 
				(Util::stricmp(name.c_str(), "Favorites.xml") != 0) &&
				(Util::stricmp(l_ext.c_str(), ".jc!") != 0) &&
				(Util::stricmp(l_ext.c_str(), ".dmf") != 0) &&
				(Util::stricmp(l_ext.c_str(), ".!ut") != 0) &&
				(Util::stricmp(l_ext.c_str(), ".bc!") != 0) &&
				(Util::stricmp(l_ext.c_str(), ".GetRight") != 0) &&
				(Util::stricmp(l_ext.c_str(), ".dctmp") != 0) &&
				(Util::stricmp(l_ext.c_str(), Download::ANTI_FRAG_EXT) != 0) ){
				const int64_t size = i->getSize();
#ifdef PPA_INCLUDE_SSL
				const string fileName = aName + name;
				if(Util::stricmp(fileName, SETTING(TLS_PRIVATE_KEY_FILE)) == 0) {
					continue;
				}
#endif
				try {
	                const string fname = Text::toLower(name);
	                const string fpath = Text::toLower(aName);
					if(HashManager::getInstance()->checkTTH(fname,fpath, size, i->getLastWriteTime()))
						lastFileIter = dir->files.insert(lastFileIter, Directory::File(name, size, dir, 
						     HashManager::getInstance()->getTTH(fname,fpath, size)
						));
				} catch(const HashException&) {
				}
			}
		}
	}

	return dir;
}

void ShareManager::addTree(Directory& dir) {
	bloom.add(Text::toLower(dir.getName()));

	for(Directory::MapIter i = dir.directories.begin(); i != dir.directories.end(); ++i) {
		addTree(*i->second);
	}

	dir.size = 0;

	for(Directory::File::Iter i = dir.files.begin(); i != dir.files.end(); ) {
		addFile(dir, i++);
	}
}

void ShareManager::rebuildIndices() {
	sharedSize = 0;
	tthIndex.clear();
	bloom.clear();

	for(Directory::Map::const_iterator i = directories.begin(); i != directories.end(); ++i) {
		addTree(*i->second);
	}
}

void ShareManager::addFile(Directory& dir, const Directory::File::Iter& i) {
   const Directory::File& f = *i;
   if(f.getName().size()) //[+]PPA
   {
	HashFileIter j = tthIndex.find(f.getTTH());
	if(j == tthIndex.end()) {
		const int64_t l_size =  f.getSize();
		dir.size   += l_size;
		sharedSize += l_size;
	}
	dir.addType(getType(f.getName()));
	tthIndex.insert(make_pair(f.getTTH(), i));
	bloom.add(Text::toLower(f.getName()));
   }
}

void ShareManager::refresh(bool dirs /* = false */, bool aUpdate /* = true */, bool block /* = false */) throw() {
	if(Thread::safeExchange(refreshing, 1) == 1) {
		LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_IN_PROGRESS));
		return;
	}

	update = aUpdate;
	refreshDirs = dirs;
	join();
	bool cached = false;
	if(initial) {
		cached = loadCache();
		initial = false;
	}
	try {
		start();
		if(block && !cached) {
			join();
		} else {
			setThreadPriority(Thread::LOW);
		}
	} catch(const ThreadException& e) {
		LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_FAILED) + e.getError());
	}
}

StringPairList ShareManager::getDirectories() const throw() {
	Lock l(cs);
	StringPairList ret;
	for(Directory::Map::const_iterator i = directories.begin(); i != directories.end(); ++i) {
		ret.push_back(make_pair(i->second->getName(), i->first));
	}
	return ret;
}

int ShareManager::run() {
	
	StringPairList dirs = getDirectories();
	// Don't need to refresh if no directories are shared
	if(dirs.begin() == dirs.end())		
		refreshDirs = false;
	{
		if(refreshDirs) {
			LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_INITIATED));
			sharedSize = 0;
			lastFullUpdate = GET_TICK();

			Directory::Map newDirs;
			for(StringPairIter i = dirs.begin(); i != dirs.end(); ++i) {
				Directory* dp = buildTree(i->second, 0);
				dp->setName(i->first);
				newDirs.insert(make_pair(i->second, dp));
			}

			{
				Lock l(cs);
				for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
					delete i->second;
				}
				directories = newDirs;

				rebuildIndices();
			}
			refreshDirs = false;

	LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_FINISHED));
		}
	}

	if(update) {
		ClientManager::getInstance()->infoUpdated(false);
	}
	refreshing = 0;
	return 0;
}
		
void ShareManager::generateXmlList() {
	Lock l(cs);
	if(xmlDirty && (lastXmlUpdate + 15 * 60 * 1000 < GET_TICK() || lastXmlUpdate < lastFullUpdate)) {
		listN++;

		try {
			string tmp2;
			string indent;

			string newXmlName = Util::getConfigPath() + "files" + Util::toString(listN) + ".xml.bz2";
			{
				File f(newXmlName, File::WRITE, File::TRUNCATE | File::CREATE);
				// We don't care about the leaves...
				CalcOutputStream<TTFilter<1024*1024*1024>, false> bzTree(&f);
				FilteredOutputStream<BZFilter, false> bzipper(&bzTree);
				CountOutputStream<false> count(&bzipper);
				CalcOutputStream<TTFilter<1024*1024*1024>, false> newXmlFile(&count);

				newXmlFile.write(SimpleXML::utf8Header);
				newXmlFile.write("<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"/\" Generator=\"DC++ " DCVERSIONSTRING "\">\r\n");
				for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
					i->second->toXml(newXmlFile, indent, tmp2, true);
				}
				newXmlFile.write("</FileListing>");
				newXmlFile.flush();

				xmlListLen = count.getCount();

				newXmlFile.getFilter().getTree().finalize();
				bzTree.getFilter().getTree().finalize();

				xmlRoot = newXmlFile.getFilter().getTree().getRoot();
				bzXmlRoot = bzTree.getFilter().getTree().getRoot();
			}

			if(!Util::fileExists(Util::getConfigPath() + "Emptyfiles.xml.bz2")) {
					string emptyXmlName = Util::getConfigPath() + "Emptyfiles.xml.bz2"; // Hide Share Mod
					FilteredOutputStream<BZFilter, true> emptyXmlFile(new File(emptyXmlName, File::WRITE, File::TRUNCATE | File::CREATE));
					emptyXmlFile.write(SimpleXML::utf8Header);
					emptyXmlFile.write("<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"/\" Generator=\"DC++ " DCVERSIONSTRING "\">\r\n"); // Hide Share Mod
					emptyXmlFile.write("</FileListing>");
					emptyXmlFile.flush();
			}

			if(bzXmlRef.get()) {
				bzXmlRef.reset();
				File::deleteFile(getBZXmlFile());
			}

			try {
				File::renameFile(newXmlName, Util::getConfigPath() + "files.xml.bz2");
				newXmlName = Util::getConfigPath() + "files.xml.bz2";
			} catch(const FileException& e) {
                          LogManager::getInstance()->message("Error renaming cache " + newXmlName + " to files.xml.bz2: " + e.getError());
			}
			bzXmlRef = auto_ptr<File>(new File(newXmlName, File::READ, File::OPEN));
			setBZXmlFile(newXmlName);
			bzXmlListLen = File::getSize(newXmlName);
		} catch(const Exception&) {
			// No new file lists...
		}

		xmlDirty = false;
		lastXmlUpdate = GET_TICK();
	}
}

MemoryInputStream* ShareManager::generatePartialList(const string& dir, bool recurse) const {
	if(dir[0] != '/' || dir[dir.size()-1] != '/')
		return 0;

	string xml = SimpleXML::utf8Header;
	string tmp;
	xml += "<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"" + SimpleXML::escape(dir, tmp, false) + "\" Generator=\"DC++ " DCVERSIONSTRING "\">\r\n";
	StringOutputStream sos(xml);
	string indent = "\t";

	Lock l(cs);
	if(dir == "/") {
		for(Directory::Map::const_iterator i = directories.begin(); i != directories.end(); ++i) {
			tmp.clear();
			i->second->toXml(sos, indent, tmp, recurse);
		}
	} else {
		string::size_type i = 1, j = 1;
		Directory::Map::const_iterator it = directories.end();
		bool first = true;
		while( (i = dir.find('/', j)) != string::npos) {
			if(i == j) {
				j++;
				continue;
			}

			if(first) {
				first = false;
				it = getByVirtual(dir.substr(j, i-j));

				if(it == directories.end())
					return 0;
			} else {
				Directory::Map::const_iterator it2 = it->second->directories.find(dir.substr(j, i-j));
				if(it2 == it->second->directories.end()) {
					return 0;
				}
				it = it2;
			}
			j = i + 1;
		}
		if (first) return NULL;
		for(Directory::Map::const_iterator it2 = it->second->directories.begin(); it2 != it->second->directories.end(); ++it2) {
			it2->second->toXml(sos, indent, tmp, recurse);
		}
		it->second->filesToXml(sos, indent, tmp);
	}

	xml += "</FileListing>";
	return new MemoryInputStream(xml);
}

#define LITERAL(n) n, sizeof(n)-1
void ShareManager::Directory::toXml(OutputStream& xmlFile, string& indent, string& tmp2, bool fullList) const {
	xmlFile.write(indent);
	xmlFile.write(LITERAL("<Directory Name=\""));
	xmlFile.write(SimpleXML::escape(name, tmp2, true));

	if(fullList) {
		xmlFile.write(LITERAL("\">\r\n"));

		indent += '\t';
		for(Map::const_iterator i = directories.begin(); i != directories.end(); ++i) {
			i->second->toXml(xmlFile, indent, tmp2, fullList);
		}

		filesToXml(xmlFile, indent, tmp2);

		indent.erase(indent.length()-1);
		xmlFile.write(indent);
		xmlFile.write(LITERAL("</Directory>\r\n"));
	} else {
		if(directories.empty() && files.empty()) {
			xmlFile.write(LITERAL("\" />\r\n"));
		} else {
			xmlFile.write(LITERAL("\" Incomplete=\"1\" />\r\n"));
		}
	}
}

void ShareManager::Directory::filesToXml(OutputStream& xmlFile, string& indent, string& tmp2) const {
	for(Directory::File::Set::const_iterator i = files.begin(); i != files.end(); ++i) {
		const Directory::File& f = *i;

		xmlFile.write(indent);
		xmlFile.write(LITERAL("<File Name=\""));
		xmlFile.write(SimpleXML::escape(f.getName(), tmp2, true));
		xmlFile.write(LITERAL("\" Size=\""));
		xmlFile.write(Util::toString(f.getSize()));
		xmlFile.write(LITERAL("\" TTH=\""));
		tmp2.clear();
		xmlFile.write(f.getTTH().toBase32(tmp2));
		xmlFile.write(LITERAL("\"/>\r\n"));
	}
}

// These ones we can look up as ints (4 bytes...)...

static const char* typeAudio[] = { ".mp3", ".ogg",".wav", ".wma", ".mp2", ".mid", ".669", ".aac", ".aif", ".amf", ".ams", ".ape", ".dbm", ".dmf", ".dsm", ".far", ".mdl", ".med", ".mod", ".mol", ".mp1", ".mp4", ".mpa", ".mpc", ".mpp", ".mtm", ".nst", ".okt", ".psm", ".ptm", ".rmi", ".s3m", ".stm", ".ult", ".umx", ".wow" };
static const char* typeCompressed[] = { ".rar", ".zip",".ace", ".arj", ".hqx", ".lha", ".sea", ".tar", ".tgz", ".uc2"};
static const char* typeDocument[] = { ".htm", ".doc", ".txt", ".nfo", ".pdf", ".chm" };
static const char* typeExecutable[] = { ".exe", ".com",
                                        ".msi" //[+]PPA
                                      };
static const char* typePicture[] = { ".jpg", ".gif", ".png", ".eps", ".img", ".pct", ".psp", ".pic", ".tif", ".rle", ".bmp", ".pcx", ".jpe", ".dcx", ".emf", ".ico", ".psd", ".tga", ".wmf", ".xif" };
static const char* typeVideo[] = { ".avi", ".mpg", ".mov", ".asf", ".pxp", ".wmv", ".ogm", ".mkv", ".m1v", ".m2v", ".mpe", ".mps", ".mpv", ".ram", ".vob" };
//[+]PPA
static const char* typeCDImage[] = { ".iso", ".nrg", ".mdf", ".mds", ".vcd", ".isz", ".bwt",".ccd",".cdi",".pdi",".cue" };

static const string type2Audio[] = { ".au", ".it", ".ra", ".xm", ".aiff", ".flac", ".midi", };
static const string type2Compressed[] = { ".7z", //[+]PPA
                                         ".gz" };
static const string type2Picture[] = { ".ai", ".ps", ".pict", ".jpeg", ".tiff" };
static const string type2Video[] = { ".rm", ".divx", ".mpeg", ".mp1v", ".mp2v", ".mpv1", ".mpv2", ".qt", ".rv", ".vivo", 
".ts" 
/*
�������� � ������ ������ ����������� ���������� .ts
���� � ���, ��� HDTV-����� �������� � � transport stream ��������� (.ts). � ����� � � ������� ������ ��� HDTV ������ (����� ������ �� ����� 3 ���), �������� ������ ������ � mpeg4 ���� mkv, mp4, avi. � �������� ts ���������� sad.gif
*/
};

#define IS_TYPE(x) ( type == (*((uint32_t*)x)) )
#define IS_TYPE2(x) (Util::stricmp(aString.c_str() + aString.length() - x.length(), x.c_str()) == 0)

bool ShareManager::checkType(const string& aString, int aType) {
	if(aType == SearchManager::TYPE_ANY)
		return true;

	if(aString.length() < 5)
		return false;
	
	const char* c = aString.c_str() + aString.length() - 3;
	if(!Text::isAscii(c))
		return false;

	uint32_t type = '.' | (Text::asciiToLower(c[0]) << 8) | (Text::asciiToLower(c[1]) << 16) | (((uint32_t)Text::asciiToLower(c[2])) << 24);

	switch(aType) {
	case SearchManager::TYPE_AUDIO:
		{
			for(size_t i = 0; i < COUNTOF(typeAudio); i++) {
				if(IS_TYPE(typeAudio[i])) {
					return true;
				}
			}
			for(size_t i = 0; i < COUNTOF(type2Audio); i++) {
				if(IS_TYPE2(type2Audio[i])) {
					return true;
				}
			}
		}
		break;
	case SearchManager::TYPE_COMPRESSED:
		{
			for(size_t i = 0; i < COUNTOF(typeCompressed); i++) {
				if(IS_TYPE(typeCompressed[i])) {
					return true;
				}
			}
			if( IS_TYPE2(type2Compressed[0]) ) {
				return true;
			}
		}
		break;
	case SearchManager::TYPE_DOCUMENT:
		for(size_t i = 0; i < COUNTOF(typeDocument); i++) {
			if(IS_TYPE(typeDocument[i])) {
				return true;
			}
		}
		break;
	case SearchManager::TYPE_EXECUTABLE:
		if(IS_TYPE(typeExecutable[0]) || IS_TYPE(typeExecutable[1])) {
			return true;
		}
		break;
	case SearchManager::TYPE_PICTURE:
		{
			for(size_t i = 0; i < COUNTOF(typePicture); i++) {
				if(IS_TYPE(typePicture[i])) {
					return true;
				}
			}
			for(size_t i = 0; i < (sizeof(type2Picture) / sizeof(type2Picture[0])); i++) {
				if(IS_TYPE2(type2Picture[i])) {
					return true;
				}
			}
		}
		break;
	case SearchManager::TYPE_VIDEO:
		{
			for(size_t i = 0; i < COUNTOF(typeVideo); i++) {
				if(IS_TYPE(typeVideo[i])) {
					return true;
				}
			}
			for(size_t i = 0; i < COUNTOF(type2Video); i++) {
				if(IS_TYPE2(type2Video[i])) {
					return true;
				}
			}
		}
		break;
//[+]PPA
	case SearchManager::TYPE_CD_IMAGE:
		{
			for(size_t i = 0; i < COUNTOF(typeCDImage); i++) {
				if(IS_TYPE(typeCDImage[i])) {
					return true;
				}
			}
		}
		break;
	default:
		dcasserta(0);
		break;
	}
	return false;
}

SearchManager::TypeModes ShareManager::getType(const string& aFileName) const throw() {
	if(aFileName[aFileName.length() - 1] == PATH_SEPARATOR) {
		return SearchManager::TYPE_DIRECTORY;
	}

	if(checkType(aFileName, SearchManager::TYPE_VIDEO))
		return SearchManager::TYPE_VIDEO;
	else if(checkType(aFileName, SearchManager::TYPE_AUDIO))
		return SearchManager::TYPE_AUDIO;
	else if(checkType(aFileName, SearchManager::TYPE_COMPRESSED))
		return SearchManager::TYPE_COMPRESSED;
	else if(checkType(aFileName, SearchManager::TYPE_DOCUMENT))
		return SearchManager::TYPE_DOCUMENT;
	else if(checkType(aFileName, SearchManager::TYPE_EXECUTABLE))
		return SearchManager::TYPE_EXECUTABLE;
	else if(checkType(aFileName, SearchManager::TYPE_PICTURE))
		return SearchManager::TYPE_PICTURE;
	if(checkType(aFileName, SearchManager::TYPE_CD_IMAGE)) //[+]PPA
		return SearchManager::TYPE_CD_IMAGE;

	return SearchManager::TYPE_ANY;
}

/**
 * Alright, the main point here is that when searching, a search string is most often found in 
 * the filename, not directory name, so we want to make that case faster. Also, we want to
 * avoid changing StringLists unless we absolutely have to --> this should only be done if a string
 * has been matched in the directory name. This new stringlist should also be used in all descendants,
 * but not the parents...
 */
void ShareManager::Directory::search(SearchResult::List& aResults, StringSearch::List& aStrings, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) const throw() {
	// Skip everything if there's nothing to find here (doh! =)
	if(!hasType(aFileType))
		return;

	StringSearch::List* cur = &aStrings;
	auto_ptr<StringSearch::List> newStr;

	// Find any matches in the directory name
	for(StringSearch::List::const_iterator k = aStrings.begin(); k != aStrings.end(); ++k) {
		if(k->match(name)) {
			if(!newStr.get()) {
				newStr = auto_ptr<StringSearch::List>(new StringSearch::List(aStrings));
			}
			newStr->erase(remove(newStr->begin(), newStr->end(), *k), newStr->end());
		}
	}

	if(newStr.get() != 0) {
		cur = newStr.get();
	}

	bool sizeOk = (aSearchType != SearchManager::SIZE_ATLEAST) || (aSize == 0);
	if( (cur->empty()) && 
		(((aFileType == SearchManager::TYPE_ANY) && sizeOk) || (aFileType == SearchManager::TYPE_DIRECTORY)) ) {
		// We satisfied all the search words! Add the directory...(NMDC searches don't support directory size)
		SearchResult* sr = new SearchResult(SearchResult::TYPE_DIRECTORY, 0, getFullName(), TTHValue());
		aResults.push_back(sr);
		ShareManager::getInstance()->setHits(ShareManager::getInstance()->getHits()+1);
	}

	if(aFileType != SearchManager::TYPE_DIRECTORY) {
		for(File::Set::const_iterator i = files.begin(); i != files.end(); ++i) {
			
			if(aSearchType == SearchManager::SIZE_ATLEAST && aSize > i->getSize()) {
				continue;
			} else if(aSearchType == SearchManager::SIZE_ATMOST && aSize < i->getSize()) {
				continue;
			}	
			StringSearch::List::const_iterator j = cur->begin();
			for(; j != cur->end() && j->match(i->getName()); ++j) 
				;	// Empty
			
			if(j != cur->end())
				continue;
			
			// Check file type...
			if(checkType(i->getName(), aFileType)) {
				SearchResult* sr = new SearchResult(SearchResult::TYPE_FILE, i->getSize(), getFullName() + i->getName(), i->getTTH());
				aResults.push_back(sr);
				ShareManager::getInstance()->setHits(ShareManager::getInstance()->getHits()+1);
				if(aResults.size() >= maxResults) {
					break;
				}
			}
		}
	}

	for(Directory::Map::const_iterator l = directories.begin(); (l != directories.end()) && (aResults.size() < maxResults); ++l) {
			l->second->search(aResults, *cur, aSearchType, aSize, aFileType, aClient, maxResults);
	}
}

void ShareManager::search(SearchResult::List& results, const string& aString, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) throw() {
	Lock l(cs);
	if(aFileType == SearchManager::TYPE_TTH) {
		if(aString.compare(0, 4, "TTH:") == 0) {
			TTHValue tth(aString.substr(4));
			HashFileMap::const_iterator i = tthIndex.find(tth);
			if(i != tthIndex.end() && i->second->getParent()) {
				SearchResult* sr = new SearchResult(SearchResult::TYPE_FILE, i->second->getSize(), 
					i->second->getParent()->getFullName() + i->second->getName(), i->second->getTTH());

				results.push_back(sr);
				ShareManager::getInstance()->addHits(1);
			}
		}
		return;
	}
	StringTokenizer<string> t(Text::toLower(aString), '$');
	StringList& sl = t.getTokens();
	if(!bloom.match(sl))
		return;

	StringSearch::List ssl;
	for(StringList::const_iterator i = sl.begin(); i != sl.end(); ++i) {
		if(!i->empty()) {
			ssl.push_back(StringSearch(*i));
		}
	}
	if(ssl.empty())
		return;

	for(Directory::Map::const_iterator j = directories.begin(); (j != directories.end()) && (results.size() < maxResults); ++j) {
		j->second->search(results, ssl, aSearchType, aSize, aFileType, aClient, maxResults);
	}
}

namespace {
	inline uint16_t toCode(char a, char b) { return (uint16_t)a | ((uint16_t)b)<<8; }
}

ShareManager::AdcSearch::AdcSearch(const StringList& params) : include(&includeX), gt(0), 
	lt(numeric_limits<int64_t>::max()), hasRoot(false), isDirectory(false)
{
	for(StringIterC i = params.begin(); i != params.end(); ++i) {
		const string& p = *i;
		if(p.length() <= 2)
			continue;

		uint16_t cmd = toCode(p[0], p[1]);
		if(toCode('T', 'R') == cmd) {
			hasRoot = true;
			root = TTHValue(p.substr(2));
			return;
		} else if(toCode('A', 'N') == cmd) {
			includeX.push_back(StringSearch(p.substr(2)));		
		} else if(toCode('N', 'O') == cmd) {
			exclude.push_back(StringSearch(p.substr(2)));
		} else if(toCode('E', 'X') == cmd) {
			ext.push_back(p.substr(2));
		} else if(toCode('G', 'E') == cmd) {
			gt = Util::toInt64(p.substr(2));
		} else if(toCode('L', 'E') == cmd) {
			lt = Util::toInt64(p.substr(2));
		} else if(toCode('E', 'Q') == cmd) {
			lt = gt = Util::toInt64(p.substr(2));
		} else if(toCode('T', 'Y') == cmd) {
			isDirectory = (p[2] == '2');
		}
	}
}

void ShareManager::Directory::search(SearchResult::List& aResults, AdcSearch& aStrings, StringList::size_type maxResults) const throw() {
	StringSearch::List* cur = aStrings.include;
	StringSearch::List* old = aStrings.include;

	auto_ptr<StringSearch::List> newStr;

	// Find any matches in the directory name
	for(StringSearch::List::const_iterator k = cur->begin(); k != cur->end(); ++k) {
		if(k->match(name) && !aStrings.isExcluded(name)) {
			if(!newStr.get()) {
				newStr = auto_ptr<StringSearch::List>(new StringSearch::List(*cur));
			}
			newStr->erase(remove(newStr->begin(), newStr->end(), *k), newStr->end());
		}
	}

	if(newStr.get() != 0) {
		cur = newStr.get();
	}

	bool sizeOk = (aStrings.gt == 0);
	if( cur->empty() && aStrings.ext.empty() && sizeOk ) {
		// We satisfied all the search words! Add the directory...
		SearchResult* sr = new SearchResult(SearchResult::TYPE_DIRECTORY, getSize(), getFullName(), TTHValue());
		aResults.push_back(sr);
		ShareManager::getInstance()->setHits(ShareManager::getInstance()->getHits()+1);
	}

	if(!aStrings.isDirectory) {
		for(File::Set::const_iterator i = files.begin(); i != files.end(); ++i) {

			if(!(i->getSize() >= aStrings.gt)) {
				continue;
			} else if(!(i->getSize() <= aStrings.lt)) {
				continue;
			}	

			if(aStrings.isExcluded(i->getName()))
				continue;

			StringSearch::List::const_iterator j = cur->begin();
			for(; j != cur->end() && j->match(i->getName()); ++j) 
				;	// Empty

			if(j != cur->end())
				continue;

			// Check file type...
			if(aStrings.hasExt(i->getName())) {

				SearchResult* sr = new SearchResult(SearchResult::TYPE_FILE, 
					i->getSize(), getFullName() + i->getName(), i->getTTH());
				aResults.push_back(sr);
				ShareManager::getInstance()->addHits(1);
				if(aResults.size() >= maxResults) {
					return;
				}
			}
		}
	}

	for(Directory::Map::const_iterator l = directories.begin(); (l != directories.end()) && (aResults.size() < maxResults); ++l) {
		l->second->search(aResults, aStrings, maxResults);
	}
	aStrings.include = old;
}

void ShareManager::search(SearchResult::List& results, const StringList& params, StringList::size_type maxResults) throw() {
	AdcSearch srch(params);	

	Lock l(cs);

	if(srch.hasRoot) {
		HashFileMap::const_iterator i = tthIndex.find(srch.root);
		if(i != tthIndex.end()) {
			SearchResult* sr = new SearchResult(SearchResult::TYPE_FILE, 
				i->second->getSize(), i->second->getParent()->getFullName() + i->second->getName(), 
				i->second->getTTH());
			results.push_back(sr);
			addHits(1);
		}
		return;
	}

	for(StringSearch::List::const_iterator i = srch.includeX.begin(); i != srch.includeX.end(); ++i) {
		if(!bloom.match(i->getPattern()))
			return;
	}

	for(Directory::Map::const_iterator j = directories.begin(); (j != directories.end()) && (results.size() < maxResults); ++j) {
		j->second->search(results, srch, maxResults);
	}
}

ShareManager::Directory* ShareManager::getDirectory(const string& fname) const {
	for(Directory::MapIter mi = directories.begin(); mi != directories.end(); ++mi) {
		if(Util::strnicmp(fname, mi->first, mi->first.length()) == 0) {
			Directory* d = mi->second;

			string::size_type i;
			string::size_type j = mi->first.length();
			while( (i = fname.find(PATH_SEPARATOR, j)) != string::npos) {
				mi = d->directories.find(fname.substr(j, i-j));
				j = i + 1;
				if(mi == d->directories.end())
					return NULL;
				d = mi->second;
			}
			return d;
		}
	}
	return NULL;
}

void ShareManager::on(DownloadManagerListener::Complete, Download* d, bool) throw() {
/* [-]PPA 
if(BOOLSETTING(ADD_FINISHED_INSTANTLY)) 
*/
	{
		// Check if finished download is supposed to be shared
		Lock l(cs);
		const string& n = d->getTarget();
		for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
			if(Util::strnicmp(i->first, n, i->first.size()) == 0 && n[i->first.size()] == PATH_SEPARATOR) {
//[-]PPA  	    string s = n.substr(i->first.size()+1);
				try {
					// Schedule for hashing, it'll be added automatically later on...
	                const string fname = Text::toLower(Util::getFileName(n));
	                const string fpath = Text::toLower(Util::getFilePath(n));
					HashManager::getInstance()->checkTTH(fname,fpath, d->getSize(), 0);
				} catch(const Exception&) {
					// Not a vital feature...
				}
				break;
			}
		}
	}
}

void ShareManager::on(HashManagerListener::TTHDone, const string& fname, const TTHValue& root) throw() {
	Lock l(cs);
	Directory* d = getDirectory(fname);
	if(d) {
		Directory::File::Set::const_iterator i = d->findFile(Util::getFileName(fname));
		if(i != d->files.end()) {
			if(root != i->getTTH())
				tthIndex.erase(i->getTTH());
			// Get rid of false constness...
			Directory::File* f = const_cast<Directory::File*>(&(*i));
			f->setTTH(root);
			tthIndex.insert(make_pair(f->getTTH(), i));
		} else {
			const string name = Util::getFileName(fname);
			const int64_t size = File::getSize(fname);
			Directory::File::Iter it = d->files.insert(Directory::File(name, size, d, root)).first;
			addFile(*d, it);
		}
		setDirty();
	}
}

void ShareManager::on(TimerManagerListener::Minute, uint32_t tick) throw() {
	if(SETTING(AUTO_REFRESH_TIME) > 0) {
		if(lastFullUpdate + SETTING(AUTO_REFRESH_TIME) * 60 * 1000 < tick) {
			refresh(true, true);
		}
	}
}

bool ShareManager::shareFolder(const string& path, bool thoroughCheck /* = false */) const {
	if(thoroughCheck)	// check if it's part of the share before checking if it's in the exclusions
	{
		bool result = false;
		for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i)
		{
			// is it a perfect match
			if((path.size() == i->first.size()) && (Util::stricmp(path, i->first) == 0))
				return true;
			else if (path.size() > i->first.size()) // this might be a subfolder of a shared folder
			{
				string temp = path.substr(0, i->first.size());
				// if the left-hand side matches and there is a \ in the remainder then it is a subfolder
				if((Util::stricmp(temp, i->first) == 0) && (path.find('\\', i->first.size()) != string::npos))
				{
					result = true;
					break;
				}
			}
		}

		if(!result)
			return false;
	}

	// check if it's an excluded folder or a sub folder of an excluded folder
	for(StringIterC j = notShared.begin(); j != notShared.end(); ++j)
	{		
		if(Util::stricmp(path, *j) == 0)
			return false;

		if(thoroughCheck)
		{
			if(path.size() > (*j).size())
			{
				string temp = path.substr(0, (*j).size());
				if((Util::stricmp(temp, *j) == 0) && (path[(*j).size()] == '\\'))
					return false;
			}
		}
	}
	return true;
}

int64_t ShareManager::addExcludeFolder(const string &path) {
	// make sure this is a sub folder of a shared folder
	bool result = false;
	for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i)
	{
		if(path.size() > i->first.size())
		{
			string temp = path.substr(0, i->first.size());
			if(Util::stricmp(temp, i->first) == 0)
			{
				result = true;
				break;
			}
		}
	}

	if(!result)
		return 0;

	// Make sure this not a subfolder of an already excluded folder
	for(StringIterC j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() >= (*j).size())
		{
			string temp = path.substr(0, (*j).size());
			if(Util::stricmp(temp, *j) == 0)
				return 0;
		}
	}

	// remove all sub folder excludes
	int64_t bytesNotCounted = 0;
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() < (*j).size())
		{
			string temp = (*j).substr(0, path.size());
			if(Util::stricmp(temp, path) == 0)
			{
				bytesNotCounted += Util::getDirSize(*j);
				j = notShared.erase(j);
				j--;
			}
		}
	}

	// add it to the list
	notShared.push_back(path);

	int64_t bytesRemoved = Util::getDirSize(path);

	return (bytesRemoved - bytesNotCounted);
}

int64_t ShareManager::removeExcludeFolder(const string &path, bool returnSize /* = true */) {
	int64_t bytesAdded = 0;
	// remove all sub folder excludes
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() <= (*j).size())
		{
			string temp = (*j).substr(0, path.size());
			if(Util::stricmp(temp, path) == 0)
			{
				if(returnSize) // this needs to be false if the files don't exist anymore
					bytesAdded += Util::getDirSize(*j);
				
				j = notShared.erase(j);
				j--;
			}
		}
	}
	
	return bytesAdded;
}

bool ShareManager::shareDownloads() {
	string downloadPath = SETTING(DOWNLOAD_DIRECTORY);
	string prevPath = SETTING(DOWNLOAD_DIRECTORY_SHARED);
	if (downloadPath != prevPath) {
		SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_DIRECTORY_SHARED, downloadPath);
		if (!shareFolder(downloadPath, true)) {
			string baseShareName = "MyDownloads";
			string shareName = baseShareName;
			size_t counter = 0;
			while (hasVirtual(shareName)) {
				shareName = baseShareName + Util::toString(++counter);
			}
			try {
				addDirectory(downloadPath, shareName);
				return true;
			}
			catch (Exception& e) {
				LOG_MESSAGE(string("Error sharing downloads: ") + e.getError());
			}
		}
	}
	return false;
}

/**
 * @file
 * $Id: ShareManager.cpp,v 1.3.2.1 2008/06/14 16:48:28 alexey Exp $
 */
