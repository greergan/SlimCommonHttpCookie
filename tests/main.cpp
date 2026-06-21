#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>
#include <slim/common/http/cookie.h>

using slim::common::http::Cookie;
using slim::common::http::HttpHeaderException;
using slim::common::http::ErrorStatus;

// ---------------------------------------------------------------------------
// Status Verification
// ---------------------------------------------------------------------------
TEST_CASE("cookie::status verification", "[cookie][status]") {
    using namespace slim::common::http::error::status;
    SECTION("Size matches enum count") { REQUIRE(strings.size() == static_cast<std::size_t>(ErrorStatus::END)); }
    SECTION("Mapping of specific status codes") {
        REQUIRE(to_string(ErrorStatus::OK) == "OK");
        REQUIRE(to_string(ErrorStatus::CookieTooLarge) == "Cookie exceeds the maximum allowed size");
        REQUIRE(to_string(ErrorStatus::CookieNameEmpty) == "Cookie name cannot be empty");
        REQUIRE(to_string(ErrorStatus::CookieSameSiteNoneRequiresSecure) == "Cookie SameSite=None requires the Secure attribute");
        REQUIRE(to_string(ErrorStatus::CookieValueUnmatchedQuote) == "Cookie value has an unmatched quotation mark");
    }
    SECTION("Out-of-bounds handling") { REQUIRE(to_string(static_cast<ErrorStatus>(99)) == "Unknown"); }
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
TEST_CASE("cookie::constructor", "[cookie][constructor]") {
    SECTION("valid name and value") {
        Cookie c("session", "abc123");
        REQUIRE(c.get_name() == "session");
        REQUIRE(c.get_value() == "abc123");
    }
    SECTION("trims whitespace from name and value") {
        Cookie c("  session  ", "  abc123  ");
        REQUIRE(c.get_name() == "session");
        REQUIRE(c.get_value() == "abc123");
    }
    SECTION("throws on empty name") { REQUIRE_THROWS_AS(Cookie("", "abc123"), HttpHeaderException); }
    SECTION("accepts empty value") { REQUIRE_NOTHROW(Cookie("session", "")); }
    SECTION("throws on invalid name character") { REQUIRE_THROWS_AS(Cookie("bad=name", "abc123"), HttpHeaderException); }
    SECTION("throws on invalid value character") { REQUIRE_THROWS_AS(Cookie("session", "bad value"), HttpHeaderException); }
    SECTION("valid cookie serializes correctly") {
        Cookie c("session", "abc123");
        REQUIRE(c.serialize() == "Set-Cookie: session=abc123\r\n");
    }
}

// ---------------------------------------------------------------------------
// set_name
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_name", "[cookie][set_name]") {
    Cookie c;
    SECTION("accepts a valid token") {
        REQUIRE(c.set_name("   session_id   ") == ErrorStatus::OK);
        REQUIRE(c.get_name() == "session_id");
    }
    SECTION("rejects an empty string") { REQUIRE(c.set_name("") != ErrorStatus::OK); }
    SECTION("rejects a name containing a separator character") { REQUIRE(c.set_name("bad=name") != ErrorStatus::OK); }
    SECTION("rejects a name with a space") { REQUIRE(c.set_name("bad name") != ErrorStatus::OK); }
    SECTION("rejects a name with a control character") { REQUIRE(c.set_name("bad\x01name") != ErrorStatus::OK); }
    SECTION("rejects a name containing a colon") { REQUIRE(c.set_name("bad:name") != ErrorStatus::OK); }
    SECTION("trims leading and trailing whitespace before storing") {
        REQUIRE(c.set_name("  session_id  ") == ErrorStatus::OK);
        REQUIRE(c.get_name() == "session_id");
    }
}

// ---------------------------------------------------------------------------
// operator==
// ---------------------------------------------------------------------------
TEST_CASE("cookie::operator==", "[cookie][operator]") {
    SECTION("identical name, domain, and path are equal") {
        Cookie c1("session", "abc123");
        c1.set_domain("example.com");
        c1.set_path("/");

        Cookie c2("session", "abc123");
        c2.set_domain("example.com");
        c2.set_path("/");

        REQUIRE(c1 == c2);
    }
    SECTION("same name and domain, different path are not equal") {
        Cookie c1("session", "abc123");
        c1.set_domain("example.com");
        c1.set_path("/");

        Cookie c2("session", "abc123");
        c2.set_domain("example.com");
        c2.set_path("/api");

        REQUIRE(c1 != c2);
    }
    SECTION("same name and path, different domain are not equal") {
        Cookie c1("session", "abc123");
        c1.set_domain("example.com");
        c1.set_path("/");

        Cookie c2("session", "abc123");
        c2.set_domain("sub.example.com");
        c2.set_path("/");

        REQUIRE(c1 != c2);
    }
    SECTION("different name, same domain and path are not equal") {
        Cookie c1("session", "abc123");
        c1.set_domain("example.com");
        c1.set_path("/");

        Cookie c2("user", "abc123");
        c2.set_domain("example.com");
        c2.set_path("/");

        REQUIRE(c1 != c2);
    }
    SECTION("same name, no domain, no path are equal") {
        Cookie c1("session", "abc123");
        Cookie c2("session", "xyz789");
        REQUIRE(c1 == c2);
    }
    SECTION("same name, one has domain and one does not are not equal") {
        Cookie c1("session", "abc123");
        c1.set_domain("example.com");

        Cookie c2("session", "abc123");

        REQUIRE(c1 != c2);
    }
    SECTION("same name, one has path and one does not are not equal") {
        Cookie c1("session", "abc123");
        c1.set_path("/");

        Cookie c2("session", "abc123");

        REQUIRE(c1 != c2);
    }
    SECTION("different values with same name, domain, and path are equal") {
        Cookie c1("session", "abc123");
        c1.set_domain("example.com");
        c1.set_path("/");

        Cookie c2("session", "xyz789");
        c2.set_domain("example.com");
        c2.set_path("/");

        REQUIRE(c1 == c2);
    }
    SECTION("name comparison is case-sensitive") {
        Cookie c1("Session", "abc123");
        Cookie c2("session", "abc123");
        REQUIRE(c1 != c2);
    }
    SECTION("equal and not-equal with default-constructed cookies") {
        Cookie c1, c2, c3;
        REQUIRE(c1.set_name("cookie_name") == ErrorStatus::OK);
        REQUIRE(c2.set_name("cookie_name") == ErrorStatus::OK);
        REQUIRE(c1 == c2);
        REQUIRE(c3.set_name("cookie_three") == ErrorStatus::OK);
        REQUIRE(c1 != c3);
    }
}

// ---------------------------------------------------------------------------
// set_value
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_value", "[cookie][set_value]") {
    Cookie c;
    SECTION("accepts a plain ASCII value") {
        REQUIRE(c.set_value("\t abc123\t") == ErrorStatus::OK);
        REQUIRE(c.get_value() == "abc123");
    }
    SECTION("accepts a properly double-quoted value") { REQUIRE(c.set_value("\"abc123\"") == ErrorStatus::OK); }
    SECTION("rejects an unmatched leading double quote") { REQUIRE(c.set_value("\"abc") != ErrorStatus::OK); }
    SECTION("rejects a value with a space character") { REQUIRE(c.set_value("bad value") != ErrorStatus::OK); }
    SECTION("rejects a value with a semicolon") { REQUIRE(c.set_value("bad;value") != ErrorStatus::OK); }
    SECTION("rejects a value with a DEL character") { REQUIRE(c.set_value("bad\x7Fvalue") != ErrorStatus::OK); }
    SECTION("trims leading and trailing whitespace before storing") {
        REQUIRE(c.set_value("  abc123  ") == ErrorStatus::OK);
        REQUIRE(c.get_value() == "abc123");
    }
}

// ---------------------------------------------------------------------------
// set_path
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_path", "[cookie][set_path]") {
    Cookie c;
    SECTION("accepts the root path") {
        REQUIRE(c.set_path("/") == ErrorStatus::OK);
        REQUIRE(c.get_path() == "/");
    }
    SECTION("accepts a multi-segment path") {
        REQUIRE(c.set_path("/api/v1/resource") == ErrorStatus::OK);
        REQUIRE(c.get_path() == "/api/v1/resource");
    }
    SECTION("rejects a path that does not start with slash") { REQUIRE(c.set_path("api/v1") != ErrorStatus::OK); }
    SECTION("accepts an empty path") { REQUIRE(c.set_path("") == ErrorStatus::OK); }
    SECTION("rejects a path containing a semicolon") { REQUIRE(c.set_path("/bad;path") != ErrorStatus::OK); }
    SECTION("rejects a path with a control character") { REQUIRE(c.set_path("/bad\x1Fpath") != ErrorStatus::OK); }
    SECTION("trims leading and trailing whitespace before storing") {
        REQUIRE(c.set_path("  /api/v1  ") == ErrorStatus::OK);
        REQUIRE(c.get_path() == "/api/v1");
    }
}

// ---------------------------------------------------------------------------
// set_domain
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_domain", "[cookie][set_domain]") {
    Cookie c;
    SECTION("accepts a simple hostname") {
        REQUIRE(c.set_domain("example.com") == ErrorStatus::OK);
        REQUIRE(c.get_domain() == "example.com");
    }
    SECTION("accepts a domain with a leading dot") { REQUIRE(c.set_domain(".example.com") == ErrorStatus::OK); }
    SECTION("accepts a subdomain") { REQUIRE(c.set_domain("sub.example.com") == ErrorStatus::OK); }
    SECTION("rejects an empty string") { REQUIRE(c.set_domain("") != ErrorStatus::OK); }
    SECTION("rejects a bare dot") { REQUIRE(c.set_domain(".") != ErrorStatus::OK); }
    SECTION("rejects a domain with a trailing dot") { REQUIRE(c.set_domain("example.com.") != ErrorStatus::OK); }
    SECTION("rejects a label starting with a hyphen") { REQUIRE(c.set_domain("-bad.example.com") != ErrorStatus::OK); }
    SECTION("rejects a label ending with a hyphen") { REQUIRE(c.set_domain("bad-.example.com") != ErrorStatus::OK); }
    SECTION("rejects a domain exceeding 253 characters") {
        std::string long_domain(63, 'a');
        long_domain += ".";
        long_domain += std::string(63, 'b');
        long_domain += ".";
        long_domain += std::string(63, 'c');
        long_domain += ".";
        long_domain += std::string(63, 'd');
        REQUIRE(c.set_domain(long_domain) != ErrorStatus::OK);
    }
    SECTION("rejects a label with an invalid character") { REQUIRE(c.set_domain("bad_label.example.com") != ErrorStatus::OK); }
    SECTION("trims leading and trailing whitespace before storing") {
        REQUIRE(c.set_domain("  example.com  ") == ErrorStatus::OK);
        REQUIRE(c.get_domain() == "example.com");
    }
    SECTION("rejects an all-digit TLD") { CHECK(c.set_domain("example.123") == ErrorStatus::CookieDomainNumericTld); }
    SECTION("rejects a multi-label all-digit TLD") { CHECK(c.set_domain("foo.bar.456") == ErrorStatus::CookieDomainNumericTld); }
    SECTION("rejects a single-digit TLD") { CHECK(c.set_domain("test.0") == ErrorStatus::CookieDomainNumericTld); }
    SECTION("rejects a digit-suffixed TLD") { CHECK(c.set_domain("example.co1") == ErrorStatus::CookieDomainNumericTld); }
    SECTION("rejects a digit-prefixed TLD") { CHECK(c.set_domain("example.1com") == ErrorStatus::CookieDomainNumericTld); }
    SECTION("rejects a digit embedded in TLD") { CHECK(c.set_domain("example.c0m") == ErrorStatus::CookieDomainNumericTld); }
    SECTION("accepts digits in non-TLD labels") { CHECK(c.set_domain("test1.example.com") == ErrorStatus::OK); }
}

// ---------------------------------------------------------------------------
// set_expires
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_expires", "[cookie][set_expires]") {
    Cookie c;
    SECTION("accepts an RFC 1123 date") {
        REQUIRE(c.set_expires("  Thu, 01 Jan 2099 00:00:00 GMT  ") == ErrorStatus::OK);
        REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
    }
    SECTION("accepts an RFC 850 date") {
        REQUIRE(c.set_expires("Thursday, 01-Jan-99 00:00:00 GMT") == ErrorStatus::OK);
    }
    SECTION("accepts an ANSI C asctime date") {
        REQUIRE(c.set_expires("Thu Jan  1 00:00:00 2099") == ErrorStatus::OK);
    }
    SECTION("rejects a malformed date string") { REQUIRE(c.set_expires("not-a-date") != ErrorStatus::OK); }
    SECTION("rejects an ISO 8601 date") { REQUIRE(c.set_expires("2099-01-01T00:00:00Z") != ErrorStatus::OK); }
    SECTION("trims leading and trailing whitespace before storing") {
        REQUIRE(c.set_expires("  Thu, 01 Jan 2099 00:00:00 GMT  ") == ErrorStatus::OK);
        REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 00:00:00 GMT");
    }
    SECTION("translates a valid asctime date with a single-digit day") {
        REQUIRE(c.set_expires("Thu Jan  1 12:30:00 2099") == ErrorStatus::OK);
        REQUIRE(c.get_expires() == "Thu, 01 Jan 2099 12:30:00 GMT");
    }
}

// ---------------------------------------------------------------------------
// set_max_age
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_max_age", "[cookie][set_max_age]") {
    Cookie c;
    SECTION("accepts zero using numeric") {
        REQUIRE(c.set_max_age(0) == ErrorStatus::OK);
        REQUIRE(c.get_max_age() == 0u);
    }
    SECTION("accepts zero using string") {
        REQUIRE(c.set_max_age("0") == ErrorStatus::OK);
        REQUIRE(c.get_max_age() == 0u);
    }
    SECTION("accepts a positive integer") {
        REQUIRE(c.set_max_age(3600) == ErrorStatus::OK);
        REQUIRE(c.get_max_age() == 3600u);
    }
    SECTION("accepts a positive integer string") {
        REQUIRE(c.set_max_age("3600") == ErrorStatus::OK);
        REQUIRE(c.get_max_age() == 3600u);
    }
    SECTION("accepts the maximum time_t value") {
        constexpr std::uint_least64_t max_val = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max());
        REQUIRE(c.set_max_age(std::to_string(max_val)) == ErrorStatus::OK);
        REQUIRE(c.get_max_age() == max_val);
    }
    SECTION("rejects a value exceeding the maximum time_t value") {
        constexpr std::uint_least64_t over_max = static_cast<std::uint_least64_t>(std::numeric_limits<std::time_t>::max()) + 1;
        REQUIRE(c.set_max_age(std::to_string(over_max)) != ErrorStatus::OK);
    }
    SECTION("translates a negative integer to 0") {
        REQUIRE(c.set_max_age("-1") == ErrorStatus::OK);
        REQUIRE(c.get_max_age() == 0);
    }
    SECTION("rejects a non-numeric string") { REQUIRE(c.set_max_age("abc") != ErrorStatus::OK); }
    SECTION("rejects a value with trailing characters") { REQUIRE(c.set_max_age("3600x") != ErrorStatus::OK); }
    SECTION("rejects an empty string") { REQUIRE(c.set_max_age("") != ErrorStatus::OK); }
}

// ---------------------------------------------------------------------------
// set_same_site
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_same_site", "[cookie][set_same_site]") {
    Cookie c;
    SECTION("accepts Strict") {
        REQUIRE(c.set_same_site(" Strict\t") == ErrorStatus::OK);
        REQUIRE(c.get_same_site() == "Strict");
    }
    SECTION("accepts Lax") {
        REQUIRE(c.set_same_site("Lax   ") == ErrorStatus::OK);
        REQUIRE(c.get_same_site() == "Lax");
    }
    SECTION("accepts None") {
        REQUIRE(c.set_same_site("     None") == ErrorStatus::OK);
        REQUIRE(c.get_same_site() == "None");
    }
    SECTION("case-insensitive: strict") {
        REQUIRE(c.set_same_site("strict") == ErrorStatus::OK);
        REQUIRE(c.get_same_site() == "strict");
    }
    SECTION("case-insensitive: lax") {
        REQUIRE(c.set_same_site("lax") == ErrorStatus::OK);
        REQUIRE(c.get_same_site() == "lax");
    }
    SECTION("case-insensitive: none") {
        REQUIRE(c.set_same_site("  none") == ErrorStatus::OK);
        REQUIRE(c.get_same_site() == "none");
    }
    SECTION("trims leading and trailing whitespace before storing") {
        REQUIRE(c.set_same_site("  Strict  ") == ErrorStatus::OK);
        REQUIRE(c.get_same_site() == "Strict");
    }
    SECTION("rejects an unrecognised value") { REQUIRE(c.set_same_site("always") != ErrorStatus::OK); }
}

// ---------------------------------------------------------------------------
// set_httponly
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_httponly", "[cookie][set_httponly]") {
    Cookie c;
    SECTION("accepts true") {
        REQUIRE(c.set_httponly(std::string_view{"true"}) == ErrorStatus::OK);
        REQUIRE(c.get_httponly() == true);
    }
    SECTION("accepts TRUE") {
        REQUIRE(c.set_httponly(std::string_view{"TRUE"}) == ErrorStatus::OK);
        REQUIRE(c.get_httponly() == true);
    }
    SECTION("accepts false") {
        REQUIRE(c.set_httponly(std::string_view{"false"}) == ErrorStatus::OK);
        REQUIRE(c.get_httponly() == false);
    }
    SECTION("accepts FALSE") {
        REQUIRE(c.set_httponly(std::string_view{"FALSE"}) == ErrorStatus::OK);
        REQUIRE(c.get_httponly() == false);
    }
    SECTION("rejects an invalid boolean string") { REQUIRE(c.set_httponly(std::string_view{"yes"}) != ErrorStatus::OK); }
}

// ---------------------------------------------------------------------------
// set_secure
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_secure", "[cookie][set_secure]") {
    Cookie c;
    SECTION("accepts true") {
        REQUIRE(c.set_secure(std::string_view{"true"}) == ErrorStatus::OK);
        REQUIRE(c.get_secure() == true);
    }
    SECTION("accepts TRUE") {
        REQUIRE(c.set_secure(std::string_view{"TRUE"}) == ErrorStatus::OK);
        REQUIRE(c.get_secure() == true);
    }
    SECTION("accepts false") {
        REQUIRE(c.set_secure(std::string_view{"false"}) == ErrorStatus::OK);
        REQUIRE(c.get_secure() == false);
    }
    SECTION("accepts FALSE") {
        REQUIRE(c.set_secure(std::string_view{"FALSE"}) == ErrorStatus::OK);
        REQUIRE(c.get_secure() == false);
    }
    SECTION("rejects an invalid boolean string") { REQUIRE(c.set_secure(std::string_view{"1"}) != ErrorStatus::OK); }
}

// ---------------------------------------------------------------------------
// set_partitioned
// ---------------------------------------------------------------------------
TEST_CASE("cookie::set_partitioned", "[cookie][set_partitioned]") {
    Cookie c;
    SECTION("accepts true") {
        REQUIRE(c.set_partitioned(std::string_view{"true"}) == ErrorStatus::OK);
        REQUIRE(c.get_partitioned() == true);
    }
    SECTION("accepts TRUE") {
        REQUIRE(c.set_partitioned(std::string_view{"TRUE"}) == ErrorStatus::OK);
        REQUIRE(c.get_partitioned() == true);
    }
    SECTION("accepts false") {
        REQUIRE(c.set_partitioned(std::string_view{"false"}) == ErrorStatus::OK);
        REQUIRE(c.get_partitioned() == false);
    }
    SECTION("accepts FALSE") {
        REQUIRE(c.set_partitioned(std::string_view{"FALSE"}) == ErrorStatus::OK);
        REQUIRE(c.get_partitioned() == false);
    }
    SECTION("rejects an invalid boolean string") { REQUIRE(c.set_partitioned(std::string_view{"no"}) != ErrorStatus::OK); }
}

// ---------------------------------------------------------------------------
// validate
// ---------------------------------------------------------------------------
TEST_CASE("cookie::validate", "[cookie][validate]") {
    Cookie c;
    SECTION("no error when SameSite is Strict and Secure is false") {
        c.set_name("cookie_name");
        c.set_same_site("Strict");
        REQUIRE(c.validate() == ErrorStatus::OK);
    }
    SECTION("error when SameSite is None and Secure is false") {
        c.set_secure("true");
        c.set_same_site("None");
        c.set_secure("false");
        REQUIRE(c.validate() != ErrorStatus::OK);
    }
    SECTION("no error when SameSite==None and Secure is true and Partitioned is true") {
        c.set_name("cookie_name");
        c.set_secure("true");
        c.set_partitioned("true");
        c.set_same_site("NONE");
        REQUIRE(c.validate() == ErrorStatus::OK);
    }
    SECTION("error when SameSite!=None and Secure is true and Partitioned is true") {
        c.set_name("cookie_name");
        c.set_secure("true");
        c.set_partitioned("true");
        c.set_same_site("lax");
        REQUIRE(c.validate() != ErrorStatus::OK);
    }
    SECTION("error when Partitioned is true but Secure is false") {
        c.set_partitioned("true");
        REQUIRE(c.validate() != ErrorStatus::OK);
    }
}

// ---------------------------------------------------------------------------
// size checks
// ---------------------------------------------------------------------------
TEST_CASE("cookie::size", "[cookie][size]") {
    Cookie cookie;
    SECTION("minimal valid cookie") {
        REQUIRE(cookie.set_name("a") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("b") == ErrorStatus::OK);
    }
    SECTION("single character name") { REQUIRE(cookie.set_name("x") == ErrorStatus::OK); }
    SECTION("single character value") { REQUIRE(cookie.set_value("y") == ErrorStatus::OK); }
    SECTION("at maximum allowed size") {
        const std::string name(2048, 'n');
        const std::string value(2048, 'v');
        REQUIRE(cookie.set_name(name) == ErrorStatus::OK);
        REQUIRE(cookie.set_value(value) == ErrorStatus::OK);
    }
    SECTION("realistic cookie") {
        REQUIRE(cookie.set_name("session_id") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("abc123def456ghi789jkl012mno345pqr678stu901vwx234yz") == ErrorStatus::OK);
    }
    SECTION("special characters in value") {
        const std::string value(4096, '\x42');
        REQUIRE(cookie.set_value(value) == ErrorStatus::OK);
    }
}

// ---------------------------------------------------------------------------
// serialize
// ---------------------------------------------------------------------------
TEST_CASE("cookie::serialize", "[cookie][serialize]") {
    SECTION("basic name and value") {
        Cookie cookie;
        REQUIRE(cookie.set_name("session") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("xyz123") == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: session=xyz123\r\n");
    }
    SECTION("with Domain attribute") {
        Cookie cookie;
        REQUIRE(cookie.set_name("id") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("42") == ErrorStatus::OK);
        REQUIRE(cookie.set_domain("example.com") == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: id=42; Domain=example.com\r\n");
    }
    SECTION("with Path attribute") {
        Cookie cookie;
        REQUIRE(cookie.set_name("id") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("42") == ErrorStatus::OK);
        REQUIRE(cookie.set_path("/api") == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: id=42; Path=/api\r\n");
    }
    SECTION("with Expires attribute") {
        Cookie cookie;
        REQUIRE(cookie.set_name("id") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("42") == ErrorStatus::OK);
        REQUIRE(cookie.set_expires("Wed, 21 Oct 2015 07:28:00 GMT") == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: id=42; Expires=Wed, 21 Oct 2015 07:28:00 GMT\r\n");
    }
    SECTION("with SameSite attribute") {
        Cookie cookie;
        REQUIRE(cookie.set_name("id") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("42") == ErrorStatus::OK);
        REQUIRE(cookie.set_same_site("Lax") == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: id=42; SameSite=Lax\r\n");
    }
    SECTION("with boolean security flags") {
        Cookie cookie;
        REQUIRE(cookie.set_name("id") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("42") == ErrorStatus::OK);
        REQUIRE(cookie.set_same_site("none") == ErrorStatus::OK);
        cookie.set_secure("true");
        cookie.set_httponly("true");
        cookie.set_partitioned("true");
        REQUIRE(cookie.serialize() == "Set-Cookie: id=42; SameSite=none; Secure; HttpOnly; Partitioned\r\n");
    }
    SECTION("single digit Max-Age") {
        Cookie cookie;
        REQUIRE(cookie.set_name("timeout") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("active") == ErrorStatus::OK);
        REQUIRE(cookie.set_max_age(0) == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: timeout=active; Max-Age=0\r\n");
    }
    SECTION("multi digit Max-Age") {
        Cookie cookie;
        REQUIRE(cookie.set_name("timeout") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("active") == ErrorStatus::OK);
        REQUIRE(cookie.set_max_age(3600) == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: timeout=active; Max-Age=3600\r\n");
    }
    SECTION("large Max-Age string input") {
        Cookie cookie;
        REQUIRE(cookie.set_name("timeout") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("active") == ErrorStatus::OK);
        REQUIRE(cookie.set_max_age("31536000") == ErrorStatus::OK);
        REQUIRE(cookie.serialize() == "Set-Cookie: timeout=active; Max-Age=31536000\r\n");
    }
    SECTION("complex multi-attribute validation pass") {
        Cookie cookie;
        REQUIRE(cookie.set_name("__Secure-user_tracker") == ErrorStatus::OK);
        REQUIRE(cookie.set_value("hash_payload_alpha_9") == ErrorStatus::OK);
        REQUIRE(cookie.set_domain("sub.domain.org") == ErrorStatus::OK);
        REQUIRE(cookie.set_path("/") == ErrorStatus::OK);
        REQUIRE(cookie.set_expires("Tue, 19 Jan 2038 03:14:07 GMT") == ErrorStatus::OK);
        REQUIRE(cookie.set_max_age(86400) == ErrorStatus::OK);
        REQUIRE(cookie.set_same_site("None") == ErrorStatus::OK);
        cookie.set_secure("true");
        cookie.set_httponly("true");
        cookie.set_partitioned("true");
        REQUIRE(cookie.validate() == ErrorStatus::OK);
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
}
