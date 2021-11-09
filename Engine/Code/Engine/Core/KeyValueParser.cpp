#include "Engine/Core/KeyValueParser.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <algorithm>
#include <locale>
#include <sstream>

KeyValueParser::KeyValueParser(const std::filesystem::path& filepath) noexcept {
    namespace FS = std::filesystem;
    if(FS::exists(filepath)) {
        if(auto contents = FileUtils::ReadStringBufferFromFile(filepath.string())) {
            Parse(contents.value_or(std::string{}));
        }
    }
}

KeyValueParser::KeyValueParser(const std::string& str) noexcept {
    Parse(str);
}

KeyValueParser::KeyValueParser(std::ifstream& file_input) noexcept {
    Parse(file_input);
}

KeyValueParser::KeyValueParser(std::istream& input) noexcept {
    Parse(input);
}

bool KeyValueParser::HasKey(const std::string& key) const noexcept {
    return _kv_pairs.find(key) != _kv_pairs.end();
}

void KeyValueParser::Parse(const std::string& input) noexcept {
    auto lines = StringUtils::SplitOnUnquoted(input, '\n', true);

    for(auto& cur_line : lines) {
        cur_line = cur_line.substr(0, cur_line.find_first_of('#'));
        if(cur_line.empty()) {
            continue;
        }
        std::size_t eq_count = CountCharNotInQuotes(cur_line, '=');
        std::size_t true_count = CountCharNotInQuotes(cur_line, '+');
        std::size_t false_count = CountCharNotInQuotes(cur_line, '-');
        bool no_eq = eq_count == 0;
        bool no_t = true_count == 0;
        bool no_f = false_count == 0;
        bool not_valid = no_eq && no_t && no_f;
        if(not_valid) {
            continue;
        }
        bool exactly_one_tf = (true_count == 1 || false_count == 1) && (true_count ^ false_count);
        bool multi_tf = !exactly_one_tf && (true_count > 0 || false_count > 0);
        bool atleast_one_eq = eq_count > 0;
        bool multi_eq = eq_count > 1;
        bool multi_params = multi_eq || (atleast_one_eq && (true_count > 0 || false_count > 0));
        bool probably_multiline = false;
        if(multi_eq || multi_tf || multi_params) {
            probably_multiline = true;
        }
        if(probably_multiline) {
            ParseMultiParams(cur_line);
            continue;
        }

        auto [key, value] = StringUtils::SplitOnFirst(cur_line, '=');

        key = StringUtils::TrimWhitespace(key);
        value = StringUtils::TrimWhitespace(value);

        //Shorthand cases
        if(StringUtils::StartsWith(key, "-")) {
            const auto sub_key = key.substr(1);
            SetValue(sub_key, false);
            continue;
        }
        if(StringUtils::StartsWith(key, "+")) {
            const auto sub_key = key.substr(1);
            SetValue(sub_key, true);
            continue;
        }

        if(key.find('"') != std::string::npos) {
            key = key.substr(key.find_first_not_of("\""), key.find_last_not_of("\""));
        }
        if(value.find('"') != std::string::npos) {
            const auto ffno = value.find_first_not_of('"');
            const auto flno = value.find_last_not_of('"');
            if(ffno == std::string::npos || flno == std::string::npos) {
                if(ffno == std::string::npos) {
                    value = value.substr(1);
                }
                if(flno == std::string::npos) {
                    value.pop_back();
                }
            } else {
                value = value.substr(ffno, flno);
            }
        }
        SetValue(key, value);
    }
}

void KeyValueParser::Parse(std::ifstream& input) noexcept {
    if(input.is_open() == false) {
        return;
    }
    if(input.good() == false) {
        return;
    }
    Parse(static_cast<std::istream&>(input));
}

void KeyValueParser::Parse(std::istream& input) noexcept {
    std::string cur_line;
    while(std::getline(input, cur_line)) {
        Parse(cur_line);
    }
}

std::map<std::string, std::string>&& KeyValueParser::Release() noexcept {
    return std::move(_kv_pairs);
}

void KeyValueParser::ParseMultiParams(const std::string& input) noexcept {
    std::string whole_line = input;
    CollapseMultiParamWhitespace(whole_line);
    ConvertFromMultiParam(whole_line);
    const auto lines = StringUtils::SplitOnUnquoted(whole_line, '\n');
    for(const auto& line : lines) {
        Parse(line);
    }
}

void KeyValueParser::ConvertFromMultiParam(std::string& whole_line) noexcept {
    //Replace space-delimited KVPs with newlines;
    auto inQuote = false;
    for(auto iter = whole_line.begin(); iter != whole_line.end(); ++iter) {
        if(*iter == '"') {
            inQuote = !inQuote;
            continue;
        }
        if(!inQuote) {
            if(*iter == ' ') {
                whole_line.replace(iter, iter + 1, "\n");
            }
        }
    }
}

void KeyValueParser::CollapseMultiParamWhitespace(std::string& whole_line) noexcept {
    //Remove spaces around equals
    auto eq_loc = std::find(whole_line.begin(), whole_line.end(), '=');
    while(eq_loc != std::end(whole_line)) {
        auto left_space_eq = eq_loc - 1;
        while(std::isspace(*left_space_eq, std::locale(""))) {
            left_space_eq--;
        }
        eq_loc = whole_line.erase(left_space_eq + 1, eq_loc);
        auto right_space_eq = eq_loc + 1;
        while(std::isspace(*right_space_eq, std::locale(""))) {
            right_space_eq++;
        }
        eq_loc = whole_line.erase(eq_loc + 1, right_space_eq);
        eq_loc = std::find(eq_loc + 1, whole_line.end(), '=');
    }
    //Remove consecutive spaces
    whole_line.erase(std::unique(whole_line.begin(), whole_line.end(),
                                 [](char lhs, char rhs) {
                                     return lhs == rhs && std::isspace(lhs, std::locale(""));
                                 }),
                     whole_line.end());
}

void KeyValueParser::SetValue(const std::string& key, const std::string& value) noexcept {
    _kv_pairs[key] = value;
}

void KeyValueParser::SetValue(const std::string& key, const bool& value) noexcept {
    _kv_pairs[key] = value ? std::string{"true"} : std::string{"false"};
}

std::size_t KeyValueParser::CountCharNotInQuotes(std::string& cur_line, char c) noexcept {
    auto inQuote = false;
    std::size_t count = 0u;
    for(auto iter = cur_line.begin(); iter != cur_line.end(); ++iter) {
        if(*iter == '"') {
            inQuote = !inQuote;
            continue;
        }
        if(!inQuote) {
            if(*iter == c) {
                ++count;
            }
        }
    }
    return count;
}
