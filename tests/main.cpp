#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>
#include <slim/common/http/cookie.h>

using slim::common::http::Cookie;
using slim::common::http::CookieException;
using slim::common::http::CookieStatus;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

TEST_CASE("Cookie constructor: valid name and value") {
    Cookie c("session", "abc123");
    REQUIRE(c.get_name() == "session");
    REQUIRE(c.get_value() == "abc123");
}

TEST_CASE("Cookie constructor: trims whitespace from name and value") {
    Cookie c("  session  ", "  abc123  ");
    REQUIRE(c.get_name() == "session");
    REQUIRE(c.get_value() == "abc123");
}

TEST_CASE("Cookie constructor: throws on empty name") {
    REQUIRE_THROWS_AS(Cookie("", "abc123"), CookieException);
}

TEST_CASE("Cookie constructor: accepts on empty value") {
    REQUIRE_NOTHROW(Cookie("session", ""));
}

TEST_CASE("Cookie constructor: throws on invalid name character") {
    REQUIRE_THROWS_AS(Cookie("bad=name", "abc123"), CookieException);
}

TEST_CASE("Cookie constructor: throws on invalid value character") {
    REQUIRE_THROWS_AS(Cookie("session", "bad value"), CookieException);
}

TEST_CASE("Cookie constructor: valid cookie serializes correctly") {
    Cookie c("session", "abc123");
    REQUIRE(c.serialize() == "Set-Cookie: session=abc123\r\n");
}

TEST_CASE("cookie compare ") {
    Cookie c1, c2, c3;

    REQUIRE(c1.set_name("cookie_name") == CookieStatus::Ok);
    REQUIRE(c2.set_name("cookie_name") == CookieStatus::Ok);
    REQUIRE(c1 == c2);

    REQUIRE(c3.set_name("cookie_three") == CookieStatus::Ok);
    REQUIRE(c1 != c3);
}

// ---------------------------------------------------------------------------
// set_name
// ---------------------------------------------------------------------------

TEST_CASE("set_name accepts a valid token") {
    Cookie c;
    REQUIRE(c.set_name("   session_id   ") == CookieStatus::Ok);
    REQUIRE(c.get_name() == "session_id");
}

TEST_CASE("set_name rejects an empty string") {
    Cookie c;
    REQUIRE(c.set_name("") != CookieStatus::Ok);
}

TEST_CASE("set_name rejects a name containing a separator character") {
    Cookie c;
    REQUIRE(c.set_name("bad=name") != CookieStatus::Ok);
}

TEST_CASE("set_name rejects a name with a space") {
    Cookie c;
    REQUIRE(c.set_name("bad name") != CookieStatus::Ok);
}

TEST_CASE("set_name rejects a name with a control character") {
    Cookie c;
    REQUIRE(c.set_name("bad\x01name") != CookieStatus::Ok);
}

TEST_CASE("set_name rejects a name containing a colon") {
    Cookie c;
    REQUIRE(c.set_name("bad:name") != CookieStatus::Ok);
}

TEST_CASE("set_name trims leading and trailing whitespace before storing") {
    Cookie c;
    REQUIRE(c.set_name("  session_id  ") == CookieStatus::Ok);
    REQUIRE(c.get_name() == "session_id");
}

// ---------------------------------------------------------------------------
// operator==
// ---------------------------------------------------------------------------

TEST_CASE("operator==: identical name, domain, and path are equal") {
    Cookie c1("session", "abc123");
    c1.set_domain("example.com");
    c1.set_path("/");

    Cookie c2("session", "abc123");
    c2.set_domain("example.com");
    c2.set_path("/");

    REQUIRE(c1 == c2);
}

TEST_CASE("operator==: same name and domain, different path are not equal") {
    Cookie c1("session", "abc123");
    c1.set_domain("example.com");
    c1.set_path("/");

    Cookie c2("session", "abc123");
    c2.set_domain("example.com");
    c2.set_path("/api");

    REQUIRE(c1 != c2);
}

TEST_CASE("operator==: same name and path, different domain are not equal") {
    Cookie c1("session", "abc123");
    c1.set_domain("example.com");
    c1.set_path("/");

    Cookie c2("session", "abc123");
    c2.set_domain("sub.example.com");
    c2.set_path("/");

    REQUIRE(c1 != c2);
}

TEST_CASE("operator==: different name, same domain and path are not equal") {
    Cookie c1("session", "abc123");
    c1.set_domain("example.com");
    c1.set_path("/");

    Cookie c2("user", "abc123");
    c2.set_domain("example.com");
    c2.set_path("/");

    REQUIRE(c1 != c2);
}

TEST_CASE("operator==: same name, no domain, no path are equal") {
    Cookie c1("session", "abc123");
    Cookie c2("session", "xyz789");
    REQUIRE(c1 == c2);
}

TEST_CASE("operator==: same name, one has domain and one does not are not equal") {
    Cookie c1("session", "abc123");
    c1.set_domain("example.com");

    Cookie c2("session", "abc123");

    REQUIRE(c1 != c2);
}

TEST_CASE("operator==: same name, one has path and one does not are not equal") {
    Cookie c1("session", "abc123");
    c1.set_path("/");

    Cookie c2("session", "abc123");

    REQUIRE(c1 != c2);
}

TEST_CASE("operator==: different values with same name, domain, and path are equal") {
    Cookie c1("session", "abc123");
    c1.set_domain("example.com");
    c1.set_path("/");

    Cookie c2("session", "xyz789");
    c2.set_domain("example.com");
    c2.set_path("/");

    REQUIRE(c1 == c2);
}

TEST_CASE("operator==: name comparison is case-sensitive") {
    Cookie c1("Session", "abc123");
    Cookie c2("session", "abc123");
    REQUIRE(c1 != c2);
}

// ---------------------------------------------------------------------------
// set_value
// ---------------------------------------------------------------------------

TEST_CASE("set_value accepts a plain ASCII value") {
    Cookie c;
    REQUIRE(c.set_value("\t abc123\t") == CookieStatus::Ok);
    REQUIRE(c.get_value() == "abc123");
}

TEST_CASE("set_value accepts a properly double-quoted value") {
    Cookie c;
    REQUIRE(c.set_value("\"abc123\"") == CookieStatus::Ok);
}

TEST_CASE("set_value rejects an unmatched leading double quote") {
    Cookie c;
    REQUIRE(c.set_value("\"abc") != CookieStatus::Ok);
}

TEST_CASE("set_value rejects a value with a space character") {
    Cookie c;
    REQUIRE(c.set_value("bad value") != CookieStatus::Ok);
}

TEST_CASE("set_value rejects a value with a semicolon") {
    Cookie c;
    REQUIRE(c.set_value("bad;value") != CookieStatus::Ok);
}

TEST_CASE("set_value rejects a value with a DEL character") {
    Cookie c;
    REQUIRE(c.set_value("bad\x7Fvalue") != CookieStatus::Ok);
}

TEST_CASE("set_value trims leading and trailing whitespace before storing") {
    Cookie c;
    REQUIRE(c.set_value("  abc123  ") == CookieStatus::Ok);
    REQUIRE(c.get_value() == "abc123");
}

// ---------------------------------------------------------------------------
// set_path
// ---------------------------------------------------------------------------

TEST_CASE("set_path accepts the root path") {
    Cookie c;
    REQUIRE(c.set_path("/") == CookieStatus::Ok);
    REQUIRE(c.get_path() == "/");
}

TEST_CASE("set_path accepts a multi-segment path") {
    Cookie c;
    REQUIRE(c.set_path("/api/v1/resource") == CookieStatus::Ok);
    REQUIRE(c.get_path() == "/api/v1/resource");
}

TEST_CASE("set_path rejects a path that does not start with slash") {
    Cookie c;
    REQUIRE(c.set_path("api/v1") != CookieStatus::Ok);
}

TEST_CASE("set_path accepts an empty path") {
    Cookie c;
    REQUIRE(c.set_path("") == CookieStatus::Ok);
}

TEST_CASE("set_path rejects a path containing a semicolon") {
    Cookie c;
    REQUIRE(c.set_path("/bad;path") != CookieStatus::Ok);
}

TEST_CASE("set_path rejects a path with a control character") {
    Cookie c;
    REQUIRE(c.set_path("/bad\x1Fpath") != CookieStatus::Ok);
}

TEST_CASE("set_path trims leading and trailing whitespace before storing") {
    Cookie c;
    REQUIRE(c.set_path("  /api/v1  ") == CookieStatus::Ok);
    REQUIRE(c.get_path() == "/api/v1");
}

// ---------------------------------------------------------------------------
// set_domain
// ---------------------------------------------------------------------------

TEST_CASE("set_domain accepts a simple hostname") {
    Cookie c;
    REQUIRE(c.set_domain("example.com") == CookieStatus::Ok);
    REQUIRE(c.get_domain() == "example.com");
}

TEST_CASE("set_domain accepts a domain with a leading dot") {
    Cookie c;
    REQUIRE(c.set_domain(".example.com") == CookieStatus::Ok);
}

TEST_CASE("set_domain accepts a subdomain") {
    Cookie c;
    REQUIRE(c.set_domain("sub.example.com") == CookieStatus::Ok);
}

TEST_CASE("set_domain rejects an empty string") {
    Cookie c;
    REQUIRE(c.set_domain("") != CookieStatus::Ok);
}

TEST_CASE("set_domain rejects a bare dot") {
    Cookie c;
    REQUIRE(c.set_domain(".") != CookieStatus::Ok);
}

TEST_CASE("set_domain rejects a domain with a trailing dot") {
    Cookie c;
    REQUIRE(c.set_domain("example.com.") != CookieStatus::Ok);
}

TEST_CASE("set_domain rejects a label starting with a hyphen") {
    Cookie c;
    REQUIRE(c.set_domain("-bad.example.com") != CookieStatus::Ok);
}

TEST_CASE("set_domain rejects a label ending with a hyphen") {
    Cookie c;
    REQUIRE(c.set_domain("bad-.example.com") != CookieStatus::Ok);
}

TEST_CASE("set_domain rejects a domain exceeding 253 characters") {
    Cookie c;
    std::string long_domain(63, 'a');
    long_domain += ".";
    long_domain += std::string(63, 'b');
    long_domain += ".";
    long_domain += std::string(63, 'c');
    long_domain += ".";
    long_domain += std::string(63, 'd');
    REQUIRE(c.set_domain(long_domain) != CookieStatus::Ok);
}

TEST_CASE("set_domain rejects a label with an invalid character") {
    Cookie c;
    REQUIRE(c.set_domain("bad_label.example.com") != CookieStatus::Ok);
}

TEST_CASE("set_domain trims leading and trailing whitespace before storing") {
    Cookie c;
    REQUIRE(c.set_domain("  example.com  ") == CookieStatus::Ok);
    REQUIRE(c.get_domain() == "example.com");
}

// ---------------------------------------------------------------------------
// set_expires
// ---------------------------------------------------------------------------

TEST_CASE("set_expires accepts an RFC 1123 date") {
    Cookie c;
    REQUIRE(c.set_expires("  Thu, 01 Jan 2099 00:00:00 GMT  ") == CookieStatus::Ok);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

TEST_CASE("set_expires accepts an RFC 850 date") {
    Cookie c;
    REQUIRE(c.set_expires("Thursday, 01-Jan-99 00:00:00 GMT") == CookieStatus::Ok);
}

TEST_CASE("set_expires accepts an ANSI C asctime date") {
    Cookie c;
    REQUIRE(c.set_expires("Thu Jan  1 00:00:00 2099") == CookieStatus::Ok);
}

TEST_CASE("set_expires rejects a malformed date string") {
    Cookie c;
    REQUIRE(c.set_expires("not-a-date") != CookieStatus::Ok);
}

TEST_CASE("set_expires rejects an ISO 8601 date") {
    Cookie c;
    REQUIRE(c.set_expires("2099-01-01T00:00:00Z") != CookieStatus::Ok);
}

TEST_CASE("set_expires trims leading and trailing whitespace before storing") {
    Cookie c;
    REQUIRE(c.set_expires("  Thu, 01 Jan 2099 00:00:00 GMT  ") == CookieStatus::Ok);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
}

TEST_CASE("set_expires translates a valid asctime date with a single-digit day") {
    Cookie c;
    REQUIRE(c.set_expires("Thu Jan  1 12:30:00 2099") == CookieStatus::Ok);
    REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 12:30:00 GMT");
}

// ---------------------------------------------------------------------------
// set_max_age
// ---------------------------------------------------------------------------

TEST_CASE("set_max_age accepts zero using numeric") {
    Cookie c;
    REQUIRE(c.set_max_age(0) == CookieStatus::Ok);
    REQUIRE(c.get_max_age() == 0u);
}

TEST_CASE("set_max_age accepts zero using string") {
    Cookie c;
    REQUIRE(c.set_max_age("0") == CookieStatus::Ok);
    REQUIRE(c.get_max_age() == 0u);
}

TEST_CASE("set_max_age accepts a positive integer") {
    Cookie c;
    REQUIRE(c.set_max_age(3600) == CookieStatus::Ok);
    REQUIRE(c.get_max_age() == 3600u);
}

TEST_CASE("set_max_age accepts a positive integer string") {
    Cookie c;
    REQUIRE(c.set_max_age("3600") == CookieStatus::Ok);
    REQUIRE(c.get_max_age() == 3600u);
}

TEST_CASE("set_max_age accepts the maximum time_t value") {
    Cookie c;
    constexpr std::uint_least64_t max_val = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
    REQUIRE(c.set_max_age(std::to_string(max_val)) == CookieStatus::Ok);
    REQUIRE(c.get_max_age() == max_val);
}

TEST_CASE("set_max_age rejects a value exceeding the maximum time_t value") {
    Cookie c;
    constexpr std::uint_least64_t over_max = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()) + 1;
    REQUIRE(c.set_max_age(std::to_string(over_max)) != CookieStatus::Ok);
}

TEST_CASE("set_max_age translates a negative integer to 0") {
    Cookie c;
    REQUIRE(c.set_max_age("-1") == CookieStatus::Ok);
    REQUIRE(c.get_max_age() == 0);
}

TEST_CASE("set_max_age rejects a non-numeric string") {
    Cookie c;
    REQUIRE(c.set_max_age("abc") != CookieStatus::Ok);
}

TEST_CASE("set_max_age rejects a value with trailing characters") {
    Cookie c;
    REQUIRE(c.set_max_age("3600x") != CookieStatus::Ok);
}

TEST_CASE("set_max_age rejects an empty string") {
    Cookie c;
    REQUIRE(c.set_max_age("") != CookieStatus::Ok);
}

// ---------------------------------------------------------------------------
// set_same_site
// ---------------------------------------------------------------------------

TEST_CASE("set_same_site accepts Strict") {
    Cookie c;
    REQUIRE(c.set_same_site(" Strict\t") == CookieStatus::Ok);
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site accepts Lax") {
    Cookie c;
    REQUIRE(c.set_same_site("Lax   ") == CookieStatus::Ok);
    REQUIRE(c.get_same_site() == "Lax");
}

TEST_CASE("set_same_site accepts None") {
    Cookie c;
    REQUIRE(c.set_same_site("     None") == CookieStatus::Ok);
    REQUIRE(c.get_same_site() == "None");
}

TEST_CASE("set_same_site is case-insensitive strict") {
    Cookie c;
    REQUIRE(c.set_same_site("strict") == CookieStatus::Ok);
    REQUIRE(c.get_same_site() == "strict");
}

TEST_CASE("set_same_site is case-insensitive lax") {
    Cookie c;
    REQUIRE(c.set_same_site("lax") == CookieStatus::Ok);
    REQUIRE(c.get_same_site() == "lax");
}

TEST_CASE("set_same_site is case-insensitive none") {
    Cookie c;
    REQUIRE(c.set_same_site("  none") == CookieStatus::Ok);
    REQUIRE(c.get_same_site() == "none");
}

TEST_CASE("set_same_site trims leading and trailing whitespace before storing") {
    Cookie c;
    REQUIRE(c.set_same_site("  Strict  ") == CookieStatus::Ok);
    REQUIRE(c.get_same_site() == "Strict");
}

TEST_CASE("set_same_site rejects an unrecognised value") {
    Cookie c;
    REQUIRE(c.set_same_site("always") != CookieStatus::Ok);
}

// ---------------------------------------------------------------------------
// set_httponly
// ---------------------------------------------------------------------------

TEST_CASE("set_httponly accepts true") {
    Cookie c;
    auto e = c.set_httponly(std::string_view{"true"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts TRUE") {
    Cookie c;
    auto e = c.set_httponly(std::string_view{"TRUE"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_httponly() == true);
}

TEST_CASE("set_httponly accepts false") {
    Cookie c;
    auto e = c.set_httponly(std::string_view{"false"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly accepts FALSE") {
    Cookie c;
    auto e = c.set_httponly(std::string_view{"FALSE"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_httponly() == false);
}

TEST_CASE("set_httponly rejects an invalid boolean string") {
    Cookie c;
    auto e = c.set_httponly(std::string_view{"yes"});
    REQUIRE(e != CookieStatus::Ok);
}

// ---------------------------------------------------------------------------
// set_secure
// ---------------------------------------------------------------------------

TEST_CASE("set_secure accepts true") {
    Cookie c;
    auto e = c.set_secure(std::string_view{"true"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts TRUE") {
    Cookie c;
    auto e = c.set_secure(std::string_view{"TRUE"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_secure() == true);
}

TEST_CASE("set_secure accepts false") {
    Cookie c;
    auto e = c.set_secure(std::string_view{"false"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure accepts FALSE") {
    Cookie c;
    auto e = c.set_secure(std::string_view{"FALSE"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_secure() == false);
}

TEST_CASE("set_secure rejects an invalid boolean string") {
    Cookie c;
    auto e = c.set_secure(std::string_view{"1"});
    REQUIRE(e != CookieStatus::Ok);
}

// ---------------------------------------------------------------------------
// set_partitioned
// ---------------------------------------------------------------------------

TEST_CASE("set_partitioned accepts true") {
    Cookie c;
    auto e = c.set_partitioned(std::string_view{"true"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_partitioned() == true);
}

TEST_CASE("set_partitioned accepts TRUE") {
    Cookie c;
    auto e = c.set_partitioned(std::string_view{"TRUE"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_partitioned() == true);
}

TEST_CASE("set_partitioned accepts false") {
    Cookie c;
    auto e = c.set_partitioned(std::string_view{"false"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned accepts FALSE") {
    Cookie c;
    auto e = c.set_partitioned(std::string_view{"FALSE"});
    REQUIRE(e == CookieStatus::Ok);
    REQUIRE(c.get_partitioned() == false);
}

TEST_CASE("set_partitioned rejects an invalid boolean string") {
    Cookie c;
    auto e = c.set_partitioned(std::string_view{"no"});
    REQUIRE(e != CookieStatus::Ok);
}

// ---------------------------------------------------------------------------
// validate_secure / validate_partitioned
// ---------------------------------------------------------------------------

TEST_CASE("validate_secure returns no error when SameSite is Strict and Secure is false") {
    Cookie c;
    c.set_name("cookie_name");
    c.set_same_site("Strict");
    auto e = c.validate();
    REQUIRE(e == CookieStatus::Ok);
}

TEST_CASE("validate_secure returns error when SameSite is None and Secure is false") {
    Cookie c;
    c.set_secure("true");
    c.set_same_site("None");
    c.set_secure("false");
    auto e = c.validate();
    REQUIRE(e != CookieStatus::Ok);
}

TEST_CASE("validate_partitioned returns no error when SameSite==None and Secure is true and Partitioned is true") {
    Cookie c;
    c.set_name("cookie_name");
    c.set_secure("true");
    c.set_partitioned("true");
    c.set_same_site("NONE");
    auto e = c.validate();
    REQUIRE(e == CookieStatus::Ok);
}

TEST_CASE("validate_partitioned returns error when SameSite!=None and Secure is true and Partitioned is true") {
    Cookie c;
    c.set_name("cookie_name");
    c.set_secure("true");
    c.set_partitioned("true");
    c.set_same_site("lax");
    auto e = c.validate();
    REQUIRE(e != CookieStatus::Ok);
}

TEST_CASE("validate_partitioned returns error when Partitioned is true but Secure is false") {
    Cookie c;
    c.set_partitioned("true");
    auto e = c.validate();
    REQUIRE(e != CookieStatus::Ok);
}

// ---------------------------------------------------------------------------
// check cookie name + value sizes
// ---------------------------------------------------------------------------

TEST_CASE("Cookie set_name and set_value: minimal valid cookie", "[cookie][size]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("a") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("b") == CookieStatus::Ok);
}

TEST_CASE("Cookie set_name: single character name", "[cookie][size]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("x") == CookieStatus::Ok);
}

TEST_CASE("Cookie set_value: single character value", "[cookie][size]") {
    Cookie cookie;
    REQUIRE(cookie.set_value("y") == CookieStatus::Ok);
}

TEST_CASE("Cookie set_name and set_value: at maximum allowed size", "[cookie][size]") {
    Cookie cookie;
    const std::string name(2048, 'n');
    const std::string value(2048, 'v');
    REQUIRE(cookie.set_name(name) == CookieStatus::Ok);
    REQUIRE(cookie.set_value(value) == CookieStatus::Ok);
}

TEST_CASE("Cookie set_name and set_value: realistic cookie", "[cookie][size]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("session_id") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz") == CookieStatus::Ok);
}

TEST_CASE("Cookie set_value: special characters", "[cookie][size]") {
    Cookie cookie;
    const std::string value(4096, '\x42');
    REQUIRE(cookie.set_value(value) == CookieStatus::Ok);
}

TEST_CASE("Cookie Serialize - Basic Name and Value", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("session") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("xyz123") == CookieStatus::Ok);

    std::string expected = "Set-Cookie: session=xyz123\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Domain Attribute", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("id") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("42") == CookieStatus::Ok);
    REQUIRE(cookie.set_domain("example.com") == CookieStatus::Ok);

    std::string expected = "Set-Cookie: id=42; Domain=example.com\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Path Attribute", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("id") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("42") == CookieStatus::Ok);
    REQUIRE(cookie.set_path("/api") == CookieStatus::Ok);

    std::string expected = "Set-Cookie: id=42; Path=/api\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Expires Attribute", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("id") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("42") == CookieStatus::Ok);
    REQUIRE(cookie.set_expires("Wed, 21 Oct 2015 07:28:00 GMT") == CookieStatus::Ok);

    std::string expected = "Set-Cookie: id=42; Expires=Wed, 21 Oct 2015 07:28:00 GMT\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With SameSite Attribute", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("id") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("42") == CookieStatus::Ok);
    REQUIRE(cookie.set_same_site("Lax") == CookieStatus::Ok);

    std::string expected = "Set-Cookie: id=42; SameSite=Lax\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - With Boolean Security Flags", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("id") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("42") == CookieStatus::Ok);
    REQUIRE(cookie.set_same_site("none") == CookieStatus::Ok);

    cookie.set_secure("true");
    cookie.set_httponly("true");
    cookie.set_partitioned("true");

    std::string expected = "Set-Cookie: id=42; SameSite=none; Secure; HttpOnly; Partitioned\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Single Digit Max-Age", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("timeout") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("active") == CookieStatus::Ok);
    REQUIRE(cookie.set_max_age(0) == CookieStatus::Ok);

    std::string expected = "Set-Cookie: timeout=active; Max-Age=0\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Multi Digit Max-Age", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("timeout") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("active") == CookieStatus::Ok);
    REQUIRE(cookie.set_max_age(3600) == CookieStatus::Ok);

    std::string expected = "Set-Cookie: timeout=active; Max-Age=3600\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Large Max-Age String Input", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("timeout") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("active") == CookieStatus::Ok);
    REQUIRE(cookie.set_max_age("31536000") == CookieStatus::Ok);

    std::string expected = "Set-Cookie: timeout=active; Max-Age=31536000\r\n";
    REQUIRE(cookie.serialize() == expected);
}

TEST_CASE("Cookie Serialize - Complex Multi-Attribute Validation Pass", "[cookie][serialize]") {
    Cookie cookie;
    REQUIRE(cookie.set_name("__Secure-user_tracker") == CookieStatus::Ok);
    REQUIRE(cookie.set_value("hash_payload_alpha_9") == CookieStatus::Ok);
    REQUIRE(cookie.set_domain("sub.domain.org") == CookieStatus::Ok);
    REQUIRE(cookie.set_path("/") == CookieStatus::Ok);
    REQUIRE(cookie.set_expires("Tue, 19 Jan 2038 03:14:07 GMT") == CookieStatus::Ok);
    REQUIRE(cookie.set_max_age(86400) == CookieStatus::Ok);
    REQUIRE(cookie.set_same_site("None") == CookieStatus::Ok);

    cookie.set_secure("true");
    cookie.set_httponly("true");
    cookie.set_partitioned("true");

    REQUIRE(cookie.validate() == CookieStatus::Ok);

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
    Cookie cookie;
    CHECK(cookie.set_domain("example.123") == CookieStatus::DomainNumericTld);
}

TEST_CASE("domain with multi-label all-digit TLD is rejected", "[domain][tld]") {
    Cookie cookie;
    CHECK(cookie.set_domain("foo.bar.456") == CookieStatus::DomainNumericTld);
}

TEST_CASE("domain with single-digit TLD is rejected", "[domain][tld]") {
    Cookie cookie;
    CHECK(cookie.set_domain("test.0") == CookieStatus::DomainNumericTld);
}

TEST_CASE("domain with digit-suffixed TLD is rejected", "[domain][tld]") {
    Cookie cookie;
    CHECK(cookie.set_domain("example.co1") == CookieStatus::DomainNumericTld);
}

TEST_CASE("domain with digit-prefixed TLD is rejected", "[domain][tld]") {
    Cookie cookie;
    CHECK(cookie.set_domain("example.1com") == CookieStatus::DomainNumericTld);
}

TEST_CASE("domain with digit embedded in TLD is rejected", "[domain][tld]") {
    Cookie cookie;
    CHECK(cookie.set_domain("example.c0m") == CookieStatus::DomainNumericTld);
}

TEST_CASE("digits in non-TLD labels are accepted", "[domain][tld]") {
    Cookie cookie;
    CHECK(cookie.set_domain("test1.example.com") == CookieStatus::Ok);
}
