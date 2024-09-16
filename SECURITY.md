# GRASS GIS Security Policy

## Reporting a Vulnerability

At GRASS GIS, we take security vulnerabilities seriously. We appreciate your
efforts in responsibly disclosing any issues you may find. To report a security
vulnerability, please follow these steps:

1. **Privately disclose the issue** by submitting a Security Advisory through
   [GitHub Security](https://github.com/OSGeo/grass/security). Please do not
   create publicly viewable issues for security vulnerabilities.
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
    - We will coordinate with you regarding public disclosure, ensuring a
      reasonable timeline for users to update before the details are made public.

## Supported Versions

Please refer to our [download section](https://grass.osgeo.org/download/)
for details on which versions are currently supported.

## Security Measures

- Code Review: We conduct code reviews to catch potential vulnerabilities during
  code submission.
- Dependency Management: We track dependencies and update them regularly to
  mitigate known security issues.
- Secure Development Practices: We use a series of security tools to detect
  potential vulnerabilities in existing and newly submitted code.

## Vulnerability Scope

Our security policy covers vulnerabilities in the GRASS GIS core codebase,
official addons, and any official distributions provided by the GRASS GIS team.

While packages in Linux and other unix-like distributions are out of scope of
this document, distribution maintainers traditionally do a great job in patching
their distributions for security vulnerabilities. Please, refer to a specific
distribution or package source if you are using packages for a specific software
distribution.

## Responsible Disclosure

We adhere to responsible disclosure practices. We appreciate your cooperation
in allowing us time to address any reported vulnerabilities before disclosing
them publicly. We ask that you refrain from disclosing any details of the
vulnerability until we have had adequate time to provide a fix.

Thank you for helping to keep GRASS GIS secure!
