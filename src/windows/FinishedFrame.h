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

#if !defined(FINISHED_FRAME_H)
#define FINISHED_FRAME_H

#include "FinishedFrameBase.h"

class FinishedFrame : public FinishedFrameBase<FinishedFrame, ResourceManager::FINISHED_DOWNLOADS, IDC_FINISHED, IDI_PEERS_FINISHED_DOWNLOADS>
{
public:
	FinishedFrame() {
		upload = false;
		boldFinished = SettingsManager::BOLD_FINISHED_DOWNLOADS;
		columnOrder = SettingsManager::FINISHED_ORDER;
		columnWidth = SettingsManager::FINISHED_WIDTHS;
		columnVisible = SettingsManager::FINISHED_VISIBLE;
	}
	~FinishedFrame() { }

	DECLARE_FRAME_WND_CLASS_EX(_T("FinishedFrame"), IDI_PEERS_FINISHED_DOWNLOADS, 0, COLOR_3DFACE);
		
private:
	void on(AddedDl, FinishedItem* entry) throw() {
		PostMessage(WM_SPEAKER, SPEAK_ADD_LINE, (WPARAM)entry);
	}
	void on(RemovedDl, FinishedItem* entry) throw() { 
		totalBytes -= entry->getChunkSize();
		totalTime -= entry->getMilliSeconds();
		PostMessage(WM_SPEAKER, SPEAK_REMOVE);
	}
	void on(RemovedAllDl) throw() { 
		PostMessage(WM_SPEAKER, SPEAK_REMOVE_ALL);
		totalBytes = 0;
		totalTime = 0;
	}
};

#endif // !defined(FINISHED_FRAME_H)

/**
 * @file
 * $Id: FinishedFrame.h,v 1.2 2007/11/29 11:52:04 alexey Exp $
 */
