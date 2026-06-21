<a href="https://codeberg.org/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>   

# SlimCommonHttpCookie

A lightweight, RFC6265-oriented HTTP cookie implementation in modern C++.  
Acts as a validating, backing store for the [SlimTS](https://codeberg.org/greergan/SlimTS) Javascript Cookie object.  
Part of the [SlimCommon](https://codeberg.org/greergan/SlimCommon) library.  
Dependency of the [SlimCommonHttpCookieStore](https://codeberg.org/greergan/SlimCommonHttpCookieStore) micro-ibrary.  
Built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).  
CI/CD supplied by unified workflows provided by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Core API](#core-api)
  - [ErrorStatus enum](#errorstatus-enum)
  - [HttpHeaderException](#httpheaderexception)
  - [Cookie class](#cookie-class)
  - [Constructors and object lifetime](#constructors-and-object-lifetime)
  - [Operators](#operators)
  - [Setters](#setters)
  - [Getters](#getters)
  - [Friend classes](#friend-classes)
  - [Validation](#validation)
  - [Serialization](#serialization)
- [Building](#building)
- [Dependencies](#dependencies)
- [Examples](#examples)

## Overview

This library provides a strict, validation-heavy HTTP cookie parser and serializer with:
- RFC 6265 compliant parsing behavior
- Strong validation for all cookie attributes
- Zero dynamic allocation in validation paths (where possible)
- Explicit status reporting via [`ErrorStatus`](https://codeberg.org/greergan/SlimCommonHttp)
- Strict parsing over permissive recovery
- Explicit validation at each setter
- Minimal runtime overhead in hot paths
- Heavy use of `noexcept`

[↑ Top](#table-of-contents)

## Features

| Feature | Description |
|--------|-------------|
| Cookie size | Limited to 4096 bytes |
| Cookie uniqueness | RFC 6265 §5.3 — identified by name (case-sensitive), domain, and path |
| Name/Value validation | Strict RFC-style character filtering |
| Domain validation | Full label + length + syntax checks |
| Expires parsing | RFC 6265 cookie-date tokenizer stored as RFC 1123 format |
| Max-Age support | String + integer parsing with overflow checks |
| Partitioned | CHIPS validation enforced on outgoing requests |
| SameSite handling | strict / lax / none validation |
| Secure / HttpOnly / Partitioned | Boolean attribute parsing |
| Serialize | Preallocated, zero-fragment string build |
| Serialize | Cookie validation with thrown exceptions |
| Error model | Strong enum-based status reporting via `ErrorStatus` (from [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)) |

[↑ Top](#table-of-contents)

## Core API

### ErrorStatus enum

`ErrorStatus` is the scoped enum provided by [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp).

Values are grouped by concern rather than by call site:

| Group | Examples | Meaning |
|-------|----------|---------|
| Name | `CookieNameEmpty`, `CookieInvalidName`, `CookieNameInvalidChar` | Cookie name is missing or contains disallowed characters |
| Name prefixes | `CookieNamePrefixRequiresSecure`, `CookieNameHostPrefixHasDomain`, `CookieNameHostPrefixInvalidPath` | `__Secure-` / `__Host-` prefix rules were violated |
| Value | `CookieInvalidValue`, `CookieValueInvalidChar`, `CookieValueUnmatchedQuote` | Cookie value is malformed |
| Domain | `CookieDomainEmpty`, `CookieDomainTooLong`, `CookieDomainBareDot`, `CookieDomainTrailingDot`, `CookieDomainNumericTld`, `CookieDomainLabelEmpty`, `CookieDomainLabelTooLong`, `CookieDomainLabelInvalidHyphen`, `CookieDomainInvalidChar` | Domain attribute fails RFC label/length/syntax checks |
| Path | `CookiePathMissingLeadingSlash`, `CookiePathInvalidChar` | Path attribute is malformed |
| Expires / Max-Age | `CookieExpiresInvalidFormat`, `CookieMaxAgeEmpty`, `CookieMaxAgeInvalidFormat`, `CookieMaxAgeTrailingChars`, `CookieMaxAgeExceedsLimit` | Date or lifetime attribute could not be parsed |
| SameSite | `CookieSameSiteInvalid`, `CookieSameSiteNoneRequiresSecure` | SameSite value or Secure-pairing rule violated |
| Partitioned (CHIPS) | `CookiePartitionedRequiresSecure`, `CookiePartitionedRequiresSameSiteNone` | Partitioned attribute used without its required companions |
| General | `CookieEmptyString`, `CookieTooLarge`, `CookieInvalidBoolean`, `CookieMalformedMissingEquals`, `CookieMalformedPairMissingEquals` | Whole-cookie or parsing-level failures |
| Header-level | `HeaderNameEmpty`, `HeaderNameInvalidChar`, `HeaderValueEmpty`, `HeaderValueInvalidChar`, `HeaderValueInvalidFolding`, `HeaderDelimiterInvalid` | Shared with other `SlimCommonHttp` header types |
| `OK` | — | No error; the operation succeeded |

Every value also has a human-readable description, retrievable without allocation:

```cpp
[[nodiscard]] constexpr std::string_view error::status::to_string(ErrorStatus status) noexcept;
```

```cpp
ErrorStatus e = c.set_domain("-bad-.com");
if (e != ErrorStatus::OK) {
    std::cerr << error::status::to_string(e) << '\n';
    // -> "Cookie domain label has a hyphen in an invalid position"
}
```

`to_string` is `constexpr` and backed by a fixed `std::array<std::string_view, ...>`, so lookups are O(1) and allocation-free; an out-of-range value safely returns `"Unknown"` rather than invoking undefined behavior.

[↑ Top](#table-of-contents)

### HttpHeaderException

`HttpHeaderException` is the exception class provided by [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp).  


[↑ Top](#table-of-contents)

### Cookie class

```cpp
slim::common::http::Cookie c;
```

### Constructors and object lifetime

| Form | Description |
|------|-------------|
| `Cookie()` | Default constructor, produces an empty cookie |
| `Cookie(std::string_view name, std::string_view value)` | Construct with name and value, validated immediately. Throws `HttpHeaderException` on failure |
| `Cookie(const Cookie&)` | Deleted — copies are not allowed |
| `Cookie& operator=(const Cookie&)` | Deleted — copies are not allowed |
| `Cookie(Cookie&&) noexcept` | Move construction is supported |
| `Cookie& operator=(Cookie&&) noexcept` | Move assignment is supported |

[↑ Top](#table-of-contents)

### Operators

| Operator | Description |
|----------|-------------|
| `bool operator==(const Cookie&) const noexcept` | Equality by name (case-sensitive), domain, and path |

Per RFC 6265 §5.3, a cookie is uniquely identified by the tuple of name, domain, and path. Two cookies sharing the same name are distinct if they differ in domain or path. Cookie names are case-sensitive — `Session` and `session` are different cookies. Value, expiry, and flags are not considered in equality.

[↑ Top](#table-of-contents)


### Setters

| Method | Description |
|--------|-------------|
| `ErrorStatus set_name(std::string_view) noexcept` | Set cookie name (validated) |
| `ErrorStatus set_value(std::string_view) noexcept` | Set cookie value (validated) |
| `ErrorStatus set_domain(std::string_view) noexcept` | Set domain with RFC checks |
| `ErrorStatus set_path(std::string_view) noexcept` | Set path (must start with `/`) |
| `ErrorStatus set_expires(std::string_view) noexcept` | Set expiry string (RFC 6265 cookie-date parser) |
| `ErrorStatus set_max_age(std::uint_least64_t) noexcept` | Set Max-Age from integer |
| `ErrorStatus set_max_age(std::string_view) noexcept` | Set Max-Age from string |
| `ErrorStatus set_same_site(std::string_view) noexcept` | Set SameSite policy |
| `void set_secure(bool) noexcept` | Set Secure flag |
| `ErrorStatus set_secure(std::string_view) noexcept` | Set Secure flag from string |
| `void set_httponly(bool) noexcept` | Set HttpOnly flag |
| `ErrorStatus set_httponly(std::string_view) noexcept` | Set HttpOnly flag from string |
| `void set_partitioned(bool) noexcept` | Set Partitioned flag |
| `ErrorStatus set_partitioned(std::string_view) noexcept` | Set Partitioned flag from string |

[↑ Top](#table-of-contents)

### Getters

| Method | Returns |
|--------|---------|
| `std::string_view get_name() const noexcept` | Cookie name |
| `std::string_view get_value() const noexcept` | Cookie value |
| `std::optional<std::string> get_domain() const noexcept` | Domain, if set |
| `std::optional<std::string> get_path() const noexcept` | Path, if set |
| `std::optional<std::string> get_expires() const noexcept` | Expires, if set |
| `std::optional<std::uint_least64_t> get_max_age() const noexcept` | Max-Age, if set |
| `std::optional<std::string> get_same_site() const noexcept` | SameSite, if set |
| `bool get_secure() const noexcept` | Secure flag (default: `false`) |
| `bool get_httponly() const noexcept` | HttpOnly flag (default: `false`) |
| `bool get_partitioned() const noexcept` | Partitioned flag (default: `false`) |

[↑ Top](#table-of-contents)

### Friend classes

```cpp
friend class CookieStore;
```

[↑ Top](#table-of-contents)

### Validation

```cpp
ErrorStatus Cookie::validate() const noexcept;
```

Checks:
- Secure/SameSite consistency
- Partitioned requires Secure
- Host prefix rules
- Domain/path correctness

[↑ Top](#table-of-contents)

### Serialization

```cpp
std::string Cookie::serialize() const;
// -> "Set-Cookie: __Secure-session=abc123; Domain=example.com; Path=/; Max-Age=3600; SameSite=None; Secure; HttpOnly; Partitioned\r\n"
```

Outputs a fully formatted `Set-Cookie` header string.  
Throws `HttpHeaderException` if validation fails or the cookie exceeds 4096 bytes.

[↑ Top](#table-of-contents)

## Building

This library is built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager). See that repository for build instructions.

[↑ Top](#table-of-contents)

## Dependencies

External package dependencies for this library are declared in the [`required_packages`](https://codeberg.org/greergan/SlimCommonHttpCookie/src/branch/master/required_packages) file at the repository root. This file is read by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) during the build process to resolve dependencies and install them if not present.

```
SlimCommonHttp
```

- [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)

[↑ Top](#table-of-contents)

## Examples

```cpp
// Parameterised constructor with status checking
slim::common::http::Cookie c("session", "abc123");

ErrorStatus e = c.set_path("/");
if (e != ErrorStatus::OK) return e;

e = c.set_secure(true);
if (e != ErrorStatus::OK) return e;
```

```cpp
// Default constructor with status checking
slim::common::http::Cookie c;

ErrorStatus e = c.set_name("session");
if (e != ErrorStatus::OK) return e;

e = c.set_value("abc123");
if (e != ErrorStatus::OK) return e;

e = c.set_path("/");
if (e != ErrorStatus::OK) return e;

e = c.set_secure(true);
if (e != ErrorStatus::OK) return e;
```

```cpp
// Exception-based usage
try {
    slim::common::http::Cookie c("session", "abc123");
    c.set_path("/");
    c.set_secure(true);

    auto header = c.serialize();
    // -> "Set-Cookie: session=abc123; Path=/; Secure\r\n"
}
catch (const slim::common::http::HttpHeaderException& e) {
    std::cerr << "Cookie error: " << e.what() << '\n';
}
catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << '\n';
}
```

```cpp
// Fully attributed cookie
try {
    slim::common::http::Cookie c("__Secure-session", "abc123");
    c.set_domain("example.com");
    c.set_path("/");
    c.set_max_age(3600);
    c.set_same_site("None");
    c.set_secure(true);
    c.set_httponly(true);
    c.set_partitioned(true);

    auto header = c.serialize();
    // -> "Set-Cookie: __Secure-session=abc123; Domain=example.com; Path=/; Max-Age=3600; SameSite=None; Secure; HttpOnly; Partitioned\r\n"
}
catch (const slim::common::http::HttpHeaderException& e) {
    std::cerr << "Cookie serialization failed: " << e.what() << '\n';
}
catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << '\n';
}
```

[↑ Top](#table-of-contents)
