#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/cookie.h>

// ---------------------------------------------------------------------------
// set_name
// ---------------------------------------------------------------------------

TEST_CASE("set_name accepts a valid token") {
    slim::common::http::Cookie c;
    auto r = c.set_name("session_id");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_name() == "session_id");
}

TEST_CASE("set_name rejects an empty string") {
    slim::common::http::Cookie c;
    auto r = c.set_name("");
    REQUIRE(r.has_error());
}

TEST_CASE("set_name rejects a name containing a separator character") {
    slim::common::http::Cookie c;
    auto r = c.set_name("bad=name");
    REQUIRE(r.has_error());
}

TEST_CASE("set_name rejects a name with a space") {
    slim::common::http::Cookie c;
    auto r = c.set_name("bad name");
    REQUIRE(r.has_error());
}

TEST_CASE("set_name rejects a name with a control character") {
    slim::common::http::Cookie c;
    auto r = c.set_name("bad\x01name");
    REQUIRE(r.has_error());
}

TEST_CASE("set_name rejects a name containing a colon") {
    slim::common::http::Cookie c;
    auto r = c.set_name("bad:name");
    REQUIRE(r.has_error());
}

TEST_CASE("set_name trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    auto r = c.set_name("  session_id  ");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_name() == "session_id");
}

// ---------------------------------------------------------------------------
// set_value
// ---------------------------------------------------------------------------

TEST_CASE("set_value accepts a plain ASCII value") {
    slim::common::http::Cookie c;
    auto r = c.set_value("abc123");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_value() == "abc123");
}

TEST_CASE("set_value accepts a properly double-quoted value") {
    slim::common::http::Cookie c;
    auto r = c.set_value("\"abc123\"");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("set_value rejects an unmatched leading double quote") {
    slim::common::http::Cookie c;
    auto r = c.set_value("\"abc");
    REQUIRE(r.has_error());
}

TEST_CASE("set_value rejects a value with a space character") {
    slim::common::http::Cookie c;
    auto r = c.set_value("bad value");
    REQUIRE(r.has_error());
}

TEST_CASE("set_value rejects a value with a semicolon") {
    slim::common::http::Cookie c;
    auto r = c.set_value("bad;value");
    REQUIRE(r.has_error());
}

TEST_CASE("set_value rejects a value with a DEL character") {
    slim::common::http::Cookie c;
    auto r = c.set_value("bad\x7Fvalue");
    REQUIRE(r.has_error());
}

TEST_CASE("set_value trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    auto r = c.set_value("  abc123  ");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_value() == "abc123");
}

// ---------------------------------------------------------------------------
// set_path
// ---------------------------------------------------------------------------

TEST_CASE("set_path accepts the root path") {
    slim::common::http::Cookie c;
    auto r = c.set_path("/");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_path() == "/");
}

TEST_CASE("set_path accepts a multi-segment path") {
    slim::common::http::Cookie c;
    auto r = c.set_path("/api/v1/resource");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_path() == "/api/v1/resource");
}

TEST_CASE("set_path rejects a path that does not start with slash") {
    slim::common::http::Cookie c;
    auto r = c.set_path("api/v1");
    REQUIRE(r.has_error());
}

TEST_CASE("set_path rejects an empty path") {
    slim::common::http::Cookie c;
    auto r = c.set_path("");
    REQUIRE(r.has_error());
}

TEST_CASE("set_path rejects a path containing a semicolon") {
    slim::common::http::Cookie c;
    auto r = c.set_path("/bad;path");
    REQUIRE(r.has_error());
}

TEST_CASE("set_path rejects a path with a control character") {
    slim::common::http::Cookie c;
    auto r = c.set_path("/bad\x1Fpath");
    REQUIRE(r.has_error());
}

TEST_CASE("set_path trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    auto r = c.set_path("  /api/v1  ");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_path() == "/api/v1");
}

// ---------------------------------------------------------------------------
// set_domain
// ---------------------------------------------------------------------------

TEST_CASE("set_domain accepts a simple hostname") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("example.com");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_domain() == "example.com");
}

TEST_CASE("set_domain accepts a domain with a leading dot") {
    slim::common::http::Cookie c;
    auto r = c.set_domain(".example.com");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("set_domain accepts a subdomain") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("sub.example.com");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("set_domain rejects an empty string") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("");
    REQUIRE(r.has_error());
}

TEST_CASE("set_domain rejects a bare dot") {
    slim::common::http::Cookie c;
    auto r = c.set_domain(".");
    REQUIRE(r.has_error());
}

TEST_CASE("set_domain rejects a domain with a trailing dot") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("example.com.");
    REQUIRE(r.has_error());
}

TEST_CASE("set_domain rejects a label starting with a hyphen") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("-bad.example.com");
    REQUIRE(r.has_error());
}

TEST_CASE("set_domain rejects a label ending with a hyphen") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("bad-.example.com");
    REQUIRE(r.has_error());
}

TEST_CASE("set_domain rejects a domain exceeding 253 characters") {
    slim::common::http::Cookie c;
    // 63-char label repeated four times with dots = 255 chars
    std::string long_domain(63, 'a');
    long_domain += ".";
    long_domain += std::string(63, 'b');
    long_domain += ".";
    long_domain += std::string(63, 'c');
    long_domain += ".";
    long_domain += std::string(63, 'd');  // total = 4*63 + 3 dots = 255
    auto r = c.set_domain(long_domain);
    REQUIRE(r.has_error());
}

TEST_CASE("set_domain rejects a label with an invalid character") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("bad_label.example.com");
    REQUIRE(r.has_error());
}

TEST_CASE("set_domain trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("  example.com  ");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_domain() == "example.com");
}

// ---------------------------------------------------------------------------
// set_expires
// ---------------------------------------------------------------------------

TEST_CASE("set_expires accepts an RFC 1123 date") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("Thu, 01 Jan 2099 00:00:00 GMT");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

TEST_CASE("set_expires accepts an RFC 850 date") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("Thursday, 01-Jan-99 00:00:00 GMT");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("set_expires accepts an ANSI C asctime date") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("Thu Jan  1 00:00:00 2099");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("set_expires rejects a malformed date string") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("not-a-date");
    REQUIRE(r.has_error());
}

TEST_CASE("set_expires rejects an ISO 8601 date") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("2099-01-01T00:00:00Z");
    REQUIRE(r.has_error());
}

TEST_CASE("set_expires trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("  Thu, 01 Jan 2099 00:00:00 GMT  ");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

// ---------------------------------------------------------------------------
// set_max_age
// ---------------------------------------------------------------------------

TEST_CASE("set_max_age accepts zero") {
    slim::common::http::Cookie c;
    auto r = c.set_max_age("0");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_max_age() == 0u);
}

TEST_CASE("set_max_age accepts a positive integer") {
    slim::common::http::Cookie c;
    auto r = c.set_max_age("3600");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_max_age() == 3600u);
}

TEST_CASE("set_max_age accepts the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr std::uint_least64_t max_val =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    auto r = c.set_max_age(std::to_string(max_val));
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_max_age() == max_val);
}

TEST_CASE("set_max_age rejects a value exceeding the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr std::uint_least64_t over_max =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()) + 1;
    auto r = c.set_max_age(std::to_string(over_max));
    REQUIRE(r.has_error());
}

TEST_CASE("set_max_age rejects a negative integer") {
    slim::common::http::Cookie c;
    auto r = c.set_max_age("-1");
    REQUIRE(r.has_error());
}

TEST_CASE("set_max_age rejects a non-numeric string") {
    slim::common::http::Cookie c;
    auto r = c.set_max_age("abc");
    REQUIRE(r.has_error());
}

TEST_CASE("set_max_age rejects a value with trailing characters") {
    slim::common::http::Cookie c;
    auto r = c.set_max_age("3600x");
    REQUIRE(r.has_error());
}

TEST_CASE("set_max_age rejects an empty string") {
    slim::common::http::Cookie c;
    auto r = c.set_max_age("");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// set_same_site
// ---------------------------------------------------------------------------

TEST_CASE("set_same_site accepts Strict") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("Strict");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site accepts Lax") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("Lax");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_same_site() == "Lax");
}

TEST_CASE("set_same_site accepts None") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("None");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_same_site() == "None");
}


TEST_CASE("set_same_site is case-insensitive strict") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("strict");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_same_site() == "strict");
}

TEST_CASE("set_same_site is case-insensitive lax") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("lax");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_same_site() == "lax");
}

TEST_CASE("set_same_site is case-insensitive none") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("none");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_same_site() == "none");
}

TEST_CASE("set_same_site trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("  Strict  ");
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site rejects an unrecognised value") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("always");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// set_httponly
// ---------------------------------------------------------------------------

TEST_CASE("set_httponly accepts true") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_httponly(std::string_view{"true"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts TRUE") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_httponly(std::string_view{"TRUE"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts false") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_httponly(std::string_view{"false"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly accepts FALSE") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_httponly(std::string_view{"FALSE"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_httponly(std::string_view{"yes"});
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// set_secure
// ---------------------------------------------------------------------------

TEST_CASE("set_secure accepts true") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_secure(std::string_view{"true"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts TRUE") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_secure(std::string_view{"TRUE"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts false") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_secure(std::string_view{"false"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure accepts FALSE") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_secure(std::string_view{"FALSE"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_secure(std::string_view{"1"});
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// set_partitioned
// ---------------------------------------------------------------------------

TEST_CASE("set_partitioned accepts true") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_partitioned(std::string_view{"true"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_partitioned() == true);
}

TEST_CASE("set_partitioned accepts TRUE") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_partitioned(std::string_view{"TRUE"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_partitioned() == true);
}

TEST_CASE("set_partitioned accepts false") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_partitioned(std::string_view{"false"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned accepts FALSE") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_partitioned(std::string_view{"FALSE"});
    REQUIRE_FALSE(r.has_error());
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    slim::ErrorInfo r = c.set_partitioned(std::string_view{"no"});
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// validate_secure / validate_partitioned
// ---------------------------------------------------------------------------

TEST_CASE("validate_secure returns no error when SameSite is Strict and Secure is false") {
    slim::common::http::Cookie c;
    c.set_same_site("Strict");
    slim::ErrorInfo r = c.validate_secure();
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("validate_secure returns error when SameSite is None and Secure is false") {
    slim::common::http::Cookie c;
    c.set_secure("true");
    c.set_same_site("None");
    c.set_secure("false");  // force via internal state; validate_secure re-checks consistency
    slim::ErrorInfo r = c.validate_secure();
    REQUIRE(r.has_error());
}

TEST_CASE("validate_partitioned returns no error when Secure is true and Partitioned is true") {
    slim::common::http::Cookie c;
    c.set_secure("true");
    c.set_partitioned("true");
    slim::ErrorInfo r = c.validate_partitioned();
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("validate_partitioned returns error when Partitioned is true but Secure is false") {
    slim::common::http::Cookie c;
    c.set_partitioned("true");
    slim::ErrorInfo r = c.validate_partitioned();
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_name (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_name accepts a valid token") {
    auto r = slim::common::http::Cookie::valid_name("session_id");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_name rejects an empty string") {
    auto r = slim::common::http::Cookie::valid_name("");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_name rejects a name containing a separator character") {
    auto r = slim::common::http::Cookie::valid_name("bad=name");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_name rejects a name with a space") {
    auto r = slim::common::http::Cookie::valid_name("bad name");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_name rejects a name with a control character") {
    auto r = slim::common::http::Cookie::valid_name("bad\x01name");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_name rejects a name containing a colon") {
    auto r = slim::common::http::Cookie::valid_name("bad:name");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_value (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_value accepts a plain ASCII value") {
    auto r = slim::common::http::Cookie::valid_value("abc123");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_value accepts a properly double-quoted value") {
    auto r = slim::common::http::Cookie::valid_value("\"abc123\"");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_value rejects an unmatched leading double quote") {
    auto r = slim::common::http::Cookie::valid_value("\"abc");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_value rejects a value with a space character") {
    auto r = slim::common::http::Cookie::valid_value("bad value");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_value rejects a value with a semicolon") {
    auto r = slim::common::http::Cookie::valid_value("bad;value");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_value rejects a value with a DEL character") {
    auto r = slim::common::http::Cookie::valid_value("bad\x7Fvalue");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_path (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_path accepts the root path") {
    auto r = slim::common::http::Cookie::valid_path("/");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_path accepts a multi-segment path") {
    auto r = slim::common::http::Cookie::valid_path("/api/v1/resource");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_path rejects a path that does not start with slash") {
    auto r = slim::common::http::Cookie::valid_path("api/v1");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_path rejects an empty path") {
    auto r = slim::common::http::Cookie::valid_path("");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_path rejects a path containing a semicolon") {
    auto r = slim::common::http::Cookie::valid_path("/bad;path");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_path rejects a path with a control character") {
    auto r = slim::common::http::Cookie::valid_path("/bad\x1Fpath");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_domain (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_domain accepts a simple hostname") {
    auto r = slim::common::http::Cookie::valid_domain("example.com");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_domain accepts a domain with a leading dot") {
    auto r = slim::common::http::Cookie::valid_domain(".example.com");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_domain accepts a subdomain") {
    auto r = slim::common::http::Cookie::valid_domain("sub.example.com");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_domain rejects an empty string") {
    auto r = slim::common::http::Cookie::valid_domain("");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_domain rejects a bare dot") {
    auto r = slim::common::http::Cookie::valid_domain(".");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_domain rejects a domain with a trailing dot") {
    auto r = slim::common::http::Cookie::valid_domain("example.com.");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_domain rejects a label starting with a hyphen") {
    auto r = slim::common::http::Cookie::valid_domain("-bad.example.com");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_domain rejects a label ending with a hyphen") {
    auto r = slim::common::http::Cookie::valid_domain("bad-.example.com");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_domain rejects a domain exceeding 253 characters") {
    std::string long_domain(63, 'a');
    long_domain += ".";
    long_domain += std::string(63, 'b');
    long_domain += ".";
    long_domain += std::string(63, 'c');
    long_domain += ".";
    long_domain += std::string(63, 'd');  // total = 4*63 + 3 dots = 255
    auto r = slim::common::http::Cookie::valid_domain(long_domain);
    REQUIRE(r.has_error());
}

TEST_CASE("valid_domain rejects a label with an invalid character") {
    auto r = slim::common::http::Cookie::valid_domain("bad_label.example.com");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_expires (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_expires accepts an RFC 1123 date") {
    auto r = slim::common::http::Cookie::valid_expires("Thu, 01 Jan 2099 00:00:00 GMT");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_expires accepts an RFC 850 date") {
    auto r = slim::common::http::Cookie::valid_expires("Thursday, 01-Jan-99 00:00:00 GMT");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_expires accepts an ANSI C asctime date") {
    auto r = slim::common::http::Cookie::valid_expires("Thu Jan  1 00:00:00 2099");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_expires rejects a malformed date string") {
    auto r = slim::common::http::Cookie::valid_expires("not-a-date");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_expires rejects an ISO 8601 date") {
    auto r = slim::common::http::Cookie::valid_expires("2099-01-01T00:00:00Z");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_max_age (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_max_age accepts zero") {
    auto r = slim::common::http::Cookie::valid_max_age(0u);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_max_age accepts a typical positive value") {
    auto r = slim::common::http::Cookie::valid_max_age(3600u);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_max_age accepts the maximum time_t value") {
    constexpr std::uint_least64_t max_val =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    auto r = slim::common::http::Cookie::valid_max_age(max_val);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_max_age rejects a value exceeding the maximum time_t value") {
    constexpr std::uint_least64_t over_max =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()) + 1;
    auto r = slim::common::http::Cookie::valid_max_age(over_max);
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_same_site (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_same_site accepts Strict") {
    auto r = slim::common::http::Cookie::valid_same_site("Strict");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_same_site accepts Lax") {
    auto r = slim::common::http::Cookie::valid_same_site("Lax");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_same_site accepts None") {
    auto r = slim::common::http::Cookie::valid_same_site("None");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_same_site is case-insensitive") {
    auto r = slim::common::http::Cookie::valid_same_site("strict");
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_same_site rejects an unrecognised value") {
    auto r = slim::common::http::Cookie::valid_same_site("always");
    REQUIRE(r.has_error());
}

TEST_CASE("valid_same_site rejects an empty string") {
    auto r = slim::common::http::Cookie::valid_same_site("");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_secure (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_secure accepts Strict with secure false") {
    auto r = slim::common::http::Cookie::valid_secure("Strict", false);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_secure accepts Lax with secure false") {
    auto r = slim::common::http::Cookie::valid_secure("Lax", false);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_secure accepts Strict with secure true") {
    auto r = slim::common::http::Cookie::valid_secure("Strict", true);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_secure accepts None with secure true") {
    auto r = slim::common::http::Cookie::valid_secure("None", true);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_secure rejects None with secure false") {
    auto r = slim::common::http::Cookie::valid_secure("None", false);
    REQUIRE(r.has_error());
}

TEST_CASE("valid_secure accepts an empty same_site with secure false") {
    auto r = slim::common::http::Cookie::valid_secure("", false);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_secure accepts an empty same_site with secure true") {
    auto r = slim::common::http::Cookie::valid_secure("", true);
    REQUIRE_FALSE(r.has_error());
}

// ---------------------------------------------------------------------------
// valid_partitioned (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_partitioned accepts secure true with partitioned true") {
    auto r = slim::common::http::Cookie::valid_partitioned(true, true);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_partitioned accepts secure true with partitioned false") {
    auto r = slim::common::http::Cookie::valid_partitioned(true, false);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_partitioned accepts secure false with partitioned false") {
    auto r = slim::common::http::Cookie::valid_partitioned(false, false);
    REQUIRE_FALSE(r.has_error());
}

TEST_CASE("valid_partitioned rejects secure false with partitioned true") {
    auto r = slim::common::http::Cookie::valid_partitioned(false, true);
    REQUIRE(r.has_error());
}