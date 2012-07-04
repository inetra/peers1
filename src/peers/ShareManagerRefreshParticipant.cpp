#include "stdafx.h"
#include "ShareManagerRefreshParticipant.h"
#include "../client/ShareManager.h"

const string ShareManagerRefreshParticipant::NAME = "ShareManagerRefreshParticipant";

void ShareManagerRefreshParticipant::execute() {
	ShareManager::getInstance()->refresh(ShareManager::getInstance()->isDirty());
}
