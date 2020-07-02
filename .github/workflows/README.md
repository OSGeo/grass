# GitHub Actions Workflows

## GitHub Super Linter

Run locally using Docker:

```
docker run --rm -e RUN_LOCAL=true -e VALIDATE_PERL=true -v $PWD:/tmp/lint github/super-linter
```

Note that you need to enable each validation separately or not specify
any to enable all.
