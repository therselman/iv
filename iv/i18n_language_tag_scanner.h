// RFC 5646 : Tags for Identifying Languages
//   http://tools.ietf.org/html/rfc5646
// RFC 5234 : Augmented BNF for Syntax Specifications: ABNF
//   http://tools.ietf.org/html/rfc5234
//
// =========================== Language Tag ABNF ===============================
//
// Language-Tag  = langtag             ; normal language tags
//               / privateuse          ; private use tag
//               / grandfathered       ; grandfathered tags
//
// langtag       = language
//                 ["-" script]
//                 ["-" region]
//                 *("-" variant)
//                 *("-" extension)
//                 ["-" privateuse]
//
// language      = 2*3ALPHA            ; shortest ISO 639 code
//                 ["-" extlang]       ; sometimes followed by
//                                     ; extended language subtags
//               / 4ALPHA              ; or reserved for future use
//               / 5*8ALPHA            ; or registered language subtag
//
// extlang       = 3ALPHA              ; selected ISO 639 codes
//                 *2("-" 3ALPHA)      ; permanently reserved
//
// script        = 4ALPHA              ; ISO 15924 code
//
// region        = 2ALPHA              ; ISO 3166-1 code
//               / 3DIGIT              ; UN M.49 code
//
// variant       = 5*8alphanum         ; registered variants
//               / (DIGIT 3alphanum)
//
// extension     = singleton 1*("-" (2*8alphanum))
//
//                                     ; Single alphanumerics
//                                     ; "x" reserved for private use
// singleton     = DIGIT               ; 0 - 9
//               / %x41-57             ; A - W
//               / %x59-5A             ; Y - Z
//               / %x61-77             ; a - w
//               / %x79-7A             ; y - z
//
// privateuse    = "x" 1*("-" (1*8alphanum))
//
// grandfathered = irregular           ; non-redundant tags registered
//               / regular             ; during the RFC 3066 era
//
// irregular     = "en-GB-oed"         ; irregular tags do not match
//               / "i-ami"             ; the 'langtag' production and
//               / "i-bnn"             ; would not otherwise be
//               / "i-default"         ; considered 'well-formed'
//               / "i-enochian"        ; These tags are all valid,
//               / "i-hak"             ; but most are deprecated
//               / "i-klingon"         ; in favor of more modern
//               / "i-lux"             ; subtags or subtag
//               / "i-mingo"           ; combination
//               / "i-navajo"
//               / "i-pwn"
//               / "i-tao"
//               / "i-tay"
//               / "i-tsu"
//               / "sgn-BE-FR"
//               / "sgn-BE-NL"
//               / "sgn-CH-DE"
//
// regular       = "art-lojban"        ; these tags match the 'langtag'
//               / "cel-gaulish"       ; production, but their subtags
//               / "no-bok"            ; are not extended language
//               / "no-nyn"            ; or variant subtags: their meaning
//               / "zh-guoyu"          ; is defined by their registration
//               / "zh-hakka"          ; and all of these are deprecated
//               / "zh-min"            ; in favor of a more modern
//               / "zh-min-nan"        ; subtag or sequence of subtags
//               / "zh-xiang"
//
// alphanum      = (ALPHA / DIGIT)     ; letters and numbers
//
#ifndef IV_I18N_LANGUAGE_TAG_SCANNER_H_
#define IV_I18N_LANGUAGE_TAG_SCANNER_H_
#include <bitset>
#include <map>
#include <iv/detail/array.h>
#include <iv/character.h>
#include <iv/stringpiece.h>
namespace iv {
namespace core {
namespace i18n {
namespace i18n_detail {

// irregular
typedef std::array<StringPiece, 17> Irregular;
static const Irregular kIrregular = { {
  "en-GB-oed",
  "i-ami",
  "i-bnn",
  "i-default",
  "i-enochian",
  "i-hak",
  "i-klingon",
  "i-lux",
  "i-mingo",
  "i-navajo",
  "i-pwn",
  "i-tao",
  "i-tay",
  "i-tsu",
  "sgn-BE-FR",
  "sgn-BE-NL",
  "sgn-CH-DE"
} };

// regular
typedef std::array<StringPiece, 9> Regular;
static const Regular kRegular = { {
  "art-lojban",
  "cel-gaulish",
  "no-bok",
  "no-nyn",
  "zh-guoyu",
  "zh-hakka",
  "zh-min",
  "zh-min-nan",
  "zh-xiang"
} };

}  // namespace i18n_detail

#define IV_EXPECT_NEXT_TAG()\
  do {\
    if (c_ != '-') {\
      if (IsEOS()) {\
        return true;\
      }\
      return false;\
    }\
    Advance();\
  } while (0)

template<typename Iter>
class LanguageTagScanner {
 public:
  typedef LanguageTagScanner<Iter> this_type;

  LanguageTagScanner(Iter it, Iter last)
    : start_(it),
      last_(last),
      pos_(it),
      c_(-1),
      valid_(),
      language_(),
      extlang_(),
      script_(),
      region_(),
      unique_(0),
      variants_(),
      extensions_(),
      privateuse_() {
    valid_ = Verify();
  }

  bool IsWellFormed() const { return valid_; }

 private:
  void Clear() {
    language_.clear();
    extlang_.clear();
    script_.clear();
    region_.clear();
    variants_.clear();
    extensions_.clear();
    privateuse_.clear();
  }

  bool Verify() {
    Init(start_);
    if (ScanLangtag(start_)) {
      return true;
    }
    Clear();
    if (ScanPrivateUse(start_)) {
      return true;
    }
    Clear();
    return IsGrandfathered(start_, last_);
  }

  bool ScanLangtag(Iter restore) {
    // langtag       = language
    //                 ["-" script]
    //                 ["-" region]
    //                 *("-" variant)
    //                 *("-" extension)
    //                 ["-" privateuse]
    if (!ScanLanguage(restore)) {
      Init(restore);
      return false;
    }

    // script
    Iter restore2 = current();
    IV_EXPECT_NEXT_TAG();
    ScanScript(restore2);

    // region
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    ScanRegion(restore2);

    // variant
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    while (ScanVariant(restore2)) {
      restore2 = current();
      IV_EXPECT_NEXT_TAG();
    }

    // extension
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    while (ScanExtension(restore2)) {
      restore2 = current();
      IV_EXPECT_NEXT_TAG();
    }

    // privateuse
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    ScanPrivateUse(restore2);

    if (!IsEOS()) {
      Init(restore);
      return false;
    }
    return true;
  }

  bool ScanScript(Iter restore) {
    // script        = 4ALPHA              ; ISO 15924 code
    const Iter s = current();
    if (!ExpectAlpha(4) || !MaybeValid()) {
      Init(restore);
      return false;
    }
    script_ = std::string(s, current());
    return true;
  }

  bool ScanRegion(Iter restore) {
    // region        = 2ALPHA              ; ISO 3166-1 code
    //               / 3DIGIT              ; UN M.49 code
    const Iter restore2 = current();
    if (ExpectAlpha(2) && MaybeValid()) {
      region_ = std::string(restore2, current());
      return true;
    }

    Init(restore2);
    for (std::size_t i = 0; i < 3; ++i) {
      if (IsEOS() || !character::IsDecimalDigit(c_)) {
        Init(restore);
        return false;
      }
      Advance();
    }
    if (!MaybeValid()) {
      Init(restore);
      return false;
    }
    region_ = std::string(restore2, current());
    return true;
  }

  bool ScanVariant(Iter restore) {
    // variant       = 5*8alphanum         ; registered variants
    //               / (DIGIT 3alphanum)
    const Iter restore2 = current();
    if (ExpectAlphanum(5)) {
      for (std::size_t i = 0; i < 3; ++i) {
        if (IsEOS() || !character::IsASCIIAlphanumeric(c_)) {
          break;
        }
        Advance();
      }
      if (MaybeValid()) {
        variants_.push_back(std::string(restore2, current()));
        return true;
      }
    }

    Init(restore2);
    if (IsEOS() || !character::IsDigit(c_)) {
      Init(restore);
      return false;
    }
    Advance();
    if (!ExpectAlphanum(3) || !MaybeValid()) {
      Init(restore);
      return false;
    }
    variants_.push_back(std::string(restore2, current()));
    return true;
  }

  // 0 to 61
  static int SingletonID(char ch) {
    assert(character::IsASCIIAlphanumeric(ch));
    // 0 to 9 is assigned to '0' to '9'
    if ('0' <= ch && ch <= '9') {
      return ch - '0';
    }
    if ('A' <= ch && ch <= 'Z') {
      return (ch - 'A') + 10;
    }
    return (ch - 'a') + 10 + 26;
  }

  bool ScanExtension(Iter restore) {
    // extension     = singleton 1*("-" (2*8alphanum))
    //
    //                                     ; Single alphanumerics
    //                                     ; "x" reserved for private use
    // singleton     = DIGIT               ; 0 - 9
    //               / %x41-57             ; A - W
    //               / %x59-5A             ; Y - Z
    //               / %x61-77             ; a - w
    //               / %x79-7A             ; y - z
    if (IsEOS() || !character::IsASCIIAlphanumeric(c_) || c_ == 'x') {
      Init(restore);
      return false;
    }
    const char target = c_;
    const int ID = SingletonID(target);
    if (unique_.test(ID)) {
      Init(restore);
      return false;
    }
    Advance();

    Iter s = pos_;
    if (!ExpectExtensionOrPrivateFollowing(2)) {
      Init(restore);
      return false;
    }

    unique_.set(ID);
    extensions_.insert(std::make_pair(target, std::string(s, current())));
    while (true) {
      Iter restore2 = current();
      s = pos_;
      if (!ExpectExtensionOrPrivateFollowing(2)) {
        Init(restore2);
        return true;
      }
      extensions_.insert(std::make_pair(target, std::string(s, current())));
    }
    return true;
  }

  bool ScanPrivateUse(Iter restore) {
    // privateuse    = "x" 1*("-" (1*8alphanum))
    if (c_ != 'x') {
      Init(restore);
      return false;
    }
    Advance();

    Iter s = pos_;
    if (!ExpectExtensionOrPrivateFollowing(1)) {
      Init(restore);
      return false;
    }

    privateuse_.push_back(std::string(s, current()));
    while (true) {
      Iter restore2 = current();
      s = pos_;
      if (!ExpectExtensionOrPrivateFollowing(1)) {
        Init(restore2);
        return true;
      }
      privateuse_.push_back(std::string(s, current()));
    }
    return true;
  }

  bool ScanLanguage(Iter restore) {
    // language      = 2*3ALPHA            ; shortest ISO 639 code
    //                 ["-" extlang]       ; sometimes followed by
    //                                     ; extended language subtags
    //               / 4ALPHA              ; or reserved for future use
    //               / 5*8ALPHA            ; or registered language subtag
    //
    // We assume, after this, '-' or EOS are following is valid.
    Iter restore2 = current();
    if (ExpectLanguageFirst()) {
      return true;
    }

    Init(restore2);
    if (ExpectAlpha(4) && MaybeValid()) {
      language_ = std::string(restore2, current());
      return true;
    }

    Init(restore2);
    if (!ExpectAlpha(5)) {
      Init(restore);
      return false;
    }
    for (std::size_t i = 0; i < 3; ++i) {
      if (IsEOS() || !character::IsASCIIAlpha(c_)) {
        break;
      }
      Advance();
    }
    if (!MaybeValid()) {
      Init(restore);
      return false;
    }

    language_ = std::string(restore2, current());
    return true;
  }

  // not simple expect
  bool ExpectLanguageFirst() {
    // first case
    // 2*3ALPHA ["-" extlang]
    //
    // extlang       = 3ALPHA              ; selected ISO 639 codes
    //                 *2("-" 3ALPHA)      ; permanently reserved
    Iter s = current();
    if (!ExpectAlpha(2)) {
      return false;
    }
    if (!IsEOS() && character::IsASCIIAlpha(c_)) {
      Advance();
    }

    Iter restore = current();
    language_ = std::string(s, restore);

    // extlang check
    if (c_ != '-') {
      return IsEOS();  // maybe valid
    }
    Advance();

    {
      const Iter s = current();

      // extlang, this is optional
      if (!ExpectAlpha(3) || !MaybeValid()) {
        Init(restore);
        return true;
      }
      assert(MaybeValid());

      restore = current();
      extlang_.push_back(std::string(s, restore));
    }

    for (std::size_t i = 0; i < 2; ++i) {
      if (c_ != '-') {
        assert(IsEOS());
        return true;  // maybe valid
      }
      Advance();
      const Iter s = current();
      if (!ExpectAlpha(3) || !MaybeValid()) {
        Init(restore);
        return true;
      }
      assert(MaybeValid());
      restore = current();
      extlang_.push_back(std::string(s, restore));
    }
    assert(MaybeValid());
    return true;
  }

  // not simple expect
  bool ExpectExtensionOrPrivateFollowing(std::size_t n) {
    // extension's 1*("-" (2*8alphanum))
    // or
    // privateuse's 1*("-" (1*8alphanum))
    if (c_ != '-') {
      return false;
    }
    Advance();
    if (!ExpectAlphanum(n)) {
      return false;
    }
    for (std::size_t i = 0, len = 8 - n; i < len; ++i) {
      if (IsEOS() || !character::IsASCIIAlphanumeric(c_)) {
        break;
      }
      Advance();
    }
    return MaybeValid();
  }

  // simple expect
  bool ExpectAlphanum(std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (IsEOS() || !character::IsASCIIAlphanumeric(c_)) {
        return false;
      }
      Advance();
    }
    return true;
  }

  // simple expect
  bool ExpectAlpha(std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (IsEOS() || !character::IsASCIIAlpha(c_)) {
        return false;
      }
      Advance();
    }
    return true;
  }

  bool IsGrandfathered(Iter pos, Iter last) {
    // grandfathered = irregular           ; non-redundant tags registered
    //               / regular             ; during the RFC 3066 era
    const std::size_t size = std::distance(pos, last);
    for (typename i18n_detail::Irregular::const_iterator
         it = i18n_detail::kIrregular.begin(),
         last = i18n_detail::kIrregular.end();
         it != last; ++it) {
      if (it->size() == size && std::equal(it->begin(), it->end(), pos)) {
        return true;
      }
    }
    for (typename i18n_detail::Regular::const_iterator
         it = i18n_detail::kRegular.begin(),
         last = i18n_detail::kRegular.end();
         it != last; ++it) {
      if (it->size() == size && std::equal(it->begin(), it->end(), pos)) {
        return true;
      }
    }
    return false;
  }

  inline bool IsEOS() const {
    return c_ < 0;
  }

  inline bool MaybeValid() const {
    return IsEOS() || c_ == '-';
  }

  void Init(Iter pos) {
    pos_ = pos;
    Advance();
  }

  inline void Advance() {
    if (pos_ == last_) {
      c_ = -1;
    } else {
      c_ = *pos_++;
    }
  }

  inline Iter current() {
    if (pos_ == last_) {
      return last_;
    }
    return pos_ - 1;
  }

  Iter start_;
  Iter last_;
  Iter pos_;
  int c_;
  bool valid_;
  std::string language_;
  std::vector<std::string> extlang_;
  std::string script_;
  std::string region_;
  std::bitset<64> unique_;
  std::vector<std::string> variants_;
  std::multimap<char, std::string> extensions_;
  std::vector<std::string> privateuse_;
};

#undef IV_EXPECT_NEXT_TAG

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_LANGUAGE_TAG_SCANNER_H_
