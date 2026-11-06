# Dependency updates

GRASS uses [Renovate](https://docs.renovatebot.com/) to propose updates of
GitHub Actions, pre-commit hooks, Docker images, tool versions used in CI and
documentation requirements. The configuration is in `renovate.json5`.

The goals are to stay reasonably current, to keep the number of pull requests,
emails and CI runs low, and to keep each update reviewable. Updating only once
a quarter or a year was rejected: Docker base images are rebuilt about every
two weeks, and a long gap makes the next update larger and riskier.

## What to expect

Updates are grouped, so each group is one pull request with one CI run. Each
group has its own day of the month (UTC), so that CI runs do not queue
together.

| Group                                | Content                                                      | Days  |
| ------------------------------------ | ------------------------------------------------------------ | ----- |
| CI actions                           | GitHub Actions, runner images, Miniforge version             | 1     |
| linters and pre-commit hooks         | pre-commit hooks, linter and Python versions in workflows    | 8     |
| Python docs and tooling requirements | pinned packages of the documentation requirements            | 8     |
| Docker base images and libraries     | base image digests and tags, GEOS, PROJ, GDAL, PDAL, gdal-grass, wxPython | 3, 17 |
| Lock file                            | `flake.lock`                                                 | 1     |

- Base images and the libraries built on top of them (for example PROJ and
  GDAL) are in the same pull request, so that the Docker workflow tests them
  together. They must not be merged separately.
- The Docker group runs twice a month because the Ubuntu images are rebuilt
  around the 1st and the 15th, and Debian and Alpine rebuild at irregular
  times.
- Only a release that is at least 7 days old is proposed. Docker digests are
  not delayed, because they have no release date.
- Major updates and new Python minor versions are not proposed until a
  maintainer ticks them in the Dependency Dashboard issue. Moving to a new
  Ubuntu release also needs the wxPython wheel URL in the Dockerfile changed.
- Renovate has a limit of 10 open pull requests, and does not rebase them
  unless they have conflicts, because each push restarts CI.
- Security updates are proposed at any time, without waiting for the schedule
  or the 7 days, and are labeled `security`.
- Updates are never merged automatically.

## Release branches

Release branches only get security updates and one pull request a month with
patch and digest updates of GitHub Actions and Docker images, never major
updates. While a release is being stabilized, require approval on the
Dependency Dashboard for these updates, by adding
`"dependencyDashboardApproval": true` to the two `release branch maintenance`
rules in `renovate.json5`, to avoid changes of the CI during the release. Stop
updating a branch when it is no longer supported by removing it from
`baseBranchPatterns`.

## Reviewing a Renovate pull request

Each pull request lists every update it contains, with the release notes. Check
that:

- the pull request comes from `renovate[bot]` and from a `renovate/*` branch,
- the diff only changes versions and digests,
- the commit SHA of each GitHub Action matches the version in the comment next
  to it,
- the CI passed. Do not merge on a failure that is not understood.

If a maintainer pushes a fix to a Renovate branch, explain it in the pull
request. Renovate stops updating the branch after that.

Do not close a Renovate pull request by hand to get rid of it: Renovate then
ignores that version. Use the Dependency Dashboard to skip an update.

## Not pinned on purpose

Runner images, GitHub Actions, base images, Miniforge and the versions marked
with `# renovate:` comments are pinned. These are not, because pinning them
would add more updates than it prevents surprises:

- apt packages and the ubuntugis PPA, as testing against current versions is
  the goal,
- the runtime Python requirements of the CI (`python_requirements.txt`,
  `optional_requirements.txt`, `binder/requirements.txt`) and the packages
  installed in the Docker images, as users get the latest versions,
- pytest plugins,
- the branch of the grass-addons repository used to build the documentation.

## Fewer emails

The configuration cannot change who is notified. Each person can use
`Watch > Custom` in the repository settings of GitHub, or filter on the sender
`renovate[bot]` in the mail client or `author:app/renovate` in the GitHub
notifications.
