#include <ctime>
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
    return slim::SlimValue{}.set_error(std::format("'{}' is not a valid boolean (expected 'true' or 'false')", s));
}

slim::SlimValue validate_expires(std::string_view s) {
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

slim::SlimValue validated_max_age(std::string_view s) {
    std::string_view trimmed = trim(s);

    if (trimmed.empty())
        return slim::SlimValue{}.set_error(std::format("'{}' => max-age cannot be empty", s));

    long long value = 0;
    auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);

    if (ec != std::errc{})
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid max-age format (expected non-negative integer)", s));

    if (ptr != trimmed.data() + trimmed.size())
        return slim::SlimValue{}.set_error(std::format("'{}' => invalid max-age format (trailing characters)", s));

    constexpr long long max_time_t = static_cast<long long>(std::numeric_limits<std::time_t>::max());

    if (value > max_time_t)
        return slim::SlimValue{}.set_error(std::format("'{}' => max-age exceeds maximum time_t limit", s));

    return value;
}

} // namespace

slim::SlimValue slim::common::http::Cookie::set_max_age(std::string_view s) {
    auto r = validated_max_age(s);
    if (r) max_age = r.get_long_long();
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_name(std::string_view s) {
    return true;
}

slim::SlimValue slim::common::http::Cookie::set_value(std::string_view s) {
    return true;
}

slim::SlimValue slim::common::http::Cookie::set_path(std::string_view s) {
    return true;
}

slim::SlimValue slim::common::http::Cookie::set_domain(std::string_view s) {
    return true;
}

slim::SlimValue slim::common::http::Cookie::set_expires(std::string_view s) {
    slim::SlimValue r = validate_expires(s);
    if (r) expires = std::string(s);
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_same_site(std::string_view s) {
    return true;
}

slim::SlimValue slim::common::http::Cookie::set_httponly(std::string_view s) {
    auto r = is_bool(s);
    if (!r.has_error()) httponly = r.get_bool();
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_partitioned(std::string_view s) {
    auto r = is_bool(s);
    if (!r.has_error()) partitioned = r.get_bool();
    return r;
}

slim::SlimValue slim::common::http::Cookie::set_secure(std::string_view s) {
    auto r = is_bool(s);
    if (!r.has_error()) secure = r.get_bool();
    return r;
}