#pragma once
#include "MerkleTree.h"
#include "forward.h"

class ChunkLog {
private:
	TTHValue m_tth;
	string m_target;
	UserPtr m_user;
public:
	ChunkLog(const TTHValue& tth, const string& target, const UserPtr& user): m_tth(tth), m_target(target), m_user(user) { }
	~ChunkLog();
	void setUser(const UserPtr& user) { m_user = user; }
	void CDECL logDownload(const char* format, ...);
};
