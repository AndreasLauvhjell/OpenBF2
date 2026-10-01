// Parses prefixed command-line options into ID/value pairs and prints registered help.
// Empty help text marks advanced options. Container layout follows the active STL.
// [CHOSEN] getKey/parseString return bool; add/remove return void. Parameter names are inferred.

#ifndef GAME_UTILITY_SIMPLEPARSER_H     // [INFERRED] guard style
#define GAME_UTILITY_SIMPLEPARSER_H

#include <string>
#include <map>
#include <list>
#include <utility>

namespace dice {
namespace hfe {

class SimpleParser
{
public:
    // [INFERRED] form - fully inlined at its only use (no symbol emitted). The member stores
    // (map ctor @+0, map ctor @+0xC, then '+' -> +0x18 at 0x8132029) match declaration order
    // map/map/char. A no-arg ctor hard-coding '+' cannot be formally excluded; `explicit` is
    // [CHOSEN], not recovered.
    explicit SimpleParser(char optionPrefix);

    // Registers an option. insert_unique semantics: a duplicate id is silently ignored (no
    // overwrite). An empty helpText makes the option an "advanced" one in showHelp.
    void add(int id, const std::string& optionName, const std::string& helpText);

    // Erases the id from both maps. Never called in the shipped binary.
    void remove(int id);

    // str is the text after a prefix char. Matches "name " (option name + one space) at position 0
    // against every registered name in ascending-id order; on a hit writes *keyId and extracts the
    // value (whitespace-delimited, or "-quoted). *keyId is set to -1 first and - oddity, preserved -
    // keeps the found id even when an unterminated quote makes the function return false.
    bool getKey(const std::string& str, int* keyId, std::string* value);

    // Clears keyList, then scans str for the prefix char, getKey()s the remainder at each hit and
    // push_backs (id,value) for each successful parse. Returns whether the prefix char occurred at
    // all - including hits where getKey failed.
    bool parseString(const std::string& str,
                     std::list<std::pair<int, std::string> >& keyList);

    // Prints the usage screen to std::cout (via an internal std::stringstream): options with help
    // text under "Available options are:", options with empty help under "Advanced options:".
    void showHelp(const std::string& appName);

private:
    std::map<int, std::string> myOptionNames;    // [INFERRED] name; +0x00
    std::map<int, std::string> myHelpTexts;      // [INFERRED] name; +0x0C
    char                       myOptionPrefix;   // [INFERRED] name; +0x18
};

} // namespace hfe
} // namespace dice

#endif // GAME_UTILITY_SIMPLEPARSER_H
