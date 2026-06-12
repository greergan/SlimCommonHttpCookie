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
| Name/Value validation | Strict RFC-style character filtering |
| Domain validation | Full label + length + syntax checks |
| Expires parsing | RFC 6265 cookie-date tokenizer stored as RFC 1123 format|
| Max-Age support | String + integer parsing with overflow checks |
| Partitioned | CHIPS validation enforced on outgoing requests |  
| SameSite handling | strict / lax / none validation |
| Secure / HttpOnly / Partitioned | Boolean attribute parsing |
| Serialize | Preallocated, zero-fragment string build |
| Serialize | Cookie validation with thown exceptions |  
| Error model | Strong enum-based status reporting |

## Core API

### Cookie class

```cpp
slim::common::http::Cookie c;
```

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
| `std::string get_name() const noexcept;` | cookie name |
| `std::string get_value() const noexcept;` | cookie value |
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

## Example

```cpp
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
    slim::common::http::Cookie c;
    
    c.set_name("session");
    c.set_value("abc123");
    c.set_path("/");
    c.set_secure("true");
    
    try {
        auto header = c.serialize();
    }
    catch (const slim::common::http::CookieException& e) {
        std::cerr << "Cookie serialization failed: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << '\n';
    }
```
