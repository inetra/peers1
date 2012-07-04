#pragma once

class TextRenderer {
public:

  class WordKey {
  private:
    int m_value;
  public:
    WordKey(int value): m_value(value) { }
    WordKey(): m_value(0) { }
    bool operator == (const WordKey& other) const { return m_value == other.m_value; }
    operator bool () const { return m_value != 0; }
	void clear() { m_value = 0; }
  };

  class Paragraph;

  class NewLine {};

  class Word {
  private:
    friend class TextRenderer::Paragraph;
    bool m_newLine;
    tstring m_text;
    HFONT m_font;
    COLORREF m_fontColor;
    SIZE m_textSize;
    POINT m_location;
    WordKey m_key;
  public:
    Word(const tstring& aText,HFONT aFont, COLORREF aFontColor);
    Word(const tstring& aText,HFONT aFont, COLORREF aFontColor, NewLine);
  };

  class Paragraph {
  private:
    int m_horizontalGap;
    int lastWordKey;
    vector<Word> words;
    /* размещает слова в одну строку, возвращает высоту полученной строки */
    int layoutWords(vector<Word*>& currentWords, int y);
  public:
    Paragraph();
    void clear();
    WordKey addWord(const Word& word);
    WordKey addWord(const tstring& text, HFONT font, COLORREF fontColor);
    WordKey addWord(const tstring& text, HFONT font, COLORREF fontColor, NewLine);
    void addWords(const tstring& text, HFONT font, COLORREF fontColor);
    /* удаляет элементы. endIndex - индекс за последним элементом */
    void removeWords(int beginIndex, int endIndex);
    /* возвращает результат - были ли изменения */
    bool setText(const WordKey& wordKey, const tstring& text);
    /* возвращает индекс или -1 */
    int indexOf(const WordKey& wordKey);
    /* размещает все слова, возможно в несколько строк, возвращает высоту всего абзаца */
    int doLayout(CDCHandle dc, int paragraphWidth);
    /* рисует параграф в указанных координатах */
    void draw(CDCHandle dc, int x, int y) const;
    /* возвращает количество слов */
    size_t size() const { return words.size(); }
    /* возвращает ширину текста */
    int getWidth() const;
    /* устанавливает интервал между словами */
    void setHorizontalGap(int value) { m_horizontalGap = value; }
  };

  class TextBlock : public Paragraph {
  private:
    int m_height;
  public:
    int doLayout(HWND hwnd, int paragraphWidth) {
      CClientDC dc(hwnd);
      return doLayout((HDC) dc, paragraphWidth);
    }

    int doLayout(CDCHandle dc, int paragraphWidth) {
      m_height = Paragraph::doLayout(dc, paragraphWidth);
      return m_height;
    }

    int getHeight() const { return m_height; }
  };

};
