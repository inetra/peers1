#pragma once
#include "../../windows/resource.h"
#include "../SimpleXML.h"
#ifdef _DEBUG
#include <set>
#endif

class IPBlock {
public:
  int id;
  string name;
  string addressList;
};

class IpBlocksLoader:  private SimpleXMLReader::CallBack {
private:
  string m_currentBlockName;
  int m_currentBlockId;
  vector<IPBlock> m_blocks;
#ifdef _DEBUG
  set<int> m_operatorIds;
#endif

  static pair<bool,string> loadIpBlocksXml();
  static string loadIpBlocksResourceXml();

  void startTag(const string& name, StringPairList& attribs, bool /*simple*/);

  void endTag(const string& name, const string& data);

  void load();
public:
  IpBlocksLoader(): m_currentBlockId(0) {
    load();
  }

  static tstring getIpBlocksFilename();
  static pair<int64_t,string> getIpBlocksInfo();
  
  string findById(const int id) const;

  string getNameById(const int id) const;

  size_t size() const { return m_blocks.size(); }

  const IPBlock& operator [] (int index) const {
    return m_blocks[index];
  }
  
};
