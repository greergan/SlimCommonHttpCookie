#include <cctype>
#include <charconv>
#include <ctime>
#include <iomanip>
#include <limits>
#include <locale>
#include <slim/SlimValue.hpp>
#include <sstream>
#include <string>
#include <string_view>

#include <slim/common/http/cookie.h>

namespace {

struct AsciiTables {
    std::array<char, 256> to_lower{};
    std::array<bool, 256> is_alnum{};
    std::array<bool, 256> is_space{};
    std::array<bool, 256> is_cookie_char{};

    constexpr AsciiTables() {
        for (size_t i = 0; i < 256; ++i) {
            to_lower[i] = (i >= 'A' && i <= 'Z') ? static_cast<char>(i + 32) : static_cast<char>(i);
            is_alnum[i] = (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9');
            is_space[i] = (i == ' ' || i == '\t' || i == '\r' || i == '\n' || i == '\v' || i == '\f');

            // Valid RFC 6265 cookie characters table
            unsigned char uc = static_cast<unsigned char>(i);
            is_cookie_char[i] = (uc == 0x21)
                                || (uc >= 0x23 && uc <= 0x2B)
                                || (uc >= 0x2D && uc <= 0x3A)
                                || (uc >= 0x3C && uc <= 0x5B)
                                || (uc >= 0x5D && uc <= 0x7E);
        }
    }
};

constexpr AsciiTables ascii{};

constexpr bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (ascii.to_lower[static_cast<unsigned char>(a[i])] !=
            static_cast<unsigned char>(b[i])) {
            return false;
        }
    }
    return true;
}

constexpr bool istarts_with(std::string_view s, std::string_view prefix) noexcept {
    if (s.size() < prefix.size()) return false;
    return iequals(s.substr(0, prefix.size()), prefix);
}

constexpr std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.front())]) {
        s.remove_prefix(1);
    }
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.back())]) {
        s.remove_suffix(1);
    }
    return s;
}

constexpr COOKIE::STATUS get_bool(std::string_view s, bool& b) noexcept {
    std::string_view trimmed = trim(s);
    if (iequals(trimmed, "true")) {
        b = true;
        return COOKIE::STATUS::OK;
    }
    if (iequals(trimmed, "false")) {
        b = false;
        return COOKIE::STATUS::OK;
    }
    return COOKIE::STATUS::INVALID_BOOLEAN;
}

// Helper to convert 3-letter month/day abbreviations to integers efficiently
constexpr int month_abbr_to_int(std::string_view s) noexcept {
    if (s.size() < 3) return -1;
    // Map combinations manually or pack chars into a single 24-bit uint for switch
    uint32_t val = (static_cast<uint32_t>(ascii.to_lower[static_cast<unsigned char>(s[0])]) << 16) |
                   (static_cast<uint32_t>(ascii.to_lower[static_cast<unsigned char>(s[1])]) << 8)  |
                    static_cast<uint32_t>(ascii.to_lower[static_cast<unsigned char>(s[2])]);
    switch(val) {
        case 0x6a616e: return 0;  // "jan"
        case 0x666562: return 1;  // "feb"
        case 0x6d6172: return 2;  // "mar"
        case 0x617072: return 3;  // "apr"
        case 0x6d6179: return 4;  // "may"
        case 0x6a756e: return 5;  // "jun"
        case 0x6a756c: return 6;  // "jul"
        case 0x617567: return 7;  // "aug"
        case 0x736570: return 8;  // "sep"
        case 0x6f6374: return 9;  // "oct"
        case 0x6e6f76: return 10; // "nov"
        case 0x646563: return 11; // "dec"
        default: return -1;
    }
}

constexpr COOKIE::STATUS validate_domain(std::string_view s) noexcept {
    if (s.empty())
        return COOKIE::STATUS::DOMAIN_EMPTY;

    if (s.front() == '.') s.remove_prefix(1);

    if (s.empty())
        return COOKIE::STATUS::DOMAIN_BARE_DOT;

    if (s.back() == '.')
        return COOKIE::STATUS::DOMAIN_TRAILING_DOT;

    if (s.size() > 253)
        return COOKIE::STATUS::DOMAIN_TOO_LONG;

    size_t label_start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == '.') {
            std::string_view label = s.substr(label_start, i - label_start);

            if (label.empty())
                return COOKIE::STATUS::DOMAIN_LABEL_EMPTY;
            if (label.size() > 63)
                return COOKIE::STATUS::DOMAIN_LABEL_TOO_LONG;
            if (label.front() == '-' || label.back() == '-')
                return COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN;

            for (char c : label) {
                //if (!ascii.is_alnum[static_cast<unsigned char>(c)] && c != '-')
                if (!ascii.is_alnum[static_cast<unsigned char>(c)] && c != '-')
                    return COOKIE::STATUS::DOMAIN_INVALID_CHAR;
            }
            label_start = i + 1;
        }
    }

    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_expires(std::string_view s) noexcept {
    // Basic structural length guard
    if (s.size() < 20) return COOKIE::STATUS::EXPIRES_INVALID_FORMAT;

    // Fast inline token checks mimicking strptime without instantiation costs
    // Format 1: RFC 1123 "Sun, 06 Nov 1994 08:49:37 GMT"
    if (s.size() == 29 && s.substr(26) == "GMT" && s[4] == ' ' && s[7] == ' ') {
        int day = 0;
        auto [p1, ec1] = std::from_chars(s.data() + 5, s.data() + 7, day);
        int month = month_abbr_to_int(s.substr(8, 3));
        if (ec1 == std::errc{} && month != -1) return COOKIE::STATUS::OK;
    }

    // Format 2: RFC 850 "Sunday, 06-Nov-94 08:49:37 GMT"
    if (s.ends_with("GMT") && s.find('-') != std::string_view::npos) {
        size_t dash1 = s.find('-');
        if (dash1 != std::string_view::npos && dash1 >= 3 && s[dash1 + 4] == '-') {
            int month = month_abbr_to_int(s.substr(dash1 + 1, 3));
            if (month != -1) return COOKIE::STATUS::OK;
        }
    }

    // Fallback: Check if it matches ANSI C asctime layout
    int month_alt = month_abbr_to_int(s.substr(4, 3));
    if (month_alt != -1 && s.size() == 24) return COOKIE::STATUS::OK;

    return COOKIE::STATUS::EXPIRES_INVALID_FORMAT;
}

constexpr COOKIE::STATUS validate_path(std::string_view s) noexcept {
    if (s.empty())
        return COOKIE::STATUS::PATH_EMPTY;

    if (s.front() != '/')
        return COOKIE::STATUS::PATH_MISSING_LEADING_SLASH;

    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x1F || uc == 0x7F)
            return COOKIE::STATUS::PATH_INVALID_CHAR;
        if (c == ';')
            return COOKIE::STATUS::PATH_INVALID_CHAR;
    }

    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_max_age(std::uint_least64_t v) noexcept {
    constexpr auto max_val = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    if (v > max_val)
        return COOKIE::STATUS::MAX_AGE_EXCEEDS_LIMIT;
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS get_max_age_value(std::string_view s, std::optional<std::uint_least64_t>& v) noexcept {
    std::string_view trimmed = trim(s);

    if (trimmed.empty())
        return COOKIE::STATUS::MAX_AGE_EMPTY;

    std::uint_least64_t temp_value = 0;
    auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), temp_value);

    if (ec != std::errc{})
        return COOKIE::STATUS::MAX_AGE_INVALID_FORMAT;

    if (ptr != trimmed.data() + trimmed.size())
        return COOKIE::STATUS::MAX_AGE_TRAILING_CHARS;

    COOKIE::STATUS e = validate_max_age(temp_value);
    if(e == COOKIE::STATUS::OK)
        v = temp_value;

    return e;
}

constexpr COOKIE::STATUS validate_name(std::string_view s) noexcept {
    if (s.empty())
        return COOKIE::STATUS::NAME_EMPTY;

    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);

        if (uc <= 0x20 || uc >= 0x7F)
            return COOKIE::STATUS::NAME_INVALID_CHAR;

        switch (c) {
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
            case '{': case '}':
                return COOKIE::STATUS::NAME_INVALID_CHAR;
        }
    }

    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_partitioned(bool secure, bool partitioned) noexcept {
    if (partitioned && !secure)
        return COOKIE::STATUS::PARTITIONED_REQUIRES_SECURE;
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_secure(std::string_view same_site, bool secure) noexcept {
    if (iequals(same_site, "none") && !secure)
        return COOKIE::STATUS::SAMESITE_NONE_REQUIRES_SECURE;
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_same_site(std::string_view s) noexcept {
    if (iequals(s, "strict")) return COOKIE::STATUS::OK;
    if (iequals(s, "lax"))    return COOKIE::STATUS::OK;
    if (iequals(s, "none"))   return COOKIE::STATUS::OK;
    return COOKIE::STATUS::SAMESITE_INVALID;
}

constexpr COOKIE::STATUS validate_value(std::string_view s) noexcept {
    if (s.empty())
        return COOKIE::STATUS::VALUE_EMPTY;

    if (s.front() == '"') {
        if (s.size() < 2 || s.back() != '"')
            return COOKIE::STATUS::VALUE_UNMATCHED_QUOTE;
        s = s.substr(1, s.size() - 2);
    }

    // Lookups replace nested comparisons
    for (char c : s) {
        if (!ascii.is_cookie_char[static_cast<unsigned char>(c)])
            return COOKIE::STATUS::VALUE_INVALID_CHAR;
    }
    return COOKIE::STATUS::OK;
}

} // namespace

COOKIE::STATUS slim::common::http::Cookie::set_domain(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_domain(trimmed);
    if(e == COOKIE::STATUS::OK)
        domain = trimmed;
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_domain(std::string_view s) {
    return validate_domain(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_expires(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_expires(trimmed);
    if(e == COOKIE::STATUS::OK)
        expires = trimmed;
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_expires(std::string_view s) {
    return validate_expires(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_max_age(std::string_view s) {
    return get_max_age_value(s, max_age);
}

COOKIE::STATUS slim::common::http::Cookie::valid_max_age(std::uint_least64_t v) {
    return validate_max_age(v);
}

COOKIE::STATUS slim::common::http::Cookie::set_name(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_name(trimmed);
    if(e == COOKIE::STATUS::OK)
        name = trimmed;
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_name(std::string_view s) {
    return validate_name(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_path(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_path(trimmed);
    if(e == COOKIE::STATUS::OK)
        path = trimmed;
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_path(std::string_view s) {
    return validate_path(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_value(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_value(trimmed);
    if(e == COOKIE::STATUS::OK)
        value = trimmed;
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_value(std::string_view s) {
    return validate_value(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_same_site(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_same_site(trimmed);
    if(e == COOKIE::STATUS::OK)
        same_site = trimmed;
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_same_site(std::string_view s) {
    return validate_same_site(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_httponly(std::string_view s) {
    return get_bool(s, httponly);
}

COOKIE::STATUS slim::common::http::Cookie::set_partitioned(std::string_view s) {
    return get_bool(s, partitioned);
}

COOKIE::STATUS slim::common::http::Cookie::valid_partitioned(const bool secure, const bool partitioned) {
    return ::validate_partitioned(secure, partitioned);
}

COOKIE::STATUS slim::common::http::Cookie::validate_partitioned() {
    return ::validate_partitioned(secure, partitioned);
}

COOKIE::STATUS slim::common::http::Cookie::set_secure(std::string_view s) {
    return get_bool(s,  secure);
}

COOKIE::STATUS slim::common::http::Cookie::valid_secure(std::string_view same_site, const bool secure) {
    return ::validate_secure(trim(same_site), secure);
}

COOKIE::STATUS slim::common::http::Cookie::validate_secure() {
    return ::validate_secure(same_site.value_or(""), secure);
}

std::string slim::common::http::Cookie::serialize() const {
    std::ostringstream ss;
    ss << name << "=" << value;
    if (domain) ss << "; Domain=" << *domain;
    if (path) ss << "; Path=" << *path;
    if (expires) ss << "; Expires=" << *expires;
    if (max_age) ss << "; Max-Age=" << *max_age;
    if (same_site) ss << "; SameSite=" << *same_site;
    if (secure) ss << "; Secure";
    if (httponly) ss << "; HttpOnly";
    if (partitioned) ss << "; Partitioned";
    return ss.str();
}
