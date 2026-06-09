#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/cookie.h>

// ---------------------------------------------------------------------------
// set_name
// ---------------------------------------------------------------------------

TEST_CASE("set_name accepts a valid token") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("session_id");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_name() == "session_id");
}

TEST_CASE("set_name rejects an empty string") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_name rejects a name containing a separator character") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("bad=name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_name rejects a name with a space") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("bad name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_name rejects a name with a control character") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("bad\x01name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_name rejects a name containing a colon") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("bad:name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_name trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("  session_id  ");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_name() == "session_id");
}

// ---------------------------------------------------------------------------
// set_value
// ---------------------------------------------------------------------------

TEST_CASE("set_value accepts a plain ASCII value") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_value("abc123");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_value() == "abc123");
}

TEST_CASE("set_value accepts a properly double-quoted value") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_value("\"abc123\"");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("set_value rejects an unmatched leading double quote") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_value("\"abc");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_value rejects a value with a space character") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_value("bad value");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_value rejects a value with a semicolon") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_value("bad;value");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_value rejects a value with a DEL character") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_value("bad\x7Fvalue");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_value trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_value("  abc123  ");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_value() == "abc123");
}

// ---------------------------------------------------------------------------
// set_path
// ---------------------------------------------------------------------------

TEST_CASE("set_path accepts the root path") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("/");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_path() == "/");
}

TEST_CASE("set_path accepts a multi-segment path") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("/api/v1/resource");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_path() == "/api/v1/resource");
}

TEST_CASE("set_path rejects a path that does not start with slash") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("api/v1");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_path rejects an empty path") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_path rejects a path containing a semicolon") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("/bad;path");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_path rejects a path with a control character") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("/bad\x1Fpath");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_path trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("  /api/v1  ");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_path() == "/api/v1");
}

// ---------------------------------------------------------------------------
// set_domain
// ---------------------------------------------------------------------------

TEST_CASE("set_domain accepts a simple hostname") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("example.com");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_domain() == "example.com");
}

TEST_CASE("set_domain accepts a domain with a leading dot") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain(".example.com");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("set_domain accepts a subdomain") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("sub.example.com");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("set_domain rejects an empty string") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_domain rejects a bare dot") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain(".");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_domain rejects a domain with a trailing dot") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("example.com.");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_domain rejects a label starting with a hyphen") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("-bad.example.com");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_domain rejects a label ending with a hyphen") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("bad-.example.com");
    REQUIRE(e != COOKIE::STATUS::OK);
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
    COOKIE::STATUS e = c.set_domain(long_domain);
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_domain rejects a label with an invalid character") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("bad_label.example.com");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_domain trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_domain("  example.com  ");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_domain() == "example.com");
}

// ---------------------------------------------------------------------------
// set_expires
// ---------------------------------------------------------------------------

TEST_CASE("set_expires accepts an RFC 1123 date") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_expires("Thu, 01 Jan 2099 00:00:00 GMT");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

TEST_CASE("set_expires accepts an RFC 850 date") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_expires("Thursday, 01-Jan-99 00:00:00 GMT");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("set_expires accepts an ANSI C asctime date") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_expires("Thu Jan  1 00:00:00 2099");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("set_expires rejects a malformed date string") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_expires("not-a-date");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_expires rejects an ISO 8601 date") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_expires("2099-01-01T00:00:00Z");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_expires trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_expires("  Thu, 01 Jan 2099 00:00:00 GMT  ");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

// ---------------------------------------------------------------------------
// set_max_age
// ---------------------------------------------------------------------------

TEST_CASE("set_max_age accepts zero") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("0");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == 0u);
}

TEST_CASE("set_max_age accepts a positive integer") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("3600");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == 3600u);
}

TEST_CASE("set_max_age accepts the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr std::uint_least64_t max_val =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    COOKIE::STATUS e = c.set_max_age(std::to_string(max_val));
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == max_val);
}

TEST_CASE("set_max_age rejects a value exceeding the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr std::uint_least64_t over_max =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()) + 1;
    COOKIE::STATUS e = c.set_max_age(std::to_string(over_max));
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_max_age rejects a negative integer") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("-1");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_max_age rejects a non-numeric string") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("abc");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_max_age rejects a value with trailing characters") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("3600x");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_max_age rejects an empty string") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// set_same_site
// ---------------------------------------------------------------------------

TEST_CASE("set_same_site accepts Strict") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("Strict");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site accepts Lax") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("Lax");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "Lax");
}

TEST_CASE("set_same_site accepts None") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("None");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "None");
}


TEST_CASE("set_same_site is case-insensitive strict") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("strict");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "strict");
}

TEST_CASE("set_same_site is case-insensitive lax") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("lax");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "lax");
}

TEST_CASE("set_same_site is case-insensitive none") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("none");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "none");
}

TEST_CASE("set_same_site trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("  Strict  ");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site rejects an unrecognised value") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("always");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// set_httponly
// ---------------------------------------------------------------------------

TEST_CASE("set_httponly accepts true") {
    slim::common::http::Cookie c;
    auto e = c.set_httponly(std::string_view{"true"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts TRUE") {
    slim::common::http::Cookie c;
    auto e = c.set_httponly(std::string_view{"TRUE"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts false") {
    slim::common::http::Cookie c;
    auto e = c.set_httponly(std::string_view{"false"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly accepts FALSE") {
    slim::common::http::Cookie c;
    auto e = c.set_httponly(std::string_view{"FALSE"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    auto e = c.set_httponly(std::string_view{"yes"});
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// set_secure
// ---------------------------------------------------------------------------

TEST_CASE("set_secure accepts true") {
    slim::common::http::Cookie c;
    auto e = c.set_secure(std::string_view{"true"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts TRUE") {
    slim::common::http::Cookie c;
    auto e = c.set_secure(std::string_view{"TRUE"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts false") {
    slim::common::http::Cookie c;
    auto e = c.set_secure(std::string_view{"false"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure accepts FALSE") {
    slim::common::http::Cookie c;
    auto e = c.set_secure(std::string_view{"FALSE"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    auto e = c.set_secure(std::string_view{"1"});
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// set_partitioned
// ---------------------------------------------------------------------------

TEST_CASE("set_partitioned accepts true") {
    slim::common::http::Cookie c;
    auto e = c.set_partitioned(std::string_view{"true"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_partitioned() == true);
}

TEST_CASE("set_partitioned accepts TRUE") {
    slim::common::http::Cookie c;
    auto e = c.set_partitioned(std::string_view{"TRUE"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_partitioned() == true);
}

TEST_CASE("set_partitioned accepts false") {
    slim::common::http::Cookie c;
    auto e = c.set_partitioned(std::string_view{"false"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned accepts FALSE") {
    slim::common::http::Cookie c;
    auto e = c.set_partitioned(std::string_view{"FALSE"});
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    auto e = c.set_partitioned(std::string_view{"no"});
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// validate_secure / validate_partitioned
// ---------------------------------------------------------------------------

TEST_CASE("validate_secure returns no error when SameSite is Strict and Secure is false") {
    slim::common::http::Cookie c;
    c.set_same_site("Strict");
    auto e = c.validate_secure();
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("validate_secure returns error when SameSite is None and Secure is false") {
    slim::common::http::Cookie c;
    c.set_secure("true");
    c.set_same_site("None");
    c.set_secure("false");  // force via internal state; validate_secure re-checks consistency
    auto e = c.validate_secure();
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("validate_partitioned returns no error when Secure is true and Partitioned is true") {
    slim::common::http::Cookie c;
    c.set_secure("true");
    c.set_partitioned("true");
    auto e = c.validate_partitioned();
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("validate_partitioned returns error when Partitioned is true but Secure is false") {
    slim::common::http::Cookie c;
    c.set_partitioned("true");
    auto e = c.validate_partitioned();
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_name (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_name accepts a valid token") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_name("session_id");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_name rejects an empty string") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_name("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_name rejects a name containing a separator character") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_name("bad=name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_name rejects a name with a space") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_name("bad name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_name rejects a name with a control character") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_name("bad\x01name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_name rejects a name containing a colon") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_name("bad:name");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_value (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_value accepts a plain ASCII value") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_value("abc123");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_value accepts a properly double-quoted value") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_value("\"abc123\"");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_value rejects an unmatched leading double quote") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_value("\"abc");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_value rejects a value with a space character") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_value("bad value");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_value rejects a value with a semicolon") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_value("bad;value");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_value rejects a value with a DEL character") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_value("bad\x7Fvalue");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_path (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_path accepts the root path") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_path("/");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_path accepts a multi-segment path") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_path("/api/v1/resource");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_path rejects a path that does not start with slash") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_path("api/v1");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_path rejects an empty path") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_path("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_path rejects a path containing a semicolon") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_path("/bad;path");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_path rejects a path with a control character") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_path("/bad\x1Fpath");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_domain (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_domain accepts a simple hostname") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain("example.com");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain accepts a domain with a leading dot") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain(".example.com");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain accepts a subdomain") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain("sub.example.com");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain rejects an empty string") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain rejects a bare dot") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain(".");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain rejects a domain with a trailing dot") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain("example.com.");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain rejects a label starting with a hyphen") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain("-bad.example.com");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain rejects a label ending with a hyphen") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain("bad-.example.com");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain rejects a domain exceeding 253 characters") {
    std::string long_domain(63, 'a');
    long_domain += ".";
    long_domain += std::string(63, 'b');
    long_domain += ".";
    long_domain += std::string(63, 'c');
    long_domain += ".";
    long_domain += std::string(63, 'd');  // total = 4*63 + 3 dots = 255
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain(long_domain);
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_domain rejects a label with an invalid character") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_domain("bad_label.example.com");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_expires (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_expires accepts an RFC 1123 date") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_expires("Thu, 01 Jan 2099 00:00:00 GMT");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_expires accepts an RFC 850 date") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_expires("Thursday, 01-Jan-99 00:00:00 GMT");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_expires accepts an ANSI C asctime date") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_expires("Thu Jan  1 00:00:00 2099");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_expires rejects a malformed date string") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_expires("not-a-date");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_expires rejects an ISO 8601 date") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_expires("2099-01-01T00:00:00Z");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_max_age (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_max_age accepts zero") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_max_age(0u);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_max_age accepts a typical positive value") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_max_age(3600u);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_max_age accepts the maximum time_t value") {
    constexpr std::uint_least64_t max_val =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    COOKIE::STATUS e = slim::common::http::Cookie::valid_max_age(max_val);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_max_age rejects a value exceeding the maximum time_t value") {
    constexpr std::uint_least64_t over_max =
        static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()) + 1;
    COOKIE::STATUS e = slim::common::http::Cookie::valid_max_age(over_max);
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_same_site (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_same_site accepts Strict") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_same_site("Strict");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_same_site accepts Lax") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_same_site("Lax");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_same_site accepts None") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_same_site("None");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_same_site is case-insensitive") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_same_site("strict");
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_same_site rejects an unrecognised value") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_same_site("always");
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_same_site rejects an empty string") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_same_site("");
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_secure (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_secure accepts Strict with secure false") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_secure("Strict", false);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_secure accepts Lax with secure false") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_secure("Lax", false);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_secure accepts Strict with secure true") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_secure("Strict", true);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_secure accepts None with secure true") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_secure("None", true);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_secure rejects None with secure false") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_secure("None", false);
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("valid_secure accepts an empty same_site with secure false") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_secure("", false);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_secure accepts an empty same_site with secure true") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_secure("", true);
    REQUIRE(e == COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// valid_partitioned (static)
// ---------------------------------------------------------------------------

TEST_CASE("valid_partitioned accepts secure true with partitioned true") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_partitioned(true, true);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_partitioned accepts secure true with partitioned false") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_partitioned(true, false);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_partitioned accepts secure false with partitioned false") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_partitioned(false, false);
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("valid_partitioned rejects secure false with partitioned true") {
    COOKIE::STATUS e = slim::common::http::Cookie::valid_partitioned(false, true);
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// status_string - valid status codes
// ---------------------------------------------------------------------------

TEST_CASE("status_string returns OK for STATUS::OK") {
	auto str = COOKIE::status_string(COOKIE::STATUS::OK);
	REQUIRE(str == "OK");
	REQUIRE(str.length() == 2);
}

TEST_CASE("status_string returns INVALID_BOOLEAN for STATUS::INVALID_BOOLEAN") {
	auto str = COOKIE::status_string(COOKIE::STATUS::INVALID_BOOLEAN);
	REQUIRE(str == "INVALID_BOOLEAN");
	REQUIRE(!str.empty());
}

TEST_CASE("status_string returns DOMAIN_EMPTY for STATUS::DOMAIN_EMPTY") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_EMPTY);
	REQUIRE(str == "DOMAIN_EMPTY");
}

TEST_CASE("status_string returns DOMAIN_BARE_DOT for STATUS::DOMAIN_BARE_DOT") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_BARE_DOT);
	REQUIRE(str == "DOMAIN_BARE_DOT");
}

TEST_CASE("status_string returns DOMAIN_TRAILING_DOT for STATUS::DOMAIN_TRAILING_DOT") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_TRAILING_DOT);
	REQUIRE(str == "DOMAIN_TRAILING_DOT");
}

TEST_CASE("status_string returns DOMAIN_TOO_LONG for STATUS::DOMAIN_TOO_LONG") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_TOO_LONG);
	REQUIRE(str == "DOMAIN_TOO_LONG");
}

TEST_CASE("status_string returns DOMAIN_LABEL_EMPTY for STATUS::DOMAIN_LABEL_EMPTY") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_LABEL_EMPTY);
	REQUIRE(str == "DOMAIN_LABEL_EMPTY");
}

TEST_CASE("status_string returns DOMAIN_LABEL_TOO_LONG for STATUS::DOMAIN_LABEL_TOO_LONG") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_LABEL_TOO_LONG);
	REQUIRE(str == "DOMAIN_LABEL_TOO_LONG");
}

TEST_CASE("status_string returns DOMAIN_LABEL_INVALID_HYPHEN for STATUS::DOMAIN_LABEL_INVALID_HYPHEN") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN);
	REQUIRE(str == "DOMAIN_LABEL_INVALID_HYPHEN");
}

TEST_CASE("status_string returns DOMAIN_INVALID_CHAR for STATUS::DOMAIN_INVALID_CHAR") {
	auto str = COOKIE::status_string(COOKIE::STATUS::DOMAIN_INVALID_CHAR);
	REQUIRE(str == "DOMAIN_INVALID_CHAR");
}

TEST_CASE("status_string returns EXPIRES_INVALID_FORMAT for STATUS::EXPIRES_INVALID_FORMAT") {
	auto str = COOKIE::status_string(COOKIE::STATUS::EXPIRES_INVALID_FORMAT);
	REQUIRE(str == "EXPIRES_INVALID_FORMAT");
}

TEST_CASE("status_string returns PATH_EMPTY for STATUS::PATH_EMPTY") {
	auto str = COOKIE::status_string(COOKIE::STATUS::PATH_EMPTY);
	REQUIRE(str == "PATH_EMPTY");
}

TEST_CASE("status_string returns PATH_MISSING_LEADING_SLASH for STATUS::PATH_MISSING_LEADING_SLASH") {
	auto str = COOKIE::status_string(COOKIE::STATUS::PATH_MISSING_LEADING_SLASH);
	REQUIRE(str == "PATH_MISSING_LEADING_SLASH");
}

TEST_CASE("status_string returns PATH_INVALID_CHAR for STATUS::PATH_INVALID_CHAR") {
	auto str = COOKIE::status_string(COOKIE::STATUS::PATH_INVALID_CHAR);
	REQUIRE(str == "PATH_INVALID_CHAR");
}

TEST_CASE("status_string returns MAX_AGE_EXCEEDS_LIMIT for STATUS::MAX_AGE_EXCEEDS_LIMIT") {
	auto str = COOKIE::status_string(COOKIE::STATUS::MAX_AGE_EXCEEDS_LIMIT);
	REQUIRE(str == "MAX_AGE_EXCEEDS_LIMIT");
}

TEST_CASE("status_string returns MAX_AGE_EMPTY for STATUS::MAX_AGE_EMPTY") {
	auto str = COOKIE::status_string(COOKIE::STATUS::MAX_AGE_EMPTY);
	REQUIRE(str == "MAX_AGE_EMPTY");
}

TEST_CASE("status_string returns MAX_AGE_INVALID_FORMAT for STATUS::MAX_AGE_INVALID_FORMAT") {
	auto str = COOKIE::status_string(COOKIE::STATUS::MAX_AGE_INVALID_FORMAT);
	REQUIRE(str == "MAX_AGE_INVALID_FORMAT");
}

TEST_CASE("status_string returns MAX_AGE_TRAILING_CHARS for STATUS::MAX_AGE_TRAILING_CHARS") {
	auto str = COOKIE::status_string(COOKIE::STATUS::MAX_AGE_TRAILING_CHARS);
	REQUIRE(str == "MAX_AGE_TRAILING_CHARS");
}

TEST_CASE("status_string returns NAME_EMPTY for STATUS::NAME_EMPTY") {
	auto str = COOKIE::status_string(COOKIE::STATUS::NAME_EMPTY);
	REQUIRE(str == "NAME_EMPTY");
}

TEST_CASE("status_string returns NAME_INVALID_CHAR for STATUS::NAME_INVALID_CHAR") {
	auto str = COOKIE::status_string(COOKIE::STATUS::NAME_INVALID_CHAR);
	REQUIRE(str == "NAME_INVALID_CHAR");
}

TEST_CASE("status_string returns NAME_PREFIX_REQUIRES_SECURE for STATUS::NAME_PREFIX_REQUIRES_SECURE") {
	auto str = COOKIE::status_string(COOKIE::STATUS::NAME_PREFIX_REQUIRES_SECURE);
	REQUIRE(str == "NAME_PREFIX_REQUIRES_SECURE");
}

TEST_CASE("status_string returns NAME_HOST_PREFIX_HAS_DOMAIN for STATUS::NAME_HOST_PREFIX_HAS_DOMAIN") {
	auto str = COOKIE::status_string(COOKIE::STATUS::NAME_HOST_PREFIX_HAS_DOMAIN);
	REQUIRE(str == "NAME_HOST_PREFIX_HAS_DOMAIN");
}

TEST_CASE("status_string returns NAME_HOST_PREFIX_INVALID_PATH for STATUS::NAME_HOST_PREFIX_INVALID_PATH") {
	auto str = COOKIE::status_string(COOKIE::STATUS::NAME_HOST_PREFIX_INVALID_PATH);
	REQUIRE(str == "NAME_HOST_PREFIX_INVALID_PATH");
}

TEST_CASE("status_string returns PARTITIONED_REQUIRES_SECURE for STATUS::PARTITIONED_REQUIRES_SECURE") {
	auto str = COOKIE::status_string(COOKIE::STATUS::PARTITIONED_REQUIRES_SECURE);
	REQUIRE(str == "PARTITIONED_REQUIRES_SECURE");
}

TEST_CASE("status_string returns SAMESITE_NONE_REQUIRES_SECURE for STATUS::SAMESITE_NONE_REQUIRES_SECURE") {
	auto str = COOKIE::status_string(COOKIE::STATUS::SAMESITE_NONE_REQUIRES_SECURE);
	REQUIRE(str == "SAMESITE_NONE_REQUIRES_SECURE");
}

TEST_CASE("status_string returns SAMESITE_INVALID for STATUS::SAMESITE_INVALID") {
	auto str = COOKIE::status_string(COOKIE::STATUS::SAMESITE_INVALID);
	REQUIRE(str == "SAMESITE_INVALID");
}

TEST_CASE("status_string returns VALUE_EMPTY for STATUS::VALUE_EMPTY") {
	auto str = COOKIE::status_string(COOKIE::STATUS::VALUE_EMPTY);
	REQUIRE(str == "VALUE_EMPTY");
}

TEST_CASE("status_string returns VALUE_UNMATCHED_QUOTE for STATUS::VALUE_UNMATCHED_QUOTE") {
	auto str = COOKIE::status_string(COOKIE::STATUS::VALUE_UNMATCHED_QUOTE);
	REQUIRE(str == "VALUE_UNMATCHED_QUOTE");
}

TEST_CASE("status_string returns VALUE_INVALID_CHAR for STATUS::VALUE_INVALID_CHAR") {
	auto str = COOKIE::status_string(COOKIE::STATUS::VALUE_INVALID_CHAR);
	REQUIRE(str == "VALUE_INVALID_CHAR");
}

// ---------------------------------------------------------------------------
// status_string - string properties
// ---------------------------------------------------------------------------

TEST_CASE("status_string never returns empty string for valid status") {
	for (int i = 0; i < static_cast<int>(COOKIE::STATUS::_COUNT); ++i) {
		auto status = static_cast<COOKIE::STATUS>(i);
		auto str = COOKIE::status_string(status);
		REQUIRE(!str.empty());
	}
}

TEST_CASE("status_string returns UNKNOWN for invalid status value") {
	auto invalid_status = static_cast<COOKIE::STATUS>(
		static_cast<uint8_t>(COOKIE::STATUS::_COUNT) + 1
	);
	auto str = COOKIE::status_string(invalid_status);
	REQUIRE(str == "UNKNOWN");
}

TEST_CASE("status_string returns string_view") {
	auto str = COOKIE::status_string(COOKIE::STATUS::OK);
	static_assert(std::is_same_v<decltype(str), std::string_view>,
		"status_string should return std::string_view");
	REQUIRE(str == "OK");
}

TEST_CASE("status_string is constexpr") {
	constexpr auto str = COOKIE::status_string(COOKIE::STATUS::OK);
	static_assert(str == "OK");
	REQUIRE(str == "OK");
}

TEST_CASE("status_string noexcept") {
	static_assert(noexcept(COOKIE::status_string(COOKIE::STATUS::OK)));
	REQUIRE(true);
}
