/* 
 * 
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

#if !defined(AGEMOTIONSETUP_H__)
#define AGEMOTIONSETUP_H__

#include "../client/Util.h"

// CAGEmotion

class CAGEmotion {
public:
	typedef CAGEmotion* Ptr;
	typedef list<Ptr> List;
	typedef List::const_iterator Iter;

	CAGEmotion() : m_EmotionBmp(NULL), m_ImagePos(-1)
	{
	}
	~CAGEmotion() {	}
	
	bool Create(const tstring& strEmotionText, string& strEmotionBmpPath);

	const tstring& getEmotionText() const { return m_EmotionText; }
	HBITMAP getEmotionBmp() const {	return m_EmotionBmp; }
	HBITMAP getEmotionBmp(const COLORREF &clrBkColor);
	const string& getEmotionBmpPath() const { return m_EmotionBmpPath; }
	long  getImagePos() const { return m_ImagePos; }
	void setImagePos(long ImagePos) { m_ImagePos = ImagePos; }
	static void setImageList(CImageList* pImagesList) { m_pImagesList = pImagesList; }

protected:
	tstring		m_EmotionText;
	string		m_EmotionBmpPath;
	HBITMAP		m_EmotionBmp;
	long		m_ImagePos;
	static CImageList*	m_pImagesList;
};

// CAGEmotionSetup

class CAGEmotionSetup {
public:
	CAGEmotionSetup() : useEmoticons(false) { }
	virtual ~CAGEmotionSetup();

	bool LoadEmotion(const string& p_file_name);
	bool InitImages();
	bool Create();

	// Variables
	GETSET(bool, useEmoticons, UseEmoticons);

	const CAGEmotion::List& getEmoticonsList() const { return EmotionsList; }
        /* количество картинок в выбранном пользователем файле */
	int m_CountSelEmotions; //[+]PPA

protected:
	CImageList		 m_images;
	CAGEmotion::List EmotionsList;
private:
	 set<string> m_FilterEmotiion;
         void loadOtherPacks(const string& curentName);
	 void cleanup();
};

#endif // AGEMOTIONSETUP_H__
