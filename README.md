<a href="https://codeberg.org/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>   

# SlimCommonHttpCookie

A lightweight, RFC6265-oriented HTTP cookie implementation in modern C++.  
Acts as a validating, backing store for the [SlimTS](https://codeberg.org/greergan/SlimTS) Javascript Cookie object.  
Part of the [SlimCommon](https://codeberg.org/greergan/SlimCommon) library.  
Dependency of the [SlimCommonHttpCookieStore](https://codeberg.org/greergan/SlimCommonHttpCookieStore) micro-ibrary.  
Built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Core API](#core-api)
  - [CookieStatus enum](#cookiestatus-enum)
  - [CookieException](#cookieexception)
  - [Cookie class](#cookie-class)
  - [Constructors and object lifetime](#constructors-and-object-lifetime)
  - [Operators](#operators)
  - [Setters](#setters)
  - [Getters](#getters)
  - [Validation](#validation)
  - [Serialization](#serialization)
- [Building](#building)
- [Dependencies](#dependencies)
- [Examples](#examples)
- [Forgejo Workflows](#forgejo-workflows)
  - [build.yml — Continuous Integration](#buildyml--continuous-integration)
  - [publish.yaml — Release & Publish](#publishyaml--release--publish)

## Overview

This library provides a strict, validation-heavy HTTP cookie parser and serializer with:
- RFC 6265 compliant parsing behavior
- Strong validation for all cookie attributes
- Zero dynamic allocation in validation paths (where possible)
- Explicit status reporting via `CookieStatus`
- Strict parsing over permissive recovery
- Explicit validation at each setter
- Minimal runtime overhead in hot paths
- Heavy use of `noexcept`

[↑ Top](#slimcommonhttpcookie)

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
| Error model | Strong enum-based status reporting via `CookieStatus` |

[↑ Top](#slimcommonhttpcookie)

## Core API

### CookieStatus enum

All setters and `validate()` return a `CookieStatus`. The full set of values is:

| Value | Meaning |
|-------|---------|
| `OK` | Success |
| `CookieTooLarge` | Cookie exceeds the 4096-byte limit |
| `DomainBareDot` | Domain cannot consist of a bare dot |
| `DomainEmpty` | Domain name cannot be empty |
| `DomainInvalidChar` | Domain contains an invalid character |
| `DomainLabelEmpty` | Domain label cannot be empty |
| `DomainLabelInvalidHyphen` | Domain label has a hyphen in an invalid position |
| `DomainLabelTooLong` | Domain label exceeds the maximum allowed length |
| `DomainNumericTld` | Top-level domain must not be numeric |
| `DomainTooLong` | Domain name exceeds the maximum allowed length |
| `DomainTrailingDot` | Domain name must not end with a trailing dot |
| `EmptyCookieString` | Cookie string cannot be empty |
| `ExpiresInvalidFormat` | Expires attribute has an invalid date format |
| `InvalidBoolean` | Expected a valid boolean value |
| `InvalidCookieName` | Cookie name is invalid |
| `InvalidCookieValue` | Cookie value is invalid |
| `MalformedCookieMissingEquals` | Malformed cookie: missing `=` separator |
| `MalformedCookiePairMissingEquals` | Malformed cookie pair: missing `=` separator |
| `MaxAgeEmpty` | Max-Age attribute cannot be empty |
| `MaxAgeExceedsLimit` | Max-Age value exceeds the maximum allowed limit |
| `MaxAgeInvalidFormat` | Max-Age attribute has an invalid format |
| `MaxAgeTrailingChars` | Max-Age value has unexpected trailing characters |
| `NameEmpty` | Cookie name cannot be empty |
| `NameHostPrefixHasDomain` | The `__Host-` prefix cannot be combined with a Domain attribute |
| `NameHostPrefixInvalidPath` | The `__Host-` prefix requires the Path attribute to be `/` |
| `NameInvalidChar` | Cookie name contains an invalid character |
| `NamePrefixRequiresSecure` | Cookie name prefix requires the Secure attribute |
| `PartitionedRequiresSameSiteNone` | Partitioned cookies require `SameSite=None` |
| `PartitionedRequiresSecure` | Partitioned cookies require the Secure attribute |
| `PathInvalidChar` | Path contains an invalid character |
| `PathMissingLeadingSlash` | Path must begin with a forward slash |
| `SameSiteInvalid` | SameSite attribute has an invalid value |
| `SameSiteNoneRequiresSecure` | `SameSite=None` requires the Secure attribute |
| `ValueInvalidChar` | Cookie value contains an invalid character |
| `ValueUnmatchedQuote` | Cookie value has an unmatched quotation mark |

Status values can be converted to a human-readable string via:

```cpp
std::string_view msg = slim::common::http::cookie::status::to_string(status);
```

[↑ Top](#slimcommonhttpcookie)

### CookieException

Thrown by the parameterised constructor and `serialize()` on validation failure.

```cpp
class CookieException : public std::logic_error {
public:
    explicit CookieException(CookieStatus e);   // formats "Invalid Cookie: <status string>"
    explicit CookieException(std::string msg);  // arbitrary message
};
```

[↑ Top](#slimcommonhttpcookie)

### Cookie class

```cpp
slim::common::http::Cookie c;
```

### Constructors and object lifetime

| Form | Description |
|------|-------------|
| `Cookie()` | Default constructor, produces an empty cookie |
| `Cookie(std::string_view name, std::string_view value)` | Construct with name and value, validated immediately. Throws `CookieException` on failure |
| `Cookie(const Cookie&)` | Deleted — copies are not allowed |
| `Cookie& operator=(const Cookie&)` | Deleted — copies are not allowed |
| `Cookie(Cookie&&) noexcept` | Move construction is supported |
| `Cookie& operator=(Cookie&&) noexcept` | Move assignment is supported |

[↑ Top](#slimcommonhttpcookie)

### Operators

| Operator | Description |
|----------|-------------|
| `bool operator==(const Cookie&) const noexcept` | Equality by name (case-sensitive), domain, and path |
| `bool operator!=(const Cookie&) const noexcept` | Inequality — inverse of `==` |

Per RFC 6265 §5.3, a cookie is uniquely identified by the tuple of name, domain, and path. Two cookies sharing the same name are distinct if they differ in domain or path. Cookie names are case-sensitive — `Session` and `session` are different cookies. Value, expiry, and flags are not considered in equality.

[↑ Top](#slimcommonhttpcookie)

### Setters

| Method | Description |
|--------|-------------|
| `CookieStatus set_name(std::string_view) noexcept` | Set cookie name (validated) |
| `CookieStatus set_value(std::string_view) noexcept` | Set cookie value (validated) |
| `CookieStatus set_domain(std::string_view) noexcept` | Set domain with RFC checks |
| `CookieStatus set_path(std::string_view) noexcept` | Set path (must start with `/`) |
| `CookieStatus set_expires(std::string_view) noexcept` | Set expiry string (RFC 6265 cookie-date parser) |
| `CookieStatus set_max_age(std::uint_least64_t) noexcept` | Set Max-Age from integer |
| `CookieStatus set_max_age(std::string_view) noexcept` | Set Max-Age from string |
| `CookieStatus set_same_site(std::string_view) noexcept` | Set SameSite policy |
| `void set_secure(bool) noexcept` | Set Secure flag |
| `CookieStatus set_secure(std::string_view) noexcept` | Set Secure flag from string |
| `void set_httponly(bool) noexcept` | Set HttpOnly flag |
| `CookieStatus set_httponly(std::string_view) noexcept` | Set HttpOnly flag from string |
| `void set_partitioned(bool) noexcept` | Set Partitioned flag |
| `CookieStatus set_partitioned(std::string_view) noexcept` | Set Partitioned flag from string |

[↑ Top](#slimcommonhttpcookie)

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

[↑ Top](#slimcommonhttpcookie)

### Validation

```cpp
CookieStatus Cookie::validate() const noexcept;
```

Checks:
- Secure/SameSite consistency
- Partitioned requires Secure
- Host prefix rules
- Domain/path correctness

[↑ Top](#slimcommonhttpcookie)

### Serialization

```cpp
std::string Cookie::serialize() const;
```

Outputs a fully formatted `Set-Cookie` header string.  
Throws `CookieException` if validation fails or the cookie exceeds 4096 bytes.

[↑ Top](#slimcommonhttpcookie)

## Building

This library is built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager). See that repository for build instructions.

[↑ Top](#slimcommonhttpcookie)

## Dependencies

External package dependencies for this library are declared in the `required_packages` file at the repository root. This file is read by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) during the build process to resolve SlimCommon micro-libraries before compilation begins.

```
none
```

[↑ Top](#slimcommonhttpcookie)

## Examples

```cpp
// Parameterised constructor with status checking
slim::common::http::Cookie c("session", "abc123");

CookieStatus e = c.set_path("/");
if (e != CookieStatus::OK) return e;

e = c.set_secure(true);
if (e != CookieStatus::OK) return e;
```

```cpp
// Default constructor with status checking
slim::common::http::Cookie c;

CookieStatus e = c.set_name("session");
if (e != CookieStatus::OK) return e;

e = c.set_value("abc123");
if (e != CookieStatus::OK) return e;

e = c.set_path("/");
if (e != CookieStatus::OK) return e;

e = c.set_secure(true);
if (e != CookieStatus::OK) return e;
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

[↑ Top](#slimcommonhttpcookie)

## Forgejo Workflows

Two workflows are defined under `.forgejo/workflows/`.

### `build.yml` — Continuous Integration

Triggers on every push to `master` and can also be run manually via `workflow_dispatch`.

Runs on a self-hosted runner using the `slim-toolchain` container image (privileged mode required). Steps:

1. Checks out this repository.
2. Checks out [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) into `./SlimLibraryPackager/`.
3. Runs `update_env.sh` from SlimLibraryPackager to configure the build environment.
4. Invokes `make` with `RELEASE_TYPE=Release` and `SHARED_ONLY=OFF`.

**Required Forgejo variables** (set under *Settings → Variables*):

| Variable | Description |
|----------|-------------|
| `GIT_URL` | Base URL of the Forgejo instance (e.g. `https://git.example.com`) |
| `REPO_OWNER` | Repository owner/organisation name |

[↑ Top](#slimcommonhttpcookie)

### `publish.yaml` — Release & Publish

Triggers on version tags matching `v*` (e.g. `v1.2.3`) and can also be run manually with an optional `version` input.

Uses the same runner and container as the build workflow. Steps:

1. Checks out this repository at the triggering ref.
2. Checks out [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) into `./SlimLibraryPackager/`.
3. Validates that all required variables and secrets are present, exiting early with a descriptive error if any are missing.
4. Determines the version to publish:
   - Manual `workflow_dispatch` input takes precedence.
   - Otherwise the version is stripped from the tag name (e.g. `v1.2.3` → `1.2.3`).
   - Falls back to `0.0.0` if neither is available.
5. Runs `update_env.sh` to configure the build environment.
6. Invokes `make packages` to produce `.deb` and `.rpm` packages under `dist/`.
7. Publishes each package to the Forgejo Generic Package Registry under `<repo>/<version>/<filename>` using HTTP PUT via `curl`.

**Required Forgejo variables** (set under *Settings → Variables*):

| Variable | Description |
|----------|-------------|
| `GIT_URL` | Base URL of the Forgejo instance |
| `REPO_OWNER` | Repository owner/organisation name |

**Required Forgejo secrets** (set under *Settings → Secrets*):

| Secret | Description |
|--------|-------------|
| `REGISTRY_USER` | Username for authenticating with the package registry |
| `REGISTRY_TOKEN` | Password or token for authenticating with the package registry |

[↑ Top](#slimcommonhttpcookie)
