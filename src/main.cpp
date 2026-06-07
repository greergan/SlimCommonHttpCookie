#include <cctype>
#include <charconv>
#include <ctime>
#include <format>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>

#include <slim/common/http/cookie.h>

namespace {

constexpr bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            static_cast<unsigned char>(b[i])) {
            return false;
        }
    }
    return true;
}

constexpr std::string_view trim(std::string_view s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    if (start == s.size()) return {};

    size_t end = s.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end]))) {
        --end;
    }
    return s.substr(start, end - start + 1);
}

slim::SlimValue get_bool(std::string_view s) {
    std::string_view trimmed = trim(s);
    if (iequals(trimmed, "true")) return true;
    if (iequals(trimmed, "false")) return false;
    return slim::SlimValue{}.set_error(std::format("'{}' => invalid boolean (expected 'true' or 'false')", s));
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
    std::tm tm{};
    std::istringstream ss{std::string{s}};
    ss.imbue(std::locale{"C"});

    const char* formats[] = {
        "%a, %d %b %Y %H:%M:%S GMT", // RFC 1123 (Preferred)
        "%A, %d-%b-%y %H:%M:%S GMT", // RFC 850 (Obsolete)
        "%a %b %d %H:%M:%S %Y"       // ANSI C asctime (Required)
    };

    for (const char* fmt : formats) {
        ss.clear();
        ss.seekg(0);
        ss >> std::get_time(&tm, fmt);
        if (!ss.fail()) return {};
    }

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
                return slim::ErrorInfo{std::format("'{}' => invalid cookie name (separator not permitted) => '{}'", s, c)};
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

    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        bool valid = uc == 0x21
                  || (uc >= 0x23 && uc <= 0x2B)
                  || (uc >= 0x2D && uc <= 0x3A)
                  || (uc >= 0x3C && uc <= 0x5B)
                  || (uc >= 0x5D && uc <= 0x7E);
        if (!valid)
            return slim::ErrorInfo{std::format("'{}' => invalid cookie value (unexpected character) => '{}'", s, c)};
    }

    return {};
}

} // namespace

slim::ErrorInfo slim::common::http::Cookie::set_domain(std::string_view s) {
    domain = std::string{trim(s)};
    return validate_domain(domain.value());
}

slim::ErrorInfo slim::common::http::Cookie::valid_domain(std::string_view s) {
    return validate_domain(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_expires(std::string_view s) {
    expires = std::string{trim(s)};
    return validate_expires(expires.value());
}

slim::ErrorInfo slim::common::http::Cookie::valid_expires(std::string_view s) {
    return validate_expires(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_max_age(std::string_view s) {
    std::string_view trimmed = trim(s);

    if (trimmed.empty())
        return ErrorInfo{std::format("'{}' => max-age cannot be empty", s)};

    auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), *max_age);

    if (ec != std::errc{})
        return ErrorInfo{std::format("'{}' => invalid max-age format (expected non-negative integer)", s)};

    if (ptr != trimmed.data() + trimmed.size())
        return ErrorInfo{std::format("'{}' => invalid max-age format (trailing characters)", s)};

    return validate_max_age(max_age.value());
}

slim::ErrorInfo slim::common::http::Cookie::valid_max_age(std::uint_least64_t v) {
    return validate_max_age(v);
}

slim::ErrorInfo slim::common::http::Cookie::set_name(std::string_view s) {
    name = std::string{trim(s)};
    return validate_name(name);
}

slim::ErrorInfo slim::common::http::Cookie::valid_name(std::string_view s) {
    return validate_name(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_path(std::string_view s) {
    path = std::string{trim(s)};
    return validate_path(path.value());
}

slim::ErrorInfo slim::common::http::Cookie::valid_path(std::string_view s) {
    return validate_path(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_value(std::string_view s) {
    value = std::string{trim(s)};
    return validate_value(value);
}

slim::ErrorInfo slim::common::http::Cookie::valid_value(std::string_view s) {
    return validate_value(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_same_site(std::string_view s) {
    same_site = std::string{trim(s)};
    return validate_same_site(same_site.value());
}

slim::ErrorInfo slim::common::http::Cookie::valid_same_site(std::string_view s) {
    return validate_same_site(trim(s));
}

slim::ErrorInfo slim::common::http::Cookie::set_httponly(std::string_view s) {
    slim::SlimValue r = get_bool(s);
    if (r.has_error()) return r.get_error();
    httponly = r.get_bool();
    return {};
}

slim::ErrorInfo slim::common::http::Cookie::set_partitioned(std::string_view s) {
    slim::SlimValue r = get_bool(s);
    if (r.has_error()) return r.get_error();
    partitioned = r.get_bool();
    return {};
}

slim::ErrorInfo slim::common::http::Cookie::valid_partitioned(const bool secure, const bool partitioned) {
    return ::validate_partitioned(secure, partitioned);
}

slim::ErrorInfo slim::common::http::Cookie::validate_partitioned() {
    return ::validate_partitioned(secure, partitioned);
}

slim::ErrorInfo slim::common::http::Cookie::set_secure(std::string_view s) {
    slim::SlimValue r = get_bool(s);
    if (r.has_error()) return r.get_error();
    secure = r.get_bool();
    return {};
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
