#ifndef _BTREE_MAP_H
#define _BTREE_MAP_H

#pragma once
// $Id: BTreeMap.h,v 1.1.2.1 2008/12/24 03:16:40 alexey Exp $
#include <assert.h>
#include <memory.h>
#include <string.h>

template <class KEY, class VALUE> class BTreeMapBaseIterator;
template <class KEY, class VALUE> class BTreeMapIterator;
template <class KEY, class VALUE> class BTreeMapKeyIterator;
template <class KEY, class VALUE> class BTreeMapValueIterator;

template <class KEY, class VALUE, int PageSize = 32> class BTreeMap {
  friend class BTreeMapBaseIterator<KEY,VALUE>;
public:

  class Entry {
  public:
    KEY key;
    VALUE value;
  };

private:

  class IndexPage;
  
  class IndexItem : public Entry {
  public:
    IndexPage* page;
    inline IndexItem():page(NULL) { }
  };

  class IndexPage {
  public:
    int count;
    IndexPage* page0;
    IndexItem items[PageSize];
    inline IndexPage(): count(0), page0(NULL) { }

    bool binSearch(const KEY& key, int &pos) const {
      int l = 0;
      int n = count;
      while (n > 0) {
        int nn = n / 2;
        int k = l + nn;
        if (key == items[k].key) {
          //����� �������
          pos=k;
          return true;
        }
        else if (key > items[k].key) {
          l += nn + 1;
          n -= nn+1;
        }
        else {
          n = nn;
        }
      }
      pos=l;    
      return false;
    }

    IndexPage* getPage(int index) {
      if (index == 0) return page0;
      else return items[index - 1].page;
    }

    void moveItemsUp(int src, int dst, int amount) {
      assert(src < dst);
      assert(amount > 0);
      for (int i = amount; --i >= 0;) {
        items[dst + i] = items[src + i];
      }
      for (int i = dst - src; --i >= 0;) {
        items[src + i].page = NULL;
      }
    }

    void moveItemsDown(int src, int dst, int amount) {
      assert(src > dst);
      assert(amount > 0);
      for (int i = 0; i < amount; ++i) {
        items[dst + i] = items[src + i];
      }
      for (int i = src - dst; --i >= 0;) {
        items[dst + amount + i].page = NULL;
      }
    }

    void moveItems(int src, int dst, int amount) {
      if (amount > 0) {
        if (src < dst) {
          moveItemsUp(src, dst, amount);
        }
        else {
          moveItemsDown(src, dst, amount);
        }
      }
    }
    
  };

  IndexPage* m_root;

  inline IndexPage* createPage() {
    return new IndexPage();
  }

  inline void destroyPage(IndexPage* page) {
    delete page;
  }

  void moveItem(IndexItem& src, IndexItem& dst) {
    dst.key = src.key;
    dst.value = src.value;
    dst.page = src.page;
    src.page = NULL;
  }

  void copyKeys(IndexPage& index,IndexPage& newIndex) {
    assert(index.count == PageSize && newIndex.count == 0);
    for (int i=PageSize / 2; i < PageSize; ++i) {
      moveItem(index.items[i], newIndex.items[i-(PageSize / 2)]);
    }
    newIndex.count = PageSize / 2;
    index.count = PageSize / 2;
  }

  void clearPage(IndexPage* page) {
    if (page != NULL) {
      clearPage(page->page0);
      for (int i=0; i < page->count; ++i) {
        clearPage(page->items[i].page);
      }
      destroyPage(page);
    }
  }

  int countKeys(IndexPage* page) const {
    if (page == NULL) {
      return 0;
    }
    else {
      int result = page->count + countKeys(page->page0);
      for (int i=0; i < page->count; ++i) {
        result += countKeys(page->items[i].page);
      }
      return result;
    }
  }

protected:

  void _append(IndexItem& item, IndexPage& Index, IndexItem& back, bool& h) {
    int l;
    if (Index.binSearch(item.key,l)) {
      Index.items[l].value = item.value;
      h=false;
      return;
    }
    IndexPage *fNext = Index.getPage(l);
    if (fNext==NULL) {
      // ����� ���������
      _insert(item,Index,l,back,h); 
    }
    else {
      IndexItem uback;
      _append(item, *fNext, uback, h);
      // ����� �������� ������� �� back
      if (h) _insert(uback, Index, l, back, h);
    }
  }

  VALUE* appendKey(const KEY& key, IndexPage& Index, IndexItem& back, bool& h) {
    int l;
    if (Index.binSearch(key, l)) {
      h = false;
      return &Index.items[l].value;
    }
    IndexPage *fNext = Index.getPage(l);
    if (fNext==NULL) {
      // ����� ���������
      IndexItem item;
      item.key = key;
      return insertKey(item,Index,l,back,h); 
    }
    else {
      IndexItem uback;
      VALUE* result = appendKey(key, *fNext, uback, h);
      // ����� �������� ������� �� back
      if (h) {
        VALUE* x = insertKey(uback, Index, l, back, h);
        if (result == NULL) {
          result = x;
        }
      }
      return result;
    }
  }

  void _appendRoot(IndexItem& top) {
    IndexPage* prev = m_root;
    m_root = createPage();
    m_root->page0 = prev;
    moveItem(top, m_root->items[0]);
    m_root->count = 1;
  }

  void _insert(IndexItem& item, IndexPage& Index, int l, IndexItem& back, bool& h) {
    if (Index.count < PageSize) { 
      // ������
      Index.moveItems(l, l+1, Index.count - l);
      moveItem(item, Index.items[l]);
      ++Index.count;
      h=false;
    }
    else {
      // �� ������
      h=true;
      IndexPage* newIndex=createPage();
      if (l == PageSize / 2) moveItem(item, back);
      else {
        if (l < PageSize / 2) {
          moveItem(Index.items[PageSize / 2-1],back);
          Index.moveItems(l, l+1, (PageSize / 2) - (l + 1));
        }
        else {
          moveItem(Index.items[PageSize / 2], back);
          --l;
          Index.moveItems(PageSize / 2+1, PageSize / 2, l-PageSize / 2);
        }
        moveItem(item, Index.items[l]);
      }
      copyKeys(Index, *newIndex);
      newIndex->page0 = back.page;
      back.page = newIndex;
    }
  }

  VALUE* insertKey(IndexItem& item, IndexPage& Index, int l, IndexItem& back, bool& h) {
    if (Index.count < PageSize) { 
      // ������
      Index.moveItems(l, l+1, Index.count - l);
      moveItem(item, Index.items[l]);
      ++Index.count;
      h=false;
      return &Index.items[l].value;
    }
    else {
      VALUE* result;
      // �� ������
      h=true;
      IndexPage* newIndex=createPage();
      if (l == PageSize / 2) {
        moveItem(Index.items[PageSize / 2-1],back);
        result = NULL;
      }
      else {
        if (l < PageSize / 2) {
          moveItem(Index.items[PageSize / 2-1],back);
          Index.moveItems(l, l+1, (PageSize / 2) - (l + 1));
          result = &Index.items[l].value;
        }
        else {
          moveItem(Index.items[PageSize / 2], back);
          --l;
          Index.moveItems(PageSize / 2+1, PageSize / 2, l-PageSize / 2);
          result = &newIndex->items[l-PageSize / 2].value;
        }
        moveItem(item, Index.items[l]);
      }
      copyKeys(Index, *newIndex);
      newIndex->page0 = back.page;
      back.page = newIndex;
      return result;
    }
  }

  void remove(const KEY& key, bool& isDeleted, IndexPage* Index, bool& h) {
    assert(Index != NULL);
    int l;
    bool found = Index->binSearch(key, l);
    assert(l>=0 && l<=Index->count);
    IndexPage *fNext = Index->getPage(l);
    if (found) {
      isDeleted=true;
      if (fNext==NULL) {
        //DestroyItem(Index->items[l]);
        --Index->count;
        Index->moveItems(l+1,l,Index->count-l);
        h=Index->count < PageSize / 2;
      }
      else {
        _del(Index,fNext,l,h);
        if (h) _underflow(Index,fNext,l-1,h);
      }
    }
    else {
      if (fNext!=NULL) {
        remove(key,isDeleted,fNext,h);
        if (h) _underflow(Index,fNext,l-1,h);
      }
      else h=false;
    }
  }

  void _del(IndexPage* Index, IndexPage* Next, int k, bool& h) {
    IndexPage* q=Next->items[Next->count-1].page;
    if (q != NULL) {
      _del(Index,q,k,h);
      if (h) _underflow(Next,q,Next->count-1,h);
    }
    else {
      Next->items[Next->count-1].page = Index->items[k].page;
      //DestroyItem(Index->items[k]);
      moveItem(Next->items[Next->count-1], Index->items[k]);
      --Next->count;
      h=Next->count < PageSize / 2;
    }
  }

  void _underflow(IndexPage* c,IndexPage* a, int s, bool& h) {
    //'A' - �������� � ���������, 'C' - ��������-������, 'S' - ������ 'A' �� 'C'
    assert(s >= -1 && s < PageSize && s < c->count);
    assert(a->count == PageSize / 2-1);
    assert((s < 0 && c->page0 == a) || c->items[s].page == a);
    if (s < c->count-1) {
      ++s;
      IndexPage* b=c->items[s].page; // b-�������� ������ �� 'A'
      assert(b != NULL);
      int k=(b->count - a->count) / 2;
      //DestroyItem(a.Items[PageSize / 2-1]);
      moveItem(c->items[s], a->items[PageSize / 2 - 1]);
      a->items[PageSize / 2 - 1].page = b->page0;
      if (k > 0) {
        for (int i=0; i < k-1; ++i) {
          moveItem(b->items[i], a->items[PageSize / 2 + i]);
        }
        b->page0 = b->items[k-1].page;
        moveItem(b->items[k-1], c->items[s]);
        c->items[s].page = b;
        b->moveItems(k, 0, b->count - k);
        b->count -= k;
        a->count += k;
        h=false;
      }
      else {
        assert(b->count == PageSize / 2);
        for (int i=0; i < PageSize / 2; ++i) {
          moveItem(b->items[i], a->items[PageSize / 2 + i]);
        }
        c->moveItems(s+1, s, c->count - s - 1);
        a->count = PageSize;
        --c->count;
        destroyPage(b);
        h = c->count < PageSize / 2;
      }
    }
    else {
      IndexPage* b = c->getPage(s);
      assert(b != NULL);
      int k = (b->count - a->count) / 2;
      if (k > 0) {
        a->moveItems(0, k, a->count);
        moveItem(c->items[s], a->items[k-1]);
        a->items[k-1].page = a->page0;
        for (int i=0; i < k-1; ++i) {
          moveItem(b->items[b->count - k + 1 + i], a->items[i]);
        }
        a->page0 = b->items[b->count - k].page;
        moveItem(b->items[b->count - k], c->items[s]);
        c->items[s].page = a;
        b->count -= k;
        a->count += k;
        h = false;
      }
      else {
        moveItem(c->items[s], b->items[b->count]);
        b->items[b->count].page = a->page0;
        for (int i=0; i < PageSize / 2-1; ++i) {
          moveItem(a->items[i], b->items[b->count + 1 + i]);
        }
        b->count=PageSize;
        c->count -= 1;
        //DestroyItem(c.Items[c.Count]);
        destroyPage(a);
        h = c->count < PageSize / 2;
      }
    }
  }

public:
  BTreeMap(): m_root(NULL) {
  }

  ~BTreeMap() {
    clear();
  }

  bool isEmpty() const {
    return m_root == NULL || m_root->count == 0;
  }

  void clear() {
    clearPage(m_root);
    m_root = NULL;
  }

  int size() const {
    return countKeys(m_root);
  }

  void put(const KEY& key, const VALUE value) {
    IndexItem item;
    item.key = key;
    item.value = value;
    if (m_root == NULL) m_root = createPage();
    IndexItem top;
    bool h;
    _append(item, *m_root, top, h);
    if (h) _appendRoot(top);
  }

  VALUE& operator [] (const KEY& key) {
    if (m_root == NULL) m_root = createPage();
    IndexItem top;
    bool h;
    VALUE* result = appendKey(key, *m_root, top, h);
    if (h) _appendRoot(top);
    return *result;
  }

  bool remove(const KEY& key) {
    if (m_root != NULL) {
      bool h;
      bool result;
      remove(key, result, m_root, h);
      if (h && m_root->count == 0) {
        IndexPage* temp = m_root;
        m_root = m_root->page0;
        destroyPage(temp);
      }
      return result;
    }
    else {
      return false;
    }
  }

  bool containsKey(const KEY& key) const {
    IndexPage* index = m_root;
    while (index != NULL) {
      int l;  
      if (index->binSearch(key, l)) {
        return true;
      }
      index = index->getPage(l);
    }
    return false;
  }

  bool find(const KEY& key, VALUE& value) const {
    IndexPage* index = m_root;
    while (index != NULL) {
      int l;
      if (index->binSearch(key, l)) {
        value = index->items[l].value;
        return true;
      }
      index = index->getPage(l);
    }
    return false;
  }

  BTreeMapIterator<KEY,VALUE> iterator() const {
    return BTreeMapIterator<KEY,VALUE>(this);
  }

  BTreeMapKeyIterator<KEY,VALUE> keyIterator() const {
    return BTreeMapKeyIterator<KEY,VALUE>(this);
  }

  BTreeMapValueIterator<KEY,VALUE> valueIterator() const {
    return BTreeMapValueIterator<KEY,VALUE>(this);
  }

};

template <class KEY, class VALUE> class BTreeMapBaseIterator {
protected:

  struct CursorIndexPage {
    typename BTreeMap<KEY,VALUE>::IndexPage *index;
    int subpagePos;
    int keyPos;

    const typename BTreeMap<KEY,VALUE>::IndexItem& currentItem() const {
      return index->items[keyPos];
    }

	typename BTreeMap<KEY,VALUE>::IndexItem& currentItem() {
      return index->items[keyPos];
    }

  };

private:

  struct CursorIndexPages {
    size_t m_size;
    CursorIndexPage pages[10];
    
    CursorIndexPages(): m_size(0) { }    
    
    size_t size() const { return m_size; }
    void setLength(size_t newSize) { m_size = newSize; }

    CursorIndexPage& operator [] (size_t index) { return pages[index]; }
    const CursorIndexPage& operator [] (size_t index) const { return pages[index]; }

  };

  const BTreeMap<KEY,VALUE>* m_tree;
  CursorIndexPages m_pages;
  
  bool m_hasElement;

  bool gotoFirst(CursorIndexPages& pages) {
    if (m_tree->m_root != NULL) {
      pages.setLength(1);
      pages[0].index = m_tree->m_root;
      pages[0].subpagePos = -1;
      pages[0].keyPos = 0;
      size_t level = 0;
      for (;;) {
        typename BTreeMap<KEY,VALUE>::IndexPage* temp = pages[level].index->page0;
        if (temp == NULL) {
          break;
        }
        pages[level].subpagePos = 0;
        ++level;
        pages.setLength(level + 1);
        pages[level].index = temp;
        pages[level].subpagePos = -1;
        pages[level].keyPos = 0;
      }
      return pages[0].index->count > 0;
    }
    else {
      pages.setLength(0);
      return false;
    }
  }

  bool gotoNext(CursorIndexPages& pages) {
    ++pages[pages.size() - 1].keyPos;
    while (pages.size() > 0) {
      CursorIndexPage* l = &pages[pages.size() - 1];
      if (l->subpagePos < l->keyPos) {
        typename BTreeMap<KEY,VALUE>::IndexPage* temp;
        if (l->subpagePos < 0) {
          temp = l->index->page0;
        }
        else {
          temp = l->index->items[l->subpagePos].page;
        }
        ++l->subpagePos;
        if (temp != NULL) {
          pages.setLength(pages.size() + 1);
          l = &pages[pages.size() - 1];
          l->subpagePos = -1;
          l->keyPos = 0;
          l->index = temp;
          continue;
        }
      }
      if (l->keyPos < l->index->count) break;
      pages.setLength(pages.size() - 1);
    }
    return pages.size() > 0;
    //if not Result then _gotoLast(aPages);
  }

protected:
  
  BTreeMapBaseIterator(const BTreeMap<KEY,VALUE>* tree): m_tree(tree) {
    m_hasElement = gotoFirst(m_pages);
  }

  const CursorIndexPage& currentPage() const {
    return m_pages[m_pages.size() - 1];
  }

  CursorIndexPage& currentPage() {
    return m_pages[m_pages.size() - 1];
  }

public:

  operator bool () const {
    return m_hasElement;
  }

  bool operator ++() {
    m_hasElement = gotoNext(m_pages);
    return m_hasElement;
  }

};

template <class KEY, class VALUE> class BTreeMapIterator : public BTreeMapBaseIterator<KEY,VALUE> {
  friend class BTreeMap<KEY,VALUE>;
protected:

  BTreeMapIterator(const BTreeMap<KEY,VALUE>* tree): BTreeMapBaseIterator<KEY,VALUE>(tree) {
  }

public:

  const typename BTreeMap<KEY,VALUE>::Entry* operator -> () const {
    return &BTreeMapBaseIterator<KEY,VALUE>::currentPage().currentItem();
  }

  VALUE& getValue() {
	  return BTreeMapBaseIterator<KEY,VALUE>::currentPage().currentItem().value;
  }

};

template <class KEY, class VALUE> class BTreeMapKeyIterator : public BTreeMapBaseIterator<KEY,VALUE> {
  friend class BTreeMap<KEY,VALUE>;
protected:

  BTreeMapKeyIterator(const BTreeMap<KEY,VALUE>* tree): BTreeMapBaseIterator<KEY,VALUE>(tree) {
  }

public:

  const KEY* operator -> () const {
    return &BTreeMapBaseIterator<KEY,VALUE>::currentPage().currentItem().key;
  }

  const KEY& operator * () const {
    return BTreeMapBaseIterator<KEY,VALUE>::currentPage().currentItem().key;
  }

};

template <class KEY, class VALUE> class BTreeMapValueIterator : public BTreeMapBaseIterator<KEY,VALUE> {
  friend class BTreeMap<KEY,VALUE>;
protected:

  BTreeMapValueIterator(const BTreeMap<KEY,VALUE>* tree): BTreeMapBaseIterator<KEY,VALUE>(tree) {
  }

public:

  const VALUE* operator -> () const {
    return &BTreeMapBaseIterator<KEY,VALUE>::currentPage().currentItem().value;
  }

  const VALUE& operator * () const {
    return BTreeMapBaseIterator<KEY,VALUE>::currentPage().currentItem().value;
  }

};

#endif /* _BTREE_MAP_H */
