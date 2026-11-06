# Upstream maintenance

Original: https://github.com/lipedesign/mib3-moi3-altscreen

- `main` mirrors upstream and contains no local fixes.
- `development` is the default branch, containing maintenance files and reviewed fixes. Scheduled GitHub Actions run from the default branch.
- A daily workflow fast-forwards `main` and opens a PR from `main` into `development` when changes need review. It never merges development changes automatically and never force-pushes.
- Companion repository changes create a tracking issue. New upstream commit SHAs are included in the issue title to avoid duplicate alerts.
- Enable Actions in the fork and allow GitHub Actions to create pull requests. Enable Issues for companion alerts. These settings may require the repository owner.
- Also Watch the three upstream repositories for issues, pull requests, discussions and releases. Commit checks do not cover discussion activity.
- GitHub may disable scheduled workflows after prolonged public-repository inactivity. Check Actions periodically; scheduling is best effort.

Local setup:

```sh
git clone https://github.com/rizzocorp1/mib3-moi3-altscreen.git
cd mib3-moi3-altscreen
git remote add upstream https://github.com/lipedesign/mib3-moi3-altscreen.git
git fetch upstream
git switch development
```

Before testing firmware-specific changes, record the upstream SHA, firmware package hash, unit identification, and test results. Keep proprietary firmware and vehicle-specific diagnostic data out of the public fork.