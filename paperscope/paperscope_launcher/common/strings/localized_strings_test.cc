#include "vr/apps/cardboard/demo/common/strings/localized_strings.h"

#include <string>
#include <vector>

#include "testing/base/public/gunit.h"

using cardboard_demo::GetLocalizedStringFromMap;
using cardboard_demo::GetProbableLocalesInOrder;
using cardboard_demo::LocaleMap;

namespace {
const cardboard_demo::LocaleEntry& GetLocalizedStringsMap(
    const std::string& target_locale,
    cardboard_demo::LocaleMap& locale_map) {
  const std::vector<std::string> locales =
      GetProbableLocalesInOrder(target_locale);
  for (auto& locale : locales) {
    if (locale_map.find(locale) != locale_map.end()) {
      return locale_map.at(locale);
    }
  }
  // Fallback to last locale option.
  return locale_map.at(locales[locales.size() - 1]);
}
}  // namespace

TEST(LocalizedStrings, GetProbableLocalesInOrder) {
  LocaleMap locale_map = {
      {
          "en",
          {
              {"1", "one"},
              {"2", "two"},
          },
      },
      {
          "fr",
          {
              {"1", "une"},
              {"2", "deux"},
          },
      },
  };
  std::vector<std::string> locales = GetProbableLocalesInOrder("fr");
  // Expecting ["fr", "en"].
  EXPECT_EQ(locales.size(), 2);
  EXPECT_EQ("fr", locales[0]);
  EXPECT_EQ("en", locales[1]);
  locales = GetProbableLocalesInOrder("en");
  // Expecting "en" to be first (may be 2 "en" instances).
  EXPECT_GE(locales.size(), 1);
  EXPECT_EQ("en", locales[0]);
}

TEST(LocalizedStrings, GetLocalizedStringFromMap) {
  LocaleMap locale_map = {
    {
      "en",
      {
        {"1", "one"},
        {"2", "two"},
      },
    },
    {
      "fr",
      {
        {"1", "une"},
        {"2", "deux"},
      },
    },
  };
  LocaleMap en_map = {
    {
      "en",
      {
        {"3", "three"},
        {"4", "four"},
      },
    },
  };
  auto locale = GetLocalizedStringsMap("en", locale_map);
  EXPECT_EQ("one", locale["1"]);
  EXPECT_EQ("two", locale["2"]);
  EXPECT_EQ(locale.end(), locale.find("3"));
  EXPECT_STREQ("one", GetLocalizedStringFromMap("1", locale));
  EXPECT_STREQ("two", GetLocalizedStringFromMap("2", locale));
  EXPECT_EQ(nullptr, GetLocalizedStringFromMap("3", locale));

  locale = GetLocalizedStringsMap("en", en_map);
  EXPECT_EQ("three", locale["3"]);
  EXPECT_EQ("four", locale["4"]);
  EXPECT_EQ(locale.end(), locale.find("1"));

  locale = GetLocalizedStringsMap("fr", locale_map);
  EXPECT_EQ("une", locale["1"]);
  EXPECT_EQ("deux", locale["2"]);
  EXPECT_EQ(locale.end(), locale.find("3"));

  locale = GetLocalizedStringsMap("fr_ext", locale_map);
  EXPECT_EQ("une", locale["1"]);
  EXPECT_EQ("deux", locale["2"]);
  EXPECT_EQ(locale.end(), locale.find("3"));

  // Test that we fall back to English.
  locale = GetLocalizedStringsMap("sp", locale_map);
  EXPECT_EQ("one", locale["1"]);
  EXPECT_EQ("two", locale["2"]);
  EXPECT_EQ(locale.end(), locale.find("3"));
}
