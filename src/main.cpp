#include <cctype>
#include <charconv>
#include <ctime>
#include <format>
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

constexpr std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.front())]) {
        s.remove_prefix(1);
    }
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.back())]) {
        s.remove_suffix(1);
    }
    return s;
}

slim::ErrorInfo get_bool(std::string_view s, bool& b) {
    std::string_view trimmed = trim(s);
    if (iequals(trimmed, "true")) {
        b = true;
        return {};
    }
    if (iequals(trimmed, "false")) {
        b = false;
        return {};
    }
    return slim::ErrorInfo{std::format("'{}' => invalid boolean (expected 'true' or 'false')", s)};
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

slim::ErrorInfo validate_domain(std::string_view s) {
    if (s.empty())
        return slim::ErrorInfo{std::format("'{}' => invalid domain (empty)", s)};

    if (s.front() == '.') s.remove_prefix(1);

    if (s.empty())
        return slim::ErrorInfo{std::format("'{}' => invalid domain (bare dot)", s)};

    if (s.back() == '.')
        return slim::ErrorInfo{std::format("'{}' => invalid domain (trailing dot)", s)};

    if (s.size() > 253)
        return slim::ErrorInfo{std::format("'{}' => invalid domain (exceeds 253 character limit)", s)};

    size_t label_start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == '.') {
            std::string_view label = s.substr(label_start, i - label_start);

            if (label.empty())
                return slim::ErrorInfo{std::format("'{}' => invalid domain (empty label)", s)};
            if (label.size() > 63)
                return slim::ErrorInfo{std::format("'{}' => invalid domain label (exceeds 63 character limit)", s)};
            if (label.front() == '-' || label.back() == '-')
                return slim::ErrorInfo{std::format("'{}' => invalid domain label (cannot start or end with hyphen)", s)};

            for (char c : label) {
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-')
                    return slim::ErrorInfo{std::format("'{}' => invalid domain label (unexpected character) => '{}'", s, c)};
            }
            label_start = i + 1;
        }
    }

    return {};
}

slim::ErrorInfo validate_expires(std::string_view s) {
    // Basic structural length guard
    if (s.size() < 20) return slim::ErrorInfo{std::format("'{}' => invalid expires format", s)};

    // Fast inline token checks mimicking strptime without instantiation costs
    // Format 1: RFC 1123 "Sun, 06 Nov 1994 08:49:37 GMT"
    if (s.size() == 29 && s.substr(26) == "GMT" && s[4] == ' ' && s[7] == ' ') {
        int day = 0;
        auto [p1, ec1] = std::from_chars(s.data() + 5, s.data() + 7, day);
        int month = month_abbr_to_int(s.substr(8, 3));
        if (ec1 == std::errc{} && month != -1) return {};
    }

    // Format 2: RFC 850 "Sunday, 06-Nov-94 08:49:37 GMT"
    if (s.ends_with("GMT") && s.find('-') != std::string_view::npos) {
        size_t dash1 = s.find('-');
        if (dash1 != std::string_view::npos && dash1 >= 3 && s[dash1 + 4] == '-') {
            int month = month_abbr_to_int(s.substr(dash1 + 1, 3));
            if (month != -1) return {};
        }
    }

    // Fallback: Check if it matches ANSI C asctime layout
    int month_alt = month_abbr_to_int(s.substr(4, 3));
    if (month_alt != -1 && s.size() == 24) return {};

    return slim::ErrorInfo{std::format("'{}' => invalid expires format", s)};
}

slim::ErrorInfo validate_path(std::string_view s) {
    if (s.empty())
        return slim::ErrorInfo{std::format("'{}' => invalid path (empty)", s)};

    if (s.front() != '/')
        return slim::ErrorInfo{std::format("'{}' => invalid path (must begin with '/')", s)};

    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x1F || uc == 0x7F)
            return slim::ErrorInfo{std::format("'{}' => invalid path (control character)", s)};
        if (c == ';')
            return slim::ErrorInfo{std::format("'{}' => invalid path (semicolon not permitted)", s)};
    }

    return {};
}

slim::ErrorInfo validate_max_age(std::uint_least64_t v) {
    constexpr auto max_val = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    if (v > max_val)
        return slim::ErrorInfo{std::format("'{}' => max-age exceeds maximum time_t limit", v)};
    return {};
}

slim::ErrorInfo get_max_age_value(std::string_view s, std::optional<std::uint_least64_t>& v) {
    std::string_view trimmed = trim(s);

    if (trimmed.empty())
        return slim::ErrorInfo{std::format("'{}' => max-age cannot be empty", s)};

    std::uint_least64_t temp_value = 0;
    auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), temp_value);

    if (ec != std::errc{})
        return slim::ErrorInfo{std::format("'{}' => invalid max-age format (expected non-negative integer)", s)};

    if (ptr != trimmed.data() + trimmed.size())
        return slim::ErrorInfo{std::format("'{}' => invalid max-age format (trailing characters)", s)};

    slim::ErrorInfo e = validate_max_age(temp_value);
    if(!e.has_error())
        v = temp_value;

    return e;
}

slim::ErrorInfo validate_name(std::string_view s) {
    if (s.empty())
        return slim::ErrorInfo{std::format("'{}' => invalid cookie name (empty)", s)};

    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);

        if (uc <= 0x20 || uc >= 0x7F)
            return slim::ErrorInfo{std::format("'{}' => invalid cookie name (control character or whitespace)", s)};

        switch (c) {
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
            case '{': case '}':
                return slim::ErrorInfo{std::format("'{}' => invalid cookie name (character not permitted) => '{}'", s, c)};
        }
    }

    return {};
}

slim::ErrorInfo validate_partitioned(bool secure, bool partitioned) {
    if (partitioned && !secure)
        return slim::ErrorInfo{"Partitioned requires Secure=true"};
    return {};
}

slim::ErrorInfo validate_secure(std::string_view same_site, bool secure) {
    if (iequals(same_site, "none") && !secure)
        return slim::ErrorInfo{"SameSite=None requires Secure=true"};
    return {};
}

slim::ErrorInfo validate_same_site(std::string_view s) {
    if (iequals(s, "strict")) return {};
    if (iequals(s, "lax"))    return {};
    if (iequals(s, "none"))   return {};
    return slim::ErrorInfo{std::format("'{}' => invalid same-site value (expected 'strict', 'lax', or 'none')", s)};
}

slim::ErrorInfo validate_value(std::string_view s) {
    if (s.empty())
        return slim::ErrorInfo{std::format("'{}' => invalid cookie value (empty)", s)};

    if (s.front() == '"') {
        if (s.size() < 2 || s.back() != '"')
            return slim::ErrorInfo{std::format("'{}' => invalid cookie value (unmatched double quote)", s)};
        s = s.substr(1, s.size() - 2);
    }

    // Lookups replace nested comparisons
    for (char c : s) {
        if (!ascii.is_cookie_char[static_cast<unsigned char>(c)])
            return slim::ErrorInfo{std::format("'{}' => invalid cookie value (unexpected character) => '{}'", s, c)};
    }
    return {};
}

} // namespace

slim::ErrorInfo slim::common::http::Cookie::set_domain(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_domain(trimmed);
    if(!e.has_error())
        domain = trimmed;
    return e;
}

slim::ErrorInfo slim::common::http::Cookie::valid_domain(std::string_view s) {
    return validate_domain(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_expires(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_expires(trimmed);
    if(!e.has_error())
        expires = trimmed;
    return e;
}

slim::ErrorInfo slim::common::http::Cookie::valid_expires(std::string_view s) {
    return validate_expires(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_max_age(std::string_view s) {
    return get_max_age_value(s, max_age);
}

slim::ErrorInfo slim::common::http::Cookie::valid_max_age(std::uint_least64_t v) {
    return validate_max_age(v);
}

slim::ErrorInfo slim::common::http::Cookie::set_name(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_name(trimmed);
    if(!e.has_error())
        name = trimmed;
    return e;
}

slim::ErrorInfo slim::common::http::Cookie::valid_name(std::string_view s) {
    return validate_name(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_path(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_path(trimmed);
    if(!e.has_error())
        path = trimmed;
    return e;
}

slim::ErrorInfo slim::common::http::Cookie::valid_path(std::string_view s) {
    return validate_path(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_value(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_value(trimmed);
    if(!e.has_error())
        value = trimmed;
    return e;
}

slim::ErrorInfo slim::common::http::Cookie::valid_value(std::string_view s) {
    return validate_value(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_same_site(std::string_view s) {
    std::string_view trimmed = trim(s);
    auto e = validate_same_site(trimmed);
    if(!e.has_error())
        same_site = trimmed;
    return e;
}

slim::ErrorInfo slim::common::http::Cookie::valid_same_site(std::string_view s) {
    return validate_same_site(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_httponly(std::string_view s) {
    return get_bool(s, httponly);
}

slim::ErrorInfo slim::common::http::Cookie::set_partitioned(std::string_view s) {
    return get_bool(s, partitioned);
}

slim::ErrorInfo slim::common::http::Cookie::valid_partitioned(const bool secure, const bool partitioned) {
    return ::validate_partitioned(secure, partitioned);
}

slim::ErrorInfo slim::common::http::Cookie::validate_partitioned() {
    return ::validate_partitioned(secure, partitioned);
}

slim::ErrorInfo slim::common::http::Cookie::set_secure(std::string_view s) {
    return get_bool(s,  secure);
}

slim::ErrorInfo slim::common::http::Cookie::valid_secure(std::string_view same_site, const bool secure) {
    return ::validate_secure(trim(same_site), secure);
}

slim::ErrorInfo slim::common::http::Cookie::validate_secure() {
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
