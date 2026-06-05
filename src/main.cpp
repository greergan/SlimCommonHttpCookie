#include <ctime>
#include <iomanip>
#include <sstream>
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
    if (start == s.size()) return std::string_view{};

    size_t end = s.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end]))) {
        --end;
    }
    return s.substr(start, end - start + 1);
}

slim::SlimValue is_bool(std::string_view s) {
    std::string_view cleaned = trim(s);
    if (iequals(cleaned, "true")) return true;
    if (iequals(cleaned, "false")) return false;
    return slim::SlimValue{}.set_error(std::format("'{}' => invalid boolean (expected 'true' or 'false')", s));
}

slim::SlimValue is_expires(std::string_view s) {
    std::tm tm{};
    std::istringstream ss{std::string{s}};
    ss.imbue(std::locale{"C"});

    // RFC 7231 requires these 3 formats
    const char* formats[] = {
        "%a, %d %b %Y %H:%M:%S GMT", // RFC 1123 (Preferred)
        "%A, %d-%b-%y %H:%M:%S GMT", // RFC 850 (Obsolete)
        "%a %b %d %H:%M:%S %Y"       // ANSI C asctime (Required)
    };

    for (const char* fmt : formats) {
        ss.clear();
        ss.seekg(0);
        ss >> std::get_time(&tm, fmt);
        if (!ss.fail()) return true;
    }

    return slim::SlimValue{}.set_error(std::format("'{}' => invalid expires format", s));
}

slim::SlimValue is_domain(std::string_view s) {
    std::string_view cleaned = trim(s);

    if (cleaned.empty())
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain (empty)", s));

    // strip leading dot per RFC 6265 §5.2.3
    if (cleaned.front() == '.') cleaned.remove_prefix(1);

    if (cleaned.empty())
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain (bare dot)", s));

    if (cleaned.back() == '.')
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain (trailing dot)", s));

    if (cleaned.size() > 253)
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain (exceeds 253 character limit)", s));

    size_t label_start = 0;
    for (size_t i = 0; i <= cleaned.size(); ++i) {
        if (i == cleaned.size() || cleaned[i] == '.') {
            std::string_view label = cleaned.substr(label_start, i - label_start);

            if (label.empty())
                return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain (empty label)", s));
            if (label.size() > 63)
                return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain label (exceeds 63 character limit)", s));
            if (label.front() == '-' || label.back() == '-')
                return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain label (cannot start or end with hyphen)", s));

            for (char c : label) {
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-')
                    return slim::SlimValue{}.set_error(std::format("'{}' => invalid domain label (unexpected character) => '{}'", s, c));
            }
            label_start = i + 1;
        }
    }

    return true;
}

slim::SlimValue is_name(std::string_view s) {
    std::string_view cleaned = trim(s);

    if (cleaned.empty())
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid cookie name (empty)", s));

    for (char c : cleaned) {
        unsigned char uc = static_cast<unsigned char>(c);

        if (uc <= 0x20 || uc >= 0x7F)
            return slim::SlimValue{}.set_error(std::format("'{}' => invalid cookie name (control character or whitespace)", s));

        switch (c) {
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
            case '{': case '}':
                return slim::SlimValue{}.set_error(std::format("'{}' => invalid cookie name (separator not permitted) => '{}'", s, c));
        }
    }

    return true;
}

slim::SlimValue is_path(std::string_view s) {
    std::string_view cleaned = trim(s);

    if (cleaned.empty())
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid path (empty)", s));

    if (cleaned.front() != '/')
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid path (must begin with '/')", s));

    for (char c : cleaned) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x1F || uc == 0x7F)
            return slim::SlimValue{}.set_error(std::format("'{}' => invalid path (control character)", s));
        if (c == ';')
            return slim::SlimValue{}.set_error(std::format("'{}' => invalid path (semicolon not permitted)", s));
    }

    return true;
}

slim::SlimValue is_same_site(std::string_view s) {
    std::string_view cleaned = trim(s);
    if (iequals(cleaned, "strict")) return true;
    if (iequals(cleaned, "lax"))    return true;
    if (iequals(cleaned, "none"))   return true;
    return slim::SlimValue{}.set_error(std::format("'{}' => invalid same-site value (expected 'strict', 'lax', or 'none')", s));
}

slim::SlimValue is_value(std::string_view s) {
    std::string_view cleaned = trim(s);

    // optional DQUOTE wrapping
    if (!cleaned.empty() && cleaned.front() == '"') {
        if (cleaned.size() < 2 || cleaned.back() != '"')
            return slim::SlimValue{}.set_error(std::format("'{}' => invalid cookie value (unmatched double quote)", s));
        cleaned = cleaned.substr(1, cleaned.size() - 2);
    }

    for (char c : cleaned) {
        unsigned char uc = static_cast<unsigned char>(c);
        bool valid = uc == 0x21
                  || (uc >= 0x23 && uc <= 0x2B)
                  || (uc >= 0x2D && uc <= 0x3A)
                  || (uc >= 0x3C && uc <= 0x5B)
                  || (uc >= 0x5D && uc <= 0x7E);
        if (!valid)
            return slim::SlimValue{}.set_error(std::format("'{}' => invalid cookie value (unexpected character) => '{}'", s, c));
    }

    return true;
}

slim::SlimValue validated_max_age(std::string_view s) {
    std::string_view trimmed = trim(s);

    if (trimmed.empty())
        return slim::SlimValue{}.set_error(std::format("'{}' => max-age cannot be empty", s));

    using u_time_t = std::make_unsigned_t<std::time_t>;
    u_time_t value{};
    auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);

    if (ec != std::errc{})
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid max-age format (expected non-negative integer)", s));

    if (ptr != trimmed.data() + trimmed.size())
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid max-age format (trailing characters)", s));

    constexpr u_time_t max_val = static_cast<u_time_t>(std::numeric_limits<std::time_t>::max());
    if (value > max_val)
        return slim::SlimValue{}.set_error(std::format("'{}' => max-age exceeds maximum time_t limit", s));

    return static_cast<long long>(value);
}

} // namespace

slim::SlimValue slim::common::http::Cookie::set_max_age(std::string_view s) {
    auto r = validated_max_age(s);
    if (!r.has_error()) max_age = static_cast<max_age_t>(r.get_long_long());
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_name(std::string_view s) {
    slim::SlimValue r = is_name(s);
    if (r) name = std::string(trim(s));
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_value(std::string_view s) {
    slim::SlimValue r = is_value(s);
    if (r) value = std::string(trim(s));
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_path(std::string_view s) {
    slim::SlimValue r = is_path(s);
    if (r) path = std::string(trim(s));
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_domain(std::string_view s) {
    slim::SlimValue r = is_domain(s);
    if (r) domain = std::string(trim(s));
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_expires(std::string_view s) {
    slim::SlimValue r = is_expires(s);
    if (r) expires = std::string(trim(s));
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_httponly(std::string_view s) {
    slim::SlimValue r = is_bool(s);
    if (!r.has_error()) httponly = r.get_bool();
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_partitioned(std::string_view s) {
    slim::SlimValue r = is_bool(s);
    if (!r.has_error()) partitioned = r.get_bool();
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_same_site(std::string_view s) {
    slim::SlimValue r = is_same_site(s);
    if (!r) return r;

    auto trimmed = trim(s);

    if (iequals(trimmed, "none") && !secure)
        return slim::SlimValue{}.set_error(
            std::format("'{}' => SameSite=None requires Secure=true", s));

    same_site = std::string(trimmed);
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_secure(std::string_view s) {
    slim::SlimValue r = is_bool(s);
    if (r.has_error()) return r;

    if (!r.get_bool() && iequals(same_site, "none"))
        return slim::SlimValue{}.set_error(
            std::format("'{}' => cannot unset Secure while SameSite=None", s));

    secure = r.get_bool();
    return r;
}