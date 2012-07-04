/* 
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
#include "stdafx.h"
#include "../client/Pointer.h"
#include "AGEmotionSetup.h"

CImageList* CAGEmotion::m_pImagesList = NULL;

bool CAGEmotion::Create(const tstring& strEmotionText, string& strEmotionBmpPath) {
  m_EmotionBmp = (HBITMAP) ::LoadImage(0, Text::toT(strEmotionBmpPath).c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
  if (m_EmotionBmp == NULL) {
    dcassert(FALSE);
    return false;
  }
  m_EmotionText = strEmotionText;
  m_EmotionBmpPath = strEmotionBmpPath;
  return true;
}

HBITMAP CAGEmotion::getEmotionBmp(const COLORREF &clrBkColor) {
  if ((m_pImagesList == NULL) || (m_ImagePos < 0)) {
    return NULL;
  }
  IMAGEINFO ii;
  m_pImagesList->GetImageInfo(getImagePos(), &ii);
  int nWidth = ii.rcImage.right - ii.rcImage.left;
  int nHeight = ii.rcImage.bottom - ii.rcImage.top;
  CClientDC dc(NULL);
  CBitmap dist;
  dist.CreateCompatibleBitmap(dc, nWidth, nHeight);
  CDC memDC;
  memDC.CreateCompatibleDC(dc);
  HBITMAP oldBitmap = memDC.SelectBitmap(dist);
  memDC.FillSolidRect(0, 0, nWidth, nHeight, clrBkColor);
  m_pImagesList->Draw(memDC, getImagePos(), CPoint(0, 0), ILD_NORMAL);
  memDC.SelectBitmap(oldBitmap);
  return dist.Detach();
}

CAGEmotionSetup::~CAGEmotionSetup() {
  cleanup();
}

void CAGEmotionSetup::cleanup() {
  for_each(EmotionsList.begin(), EmotionsList.end(), DeleteFunction());
  EmotionsList.clear();
  m_images.Destroy();
  m_CountSelEmotions = 0;
  m_FilterEmotiion.clear();
}

bool CAGEmotionSetup::LoadEmotion(const string& p_file_name) {
  const string l_fileName = Util::getDataPath() + "EmoPacks\\" + p_file_name + ".xml";
  if ((p_file_name == "Disabled") || !Util::fileExists(l_fileName)) {
    return true;
  }
  try {
    SimpleXML xml;
    xml.fromXML(File(l_fileName, File::READ, File::OPEN).read());
    if (xml.findChild("Emoticons")) {
      xml.stepIn();
      while (xml.findChild("Emoticon")) {
        string strEmotionText = xml.getChildAttrib("PasteText");
        if (strEmotionText.empty()) {
          strEmotionText = xml.getChildAttrib("Expression");
        }
        while (strEmotionText[0] == ' ') // ltrim
          strEmotionText.erase(0, 1);
        if (!strEmotionText.empty()) { 
          // dcdebug("CAGEmotionSetup::Create: emotion:[%s]\n", Text::fromT(strEmotionText).c_str());
          if (m_FilterEmotiion.count(strEmotionText)) {
            //	dcdebug("CAGEmotionSetup::Create: dup emotion:[%s]\n", strEmotionText.c_str());
            continue;
          }
          m_FilterEmotiion.insert(strEmotionText); 
          string strEmotionBmpPath = xml.getChildAttrib("Bitmap");
          if (strEmotionBmpPath.size() > 0) {
            if (strEmotionBmpPath[0] == '.') {
              // Relativni cesta - dame od aplikace
              strEmotionBmpPath = Util::getDataPath() + "EmoPacks\\" + strEmotionBmpPath;
            }
            else strEmotionBmpPath = "EmoPacks\\" + strEmotionBmpPath;
          }
          CAGEmotion* pEmotion = new CAGEmotion();
          //[!]PPA for lock bmp
          //File* l__f = new File (Util::getDataPath() + strEmotionBmpPath, File::READ, File::OPEN);
          if (!pEmotion->Create(Text::toT(strEmotionText), strEmotionBmpPath)) {
            delete pEmotion;
            continue;
          }
          m_CountSelEmotions++;
          EmotionsList.push_back(pEmotion);
        } 
      }
      xml.stepOut();
    }
  } 
  catch(const Exception& e) {
    dcdebug("CAGEmotionSetup::Create: %s\n", e.getError().c_str());
    return false;
  }
  return true;
}

bool CAGEmotionSetup::InitImages() {
  CAGEmotion::setImageList(0);
  if (!m_images.Create(18, 18, ILC_COLORDDB|ILC_MASK, 0, 1)) {
    dcassert(FALSE);
    return false;
  }
  CAGEmotion::setImageList(&m_images);
  CDC oTestDC;
  if (!oTestDC.CreateCompatibleDC(NULL)) {
    return false;
  }
  for (CAGEmotion::Iter pEmotion = EmotionsList.begin(); pEmotion != EmotionsList.end(); ++pEmotion) {
    CAGEmotion* emotion = *pEmotion;
    HBITMAP hBmp = emotion->getEmotionBmp();
    const HBITMAP prevBmp = oTestDC.SelectBitmap(hBmp);
    COLORREF clrTransparent = GetPixel(oTestDC,0,0);
    oTestDC.SelectBitmap(prevBmp);
    int nImagePos = m_images.Add(hBmp, clrTransparent);
    emotion->setImagePos(nImagePos);
    DeleteObject(hBmp);
  }
  return true;
}

bool CAGEmotionSetup::Create() {
  setUseEmoticons(false);
  cleanup();
  const string l_CurentName = SETTING(EMOTICONS_FILE);
  if (l_CurentName == "Disabled") {
    return true; 
  }
  LoadEmotion(l_CurentName); //[+]PPA
#ifndef _DEBUG
  loadOtherPacks(l_CurentName);
#endif
  InitImages();
  setUseEmoticons(true);
  return true;
}

void CAGEmotionSetup::loadOtherPacks(const string& curentName) {
  const int selectedFileCountSelEmotions = m_CountSelEmotions;
  WIN32_FIND_DATA data;
  HANDLE hFind;
  hFind = FindFirstFile(Text::toT(Util::getDataPath()+"EmoPacks\\*.xml").c_str(), &data);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      string l_name = Text::fromT(data.cFileName);
      string::size_type i = l_name.rfind('.');
      l_name = l_name.substr(0, i);
      if (curentName != l_name) {
        LoadEmotion(l_name); 
      }
    } while (FindNextFile(hFind, &data));
    FindClose(hFind);
  }
  m_CountSelEmotions = selectedFileCountSelEmotions;
}
