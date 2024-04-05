# GRASS GIS Security Policy

## Reporting a Vulnerability
At GRASS GIS, we take security vulnerabilities seriously. We appreciate your efforts in responsibly disclosing any issues you may find. To report a security vulnerability, please follow these steps:
1. **Privately disclose the issue** by emailing our [Security Team](vpetras@ncsu.edu) or by submitting a Security Advisory through [GitHub Security](https://github.com/OSGeo/grass/security). Please do not create publicly viewable issues for security vulnerabilities.
2. **Provide detailed information** regarding the vulnerability, including:
    - Description of the vulnerability
    - Steps to reproduce
    - Versions affected
    - Any mitigating factors
3. **Our Response**:
    - Our security team will acknowledge receiving your report within 48 hours.
    - We will work to validate and reproduce the issue.
    - Once confirmed, we will work on a fix and release schedule.
4. **Public Disclosure**:
    - We aim to release patches for vulnerabilities as soon as possible.
    - We will coordinate with you regarding public disclosure, ensuring a reasonable timeline for users to update before the details are made public.


## Supported Versions
We prioritize the most recent stable release of GRASS GIS for security patches. However, we understand that users may still be on older versions. We will make reasonable efforts to provide patches for security vulnerabilities in older releases on a case-by-case basis, depending on the severity and feasibility.

| Version | Supported |
| ------- | --------- |
| 8.x.x   | ✅        |
| 7.x.x   | ✅        |
| < 7.0.0 | 🔴        |


## Security Measures
- Code Review: We conduct regular code reviews to catch potential vulnerabilities.
- Dependency Management: We track dependencies and update them regularly to mitigate known security issues.
- Secure Development Practices: Our developers are trained in secure coding practices to minimize the introduction of vulnerabilities.


## Vulnerability Scope
Our security policy covers vulnerabilities in the GRASS GIS core codebase, official plugins, and any official distributions provided by the GRASS GIS team.

## Responsible Disclosure
We adhere to responsible disclosure practices. We appreciate your cooperation in allowing us time to address any reported vulnerabilities before disclosing them publicly. We ask that you refrain from disclosing any details of the vulnerability until we have had adequate time to provide a fix.

Thank you for helping to keep GRASS GIS secure!
