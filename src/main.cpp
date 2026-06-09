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
    std::array<bool, 256> is_date_delimiter{};

    constexpr AsciiTables() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            to_lower[i] = (i >= 'A' && i <= 'Z') ? static_cast<char>(i + 32) : static_cast<char>(i);
            is_alnum[i] = (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9');
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

constexpr bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (ascii.to_lower[static_cast<unsigned char>(a[i])] != static_cast<unsigned char>(b[i]))
            return false;
    }
    return true;
}

constexpr bool istarts_with(std::string_view s, std::string_view prefix) noexcept {
    if (s.size() < prefix.size()) return false;
    return iequals(s.substr(0, prefix.size()), prefix);
}

constexpr std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.front())])
        s.remove_prefix(1);
    while (!s.empty() && ascii.is_space[static_cast<unsigned char>(s.back())])
        s.remove_suffix(1);
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

constexpr COOKIE::STATUS validate_domain(std::string_view s) noexcept {
    if (s.empty()) return COOKIE::STATUS::DOMAIN_EMPTY;
    if (s.front() == '.') s.remove_prefix(1);
    if (s.empty()) return COOKIE::STATUS::DOMAIN_BARE_DOT;
    if (s.back() == '.') return COOKIE::STATUS::DOMAIN_TRAILING_DOT;
    if (s.size() > 253) return COOKIE::STATUS::DOMAIN_TOO_LONG;

    size_t label_start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == '.') {
            std::string_view label = s.substr(label_start, i - label_start);
            if (label.empty()) return COOKIE::STATUS::DOMAIN_LABEL_EMPTY;
            if (label.size() > 63) return COOKIE::STATUS::DOMAIN_LABEL_TOO_LONG;
            if (label.front() == '-' || label.back() == '-') return COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN;
            for (char c : label) {
                if (!ascii.is_alnum[static_cast<unsigned char>(c)] && c != '-')
                    return COOKIE::STATUS::DOMAIN_INVALID_CHAR;
            }
            label_start = i + 1;
        }
    }
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_expires(std::string_view s) noexcept {
	// RFC 6265 §5.1.1 cookie-date algorithm.
	// Tokenises on the RFC delimiter set; tries each token as
	// time → day-of-month → month → year (first-match semantics,
	// one slot per field type); then validates all ranges.

	bool found_time  = false, found_dom  = false;
	bool found_month = false, found_year = false;
	int  hour = 0, minute = 0, second = 0;
	int  day  = 0, year   = 0;

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
					const auto d1 =
						static_cast<unsigned char>(s[tok_start + 1]);
					if (d1 >= '0' && d1 <= '9') {
						// two leading digits — valid only if third
						// char (if any) is non-digit
						const auto d2 = len > 2 ?
							static_cast<unsigned char>(
								s[tok_start + 2]) : 0u;
						d = (d0 - '0') * 10 + (d1 - '0');
						valid_dom = (len == 2 || d2 < '0' ||
							d2 > '9');
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
			const int m = month_abbr_to_int(s.substr(tok_start, len));
			if (m != -1) {
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
					if (y2 >= '0' && y2 <= '9' && y3 >= '0' && y3 <= '9')
						year = year * 100 + (y2 - '0') * 10 + (y3 - '0');
				}
				found_year = true;
				if (found_time & found_dom & found_month) break;
				continue;
			}
		}
		// unrecognised token – ignored per RFC
	}

	// ── all four fields mandatory ───────────────────────────────────
	if (!found_time | !found_dom | !found_month | !found_year)
		return COOKIE::STATUS::EXPIRES_INVALID_FORMAT;

	// ── two-digit year expansion (§5.1.1 steps 6–7) ────────────────
	if (year <= 99)
		year += (year >= 70) ? 1900 : 2000;

	// ── range checks (§5.1.1 steps 8–13) ───────────────────────────
	if (   year   < 1601          // step 8
	    || day    < 1  || day    > 31   // step 10
	    || hour        > 23             // step 11
	    || minute      > 59             // step 12
	    || second      > 59)            // step 13
		return COOKIE::STATUS::EXPIRES_INVALID_FORMAT;
	// month is always 1-12 by construction (month_abbr_to_int guarantees it)

	return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_path(std::string_view s) noexcept {
    if (s.empty()) return COOKIE::STATUS::OK;
    if (s.front() != '/') return COOKIE::STATUS::PATH_MISSING_LEADING_SLASH;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x1F || uc == 0x7F || c == ';') return COOKIE::STATUS::PATH_INVALID_CHAR;
    }
    return COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_max_age(std::uint_least64_t v) noexcept {
    return (v > static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max())) ? COOKIE::STATUS::MAX_AGE_EXCEEDS_LIMIT : COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS get_max_age_value(std::string_view s, std::optional<std::uint_least64_t>& v) noexcept {
    std::string_view trimmed = trim(s);
    if (trimmed.empty()) return COOKIE::STATUS::MAX_AGE_EMPTY;
    std::uint_least64_t temp_value = 0;
    auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), temp_value);

    if (ec != std::errc{})
        return COOKIE::STATUS::MAX_AGE_INVALID_FORMAT;

    if (ptr != trimmed.data() + trimmed.size())
        return COOKIE::STATUS::MAX_AGE_TRAILING_CHARS;

    COOKIE::STATUS e = validate_max_age(temp_value);
    if(e == COOKIE::STATUS::OK) v = temp_value;
    return e;
}

constexpr COOKIE::STATUS validate_name(std::string_view s) noexcept {
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

constexpr COOKIE::STATUS validate_partitioned(bool secure, bool partitioned) noexcept {
    return (partitioned && !secure) ? COOKIE::STATUS::PARTITIONED_REQUIRES_SECURE : COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_secure(std::string_view same_site, bool secure) noexcept {
    return (iequals(same_site, "none") && !secure) ? COOKIE::STATUS::SAMESITE_NONE_REQUIRES_SECURE : COOKIE::STATUS::OK;
}

constexpr COOKIE::STATUS validate_same_site(std::string_view s) noexcept {
    if (iequals(s, "strict") || iequals(s, "lax") || iequals(s, "none")) return COOKIE::STATUS::OK;
    return COOKIE::STATUS::SAMESITE_INVALID;
}

constexpr COOKIE::STATUS validate_value(std::string_view s) noexcept {
    if (s.empty()) return COOKIE::STATUS::OK;
    if (s.front() == '"') {
        if (s.size() < 2 || s.back() != '"') return COOKIE::STATUS::VALUE_UNMATCHED_QUOTE;
        s = s.substr(1, s.size() - 2);
    }
    for (char c : s) {
        if (!ascii.is_cookie_char[static_cast<unsigned char>(c)]) return COOKIE::STATUS::VALUE_INVALID_CHAR;
    }
    return COOKIE::STATUS::OK;
}

} // namespace

COOKIE::STATUS slim::common::http::Cookie::set_domain(std::string_view s) noexcept {
    std::string_view trimmed = trim(s);
    auto e = validate_domain(trimmed);
    if(e == COOKIE::STATUS::OK) domain = std::string(trimmed);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_domain(std::string_view s) noexcept {
    return validate_domain(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_expires(std::string_view s) noexcept {
    std::string_view trimmed = trim(s);
    auto e = validate_expires(trimmed);
    if(e == COOKIE::STATUS::OK) expires = std::string(trimmed);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_expires(std::string_view s) noexcept {
    return validate_expires(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_max_age(std::string_view s) noexcept {
    return get_max_age_value(s, max_age);
}

COOKIE::STATUS slim::common::http::Cookie::valid_max_age(std::uint_least64_t v) noexcept {
    return validate_max_age(v);
}

COOKIE::STATUS slim::common::http::Cookie::set_name(std::string_view s) noexcept {
    std::string_view trimmed = trim(s);
    auto e = validate_name(trimmed);
    if(e == COOKIE::STATUS::OK) name = std::string(trimmed);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_name(std::string_view s) noexcept {
    return validate_name(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_path(std::string_view s) noexcept {
    std::string_view trimmed = trim(s);
    auto e = validate_path(trimmed);
    if(e == COOKIE::STATUS::OK) path = std::string(trimmed);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_path(std::string_view s) noexcept {
    return validate_path(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_value(std::string_view s) noexcept {
    std::string_view trimmed = trim(s);
    auto e = validate_value(trimmed);
    if(e == COOKIE::STATUS::OK) value = std::string(trimmed);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_value(std::string_view s) noexcept {
    return validate_value(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_same_site(std::string_view s) noexcept {
    std::string_view trimmed = trim(s);
    auto e = validate_same_site(trimmed);
    if(e == COOKIE::STATUS::OK) same_site = std::string(trimmed);
    return e;
}

COOKIE::STATUS slim::common::http::Cookie::valid_same_site(std::string_view s) noexcept {
    return validate_same_site(trim(s));
}

COOKIE::STATUS slim::common::http::Cookie::set_httponly(std::string_view s) noexcept {
    return get_bool(s, httponly);
}

COOKIE::STATUS slim::common::http::Cookie::set_partitioned(std::string_view s) noexcept {
    return get_bool(s, partitioned);
}

COOKIE::STATUS slim::common::http::Cookie::valid_partitioned(const bool secure, const bool partitioned) noexcept {
    return ::validate_partitioned(secure, partitioned);
}

COOKIE::STATUS slim::common::http::Cookie::validate_partitioned() noexcept {
    return ::validate_partitioned(secure, partitioned);
}

COOKIE::STATUS slim::common::http::Cookie::set_secure(std::string_view s) noexcept {
    return get_bool(s, secure);
}

COOKIE::STATUS slim::common::http::Cookie::valid_secure(std::string_view same_site, const bool secure) noexcept {
    return ::validate_secure(trim(same_site), secure);
}

COOKIE::STATUS slim::common::http::Cookie::validate_secure() noexcept {
    return ::validate_secure(same_site.value_or(""), secure);
}

COOKIE::STATUS slim::common::http::Cookie::validate_prefixes() const noexcept {
    if (name.empty()) return COOKIE::STATUS::NAME_EMPTY;
    const bool is_secure_prefix = ::istarts_with(name, "__secure-");
    const bool is_host_prefix = ::istarts_with(name, "__host-");
    if (!is_secure_prefix && !is_host_prefix) return COOKIE::STATUS::OK;
    if (!secure) return COOKIE::STATUS::NAME_PREFIX_REQUIRES_SECURE;
    if (is_host_prefix) {
        if (domain.has_value() && !domain->empty()) return COOKIE::STATUS::NAME_HOST_PREFIX_HAS_DOMAIN;
        if (!path.has_value() || path.value() != "/") return COOKIE::STATUS::NAME_HOST_PREFIX_INVALID_PATH;
    }
    return COOKIE::STATUS::OK;
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
