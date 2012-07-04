#include "stdafx.h"
#include "../client/Exception.h"
#include "TextRenderer.h"

TextRenderer::Word::Word(const tstring& aText,HFONT aFont, COLORREF aFontColor):
m_newLine(false),
m_text(aText),
m_font(aFont),
m_fontColor(aFontColor)
{
}

TextRenderer::Word::Word(const tstring& aText,HFONT aFont, COLORREF aFontColor, NewLine):
m_newLine(true),
m_text(aText),
m_font(aFont),
m_fontColor(aFontColor) 
{
}

TextRenderer::Paragraph::Paragraph(): m_horizontalGap(4), lastWordKey(0) {
}

TextRenderer::WordKey TextRenderer::Paragraph::addWord(const Word& word) {
  words.push_back(word);
  Word& w = words.back();
  w.m_key = WordKey(++lastWordKey);
  return w.m_key;
}

TextRenderer::WordKey TextRenderer::Paragraph::addWord(const tstring& text, HFONT font, COLORREF fontColor) {
  return addWord(TextRenderer::Word(text, font, fontColor));
}

TextRenderer::WordKey TextRenderer::Paragraph::addWord(const tstring& text, HFONT font, COLORREF fontColor, NewLine newLine) {
  return addWord(TextRenderer::Word(text, font, fontColor, newLine));
}

void TextRenderer::Paragraph::addWords(const tstring& text, HFONT font, COLORREF fontColor) {
  StringTokenizer<tstring> st(text, _T(' '));
  for (WStringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
    addWord(*i, font, fontColor);
  }
}

void TextRenderer::Paragraph::removeWords(int beginIndex, int endIndex) {
  words.erase(words.begin() + beginIndex, words.begin() + endIndex);
}

int TextRenderer::Paragraph::indexOf(const WordKey& wordKey) {
  for (size_t i = 0; i < words.size(); ++i) {
    if (words[i].m_key == wordKey) {
      return i;
    }
  }
  return -1;
}

bool TextRenderer::Paragraph::setText(const WordKey& wordKey, const tstring& text) {
  for (vector<Word>::iterator i = words.begin(); i != words.end(); ++i) {
    Word& word = *i;
    if (word.m_key == wordKey) {
      if (word.m_text != text) {
        word.m_text = text;
        return true;
      }
      else {
        return false;
      }
    }
  }
  throw new Exception("Wrong WordKey");
}

void TextRenderer::Paragraph::clear() {
  words.clear();
}

int TextRenderer::Paragraph::layoutWords(vector<Word*>& currentWords, int y) {
  dcassert(!currentWords.empty());
  int lineHeight = 0;
  for (vector<Word*>::iterator j = currentWords.begin(); j != currentWords.end(); ++j) {
    Word* w = *j;
    if (w->m_textSize.cy > lineHeight) {
      lineHeight = w->m_textSize.cy;
    }
  }
  for (vector<Word*>::iterator j = currentWords.begin(); j != currentWords.end(); ++j) {
    Word* w = *j;
    w->m_location.y = y + ((lineHeight - w->m_textSize.cy) / 2);
  }
  return lineHeight;
}

int TextRenderer::Paragraph::doLayout(CDCHandle dc, int paragraphWidth) {
  dcassert(!words.empty());
  bool isFirstWord = true;
  HFONT lastFont = NULL;
  for (vector<Word>::iterator i = words.begin(); i != words.end(); ++i) {
    Word& w = *i;
    if (isFirstWord) {
      isFirstWord = false;
      dc.SelectFont(w.m_font);
      lastFont = w.m_font;
    }
    else if (lastFont != w.m_font) {
      dc.SelectFont(w.m_font);
      lastFont = w.m_font;
    }
    if (!w.m_text.empty()) {
      dc.GetTextExtent(w.m_text.c_str(), w.m_text.length(), &w.m_textSize);
    }
    else {
      dc.GetTextExtent(_T(" "), 1, &w.m_textSize);
      w.m_textSize.cx = 0;
    }
  }
  int x = 0;
  int y = 0;
  vector<Word*> currentWords;
  for (vector<Word>::iterator i = words.begin(); i != words.end(); ++i) {
    Word* w = &*i;
    if (!currentWords.empty() && (w->m_newLine || x + w->m_textSize.cx > paragraphWidth)) {
      // начинаем новую строку
      x = 0;
      y += layoutWords(currentWords, y);
      currentWords.clear();
    }
    w->m_location.x = x;
    x += w->m_textSize.cx + m_horizontalGap;
    currentWords.push_back(w);
  }
  return y + layoutWords(currentWords, y);
}

void TextRenderer::Paragraph::draw(CDCHandle dc, int x, int y) const {
  bool isFirstWord = true;
  HFONT lastFont = NULL;
  COLORREF lastColor = 0;
  for (vector<Word>::const_iterator i = words.begin(); i != words.end(); ++i) {
    const Word& w = *i;
    if (!w.m_text.empty()) {
      if (isFirstWord) {
        isFirstWord = false;
        dc.SelectFont(w.m_font);
        lastFont = w.m_font;
        dc.SetTextColor(w.m_fontColor);
        lastColor = w.m_fontColor;
      }
      else {
        if (lastFont != w.m_font) {
          dc.SelectFont(w.m_font);
          lastFont = w.m_font;
        }
        if (lastColor != w.m_fontColor) {
          dc.SetTextColor(w.m_fontColor);
          lastColor = w.m_fontColor;
        }
      }
      dc.TextOut(x + w.m_location.x, y + w.m_location.y, w.m_text.c_str(), w.m_text.length());
    }
  }
}

int TextRenderer::Paragraph::getWidth() const {
  int result = 0;
  for (vector<Word>::const_iterator i = words.begin(); i != words.end(); ++i) {
    const Word& w = *i;
    if (!w.m_text.empty()) {
      result = max(result, (int) (w.m_location.x + w.m_textSize.cx));
    }
  }
  return result;
}
