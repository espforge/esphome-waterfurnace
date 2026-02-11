# Release & Deployment Guide

This guide covers creating releases, testing deployments, and contributing to this repository.

## 📦 Creating Releases (Main Repository)

### Prerequisites
- GitHub CLI (`gh`) installed and authenticated
- Write access to the repository
- Clean working directory

### Release Process

Use the `release.sh` script to create releases:

```bash
./release.sh <version> -y
```

**Examples:**
```bash
./release.sh 0.2.0-beta -y    # Beta release
./release.sh 1.0.0 -y          # Stable release
./release.sh 1.1.0-rc1 -y      # Release candidate
```

### What Happens Automatically

1. ✅ Updates version in `waterfurnace-esp32-s3.yaml`
2. ✅ Creates git commit for version bump
3. ✅ Creates and pushes git tag (e.g., `v1.0.0`)
4. ✅ Creates GitHub release (marked as pre-release if beta/rc)
5. ✅ Triggers **Publish Firmware** workflow (~4 minutes)
   - Builds firmware (`.bin`, `.elf`, checksums)
   - Uploads assets to the release
6. ✅ Triggers **Publish Pages** workflow
   - Rebuilds installation page with latest firmware

**Important:** Releases should only be created from the `main` branch. For testing feature branches, see the testing sections below.

---

## 🧪 Testing Releases (Fork Workflow)

To test releases without affecting the main repository:

### 1. Fork Setup

```bash
# Fork on GitHub, then clone
git clone https://github.com/YOUR_USERNAME/esphome-waterfurnace.git
cd esphome-waterfurnace

# Enable GitHub Pages in your fork
# Settings → Pages → Source: "GitHub Actions"
```

### 2. Test Your Changes

```bash
# Make changes
git add .
git commit -m "Test feature X"
git push

# Create test release
./release.sh 0.1.0-test -y
```

**Your test environment:**
- Releases: `https://github.com/YOUR_USERNAME/esphome-waterfurnace/releases`
- Install page: `https://YOUR_USERNAME.github.io/esphome-waterfurnace/`

### 3. What Auto-Configures?

✅ **No configuration changes needed!** Everything auto-detects your fork:
- GitHub Pages `baseurl` (auto-detects from repo name)
- GitHub Pages `url` (auto-detects from username)
- GitHub Actions workflows (use `${{ github.repository }}`)
- Release script (detects current branch)

### 4. Optional: Customize Project Identity

Only needed if you want a different project name:

**waterfurnace-esp32-s3.yaml** (line 9):
```yaml
project:
  name: espforge.waterfurnace  # Change to: yourname.waterfurnace
```

**waterfurnace-esp32-s3.factory.yaml** (line 6):
```yaml
project:
  name: espforge.waterfurnace  # Change to: yourname.waterfurnace
```

### 5. Contribute Back

```bash
# Create feature branch
git checkout -b feature/awesome-improvement
# ... make changes ...
git push origin feature/awesome-improvement

# Open PR on GitHub to espforge/esphome-waterfurnace
```

---

## 🔬 Testing CI Without a Release

### Option 1: Pull Request Testing

Push to a branch and open a PR:
```bash
git checkout -b test-branch
# ... make changes ...
git push origin test-branch
# Open PR on GitHub
```

The **CI** workflow automatically runs and builds both YAML files against ESPHome stable/beta/dev versions.

### Option 2: Manual Workflow Dispatch

Test the firmware build pipeline without creating a release:

1. Go to **Actions** tab on GitHub
2. Select **Publish Firmware** workflow
3. Click **Run workflow**
4. Select your branch
5. Click **Run workflow**

This builds the firmware and creates an artifact you can download, but doesn't create a release or upload to Pages.

### Option 3: Test Branch Directly in ESPHome

The easiest way to test component changes - reference the branch directly:

```yaml
# In your ESPHome config
external_components:
  - source:
      type: git
      url: https://github.com/espforge/esphome-waterfurnace
      ref: feature/my-test-branch  # Your branch name

waterfurnace:
  id: wf
  # ... rest of config
```

This pulls the component code directly from your branch without needing a release. Perfect for:
- Testing new features
- Trying bug fixes
- Development iteration

---

## 🛠️ Workflow Architecture

### Workflows Overview

**1. CI** (`.github/workflows/ci.yml`)
- **Triggers:** Pull requests, daily schedule
- **What it does:** Builds YAML configs against ESPHome stable/beta/dev
- **Purpose:** Validate changes work across ESPHome versions

**2. Publish Firmware** (`.github/workflows/publish-firmware.yml`)
- **Triggers:** Release published, manual dispatch
- **What it does:** Builds and uploads firmware assets
- **Outputs:** `.bin`, `.elf`, checksums, manifest.json

**3. Publish Pages** (`.github/workflows/publish-pages.yml`)
- **Triggers:** Push to main (static/** changes), after Publish Firmware completes
- **What it does:** Builds Jekyll site with latest firmware
- **Output:** https://espforge.github.io/esphome-waterfurnace/

### GitHub Pages Setup

**Required:** GitHub Pages must be enabled in repository settings.

**Settings → Pages:**
- Source: **GitHub Actions** (not legacy branch-based)
- Custom domain: (optional)

The Pages site is built from `static/` using Jekyll with the Slate theme. Firmware files are downloaded from the latest release and embedded in the deployment.

---

## 📋 Version Numbering

Follow [Semantic Versioning](https://semver.org/):

- `X.Y.Z` - Stable release (e.g., `1.0.0`, `1.2.3`)
- `X.Y.Z-beta` - Beta release (e.g., `0.2.0-beta`)
- `X.Y.Z-rc1` - Release candidate (e.g., `1.0.0-rc1`)
- `X.Y.Z-alpha` - Alpha/experimental (e.g., `0.1.0-alpha`)

The release script automatically marks releases with `-` in the version as pre-releases on GitHub.

---

## 🚨 Troubleshooting

### Release script fails with "Tag already exists"

```bash
# Delete tag locally and remotely
git tag -d v1.0.0
git push origin :refs/tags/v1.0.0

# Try again
./release.sh 1.0.0 -y
```

### Firmware build fails

Check the workflow logs:
```bash
gh run list --limit 5
gh run view <run-id> --log-failed
```

Common issues:
- Missing `secrets.yaml` → CI workflow creates placeholder automatically
- Component compilation error → Check C++ code in `components/`
- ESPHome version incompatibility → Update constraints in `waterfurnace-esp32-s3.yaml`

### Pages deployment fails

```bash
# Check if Pages is enabled
gh api repos/espforge/esphome-waterfurnace/pages

# Re-run Pages workflow
gh run list --workflow=publish-pages.yml
gh run rerun <run-id>
```

### CSS/Theme not loading on Pages

- Verify `baseurl` is not explicitly set in `static/_config.yml` (auto-detected)
- Check that `remote_theme: pages-themes/slate@v0.2.0` is present
- Clear browser cache and hard refresh

---

## 📝 Checklist for Contributors

Before creating a release:

- [ ] All tests pass (`tests/run_tests.sh`)
- [ ] CI workflow passes on your branch
- [ ] Version number follows semantic versioning
- [ ] Changes documented in commit messages
- [ ] For major changes: update README.md

For main repository maintainers:

- [ ] Review and merge PRs
- [ ] Create release from `main` branch
- [ ] Verify firmware builds successfully
- [ ] Test installation page in browser
- [ ] Announce release (Discussions, Discord, etc.)
