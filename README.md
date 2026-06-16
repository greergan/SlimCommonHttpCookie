<a href="https://codeberg.org/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>   

# SlimCommonHTTPCookie

A lightweight, RFC6265-oriented HTTP cookie implementation in modern C++.  
Acts as a validating, backing store for [SlimTS's](https://github.com/greergan/SlimTS) Javascript Cookie object.

## Overview

This library provides a strict, validation-heavy HTTP cookie parser and serializer with:
- RFC 6265 compliant parsing behavior
- Strong validation for all cookie attributes
- Zero dynamic allocation in validation paths (where possible)
- Explicit status reporting via `COOKIE::STATUS`
- Strict parsing over permissive recovery
- Explicit validation at each setter
- Minimal runtime overhead in hot paths
- Heavy use of `noexcept`

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
| Error model | Strong enum-based status reporting |

## Core API

### Cookie class

```cpp
slim::common::http::Cookie c;
```

### Constructors

| Constructor | Description |
|-------------|-------------|
| `Cookie()` | Default constructor, produces an empty cookie |
| `Cookie(std::string_view name, std::string_view value)` | Construct with name and value, validated immediately |

The parameterised constructor validates name and value immediately. Throws `CookieException` on any validation failure.

Copy and move construction and assignment are deleted.

### Operators

| Operator | Description |
|----------|-------------|
| `bool operator==(const Cookie&) const noexcept;` | Equality by name (case-sensitive), domain, and path |

Per RFC 6265 §5.3, a cookie is uniquely identified by the tuple of name, domain, and path. Two cookies sharing the same name are distinct if they differ in domain or path. Cookie names are case-sensitive — `Session` and `session` are different cookies. Value, expiry, and flags are not considered in equality.

### Setters

| Method | Description |
|--------|-------------|
| `COOKIE::STATUS set_name(std::string_view) noexcept;` | Set cookie name (validated) |
| `COOKIE::STATUS set_value(std::string_view) noexcept;` | Set cookie value (validated) |
| `COOKIE::STATUS set_domain(std::string_view) noexcept;` | Set domain with RFC checks |
| `COOKIE::STATUS set_path(std::string_view) noexcept;` | Set path (must start with `/`) |
| `COOKIE::STATUS set_expires(std::string_view) noexcept;` | Set expiry string (RFC parser) |
| `COOKIE::STATUS set_max_age(std::uint_least64_t) noexcept;` | Set Max-Age |
| `COOKIE::STATUS set_max_age(std::string_view) noexcept;` | Set Max-Age |
| `COOKIE::STATUS set_same_site(std::string_view) noexcept;` | Set SameSite policy |
| `void set_secure(bool) noexcept;` | Set Secure flag |
| `COOKIE::STATUS set_secure(std::string_view) noexcept;` | Set Secure flag |
| `void set_httponly(bool) noexcept;` | Set HttpOnly flag |
| `COOKIE::STATUS set_httponly(std::string_view) noexcept;` | Set HttpOnly flag |
| `void set_partitioned(bool) noexcept;` | Set Partitioned flag |
| `COOKIE::STATUS set_partitioned(std::string_view) noexcept;` | Set Partitioned flag |

### Getters

| Method | Returns |
|--------|--------|
| `std::string_view get_name() const noexcept;` | cookie name |
| `std::string_view get_value() const noexcept;` | cookie value |
| `std::optional<std::string> get_domain() const noexcept;` | optional domain |
| `std::optional<std::string> get_path() const noexcept;` | optional path |
| `std::optional<std::string> get_expires() const noexcept;` | optional expires |
| `std::optional<std::uint_least64_t> get_max_age() const noexcept;` | optional max-age |
| `std::optional<std::string> get_same_site() const noexcept;` | optional samesite |
| `bool get_secure() const noexcept;` | secure (default=false) |
| `bool get_httponly() const noexcept;` | httponly (default=false) |
| `bool get_partitioned() const noexcept;` | partitioned (default=false) |

### Validation

```cpp
COOKIE::STATUS Cookie::validate() const noexcept;
```

Checks:
- Secure/SameSite consistency
- Partitioned requires Secure
- Host prefix rules
- Domain/path correctness

### Serialization

```cpp
std::string Cookie::serialize() const;
```

Outputs a fully formatted `Set-Cookie` header string.  
Throws `CookieException` if validation fails or the cookie exceeds 4096 bytes.

## Example

```cpp
// Constructor with status checking
slim::common::http::Cookie c("session", "abc123");

COOKIE::STATUS e = c.set_path("/");
if(e != COOKIE::STATUS::OK) return e;

e = c.set_secure(true);
if(e != COOKIE::STATUS::OK) return e;
```

```cpp
// Default constructor with status checking
slim::common::http::Cookie c;

COOKIE::STATUS e = c.set_name("session");
if(e != COOKIE::STATUS::OK) return e;

e = c.set_value("abc123");
if(e != COOKIE::STATUS::OK) return e;

e = c.set_path("/");
if(e != COOKIE::STATUS::OK) return e;

e = c.set_secure(true);
if(e != COOKIE::STATUS::OK) return e;
```

```cpp
// Constructor with exception handling
try {
    slim::common::http::Cookie c("session", "abc123");
    c.set_path("/");
    c.set_secure(true);

    auto header = c.serialize();
    // -> "Set-Cookie: session=abc123; Path=/; Secure\r\n"
}
catch (const slim::common::http::CookieException& e) {
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
catch (const slim::common::http::CookieException& e) {
    std::cerr << "Cookie serialization failed: " << e.what() << '\n';
}
catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << '\n';
}
```
