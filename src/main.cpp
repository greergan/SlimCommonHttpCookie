#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iterator> // used for error handling
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include <slim/common/http/cookie.h>

namespace {

struct AsciiTables {
    std::array<char, 256> to_lower{};
    std::array<bool, 256> is_alnum{};
    std::array<bool, 256> is_digit{};
    std::array<bool, 256> is_space{};
    std::array<bool, 256> is_cookie_char{};
    std::array<bool, 256> is_date_delimiter{};

    constexpr AsciiTables() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            to_lower[i] = (i >= 'A' && i <= 'Z') ? static_cast<char>(i + 32) : static_cast<char>(i);
            is_alnum[i] = (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9');
            is_digit[i] = (i >= '0' && i <= '9');
            is_space[i] = (i == ' ' || i == '\t' || i == '\r' || i == '\n' || i == '\v' || i == '\f');

            unsigned char uc = static_cast<unsigned char>(i);
            is_cookie_char[i] = (uc == 0x21)
                                || (uc >= 0x23 && uc <= 0x2B)
                                || (uc >= 0x2D && uc <= 0x3A)
                                || (uc >= 0x3C && uc <= 0x5B)
                                || (uc >= 0x5D && uc <= 0x7E);

            // RFC 6265 §5.1.1: delimiter = %x09 / %x20-2F / %x3B-40 / %x5B-60 / %x7B-7E
            is_date_delimiter[i] = (i == 0x09)
                            || (i >= 0x20 && i <= 0x2F)
                            || (i >= 0x3B && i <= 0x40)
                            || (i >= 0x5B && i <= 0x60)
                            || (i >= 0x7B && i <= 0x7E);
        }
    }
};

constexpr AsciiTables ascii{};

constexpr std::size_t count_digits(std::uint_least64_t n) noexcept {
    if (n == 0) return 1;
    std::size_t digits = 0;
    // Fast unrolled structural checks for typical cookie lifespans
    if (n >= 1000000000000000000ULL) { digits += 18; n /= 1000000000000000000ULL; }
    if (n >= 1000000000ULL)          { digits += 9;  n /= 1000000000ULL; }
    if (n >= 100000ULL)              { digits += 5;  n /= 100000ULL; }
    if (n >= 100ULL)                 { digits += 2;  n /= 100ULL; }
    if (n >= 10ULL)                  { digits += 1;  n /= 10ULL; }
    return digits + 1;
}

constexpr bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (ascii.to_lower[static_cast<unsigned char>(a[i])] != static_cast<unsigned char>(b[i])) return false;

    return true;
}

constexpr void trim(std::string_view& s) noexcept {
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.front())]) s.remove_prefix(1);
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.back())]) s.remove_suffix(1);
}

constexpr COOKIE::STATUS get_bool(std::string_view s, bool& b) noexcept {
    trim(s);
    if (iequals(s, "true")) {
        b = true;
        return COOKIE::STATUS::OK;
    }
    if (iequals(s, "false")) {
        b = false;
        return COOKIE::STATUS::OK;
    }
    return COOKIE::STATUS::INVALID_BOOLEAN;
}

constexpr int month_abbr_to_int(std::string_view s) noexcept {
    if (s.size() < 3) return -1;
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

constexpr bool validate_cookie_size(std::string_view name, std::string_view value) noexcept {
    // RFC 6265 recommends a limit of 4096 bytes for name + value
    return (name.size() + value.size()) <= 4096;
}

constexpr COOKIE::STATUS validate_domain(std::string_view& s) noexcept {
    trim(s);
    if (s.empty()) return COOKIE::STATUS::DOMAIN_EMPTY;

    // RFC 6265 §5.2.3 – strip a single leading dot, then re-check emptiness.
    const bool had_leading_dot = (s.front() == '.');
    if (had_leading_dot) s.remove_prefix(1);
    if (s.empty()) return COOKIE::STATUS::DOMAIN_BARE_DOT;

    if (s.back() == '.') return COOKIE::STATUS::DOMAIN_TRAILING_DOT;
    if (s.size() > 253) return COOKIE::STATUS::DOMAIN_TOO_LONG;

    for (std::string_view rem = s; !rem.empty();) {
        const auto dot = rem.find('.');
        const std::string_view label = rem.substr(0, dot);

        if (label.empty()) return COOKIE::STATUS::DOMAIN_LABEL_EMPTY;
        if (label.size() > 63) return COOKIE::STATUS::DOMAIN_LABEL_TOO_LONG;
        if (label.front() == '-' || label.back() == '-') return COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN;

        for (char c : label) {
            const unsigned char uc = static_cast<unsigned char>(c);
            if (dot == std::string_view::npos && ascii.is_digit[uc]) return COOKIE::STATUS::DOMAIN_NUMERIC_TLD;
            if (!ascii.is_alnum[uc] && c != '-') return COOKIE::STATUS::DOMAIN_INVALID_CHAR;
        }

        rem.remove_prefix(dot == std::string_view::npos ? rem.size() : dot + 1);
    }

    if (had_leading_dot) s = std::string_view(s.data() - 1, s.size() + 1);
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_expires(std::string_view& s, std::optional<std::string>& expires) noexcept {
	// RFC 6265 §5.1.1 cookie-date algorithm.
	// RFC 1123 "Sun, 06 Nov 1994 08:49:37 GMT"
	// RFC 850  "Weekday, DD-Mon-YY HH:MM:SS GMT"
	// ANSI C asctime "Wdy Mon DD HH:MM:SS YYYY"
	// if the date is valid it stores it as RFC 1123 format

	trim(s);

	bool found_time  = false, found_dom  = false;
	bool found_month = false, found_year = false;
	int  hour = 0, minute = 0, second = 0;
	int  day  = 0, month = 0,  year   = 0;

	std::size_t pos = 0;

	while (pos < s.size()) {
		// ── skip delimiters ────────────────────────────────────────
		while (pos < s.size() && ascii.is_date_delimiter[static_cast<unsigned char>(s[pos])]) ++pos;
		if (pos >= s.size()) break;

		// ── scan one token and extract fields ──────────────────────
		const std::size_t tok_start = pos;
		while (pos < s.size() && !ascii.is_date_delimiter[static_cast<unsigned char>(s[pos])])	++pos;
		const std::size_t len = pos - tok_start;

		// 1) time = 2DIGIT ':' 2DIGIT ':' 2DIGIT [*OCTET]
		if (!found_time && len >= 8) {
			const auto h0 = static_cast<unsigned char>(s[tok_start + 0]);
			const auto h1 = static_cast<unsigned char>(s[tok_start + 1]);
			const auto m0 = static_cast<unsigned char>(s[tok_start + 3]);
			const auto m1 = static_cast<unsigned char>(s[tok_start + 4]);
			const auto s0 = static_cast<unsigned char>(s[tok_start + 6]);
			const auto s1 = static_cast<unsigned char>(s[tok_start + 7]);
			if (s[tok_start + 2] == ':' && s[tok_start + 5] == ':' &&
			    h0 >= '0' && h0 <= '9' && h1 >= '0' && h1 <= '9' &&
			    m0 >= '0' && m0 <= '9' && m1 >= '0' && m1 <= '9' &&
			    s0 >= '0' && s0 <= '9' && s1 >= '0' && s1 <= '9') {
				hour   = (h0 - '0') * 10 + (h1 - '0');
				minute = (m0 - '0') * 10 + (m1 - '0');
				second = (s0 - '0') * 10 + (s1 - '0');
				found_time = true;
				if (found_dom & found_month & found_year) break;   // all done
				continue;
			}
		}

		// 2) day-of-month = 1*2DIGIT *( non-digit *OCTET )
		if (!found_dom) {
			const auto d0 = static_cast<unsigned char>(s[tok_start]);
			if (d0 >= '0' && d0 <= '9') {
				int d;
				bool valid_dom;
				if (len >= 2) {
					const auto d1 =	static_cast<unsigned char>(s[tok_start + 1]);
					if (d1 >= '0' && d1 <= '9') {
						// two leading digits — valid only if third
						// char (if any) is non-digit
						const auto d2 = len > 2 ? static_cast<unsigned char>(s[tok_start + 2]) : 0u; d = (d0 - '0') * 10 + (d1 - '0');
						valid_dom = (len == 2 || d2 < '0' || d2 > '9');
					} else {
						// digit immediately followed by
						// non-digit: e.g. "1st"
						d = d0 - '0';
						valid_dom = true;
					}
				} else {
					// len == 1: bare single digit
					d = d0 - '0';
					valid_dom = true;
				}
				if (valid_dom) {
					day = d;
					found_dom = true;
					if (found_time & found_month & found_year) break;
					continue;
				}
			}
		}

		// 3) month = ( "jan" | "feb" | … ) [*OCTET]
		if (!found_month && len >= 3) {
			month = month_abbr_to_int(s.substr(tok_start, len));
			if (month != -1) {
				found_month = true;
				if (found_time & found_dom & found_year) break;
				continue;
			}
		}

		// 4) year = 2DIGIT [2DIGIT] *OCTET
		if (!found_year && len >= 2) {
			const auto y0 = static_cast<unsigned char>(s[tok_start]);
			const auto y1 = static_cast<unsigned char>(s[tok_start + 1]);
			if (y0 >= '0' && y0 <= '9' && y1 >= '0' && y1 <= '9') {
				year = (y0 - '0') * 10 + (y1 - '0');
				if (len >= 4) {
					const auto y2 = static_cast<unsigned char>(s[tok_start + 2]);
					const auto y3 =	static_cast<unsigned char>(s[tok_start + 3]);
					if (y2 >= '0' && y2 <= '9' && y3 >= '0' && y3 <= '9') year = year * 100 + (y2 - '0') * 10 + (y3 - '0');
				}
				found_year = true;
				if (found_time & found_dom & found_month) break;
				continue;
			}
		}
		// unrecognised token – ignored per RFC
	}

	// ── all four fields mandatory ───────────────────────────────────
	if (!found_time | !found_dom | !found_month | !found_year) return COOKIE::STATUS::EXPIRES_INVALID_FORMAT;

	// ── two-digit year expansion (§5.1.1 steps 6–7) ────────────────
	if (year <= 99) year += (year >= 70) ? 1900 : 2000;

	// ── range checks (§5.1.1 steps 8–13) ───────────────────────────
	if (   year   < 1601          // step 8
	    || day    < 1  || day    > 31   // step 10
	    || hour        > 23             // step 11
	    || minute      > 59             // step 12
	    || second      > 59)            // step 13
		return COOKIE::STATUS::EXPIRES_INVALID_FORMAT;

	// ── format into RFC 1123 ───────────────────────────────────────
	constexpr const char* wday_name[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	constexpr const char* mon_name[]  = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	// Tomohiko Sakamoto's algorithm for day of week
	const int y_calc = year - (month < 2 ? 1 : 0);
	constexpr int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	const int wday = (y_calc + y_calc/4 - y_calc/100 + y_calc/400 + t[month] + day) % 7;

	expires.emplace();
	expires->reserve(29); // Exact length of "Wdy, DD Mon YYYY HH:MM:SS GMT"

	expires->append(wday_name[wday]).append(", ");

	expires->push_back(static_cast<char>('0' + (day / 10)));
	expires->push_back(static_cast<char>('0' + (day % 10)));

	expires->append(" ").append(mon_name[month]).append(" ");

	expires->push_back(static_cast<char>('0' + (year / 1000)));
	expires->push_back(static_cast<char>('0' + ((year / 100) % 10)));
	expires->push_back(static_cast<char>('0' + ((year / 10) % 10)));
	expires->push_back(static_cast<char>('0' + (year % 10)));

	expires->append(" ");
	expires->push_back(static_cast<char>('0' + (hour / 10)));
	expires->push_back(static_cast<char>('0' + (hour % 10)));

	expires->append(":");
	expires->push_back(static_cast<char>('0' + (minute / 10)));
	expires->push_back(static_cast<char>('0' + (minute % 10)));

	expires->append(":");
	expires->push_back(static_cast<char>('0' + (second / 10)));
	expires->push_back(static_cast<char>('0' + (second % 10)));

	expires->append(" GMT");

	return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_path(std::string_view& s) noexcept {
    trim(s);
    if (s.empty()) return COOKIE::STATUS::OK;
    if (s.front() != '/') return COOKIE::STATUS::PATH_MISSING_LEADING_SLASH;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x1F || uc == 0x7F || c == ';') return COOKIE::STATUS::PATH_INVALID_CHAR;
    }
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_prefixes(std::string_view name, const std::optional<std::string>& domain,
        const std::optional<std::string>& path, bool secure) noexcept {
    if (name.empty()) return COOKIE::STATUS::NAME_EMPTY;
    const bool is_secure_prefix = name.starts_with("__Secure-");
    const bool is_host_prefix   = name.starts_with("__Host-");
    if (!is_secure_prefix && !is_host_prefix) return COOKIE::STATUS::OK;
    if (!secure) return COOKIE::STATUS::NAME_PREFIX_REQUIRES_SECURE;
    if (is_host_prefix) {
        if (domain.has_value() && !domain->empty()) return COOKIE::STATUS::NAME_HOST_PREFIX_HAS_DOMAIN;
        if (!path.has_value() || path.value() != "/") return COOKIE::STATUS::NAME_HOST_PREFIX_INVALID_PATH;
    }
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_max_age(std::uint_least64_t v) noexcept {
    return (v > static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()))
        ? COOKIE::STATUS::MAX_AGE_EXCEEDS_LIMIT : COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS get_max_age_value(std::string_view& s, std::optional<std::uint_least64_t>& v) noexcept {
    trim(s);
    if (s.empty()) return COOKIE::STATUS::MAX_AGE_EMPTY;
    bool is_negative = (!s.empty() && s[0] == '-');
    if(is_negative) s.remove_prefix(1);

    std::uint_least64_t temp_value = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), temp_value);

    if (ec != std::errc{}) return COOKIE::STATUS::MAX_AGE_INVALID_FORMAT;
    if (ptr != s.data() + s.size()) return COOKIE::STATUS::MAX_AGE_TRAILING_CHARS;

    if(is_negative) {
        v = 0;
        return COOKIE::STATUS::OK;
    }
    else {
        COOKIE::STATUS e = validate_max_age(temp_value);
        if(e == COOKIE::STATUS::OK)  v = temp_value;
        return e;
    }

    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_name(std::string_view& s) noexcept {
    trim(s);
    if (s.empty()) return COOKIE::STATUS::NAME_EMPTY;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x20 || uc >= 0x7F) return COOKIE::STATUS::NAME_INVALID_CHAR;
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

constexpr COOKIE::STATUS validate_partitioned(bool partitioned, bool secure,
    const std::optional<std::string>& same_site) noexcept {
    if(partitioned) {
        if(!secure) return COOKIE::STATUS::PARTITIONED_REQUIRES_SECURE;
        if(!same_site || !iequals(*same_site, "none")) return COOKIE::STATUS::PARTITIONED_REQUIRES_SAME_SITE_NONE;
    }
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_secure(const std::optional<std::string>& same_site, bool secure) noexcept {
    if(!same_site.has_value()) return COOKIE::STATUS::OK;
    return (iequals(same_site.value(), "none") && !secure) ? COOKIE::STATUS::SAMESITE_NONE_REQUIRES_SECURE : COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_same_site(std::string_view& s) noexcept {
    trim(s);
    if (iequals(s, "strict") || iequals(s, "lax") || iequals(s, "none")) return COOKIE::STATUS::OK;
    return COOKIE::STATUS::SAMESITE_INVALID;
}

constexpr COOKIE::STATUS validate_value(std::string_view& s) noexcept {
    trim(s);
    if (s.empty()) return COOKIE::STATUS::OK;
    if (s.front() == '"') {
        if (s.size() < 2 || s.back() != '"') return COOKIE::STATUS::VALUE_UNMATCHED_QUOTE;
        s.remove_prefix(1);
        s.remove_suffix(1);
    }
    for (char c : s) {
        if (!ascii.is_cookie_char[static_cast<unsigned char>(c)]) return COOKIE::STATUS::VALUE_INVALID_CHAR;
    }
    return COOKIE::STATUS::OK;
}

} // namespace

COOKIE::STATUS slim::common::http::Cookie::set_domain(std::string_view s) noexcept {
    auto e = ::validate_domain(s);
    if(e == COOKIE::STATUS::OK) domain = std::string(s);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::set_expires(std::string_view s) noexcept {
    return ::validate_expires(s, expires);
}

COOKIE::STATUS slim::common::http::Cookie::set_max_age(std::uint_least64_t v) noexcept {
    auto e = ::validate_max_age(v);
    if(e == COOKIE::STATUS::OK) max_age = v;
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::set_max_age(std::string_view s) noexcept {
    return ::get_max_age_value(s, max_age);
}

COOKIE::STATUS slim::common::http::Cookie::set_name(std::string_view s) noexcept {
    auto e = ::validate_name(s);
    if (e == COOKIE::STATUS::OK) {
        if (!::validate_cookie_size(s, value)) return COOKIE::STATUS::COOKIE_TOO_LARGE;
        name = std::string(s);
    }
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::set_path(std::string_view s) noexcept {
    auto e = ::validate_path(s);
    if(e == COOKIE::STATUS::OK) path = std::string(s);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::set_value(std::string_view s) noexcept {
    auto e = ::validate_value(s);
    if (e == COOKIE::STATUS::OK) {
        if (!::validate_cookie_size(name, s)) return COOKIE::STATUS::COOKIE_TOO_LARGE;
        value = std::string(s);
    }
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::set_same_site(std::string_view s) noexcept {
    auto e = ::validate_same_site(s);
    if(e == COOKIE::STATUS::OK) same_site = std::string(s);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::set_httponly(std::string_view s) noexcept {
    return get_bool(s, httponly);
}

COOKIE::STATUS slim::common::http::Cookie::set_partitioned(std::string_view s) noexcept {
    return ::get_bool(s, partitioned);
}

COOKIE::STATUS slim::common::http::Cookie::set_secure(std::string_view s) noexcept {
    return ::get_bool(s, secure);
}

COOKIE::STATUS slim::common::http::Cookie::validate() const noexcept {
    auto e = ::validate_secure(same_site, secure);
    if(e != COOKIE::STATUS::OK) return e;

    e = ::validate_partitioned(partitioned, secure, same_site);
    if(e != COOKIE::STATUS::OK) return e;

    e = ::validate_prefixes(name, domain, path, secure);
    if(e != COOKIE::STATUS::OK) return e;

    return COOKIE::STATUS::OK;
}

std::string slim::common::http::Cookie::serialize() const {
    auto e = validate();
    if(e != COOKIE::STATUS::OK) throw(CookieException(e));

    std::size_t total_size = 12;                           // "Set-Cookie: "
    total_size += 2;                                       // end of cookie string
    total_size += name.size() + 1 + value.size();          // "name=value"

    if (domain)      total_size += 9 + domain->size();       // "; Domain="
    if (path)        total_size += 7 + path->size();         // "; Path="
    if (expires)     total_size += 10 + expires->size();     // "; Expires="
    if (same_site)   total_size += 11 + same_site->size();   // "; SameSite="
    if (secure)      total_size += 8;                        // "; Secure"
    if (httponly)    total_size += 10;                       // "; HttpOnly"
    if (partitioned) total_size += 13;                       // "; Partitioned"

    std::size_t max_age_digits = 0;
    if (max_age.has_value()) {
        max_age_digits = ::count_digits(*max_age);
        total_size += 10 + max_age_digits;                     // "; Max-Age=" + digits
    }

    std::string result;
    result.reserve(total_size);
    result.append("Set-Cookie: ");
    result.append(name).append("=").append(value);

    if (domain)              result.append("; Domain=").append(*domain);
    if (path)                result.append("; Path=").append(*path);
    if (!max_age && expires) result.append("; Expires=").append(*expires);

    if (max_age.has_value()) {
        std::array<char, 20> buffer;
        auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), *max_age);

        if(ec != std::errc{})
            throw(CookieException("Failed to serialize Max-Age: " + std::make_error_code(ec).message()));

        result.append("; Max-Age=");
        result.append(buffer.data(), static_cast<std::size_t>(ptr - buffer.data()));
    }

    if (same_site)   result.append("; SameSite=").append(*same_site);
    if (secure)      result.append("; Secure");
    if (httponly)    result.append("; HttpOnly");
    if (partitioned) result.append("; Partitioned");

    result.append("\r\n");
    return result;
}
