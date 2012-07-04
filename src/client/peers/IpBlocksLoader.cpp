#include "stdinc.h"
#include "IpBlocksLoader.h"
#include "../../utils/SystemUtils.h"
#include "../../md5/MD5Digest.h"
#include "../client/LogManager.h"

void IpBlocksLoader::startTag(const string& name, StringPairList& attribs, bool /*simple*/) {
	if (name == "block") {
		m_currentBlockId = Util::toInt(getAttrib(attribs, "id", 0));
		m_currentBlockName = getAttrib(attribs, "name", 1);
#ifdef _DEBUG
		__dcdebug("block %d:%s\n", m_currentBlockId, Text::fromUtf8(m_currentBlockName).c_str());
		dcassert(m_operatorIds.insert(m_currentBlockId).second);
#endif
	}
}

void IpBlocksLoader::endTag(const string& name, const string& data) {
	if (name == "block") {
		dcassert(m_currentBlockId != 0);
		dcassert(!m_currentBlockName.empty());
		IPBlock b;
		b.id = m_currentBlockId;
		b.name = m_currentBlockName;
		b.addressList = Util::trim(data);
		m_blocks.push_back(b);
		m_currentBlockId = 0;
		m_currentBlockName = Util::emptyString;
	}
}

tstring IpBlocksLoader::getIpBlocksFilename() {
	tstring appDataFolder = SystemUtils::getAppDataFolder();
	if (!appDataFolder.empty()) {
		return appDataFolder + _T("conf\\ip-blocks.xml");
	}
	return Util::emptyStringT;
}

pair<bool,string> IpBlocksLoader::loadIpBlocksXml() {
	const tstring ipBlocksFilename = getIpBlocksFilename();
	if (!ipBlocksFilename.empty()) {
		try {
			return make_pair(true, File(Text::fromT(ipBlocksFilename), File::READ, File::OPEN).read());
		}
		catch (const FileException& e) {
			dcdebug("Error loading %s: %s\n", Text::fromT(ipBlocksFilename).c_str(), e.getError().c_str());
		}
	}
	return make_pair(false, loadIpBlocksResourceXml());
}

string IpBlocksLoader::loadIpBlocksResourceXml() {
	const HMODULE module = NULL;
	const HRSRC res = FindResource(module, MAKEINTRESOURCE(IDR_IP_BLOCKS), _T("XML"));
	if (res) {
		const HGLOBAL resData = LoadResource(module, res);
		if (resData) {
			const char* resourceMem = (char*) LockResource(resData);
			const size_t resourceLength = SizeofResource(module, res);
			return string(resourceMem, resourceLength);
		}
	}
	return Util::emptyString;
}

pair<int64_t,string> IpBlocksLoader::getIpBlocksInfo() {
	MD5Digest md5;
	string xml = loadIpBlocksXml().second;
	md5.update((const md5_byte_t*) xml.c_str(), xml.length());
	return make_pair(xml.length(), md5.digestAsString());
}

void IpBlocksLoader::load() {
	m_blocks.clear();
	const pair<bool,string> xml = loadIpBlocksXml();
	if (!xml.second.empty()) {
		bool retry = false;
		try {
		  SimpleXMLReader(this).fromXML(xml.second);
		}
		catch (const SimpleXMLException& e) {
			LOG_MESSAGE("Error parsing XML: " + e.getError());
			retry = true;
		}
		if (retry && xml.first) {
#ifdef _DEBUG
			m_operatorIds.clear();
#endif
			m_blocks.clear();
			try {
				SimpleXMLReader(this).fromXML(loadIpBlocksResourceXml());
			}
			catch (const SimpleXMLException& e) {
				LOG_MESSAGE("Error parsing resource XML: " + e.getError());
			}
		}
	}
}

string IpBlocksLoader::findById(const int id) const {
	for (vector<IPBlock>::const_iterator i = m_blocks.begin(); i != m_blocks.end(); ++i) {
		if (i->id == id) {
			return i->addressList;
		}
	}
	return Util::emptyString;
}

string IpBlocksLoader::getNameById(const int id) const {
	for (vector<IPBlock>::const_iterator i = m_blocks.begin(); i != m_blocks.end(); ++i) {
		if (i->id == id) {
			return i->name;
		}
	}
	return Util::emptyString;
}
