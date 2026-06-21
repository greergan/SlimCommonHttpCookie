#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include <slim/common/http/cookie.h>
#include <slim/common/utilities.h>

namespace slim::common::http {

namespace {

ErrorStatus get_bool(std::string_view s, bool& b) noexcept {
    return slim::common::utilities::get_bool(s, b) ? ErrorStatus::OK : ErrorStatus::CookieInvalidBoolean;
}

ErrorStatus validate_domain(std::string_view& s) noexcept {
    slim::common::utilities::trim(s);
    if (s.empty()) return ErrorStatus::CookieDomainEmpty;
    const bool had_leading_dot = (s.front() == '.');
    if (had_leading_dot) s.remove_prefix(1);
    if (s.empty()) return ErrorStatus::CookieDomainBareDot;
    if (s.back() == '.') return ErrorStatus::CookieDomainTrailingDot;
    if (s.size() > 253) return ErrorStatus::CookieDomainTooLong;
    for (std::string_view rem = s; !rem.empty();) {
        const auto dot = rem.find('.');
        const std::string_view label = rem.substr(0, dot);
        if (label.empty()) return ErrorStatus::CookieDomainLabelEmpty;
        if (label.size() > 63) return ErrorStatus::CookieDomainLabelTooLong;
        if (label.front() == '-' || label.back() == '-') return ErrorStatus::CookieDomainLabelInvalidHyphen;
        for (char c : label) {
            const unsigned char uc = static_cast<unsigned char>(c);
            if (dot == std::string_view::npos && slim::common::utilities::is_digit(static_cast<char>(uc))) return ErrorStatus::CookieDomainNumericTld;
            if (!slim::common::utilities::is_alnum(static_cast<char>(uc)) && c != '-') return ErrorStatus::CookieDomainInvalidChar;
        }
        rem.remove_prefix(dot == std::string_view::npos ? rem.size() : dot + 1);
    }
    if (had_leading_dot) s = std::string_view(s.data() - 1, s.size() + 1);
    return ErrorStatus::OK;
}

ErrorStatus validate_expires(std::string_view& s, std::optional<std::string>& expires) noexcept {
    slim::common::utilities::trim(s);
    bool found_time = false, found_dom = false, found_month = false, found_year = false;
    int hour = 0, minute = 0, second = 0, day = 0, month = 0, year = 0;
    std::size_t pos = 0;
    while (pos < s.size()) {
        while (pos < s.size() && slim::common::utilities::is_date_delimiter(s[pos])) ++pos;
        if (pos >= s.size()) break;
        const std::size_t tok_start = pos;
        while (pos < s.size() && !slim::common::utilities::is_date_delimiter(s[pos])) ++pos;
        const std::size_t len = pos - tok_start;
        if (!found_time && len >= 8) {
            const auto h0 = static_cast<unsigned char>(s[tok_start + 0]), h1 = static_cast<unsigned char>(s[tok_start + 1]);
            const auto m0 = static_cast<unsigned char>(s[tok_start + 3]), m1 = static_cast<unsigned char>(s[tok_start + 4]);
            const auto s0 = static_cast<unsigned char>(s[tok_start + 6]), s1 = static_cast<unsigned char>(s[tok_start + 7]);
            if (s[tok_start + 2] == ':' && s[tok_start + 5] == ':' && h0 >= '0' && h0 <= '9' && h1 >= '0' && h1 <= '9' &&
                m0 >= '0' && m0 <= '9' && m1 >= '0' && m1 <= '9' && s0 >= '0' && s0 <= '9' && s1 >= '0' && s1 <= '9') {
                hour = (h0 - '0') * 10 + (h1 - '0'); minute = (m0 - '0') * 10 + (m1 - '0');
                second = (s0 - '0') * 10 + (s1 - '0');
                found_time = true; if (found_dom && found_month && found_year) break; continue;
            }
        }
        if (!found_dom) {
            const auto d0 = static_cast<unsigned char>(s[tok_start]);
            if (d0 >= '0' && d0 <= '9') {
                int d; bool valid_dom;
                if (len >= 2) { const auto d1 = static_cast<unsigned char>(s[tok_start + 1]);
                    if (d1 >= '0' && d1 <= '9') { const auto d2 = len > 2 ? static_cast<unsigned char>(s[tok_start + 2]) : 0u;
                        d = (d0 - '0') * 10 + (d1 - '0'); valid_dom = (len == 2 || d2 < '0' || d2 > '9'); }
                    else { d = d0 - '0'; valid_dom = true; }
                } else { d = d0 - '0'; valid_dom = true; }
                if (valid_dom) { day = d; found_dom = true; if (found_time && found_month && found_year) break; continue; }
            }
        }
        if (!found_month && len >= 3) {
            month = slim::common::utilities::month_abbr_to_int(s.substr(tok_start, len));
            if (month != -1) { found_month = true; if (found_time && found_dom && found_year) break; continue; }
        }
        if (!found_year && len >= 2) {
            const auto y0 = static_cast<unsigned char>(s[tok_start]), y1 = static_cast<unsigned char>(s[tok_start + 1]);
            if (y0 >= '0' && y0 <= '9' && y1 >= '0' && y1 <= '9') {
                year = (y0 - '0') * 10 + (y1 - '0');
                if (len >= 4) { const auto y2 = static_cast<unsigned char>(s[tok_start + 2]),
                                     y3 = static_cast<unsigned char>(s[tok_start + 3]);
                    if (y2 >= '0' && y2 <= '9' && y3 >= '0' && y3 <= '9')
                        year = year * 100 + (y2 - '0') * 10 + (y3 - '0');
                }
                found_year = true; if (found_time && found_dom && found_month) break; continue;
            }
        }
    }
    if (!found_time || !found_dom || !found_month || !found_year) return ErrorStatus::CookieExpiresInvalidFormat;
    if (year <= 99) year += (year >= 70) ? 1900 : 2000;
    if (year < 1601 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59) return ErrorStatus::CookieExpiresInvalidFormat;

    constexpr const char* wday_name[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    constexpr const char* mon_name[]  = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const int y_calc = year - (month < 2 ? 1 : 0);
    constexpr int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    const int wday = (y_calc + y_calc/4 - y_calc/100 + y_calc/400 + t[month] + day) % 7;
    expires.emplace();
    expires->reserve(29);
    expires->append(wday_name[wday]).append(", ");
    expires->push_back(static_cast<char>('0' + (day / 10)));
    expires->push_back(static_cast<char>('0' + (day % 10)));
    expires->push_back(' ');
    expires->append(mon_name[month]).push_back(' ');
    expires->push_back(static_cast<char>('0' + (year / 1000)));
    expires->push_back(static_cast<char>('0' + ((year / 100) % 10)));
    expires->push_back(static_cast<char>('0' + ((year / 10) % 10)));
    expires->push_back(static_cast<char>('0' + (year % 10)));
    expires->push_back(' ');
    expires->push_back(static_cast<char>('0' + (hour / 10)));
    expires->push_back(static_cast<char>('0' + (hour % 10)));
    expires->push_back(':');
    expires->push_back(static_cast<char>('0' + (minute / 10)));
    expires->push_back(static_cast<char>('0' + (minute % 10)));
    expires->push_back(':');
    expires->push_back(static_cast<char>('0' + (second / 10)));
    expires->push_back(static_cast<char>('0' + (second % 10)));
    expires->append(" GMT");
    return ErrorStatus::OK;
}

ErrorStatus validate_path(std::string_view& s) noexcept {
    slim::common::utilities::trim(s);
    if (s.empty()) return ErrorStatus::OK;
    if (s.front() != '/') return ErrorStatus::CookiePathMissingLeadingSlash;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x1F || uc == 0x7F || c == ';') return ErrorStatus::CookiePathInvalidChar;
    }
    return ErrorStatus::OK;
}

ErrorStatus validate_prefixes(std::string_view name, const std::optional<std::string>& domain,
                               const std::optional<std::string>& path, bool secure) noexcept {
    if (name.empty()) return ErrorStatus::CookieNameEmpty;
    const bool is_secure_prefix = name.starts_with("__Secure-");
    const bool is_host_prefix   = name.starts_with("__Host-");
    if (!is_secure_prefix && !is_host_prefix) return ErrorStatus::OK;
    if (!secure) return ErrorStatus::CookieNamePrefixRequiresSecure;
    if (is_host_prefix) {
        if (domain.has_value() && !domain->empty()) return ErrorStatus::CookieNameHostPrefixHasDomain;
        if (!path.has_value() || path.value() != "/") return ErrorStatus::CookieNameHostPrefixInvalidPath;
    }
    return ErrorStatus::OK;
}

constexpr ErrorStatus validate_max_age(std::uint_least64_t v) noexcept {
    return (v > static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()))
           ? ErrorStatus::CookieMaxAgeExceedsLimit : ErrorStatus::OK;
}

ErrorStatus get_max_age_value(std::string_view& s, std::optional<std::uint_least64_t>& v) noexcept {
    slim::common::utilities::trim(s);
    if (s.empty()) return ErrorStatus::CookieMaxAgeEmpty;
    bool is_negative = (!s.empty() && s[0] == '-');
    if(is_negative) s.remove_prefix(1);
    std::uint_least64_t temp_value = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), temp_value);
    if (ec != std::errc{}) return ErrorStatus::CookieMaxAgeInvalidFormat;
    if (ptr != s.data() + s.size()) return ErrorStatus::CookieMaxAgeTrailingChars;
    if(is_negative) { v = 0; return ErrorStatus::OK; }
    else { ErrorStatus e = validate_max_age(temp_value); if(e == ErrorStatus::OK) v = temp_value; return e; }
}

ErrorStatus validate_name(std::string_view& s) noexcept {
    slim::common::utilities::trim(s);
    if (s.empty()) return ErrorStatus::CookieNameEmpty;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x20 || uc >= 0x7F) return ErrorStatus::CookieNameInvalidChar;
        switch (c) {
            case '(': case ')': case '<': case '>': case '@': case ',': case ';': case ':':
            case '\\': case '"': case '/': case '[': case ']': case '?': case '=':
            case '{': case '}': return ErrorStatus::CookieNameInvalidChar;
        }
    }
    return ErrorStatus::OK;
}

ErrorStatus validate_partitioned(bool partitioned, bool secure,
                                  const std::optional<std::string>& same_site) noexcept {
    if(partitioned) {
        if(!secure) return ErrorStatus::CookiePartitionedRequiresSecure;
        if(!same_site || !slim::common::utilities::iequals(*same_site, "none")) return ErrorStatus::CookiePartitionedRequiresSameSiteNone;
    }
    return ErrorStatus::OK;
}

ErrorStatus validate_secure(const std::optional<std::string>& same_site, bool secure) noexcept {
    if(!same_site.has_value()) return ErrorStatus::OK;
    return (slim::common::utilities::iequals(same_site.value(), "none") && !secure) ? ErrorStatus::CookieSameSiteNoneRequiresSecure : ErrorStatus::OK;
}

ErrorStatus validate_same_site(std::string_view& s) noexcept {
    slim::common::utilities::trim(s);
    if (slim::common::utilities::iequals(s, "strict") || slim::common::utilities::iequals(s, "lax") ||
        slim::common::utilities::iequals(s, "none")) return ErrorStatus::OK;
    return ErrorStatus::CookieSameSiteInvalid;
}

ErrorStatus validate_value(std::string_view& s) noexcept {
    slim::common::utilities::trim(s);
    if (s.empty()) return ErrorStatus::OK;
    if (s.front() == '"') {
        if (s.size() < 2 || s.back() != '"') return ErrorStatus::CookieValueUnmatchedQuote;
        s.remove_prefix(1); s.remove_suffix(1);
    }
    for (char c : s) { if (!slim::common::utilities::is_cookie_char(c)) return ErrorStatus::CookieValueInvalidChar; }
    return ErrorStatus::OK;
}

} // namespace

Cookie::Cookie(std::string_view n, std::string_view v) {
    auto e = set_name(n);
    if (e != ErrorStatus::OK) throw(HttpHeaderException(e));
    e = set_value(v);
    if (e != ErrorStatus::OK) throw(HttpHeaderException(e));
}

bool Cookie::operator==(const Cookie& other) const noexcept {
    return name == other.name && domain == other.domain && path == other.path;
}

ErrorStatus Cookie::set_domain(std::string_view s) noexcept {
    auto e = validate_domain(s);
    if(e == ErrorStatus::OK) domain = std::string(s);
    return e;
}

ErrorStatus Cookie::set_expires(std::string_view s) noexcept {
    return validate_expires(s, expires);
}

ErrorStatus Cookie::set_max_age(std::uint_least64_t v) noexcept {
    auto e = validate_max_age(v);
    if(e == ErrorStatus::OK) max_age = v;
    return e;
}

ErrorStatus Cookie::set_max_age(std::string_view s) noexcept {
    return get_max_age_value(s, max_age);
}

ErrorStatus Cookie::set_name(std::string_view s) noexcept {
    auto e = validate_name(s);
    if (e == ErrorStatus::OK) name = std::string(s);
    return e;
}

ErrorStatus Cookie::set_path(std::string_view s) noexcept {
    auto e = validate_path(s);
    if(e == ErrorStatus::OK) path = std::string(s);
    return e;
}

ErrorStatus Cookie::set_value(std::string_view s) noexcept {
    auto e = validate_value(s);
    if (e == ErrorStatus::OK) value = std::string(s);
    return e;
}

ErrorStatus Cookie::set_same_site(std::string_view s) noexcept {
    auto e = validate_same_site(s);
    if(e == ErrorStatus::OK) same_site = std::string(s);
    return e;
}

ErrorStatus Cookie::set_httponly(std::string_view s) noexcept { return get_bool(s, httponly); }
ErrorStatus Cookie::set_partitioned(std::string_view s) noexcept { return get_bool(s, partitioned); }
ErrorStatus Cookie::set_secure(std::string_view s) noexcept { return get_bool(s, secure); }

ErrorStatus Cookie::validate() const noexcept {
    auto e = validate_secure(same_site, secure);
    if(e != ErrorStatus::OK) return e;
    e = validate_partitioned(partitioned, secure, same_site);
    if(e != ErrorStatus::OK) return e;
    e = validate_prefixes(name, domain, path, secure);
    if(e != ErrorStatus::OK) return e;
    return ErrorStatus::OK;
}

std::string Cookie::serialize() const {
    auto e = validate();
    if(e != ErrorStatus::OK) throw(HttpHeaderException(e));
    std::size_t total_size = 12 + 2 + name.size() + 1 + value.size();
    if (domain) total_size += 9 + domain->size();
    if (path) total_size += 7 + path->size();
    if (expires) total_size += 10 + expires->size();
    if (same_site) total_size += 11 + same_site->size();
    if (secure) total_size += 8;
    if (httponly) total_size += 10;
    if (partitioned) total_size += 13;
    if (max_age.has_value()) total_size += 10 + slim::common::utilities::count_digits(*max_age);
    if(total_size > 4096) throw(HttpHeaderException(ErrorStatus::CookieTooLarge));

    std::string result; result.reserve(total_size);
    result.append("Set-Cookie: ").append(name).append("=").append(value);
    if (domain) result.append("; Domain=").append(*domain);
    if (path) result.append("; Path=").append(*path);
    if (!max_age && expires) result.append("; Expires=").append(*expires);
    if (max_age.has_value()) {
        std::array<char, 20> buffer;
        auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), *max_age);
        if(ec != std::errc{}) throw(HttpHeaderException("Failed to serialize Max-Age"));
        result.append("; Max-Age=").append(buffer.data(), static_cast<std::size_t>(ptr - buffer.data()));
    }
    if (same_site) result.append("; SameSite=").append(*same_site);
    if (secure) result.append("; Secure");
    if (httponly) result.append("; HttpOnly");
    if (partitioned) result.append("; Partitioned");
    result.append("\r\n");
    return result;
}

} // namespace slim::common::http
