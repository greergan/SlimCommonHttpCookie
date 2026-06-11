# SlimCommonHTTPCookie

A lightweight, RFC6265-oriented HTTP cookie implementation in modern C++.

## Overview

This library provides a strict, validation-heavy HTTP cookie parser and serializer with:
- RFC 6265 compliant parsing behavior
- Strong validation for all cookie attributes
- Zero dynamic allocation in validation paths (where possible)
- Explicit status reporting via `COOKIE::STATUS`
- Strict parsing over permissive recovery
- Explicit validation at each setter
- Minimal runtime overhead in hot paths

## Features

| Feature | Description |
|--------|-------------|
| Name/Value validation | Strict RFC-style character filtering |
| Domain validation | Full label + length + syntax checks |
| Expires parsing | RFC 6265 cookie-date tokenizer |
| Max-Age support | String + integer parsing with overflow checks |
| SameSite handling | strict / lax / none validation |
| Secure / HttpOnly / Partitioned | Boolean attribute parsing |
| Serialize | Preallocated, zero-fragment string build |
| Error model | Strong enum-based status reporting |

## Core API

### Cookie class

```cpp
slim::common::http::Cookie c;
```

### Setters

| Method | Description |
|--------|-------------|
| `COOKIE::STATUS set_name(std::string_view)` | Set cookie name (validated) |
| `COOKIE::STATUS set_value(std::string_view)` | Set cookie value (validated) |
| `COOKIE::STATUS set_domain(std::string_view)` | Set domain with RFC checks |
| `COOKIE::STATUS set_path(std::string_view)` | Set path (must start with `/`) |
| `COOKIE::STATUS set_expires(std::string_view)` | Set expiry string (RFC parser) |
| `COOKIE::STATUS set_max_age(std::uint_least64_t)` | Set Max-Age |
| `COOKIE::STATUS set_max_age(std::string_view)` | Set Max-Age |
| `COOKIE::STATUS set_same_site(std::string_view)` | Set SameSite policy |
| `void set_secure(bool)` | Set Secure flag |
| `COOKIE::STATUS set_secure(std::string_view)` | Set Secure flag |
| `void set_httponly(bool)` | Set HttpOnly flag |
| `COOKIE::STATUS set_httponly(std::string_view)` | Set HttpOnly flag |
| `void set_partitioned(bool)` | Set Partitioned flag |
| `COOKIE::STATUS set_partitioned(std::string_view)` | Set Partitioned flag |

### Getters

| Method | Returns |
|--------|--------|
| `std::string get_name()` | cookie name |
| `std::string get_value()` | cookie value |
| `std::optional<std::string> get_domain()` | optional domain |
| `std::optional<std::string> get_path()` | optional path |
| `std::optional<std::string> get_expires()` | optional expires |
| `std::optional<std::uint_least64_t> get_max_age()` | optional max-age |
| `std::optional<std::string> get_same_site()` | optional samesite |
| `bool get_secure()` | secure (default=false) |
| `bool get_httponly()` | httponly (default=false) |
| `bool get_partitioned()` | partitioned (default=false) |

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
std::string Cookie::serialize() const noexcept;
```

Outputs a fully formatted `Set-Cookie` header string.

## Example

```cpp
slim::common::http::Cookie c;

c.set_name("session");
c.set_value("abc123");
c.set_path("/");
c.set_secure("true");

if (c.validate() == COOKIE::STATUS::OK) {
    auto header = c.serialize();
}
```
