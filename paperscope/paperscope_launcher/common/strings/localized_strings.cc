#include "../common/strings/localized_strings.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace cardboard_demo {

std::vector<std::string> GetProbableLocalesInOrder(std::string locale) {
  std::vector<std::string> out;
  // Convert BCP 47 codes (dash separators) to ISO 15897 codes (underscores),
  // to match the canonical format used by Translation Console.
  std::replace(locale.begin(), locale.end(), '-', '_');

  // First, try using the ISO 15897 locale (e.g. "pt_BR").
  out.push_back(locale);

#if defined(ION_PLATFORM_IOS) || defined(ION_PLATFORM_ANDROID)
  // Replace common iOS script IDs with canonical ISO 15897 codes.
  if (locale == "zh_Hans") {
    locale = "zh_CN";
  } else if (locale == "zh_Hant") {
    locale = "zh_TW";
  }
  out.push_back(locale);
#endif  // defined(ION_PLATFORM_IOS) || defined(ION_PLATFORM_ANDROID)

  // Then look up the locale's ISO 639 alpha-2 code (2 letter code, e.g. "pt").
  if (locale.size() > 2) {
    locale.resize(2);
    out.push_back(locale);
  }

  // Otherwise, fallback to ISO 639-1 English ("en") which must be present in
  // any vr::LocaleMap.
  out.push_back("en");
  return out;
}

const char* GetLocalizedStringFromMap(std::string_view key,
                                      const LocaleEntry& entry) {
  auto iter = entry.find(key.data());
  return (iter == entry.end()) ? nullptr : iter->second.c_str();
}
}  // namespace cardboard_demo
