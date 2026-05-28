#!/usr/bin/python
"""A script which can be used to generate code to easily load strings.

Run this script inside a custom build step with:
 genstrings.py
     --resource_file=resources/strings/en.lproj/MyStrings.strings
     --namespace=mystrings
     --out_dir=resources/strings/
TODO(bwuest): Add unit test. b/24000998
"""

import optparse
import os
import re
import sys
from typing import Dict, List


class Error(Exception):
  """The base call of all exceptions in this module."""
  pass


class StringsParser(object):
  """A class which can be used to parse xcode .strings files."""

  def __init__(self, resource_file: str, namespace: str, out_dir: str,
               check_substitution_tokens: bool, language: str) -> None:
    """Initializes parser with the resource file to use.

    Args:
      resource_file: The name of the Localizeable.strings file.
      namespace: The namespace used for the generated classes.
      out_dir: The directory where the generated code should go.
      check_substitution_tokens: Whether or not to validate that the
          substitution tokens are valid.
      language: Target language for code generation.
    """
    self._resource_file_name = resource_file
    self._namespace = namespace
    self._out_dir = out_dir
    self._parsed_strings = {}
    self._check_substitution_tokens = check_substitution_tokens
    self._language = language

  def Run(self):
    """Parses the resource file, and generates the output files."""
    self._ParseFile()
    if self._language == 'cc':
      self._GenerateCc()
    elif self._language == 'java':
      self._GenerateJava()
    else:
      raise Error('Invalid language specified')

  def _GenerateEnumConstant(self, string_key):
    """Generates a string constant from a string key.

    Args:
        string_key: The key for the string.  This needs to be converted into
           a camelcase string, prefixed by kStr.

    Returns:
        The enum constant for the given string key.
    """
    return 'kStr' + ''.join([part[0].upper() + part[1:]
                             for part in string_key.split('_')])

  def _GenerateCc(self) -> None:
    """Generates the contents of the C++ output files, from the parsed input."""

    locale_map = self._GetFullLocaleMap()

    locale_map_initialization_functions = []
    locale_map_initialization = []
    # Inserting locales one at a time to the map, to avoid having more than one
    # on the stack at a given time, which would violate stack frame size checks.
    for locale_id, string_map in locale_map.items():
      string_map = [
          '    this_language_map["%s"] = "%s";' % i
          for i in string_map.items()
      ]

      locale_map_initialization_functions.append("""\
  void Add%(locale_id)sToMap(cardboard_demo::LocaleMap& locale_map) {
    auto& this_language_map = locale_map["%(locale_id)s"];
%(string_map)s
  }
  """ % dict(locale_id=locale_id, string_map='\n'.join(string_map)))

      locale_map_initialization.append("""\
  if (locale == "%(locale_id)s") {
    Add%(locale_id)sToMap(locale_map);
  }""" % dict(locale_id=locale_id))

    if not os.path.exists(self._out_dir):
      os.makedirs(self._out_dir)

    locale_helpers = """\
std::string GetCurrentLocale(JNIEnv* env) {
  if (!env) {
      __android_log_print(ANDROID_LOG_ERROR,
                          "cardboard_demo",
                          "GetCurrentLocale: JNIEnv is null");
    return "en";
  }

  const char* method = "toLanguageTag";
  jclass locale_class = env->FindClass("java/util/Locale");
  jmethodID get_default_method = env->GetStaticMethodID(
      locale_class, "getDefault", "()Ljava/util/Locale;");
  jobject default_locale =
      env->CallStaticObjectMethod(locale_class, get_default_method);
  jmethodID get_method =
      env->GetMethodID(locale_class, method, "()Ljava/lang/String;");
  if (get_method == nullptr) {
    __android_log_print(ANDROID_LOG_ERROR,
                        "cardboard_demo",
                        "GetCurrentLocale: GetMethodID is null.");
    return "en";
  }

  jobject get_str = env->CallObjectMethod(default_locale, get_method);
  jstring get_str_jstring = static_cast<jstring>(get_str);

  const char* data = env->GetStringUTFChars(get_str_jstring, nullptr);
  size_t size = env->GetStringUTFLength(get_str_jstring);
  std::string cpp_string(data, size);
  env->ReleaseStringUTFChars(get_str_jstring, data);

  return cpp_string;
}
"""
    locale_decls = """\
const char* GetLocalizedString(StringId string_id, JNIEnv* env);
const char* GetLocalizedString(const std::string& key, JNIEnv* env);\
"""
    locale_defs = """\
const char* GetLocalizedString(StringId string_id, JNIEnv* env) {
  return GetLocalizedStringForLocale(kLocalizedStringTable[string_id],
                                     GetCurrentLocale(env), env);
}

const char* GetLocalizedString(const std::string& key, JNIEnv* env) {
  return GetLocalizedStringForLocale(key, GetCurrentLocale(env), env);
}
"""

    cc_header = open(
        os.path.join(self._out_dir, ('%s_strings.h' % self._namespace)), 'w')
    cc_header.write("""\
// GENERATED CODE : DO NOT EDIT BY HAND
#ifndef VR_APPS_CARDBOARD_DEMO_COMMON_TOOLS_%(namespace_upper)s_STRINGS_H_
#define VR_APPS_CARDBOARD_DEMO_COMMON_TOOLS_%(namespace_upper)s_STRINGS_H_

#include <string>
#include <jni.h>

namespace %(namespace)s {

typedef enum {
%(enums)s
} StringId;

%(locale_decls)s
const char* GetLocalizedStringForLocale(StringId string_id,
                                        const std::string& locale,
                                        JNIEnv* env);
const char* GetLocalizedStringForLocale(const std::string& key,
                                        const std::string& locale,
                                        JNIEnv* env);

}  // namespace %(namespace)s

#endif  // VR_APPS_CARDBOARD_DEMO_COMMON_TOOLS_%(namespace_upper)s_STRINGS_H_
""" % dict(
    enums='\n'.join(['  %s,' % self._GenerateEnumConstant(k)
                     for k in self._parsed_strings.keys()]),
    namespace=self._namespace,
    namespace_upper=self._namespace.upper(),
    locale_decls=locale_decls))

    cc_header.close()

    # TODO(bwuest): Use a template library like cheetah or jinja2 to make this
    # generated code easier to read.
    cc_source = open(
        os.path.join(self._out_dir, ('%s_strings.cc' % self._namespace)), 'w')
    cc_source.write(
        """\
// GENERATED CODE : DO NOT EDIT BY HAND
#include "%(namespace)s_strings.h"

#include "../common/strings/localized_strings.h"

#include <android/log.h>
#include <vector>

namespace {

const char* const kLocalizedStringTable[] = {
%(parsed_strings)s
};


%(locale_map_initialization_functions)s;

void AddLocaleToMap(const std::string& locale, cardboard_demo::LocaleMap& locale_map) {
%(locale_map_initialization)s;
}

// Retrieves the LocaleMap, lazily loading in the provided locale if necessary.
const cardboard_demo::LocaleMap& GetLocaleMapWithLocale(const std::string& locale) {
  static cardboard_demo::LocaleMap s_locale_map;
  // Lazily add the locale to the map if it's not already there.
  if (s_locale_map.find(locale) == s_locale_map.end()) {
    AddLocaleToMap(locale, s_locale_map);
  }

  return s_locale_map;
}

%(locale_helpers)s\

}  // namespace

namespace %(namespace)s {

const char* GetLocalizedStringForLocale(StringId string_id,
                                        const std::string& locale,
                                        JNIEnv* env) {
  return GetLocalizedStringForLocale(kLocalizedStringTable[string_id], locale, env);
}

const char* GetLocalizedStringForLocale(const std::string& key,
                                        const std::string& target_locale,
                                        JNIEnv* env) {
  if(!env)
  {
    __android_log_print(ANDROID_LOG_ERROR,
                        "cardboard_demo",
                        "GetLocalizedStringForLocale: JNIEnv is null");
    return nullptr;
  }

  const std::vector<std::string> locales =
    cardboard_demo::GetProbableLocalesInOrder(target_locale);
  for (auto& locale : locales) {
    const cardboard_demo::LocaleMap& locale_map = GetLocaleMapWithLocale(locale);
    auto locale_entry_iter = locale_map.find(locale);
    if (locale_entry_iter != locale_map.end()) {
      const char* out =
        cardboard_demo::GetLocalizedStringFromMap(key, locale_entry_iter->second);
      if (!out) {
        continue;
      }
      return out;
    }
  }
  return nullptr;
}

%(locale_defs)s\

}  // namespace %(namespace)s
""" % dict(namespace=self._namespace,
           num_parsed_strings=len(self._parsed_strings),
           locale_defs=locale_defs,
           locale_helpers=locale_helpers,
           locale_map_initialization_functions='\n'.join(
               locale_map_initialization_functions),
           locale_map_initialization='\n'.join(locale_map_initialization),
           parsed_strings='\n'.join([
               '    "%s",  // %s' % (k, v)
               for k, v in self._parsed_strings.items()
           ])))

    cc_source.close()

  def _GenerateJava(self) -> None:
    """Generates the Java output files, from the parsed input."""

    # TODO(b/29054912): Refactor this method to be consistent with GenerateCc.

    if not os.path.exists(self._out_dir):
      os.makedirs(self._out_dir)
    strings_class = open(
        os.path.join(self._out_dir, ('%s.java' % 'Strings')), 'w')
    strings_class.write('package %s;\n\n' % self._namespace)

    strings_class.write('import java.util.HashMap;\n')
    strings_class.write('import java.util.Locale;\n')
    strings_class.write('import java.util.Map;\n')
    strings_class.write('\n')

    strings_class.write('/** @hide */\n')
    strings_class.write('public class %s {\n' % 'Strings')
    # Generate constants for each string used as the 'stringId' which is passed
    # to getStrings(String).
    for key, value in self._parsed_strings.items():
      strings_class.write('public static final String %s = "%s";\n' %
                          (key, key))

    strings_class.write('private static final '
                        'Map<String, Map<String, String>> LANGUAGE_MAP;\n')
    strings_class.write('static {\n')
    strings_class.write('  LANGUAGE_MAP = '
                        'new HashMap<String, Map<String, String>>();\n')

    locale_map = self._GetFullLocaleMap()
    for locale_id in locale_map:
      strings_class.write('  Map<String, String> %s = '
                          'new HashMap<String, String>();\n' % locale_id)
      string_map = locale_map[locale_id]
      for key, value in string_map.items():
        strings_class.write('  %s.put("%s", "%s");\n' % (locale_id, key, value))
      strings_class.write('  LANGUAGE_MAP.put("%s", %s);\n\n' %
                          (locale_id, locale_id))
    strings_class.write('}\n')
    strings_class.write('\n')
    strings_class.write('private static '
                        'Map<String, String> getLanguageMap(Locale locale) {\n')
    strings_class.write('  String language = locale.getLanguage();\n')
    strings_class.write('  if (LANGUAGE_MAP.containsKey(language)) {\n')
    strings_class.write('    return LANGUAGE_MAP.get(language);\n')
    strings_class.write('  } else {\n')
    strings_class.write('    // If language is not found, default to english.'
                        '\n')
    strings_class.write('    return LANGUAGE_MAP.get("en");\n')
    strings_class.write('  }\n')
    strings_class.write('}\n')
    strings_class.write('\n')
    strings_class.write('private static String getString('
                        'String stringId, Locale locale) {\n')
    strings_class.write('  Map<String, String> stringMap = '
                        'getLanguageMap(locale);\n')
    strings_class.write('  if (stringMap.containsKey(stringId)) {\n')
    strings_class.write('    return stringMap.get(stringId);\n')
    strings_class.write('  } else {\n')
    strings_class.write('    // If the string is not found for the specific '
                        'language, fall back to the original string.\n')
    strings_class.write('    return LANGUAGE_MAP.get("en").get(stringId);\n')
    strings_class.write('  }\n')
    strings_class.write('}\n')
    strings_class.write('\n')
    strings_class.write('public static String getString(String stringId) {\n')
    strings_class.write('  return getString(stringId, Locale.getDefault());\n')
    strings_class.write('}\n')

    strings_class.write('}\n')

    strings_class.close()

  def _ParseFile(self) -> None:
    """Parses the file at self._resource_file_name into self._parsed_strings.

    The strings file contains the strings required by the application.
    This file will consist of comment lines /* */ and strings lines:
    "key" = "value";.
    """
    self._parsed_strings = self._ParseStringsFile(
        self._resource_file_name, self._check_substitution_tokens)

  def _ParseStringsFile(self, resource_file: str,
                        check_substitution_tokens: bool) -> Dict[str, str]:
    """Parses the file at self._resource_file_name into self._parsed_strings.

    The strings file contains the strings required by the application.
    This file will consist of comment lines /* */ and strings lines:
    "key" = "value";.

    Args:
      resource_file: The name of the Localizeable.strings file.
      check_substitution_tokens: Whether or not to validate that the
          substitution tokens are valid.

    Returns:
        Map of key/value pairs of localized strings.
    """
    parsed_strings = {}
    strings_file = open(resource_file, 'r')
    for line in strings_file:
      match = re.match(r'"([a-zA-Z0-9_]*)"\s*=\s*"(.*)";$', line)
      if match:
        key = match.group(1)
        value = match.group(2)
        if check_substitution_tokens:
          substitution_tokens = re.findall(r'(\%[0-9a-zA-Z@\.\$]+)', value)
          if len(substitution_tokens) > 1:
            for substitution_token in substitution_tokens:
              # Match numbered substitutions ie. %1$d
              if not re.match(r'\%[0-9]+\$', substitution_token):
                sys.stderr.write(
                    'Line: %s includes multiple substitution parameters, '
                    'some of which are not numbered.  This can cause issues '
                    'in some languages.  You are seeing this error because '
                    '--check_substitution_tokens is passed to genstrings.py. '
                    'Remove this flag if you are not launched in languages '
                    'where this is a problem.' % line)
                exit(1)

        parsed_strings[key] = value
    strings_file.close()
    return parsed_strings

  def _GetFullLocaleMap(self):
    """Parses all the string files for each locale.

    The location of each locale is based on the location of
        self._resource_file_name.
    Returns:
        Map of a map of key/value pairs of localized strings per locale.
    """
    locale_map = {}
    # Get the directory that the base strings file is in.
    base_dir = os.path.abspath(os.path.join(self._resource_file_name,
                                            os.pardir))
    # Get the root strings directory that contains all locales.
    root_resource_dir = os.path.abspath(os.path.join(base_dir, os.pardir))
    # Get the base name of the strings file as that should be the same in each
    # locale directory.
    strings_filename = os.path.basename(self._resource_file_name)
    # Get the list of all locale directories.
    locale_dirs = os.listdir(root_resource_dir)

    # For each locale, parse the strings file and put that in a map.
    for locale_dir in locale_dirs:
      locale_file = os.path.abspath(os.path.join(root_resource_dir, locale_dir,
                                                 strings_filename))
      parsed_strings = self._ParseStringsFile(locale_file,
                                              self._check_substitution_tokens)
      # Strip off the lproj suffix so we are only left with the locale id.
      locale_id = re.sub(r'\.lproj$', '', locale_dir)
      locale_map[locale_id] = parsed_strings
    return locale_map


def main(argv: List[str]) -> int:
  """Main method."""
  try:
    parser = optparse.OptionParser()
    parser.add_option('--out_dir',
                      help='The directory where code should be placed',
                      dest='out_dir',
                      action='store',
                      default='')
    parser.add_option(
        '--resource_file',
        help='The english Localizeable.strings file to be used',
        dest='resource_file',
        action='store',
        default='../Resources/Strings/en.lproj/Localizable.string')
    parser.add_option('--namespace',
                      help='Namespace used in generated code',
                      dest='namespace',
                      action='store',
                      default='sassafras')
    parser.add_option('--check_substitution_tokens',
                      help=('When set, each string will be parsed for '
                            'substitution tokens %d, and will fail if there '
                            'are multiple substitution tokens and any are '
                            'not numbered.'),
                      dest='check_substitution_tokens',
                      action='store_true')
    parser.add_option('--language',
                      help='Output language, either C++ (cc) or Java (java)',
                      dest='language',
                      action='store',
                      default='cc')
    (options, args) = parser.parse_args(args=argv)
    if args:
      raise Error('No argument allowed.')
    strings_parser = StringsParser(options.resource_file, options.namespace,
                                   options.out_dir,
                                   options.check_substitution_tokens,
                                   options.language)
    strings_parser.Run()

    return 0
  except Error as e:
    sys.stderr.write('%s: Error: %s\n' % (os.path.basename(sys.argv[0]),
                                          e.args[0]))
    return 1


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
