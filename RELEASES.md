# Releases

## Creating a Release

Use the `release.sh` script from the `main` branch:

```bash
./release.sh <version> -y
```

```bash
./release.sh 0.2.0-beta -y    # Beta/pre-release
./release.sh 1.0.0 -y          # Stable release
```

This bumps the version, creates a git tag, publishes a GitHub release, and triggers CI workflows that build firmware and update the installation page. Versions containing `-` (beta, rc, alpha) are marked as pre-releases.

## Testing a Branch

Reference a branch directly in ESPHome without creating a release:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/espforge/esphome-waterfurnace
      ref: feature/my-branch
    refresh: 0s
```

Opening a PR also triggers CI, which builds against ESPHome stable/beta/dev.

## Testing on a Fork

Fork the repo, enable GitHub Pages (Settings > Pages > Source: "GitHub Actions"), then use `release.sh` on your fork. Workflows auto-detect your fork's URLs.

## CI Workflows

| Workflow | Trigger | Purpose |
|----------|---------|---------|
| CI | Pull requests, daily | Build against ESPHome stable/beta/dev |
| Publish Firmware | Release published | Build `.bin`/`.elf`, upload to release |
| Publish Pages | Push to main, after firmware publish | Rebuild installation page |

## Troubleshooting

**Tag already exists:**
```bash
git tag -d v1.0.0 && git push origin :refs/tags/v1.0.0
./release.sh 1.0.0 -y
```

**Check failed workflows:**
```bash
gh run list --limit 5
gh run view <run-id> --log-failed
```
