# ArchLog Security Documentation

## Security Features

### Input Validation & Sanitization

#### 1. Time Input Security (`--since` parameter)
- **Allowed characters**: Alphanumeric, space, colon (:), hyphen (-), plus (+)
- **Blocked**: Shell metacharacters, quotes, semicolons, pipes
- **Implementation**: `SecurityValidator::sanitize_time_input()`

#### 2. Unit Input Security (`-u` parameter)
- **Allowed characters**: Alphanumeric, dot (.), hyphen (-), underscore (_), at (@)
- **Blocked**: Shell metacharacters, quotes, semicolons, pipes
- **Implementation**: `SecurityValidator::sanitize_unit_input()`

#### 3. Numeric Input Security (tail, max-entries)
- **Validation**: Only digits allowed
- **Range checking**: Positive integers only
- **Implementation**: `SecurityValidator::is_valid_number()`

#### 4. Severity Level Security
- **Sanitization**: Only alphabetic characters, converted to uppercase
- **Validation**: Must match predefined severity levels
- **Implementation**: `SecurityValidator::sanitize_severity()`

### Protection Against Common Attacks

#### ✅ Command Injection Prevention
```bash
# These attacks are blocked:
./archlog --since="; rm -rf /; echo "
./archlog --unit="; whoami; echo "
./archlog-gui # GUI sanitizes all inputs
```

#### ✅ Shell Metacharacter Filtering
- Semicolons (;) - Blocked
- Pipes (|) - Blocked  
- Backticks (`) - Blocked
- Dollar signs ($) - Blocked
- Quotes (' ") - Blocked

#### ✅ Path Traversal Prevention
- Directory traversal sequences blocked
- Only safe characters allowed in paths
- No direct file system access from user input

#### ✅ Buffer Overflow Protection
- Input length validation
- Safe string handling
- Bounds checking on all inputs

### Security Testing

#### Automated Tests
- Command injection attempts
- Shell metacharacter insertion
- Buffer overflow scenarios
- Path traversal attacks
- Input validation bypass attempts

#### Test Results
```
✅ 5/5 Security Tests PASSED
✅ 0 Vulnerabilities Found
✅ No Command Injection Possible
```

### Security Best Practices Implemented

1. **Input Whitelisting**: Only allow known-safe characters
2. **Parameter Validation**: Validate all inputs before use
3. **Safe Command Construction**: No direct shell parameter passing
4. **Error Handling**: Graceful failure on invalid inputs
5. **Minimal Privileges**: No unnecessary system access

### Reporting Security Issues

If you discover a security vulnerability:
1. Do NOT create a public issue
2. Contact the maintainer privately
3. Provide detailed reproduction steps
4. Allow time for fix before disclosure

### Security Compliance

This application follows secure coding practices:
- OWASP Input Validation Guidelines
- CWE-78 Command Injection Prevention
- CWE-79 Cross-site Scripting Prevention
- CWE-120 Buffer Overflow Prevention