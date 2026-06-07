#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/cookie.h>

// ---------------------------------------------------------------------------
// set_name
// ---------------------------------------------------------------------------

TEST_CASE("set_name accepts a valid token") {
    slim::common::http::Cookie c;
    auto r = c.set_name("session_id");
    REQUIRE(r);
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
    REQUIRE(r);
    REQUIRE(c.get_name() == "session_id");
}

// ---------------------------------------------------------------------------
// set_value
// ---------------------------------------------------------------------------

TEST_CASE("set_value accepts a plain ASCII value") {
    slim::common::http::Cookie c;
    auto r = c.set_value("abc123");
    REQUIRE(r);
    REQUIRE(c.get_value() == "abc123");
}

TEST_CASE("set_value accepts a properly double-quoted value") {
    slim::common::http::Cookie c;
    auto r = c.set_value("\"abc123\"");
    REQUIRE(r);
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
    REQUIRE(r);
    REQUIRE(c.get_value() == "abc123");
}

// ---------------------------------------------------------------------------
// set_path
// ---------------------------------------------------------------------------

TEST_CASE("set_path accepts the root path") {
    slim::common::http::Cookie c;
    auto r = c.set_path("/");
    REQUIRE(r);
    REQUIRE(c.get_path() == "/");
}

TEST_CASE("set_path accepts a multi-segment path") {
    slim::common::http::Cookie c;
    auto r = c.set_path("/api/v1/resource");
    REQUIRE(r);
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
    REQUIRE(r);
    REQUIRE(c.get_path() == "/api/v1");
}

// ---------------------------------------------------------------------------
// set_domain
// ---------------------------------------------------------------------------

TEST_CASE("set_domain accepts a simple hostname") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("example.com");
    REQUIRE(r);
    REQUIRE(c.get_domain() == "example.com");
}

TEST_CASE("set_domain accepts a domain with a leading dot") {
    slim::common::http::Cookie c;
    auto r = c.set_domain(".example.com");
    REQUIRE(r);
}

TEST_CASE("set_domain accepts a subdomain") {
    slim::common::http::Cookie c;
    auto r = c.set_domain("sub.example.com");
    REQUIRE(r);
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
    REQUIRE(r);
    REQUIRE(c.get_domain() == "example.com");
}

// ---------------------------------------------------------------------------
// set_expires
// ---------------------------------------------------------------------------

TEST_CASE("set_expires accepts an RFC 1123 date") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("Thu, 01 Jan 2099 00:00:00 GMT");
    REQUIRE(r);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

TEST_CASE("set_expires accepts an RFC 850 date") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("Thursday, 01-Jan-99 00:00:00 GMT");
    REQUIRE(r);
}

TEST_CASE("set_expires accepts an ANSI C asctime date") {
    slim::common::http::Cookie c;
    auto r = c.set_expires("Thu Jan  1 00:00:00 2099");
    REQUIRE(r);
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
    REQUIRE(r);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

// ---------------------------------------------------------------------------
// set_max_age
// ---------------------------------------------------------------------------

using u_time_t = std::make_unsigned_t<std::time_t>;

TEST_CASE("set_max_age accepts zero") {
    slim::common::http::Cookie c;
    c.set_max_age(u_time_t{3600});
    auto r = c.set_max_age("0");
    REQUIRE(r == 0);
    REQUIRE(c.get_max_age() == u_time_t{0});
}

TEST_CASE("set_max_age accepts a positive integer") {
    slim::common::http::Cookie c;
    auto r = c.set_max_age("3600");
    REQUIRE(r == 3600);
    REQUIRE(c.get_max_age() == u_time_t{3600});
}

TEST_CASE("set_max_age accepts the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr u_time_t max_val = static_cast<u_time_t>(std::numeric_limits<std::time_t>::max());
    auto r = c.set_max_age(std::to_string(max_val));
    REQUIRE(r == static_cast<long long>(max_val));
    REQUIRE(c.get_max_age() == max_val);
}

TEST_CASE("set_max_age rejects a value exceeding the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr u_time_t over_max = static_cast<u_time_t>(std::numeric_limits<std::time_t>::max()) + 1;
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
    REQUIRE(r);
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site accepts Lax") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("Lax");
    REQUIRE(r);
}

TEST_CASE("set_same_site accepts None when Secure is true") {
    slim::common::http::Cookie c;
    c.set_secure(true);
    auto r = c.set_same_site("None");
    REQUIRE(r);
    REQUIRE(c.get_same_site() == "None");
}

TEST_CASE("set_same_site rejects None when Secure is false") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("None");
    REQUIRE(r.has_error());
}

TEST_CASE("set_same_site is case-insensitive") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("strict");
    REQUIRE(r);
}

TEST_CASE("set_same_site trims leading and trailing whitespace before storing") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("  Strict  ");
    REQUIRE(r);
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site rejects an unrecognised value") {
    slim::common::http::Cookie c;
    auto r = c.set_same_site("always");
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// set_httponly / set_secure / set_partitioned
// ---------------------------------------------------------------------------

TEST_CASE("set_httponly accepts true") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_httponly(std::string_view{"true"});
    REQUIRE(r);
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts TRUE") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_httponly(std::string_view{"TRUE"});
    REQUIRE(r);
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts false") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_httponly(std::string_view{"false"});
    REQUIRE(!r);
    REQUIRE(!r.has_error());
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly accepts FALSE") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_httponly(std::string_view{"FALSE"});
    REQUIRE(!r);
    REQUIRE(!r.has_error());
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_httponly(std::string_view{"yes"});
    REQUIRE(r.has_error());
}

TEST_CASE("set_secure accepts true") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_secure(std::string_view{"true"});
    REQUIRE(r);
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts TRUE") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_secure(std::string_view{"TRUE"});
    REQUIRE(r);
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts false") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_secure(std::string_view{"false"});
    REQUIRE(!r);
    REQUIRE(!r.has_error());
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure accepts FALSE") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_secure(std::string_view{"FALSE"});
    REQUIRE(!r);
    REQUIRE(!r.has_error());
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_secure(std::string_view{"1"});
    REQUIRE(r.has_error());
}

TEST_CASE("set_secure rejects false when SameSite is None") {
    slim::common::http::Cookie c;
    c.set_secure(true);
    c.set_same_site(std::string_view{"None"});
    slim::SlimValue r = c.set_secure(std::string_view{"false"});
    REQUIRE(r.has_error());
}

TEST_CASE("set_partitioned accepts false") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_partitioned(std::string_view{"false"});
    REQUIRE(!r);
    REQUIRE(!r.has_error());
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned accepts FALSE") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_partitioned(std::string_view{"FALSE"});
    REQUIRE(!r);
    REQUIRE(!r.has_error());
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned rejects an invalid boolean string") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_partitioned(std::string_view{"no"});
    REQUIRE(r.has_error());
}

// ---------------------------------------------------------------------------
// bool / long long overloads (set_X(T) variants)
// ---------------------------------------------------------------------------

TEST_CASE("set_httponly bool overload sets true") {
    slim::common::http::Cookie c;
    c.set_httponly(true);
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly bool overload sets false") {
    slim::common::http::Cookie c;
    c.set_httponly(true);   // set first so the false write is meaningful
    c.set_httponly(false);
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_secure string_view overload sets true then false") {
    slim::common::http::Cookie c;
    slim::SlimValue r = c.set_secure(std::string_view{"true"});
    REQUIRE(r);
    REQUIRE(c.get_secure() == true);
    r = c.set_secure(std::string_view{"false"});
    REQUIRE(!r);
    REQUIRE(!r.has_error());
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_partitioned bool overload sets true") {
    slim::common::http::Cookie c;
    c.set_partitioned(true);
    REQUIRE(c.get_partitioned() == true);
}

TEST_CASE("set_partitioned bool overload sets false") {
    slim::common::http::Cookie c;
    c.set_partitioned(true);
    c.set_partitioned(false);
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_max_age u_time_t overload stores the value directly") {
    slim::common::http::Cookie c;
    c.set_max_age(u_time_t{7200});
    REQUIRE(c.get_max_age() == u_time_t{7200});
}

TEST_CASE("set_max_age u_time_t overload stores zero") {
    slim::common::http::Cookie c;
    c.set_max_age(u_time_t{0});
    REQUIRE(c.get_max_age() == u_time_t{0});
}