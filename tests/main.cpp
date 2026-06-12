#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>
#include <slim/common/http/cookie.h>



TEST_CASE("cookie compare ") {
    slim::common::http::Cookie c1, c2, c3;

    COOKIE::STATUS e1 = c1.set_name("cookie_name");
    REQUIRE(e1 == COOKIE::STATUS::OK);

    COOKIE::STATUS e2 = c2.set_name("cookie_name");
    REQUIRE(e2 == COOKIE::STATUS::OK);

    REQUIRE(c1 == c2);

    COOKIE::STATUS e3 = c3.set_name("cookie_three");
    REQUIRE(e3 == COOKIE::STATUS::OK);
    REQUIRE(c1 != c3);
}

// ---------------------------------------------------------------------------
// set_name
// ---------------------------------------------------------------------------

TEST_CASE("set_name accepts a valid token") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_name("   session_id   ");
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
    COOKIE::STATUS e = c.set_value("\t abc123\t");
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

TEST_CASE("set_path accepts an empty path") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_path("");
    REQUIRE(e == COOKIE::STATUS::OK);
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
    COOKIE::STATUS e = c.set_expires("  Thu, 01 Jan 2099 00:00:00 GMT  ");
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

TEST_CASE("set_expires translates a valid asctime date with a single-digit day") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_expires("Thu Jan  1 12:30:00 2099");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 12:30:00 GMT");
}

// ---------------------------------------------------------------------------
// set_max_age
// ---------------------------------------------------------------------------

TEST_CASE("set_max_age accepts zero using numeric") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age(0);
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == 0u);
}

TEST_CASE("set_max_age accepts zero using string") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("0");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == 0u);
}

TEST_CASE("set_max_age accepts a positive integer") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age(3600);
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == 3600u);
}

TEST_CASE("set_max_age accepts a positive integer string") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("3600");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == 3600u);
}

TEST_CASE("set_max_age accepts the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr std::uint_least64_t max_val = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    COOKIE::STATUS e = c.set_max_age(std::to_string(max_val));
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == max_val);
}

TEST_CASE("set_max_age rejects a value exceeding the maximum time_t value") {
    slim::common::http::Cookie c;
    constexpr std::uint_least64_t over_max = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()) + 1;
    COOKIE::STATUS e = c.set_max_age(std::to_string(over_max));
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("set_max_age translates a negative integer to 0") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_max_age("-1");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_max_age() == 0);
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
    COOKIE::STATUS e = c.set_same_site(" Strict\t");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site accepts Lax") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("Lax    ");
    REQUIRE(e == COOKIE::STATUS::OK);
    REQUIRE(c.get_same_site() == "Lax");
}

TEST_CASE("set_same_site accepts None") {
    slim::common::http::Cookie c;
    COOKIE::STATUS e = c.set_same_site("     None");
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
    COOKIE::STATUS e = c.set_same_site("  none");
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
    c.set_name("cookie_name");
    c.set_same_site("Strict");
    auto e = c.validate();
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("validate_secure returns error when SameSite is None and Secure is false") {
    slim::common::http::Cookie c;
    c.set_secure("true");
    c.set_same_site("None");
    c.set_secure("false");  // force via internal state; validate_secure re-checks consistency
    auto e = c.validate();
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("validate_partitioned returns no error when SameSite==None and Secure is true and Partitioned is true") {
    slim::common::http::Cookie c;
    c.set_name("cookie_name");
    c.set_secure("true");
    c.set_partitioned("true");
    c.set_same_site("NONE");
    auto e = c.validate();
    REQUIRE(e == COOKIE::STATUS::OK);
}

TEST_CASE("validate_partitioned returns error when SameSite!=None and Secure is true and Partitioned is true") {
    slim::common::http::Cookie c;
    c.set_name("cookie_name");
    c.set_secure("true");
    c.set_partitioned("true");
    c.set_same_site("lax");
    auto e = c.validate();
    REQUIRE(e != COOKIE::STATUS::OK);
}

TEST_CASE("validate_partitioned returns error when Partitioned is true but Secure is false") {
    slim::common::http::Cookie c;
    c.set_partitioned("true");
    auto e = c.validate();
    REQUIRE(e != COOKIE::STATUS::OK);
}

// ---------------------------------------------------------------------------
// check cookie name + value sizes
// ---------------------------------------------------------------------------

TEST_CASE("Cookie set_name and set_value: minimal valid cookie", "[cookie][size]") {
    slim::common::http::Cookie cookie;
    const auto name_result = cookie.set_name("a");
    const auto value_result = cookie.set_value("b");
    REQUIRE(name_result == COOKIE::STATUS::OK);
    REQUIRE(value_result == COOKIE::STATUS::OK);
}

TEST_CASE("Cookie set_name: single character name", "[cookie][size]") {
    slim::common::http::Cookie cookie;
    const auto result = cookie.set_name("x");
    REQUIRE(result == COOKIE::STATUS::OK);
}

TEST_CASE("Cookie set_value: single character value", "[cookie][size]") {
    slim::common::http::Cookie cookie;
    const auto result = cookie.set_value("y");
    REQUIRE(result == COOKIE::STATUS::OK);
}

TEST_CASE("Cookie set_name and set_value: at maximum allowed size", "[cookie][size]") {
    slim::common::http::Cookie cookie;
    const std::string name(2048, 'n');
    const std::string value(2048, 'v');
    const auto name_result = cookie.set_name(name);
    const auto value_result = cookie.set_value(value);
    REQUIRE(name_result == COOKIE::STATUS::OK);
    REQUIRE(value_result == COOKIE::STATUS::OK);
}

TEST_CASE("Cookie set_name and set_value: realistic cookie", "[cookie][size]") {
    slim::common::http::Cookie cookie;
    const auto name_result = cookie.set_name("session_id");
    const auto value_result = cookie.set_value("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz");
    REQUIRE(name_result == COOKIE::STATUS::OK);
    REQUIRE(value_result == COOKIE::STATUS::OK);
}

TEST_CASE("Cookie set_value: special characters", "[cookie][size]") {
    slim::common::http::Cookie cookie;
    const std::string value(4096, '\x42');   // all 'B'
    const auto result = cookie.set_value(value);
    REQUIRE(result == COOKIE::STATUS::OK);
}

TEST_CASE("Cookie Serialize - Basic Name and Value", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("session") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("xyz123") == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: session=xyz123\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Domain Attribute", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("id") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("42") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_domain("example.com") == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: id=42; Domain=example.com\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Path Attribute", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("id") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("42") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_path("/api") == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: id=42; Path=/api\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Expires Attribute", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("id") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("42") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_expires("Wed, 21 Oct 2015 07:28:00 GMT") == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: id=42; Expires=Wed, 21 Oct 2015 07:28:00 GMT\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With SameSite Attribute", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("id") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("42") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_same_site("Lax") == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: id=42; SameSite=Lax\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Boolean Security Flags", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("id") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("42") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_same_site("none") == COOKIE::STATUS::OK);

    cookie.set_secure(true);
    cookie.set_httponly(true);
    cookie.set_partitioned(true);

    std::string expected = "Set-Cookie: id=42; SameSite=none; Secure; HttpOnly; Partitioned\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Single Digit Max-Age", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("timeout") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("active") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_max_age(0) == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: timeout=active; Max-Age=0\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Multi Digit Max-Age", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("timeout") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("active") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_max_age(3600) == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: timeout=active; Max-Age=3600\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Large Max-Age String Input", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("timeout") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("active") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_max_age("31536000") == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: timeout=active; Max-Age=31536000\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Complex Multi-Attribute Validation Pass", "[cookie][serialize]") {
    slim::common::http::Cookie cookie;
    REQUIRE(cookie.set_name("__Secure-user_tracker") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_value("hash_payload_alpha_9") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_domain("sub.domain.org") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_path("/") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_expires("Tue, 19 Jan 2038 03:14:07 GMT") == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_max_age(86400) == COOKIE::STATUS::OK);
    REQUIRE(cookie.set_same_site("None") == COOKIE::STATUS::OK);

    cookie.set_secure(true);
    cookie.set_httponly(true);
    cookie.set_partitioned(true);

    // Explicit structural cross-validation check pass
    REQUIRE(cookie.validate() == COOKIE::STATUS::OK);

    std::string expected = "Set-Cookie: __Secure-user_tracker=hash_payload_alpha_9"
                           "; Domain=sub.domain.org"
                           "; Path=/"
                           "; Max-Age=86400"
                           "; SameSite=None"
                           "; Secure"
                           "; HttpOnly"
                           "; Partitioned\r\n";

    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("domain with all-digit TLD is rejected", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example.123") == COOKIE::STATUS::DOMAIN_NUMERIC_TLD);
}

TEST_CASE("domain with multi-label all-digit TLD is rejected", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("foo.bar.456") == COOKIE::STATUS::DOMAIN_NUMERIC_TLD);
}

TEST_CASE("domain with single-digit TLD is rejected", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("test.0") == COOKIE::STATUS::DOMAIN_NUMERIC_TLD);
}

TEST_CASE("domain with digit-suffixed TLD is rejected", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example.co1") == COOKIE::STATUS::DOMAIN_NUMERIC_TLD);
}

TEST_CASE("domain with digit-prefixed TLD is rejected", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example.1com") == COOKIE::STATUS::DOMAIN_NUMERIC_TLD);
}

TEST_CASE("domain with digit embedded in TLD is rejected", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example.c0m") == COOKIE::STATUS::DOMAIN_NUMERIC_TLD);
}

TEST_CASE("digits in non-TLD labels are allowed", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("sub1.example.com") == COOKIE::STATUS::OK);
    CHECK(cookie.set_domain("123.example.com")  == COOKIE::STATUS::OK);
    CHECK(cookie.set_domain("a1b2.foo3.com")    == COOKIE::STATUS::OK);
}

TEST_CASE("valid alphabetic TLD is accepted", "[domain][tld]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example.com")   == COOKIE::STATUS::OK);
    CHECK(cookie.set_domain("example.co.uk") == COOKIE::STATUS::OK);
    CHECK(cookie.set_domain("my-site.io")    == COOKIE::STATUS::OK);
}

TEST_CASE("domain with leading hyphen in label is rejected", "[domain][hyphen]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("-example.com") == COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN);
}

TEST_CASE("domain with trailing hyphen in label is rejected", "[domain][hyphen]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example-.com") == COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN);
}

TEST_CASE("domain with leading hyphen in subdomain is rejected", "[domain][hyphen]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("sub.-example.com") == COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN);
}

TEST_CASE("domain with trailing hyphen in subdomain is rejected", "[domain][hyphen]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("sub-.example.com") == COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN);
}

TEST_CASE("domain with leading hyphen in TLD is rejected", "[domain][hyphen]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example.-com") == COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN);
}

TEST_CASE("domain with trailing hyphen in TLD is rejected", "[domain][hyphen]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("example.com-") == COOKIE::STATUS::DOMAIN_LABEL_INVALID_HYPHEN);
}

TEST_CASE("domain with hyphen in valid position is accepted", "[domain][hyphen]") {
    slim::common::http::Cookie cookie;
    CHECK(cookie.set_domain("my-site.com")        == COOKIE::STATUS::OK);
    CHECK(cookie.set_domain("my-site.example.com") == COOKIE::STATUS::OK);
    CHECK(cookie.set_domain("a-b-c.example.com")  == COOKIE::STATUS::OK);
}
