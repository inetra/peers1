#include "locale_test.h"

#if !defined (STLPORT) || !defined (_STLP_USE_NO_IOSTREAMS)
#  include <locale>
#  include <sstream>
#  include <memory>
#  include <stdexcept>

#  if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  endif

static const char* tested_locales[] = {
// name,
   "fr_FR",
   "ru_RU.koi8r",
   "en_GB",
   "en_US",
   "C"
};

void LocaleTest::_time_put_get( const locale& loc )
{
  typedef time_put<char, ostreambuf_iterator<char, char_traits<char> > > time_put_facet;
  CPPUNIT_ASSERT( has_facet<time_put_facet>(loc) );
  const time_put_facet& tmp = use_facet<time_put_facet>(loc);

  struct tm xmas = { 0, 0, 12, 25, 11, 93 };
  ostringstream ostr;
  ostr.imbue(loc);
  string format = "%B %d %Y";

  time_put_facet::iter_type ret = tmp.put(ostr, ostr, ' ', &xmas, format.data(), format.data() + format.size());
  CPPUNIT_ASSERT( !ret.failed() );

  /*
   * In other words, user conformation is required for reliable parsing
   * of user-entered dates and times, but machine-generated formats can be
   * parsed reliably. This allows parsers to be aggressive about interpreting
   * user variations on standard format.
   *
   *                                             ISO/IEC 14882, 22.2.5.1
   */
  typedef time_get<char, istreambuf_iterator<char, char_traits<char> > > time_get_facet;
  CPPUNIT_ASSERT( has_facet<time_get_facet>(loc) );
  const time_get_facet& tmg = use_facet<time_get_facet>(loc);
  basic_ios<char> io(0);
  io.imbue(loc);

  istringstream istr( ostr.str() );
  istreambuf_iterator<char, char_traits<char> > i( istr );
  istreambuf_iterator<char, char_traits<char> > e;
  ios_base::iostate err = ios_base::goodbit;
  struct tm other = { 15, 20, 9, 14, 7, 105 };

  i = tmg.get_monthname( i, e, io, err, &other );
  CPPUNIT_ASSERT( err == ios_base::goodbit );
  CPPUNIT_ASSERT( other.tm_mon == xmas.tm_mon );

  ++i; ++i; ++i; ++i; // skip day of month and spaces around it
  i = tmg.get_year( i, e, io, err, &other );

  CPPUNIT_ASSERT( err == ios_base::eofbit );
  CPPUNIT_ASSERT( other.tm_year == xmas.tm_year );

  ostringstream ostrX;
  ostrX.imbue(loc);
  format = "%x %X";

  ret = tmp.put(ostrX, ostrX, ' ', &xmas, format.data(), format.data() + format.size());
  CPPUNIT_ASSERT( !ret.failed() );

  istringstream istrX( ostrX.str() );
  istreambuf_iterator<char, char_traits<char> > j( istrX );

  err = ios_base::goodbit;

  struct tm yet_more = { 15, 20, 9, 14, 7, 105 };

  j = tmg.get_date( j, e, io, err, &yet_more );

  CPPUNIT_ASSERT( err == ios_base::goodbit );

  CPPUNIT_ASSERT( yet_more.tm_sec != xmas.tm_sec );
  CPPUNIT_ASSERT( yet_more.tm_min != xmas.tm_min );
  CPPUNIT_ASSERT( yet_more.tm_hour != xmas.tm_hour );
  CPPUNIT_ASSERT( yet_more.tm_mday == xmas.tm_mday );
  CPPUNIT_ASSERT( yet_more.tm_mon == xmas.tm_mon );
  CPPUNIT_ASSERT( yet_more.tm_year == xmas.tm_year );

  ++j; // skip space

  j = tmg.get_time( j, e, io, err, &yet_more );

  CPPUNIT_ASSERT( err == ios_base::eofbit || err == ios_base::goodbit );

  CPPUNIT_ASSERT( yet_more.tm_sec == xmas.tm_sec );
  CPPUNIT_ASSERT( yet_more.tm_min == xmas.tm_min );
  CPPUNIT_ASSERT( yet_more.tm_hour == xmas.tm_hour );
  CPPUNIT_ASSERT( yet_more.tm_mday == xmas.tm_mday );
  CPPUNIT_ASSERT( yet_more.tm_mon == xmas.tm_mon );
  CPPUNIT_ASSERT( yet_more.tm_year == xmas.tm_year );
}

template <class _Tp>
void test_supported_locale(LocaleTest inst, _Tp __test) {
  size_t n = sizeof(tested_locales) / sizeof(tested_locales[0]);
  for (size_t i = 0; i < n; ++i) {
    auto_ptr<locale> loc;
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
    try {
      loc.reset(new locale(tested_locales[i]));
    }
    catch (runtime_error const&) {
      //This locale is not supported.
      continue;
    }
#  else
    //Without exception support we only test C locale.
    if (tested_locales[i][0] != 'C' ||
        tested_locales[i][1] != 0)
      continue;
    loc.reset(new locale(tested_locales[i]));
#  endif
    CPPUNIT_MESSAGE( loc->name().c_str() );
    (inst.*__test)(*loc);
  }
}

void LocaleTest::time_put_get()
{ test_supported_locale(*this, &LocaleTest::_time_put_get); }

void LocaleTest::time_by_name()
{
#  if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  /*
   * Check of the 22.1.1.2.7 standard point. Construction of a locale
   * instance from a null pointer or an unknown name should result in
   * a runtime_error exception.
   */
  try {
    locale loc(locale::classic(), new time_put_byname<char, ostreambuf_iterator<char, char_traits<char> > >(static_cast<char const*>(0)));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
    }

  try {
    locale loc(locale::classic(), new time_put_byname<char, ostreambuf_iterator<char, char_traits<char> > >("yasli_language"));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

  try {
    locale loc(locale::classic(), new time_get_byname<char, istreambuf_iterator<char, char_traits<char> > >(static_cast<char const*>(0)));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

  try {
    locale loc(locale::classic(), new time_get_byname<char, istreambuf_iterator<char, char_traits<char> > >("yasli_language"));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

#    if !defined (STLPORT) || !defined (_STLP_NO_WCHAR_T)
  try {
    locale loc(locale::classic(), new time_put_byname<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(static_cast<char const*>(0)));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
    }

  try {
    locale loc(locale::classic(), new time_put_byname<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >("yasli_language"));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

  try {
    locale loc(locale::classic(), new time_get_byname<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(static_cast<char const*>(0)));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

  try {
    locale loc(locale::classic(), new time_get_byname<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >("yasli_language"));
    CPPUNIT_ASSERT( false );
  }
  catch (runtime_error const&) {
  }
  catch (...) {
    CPPUNIT_ASSERT( false );
  }

#    endif
#  endif
}

#endif
