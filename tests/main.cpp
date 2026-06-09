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
