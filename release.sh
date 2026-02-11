#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse arguments
AUTO_CONFIRM=false
VERSION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -y|--yes)
            AUTO_CONFIRM=true
            shift
            ;;
        *)
            if [ -z "$VERSION" ]; then
                VERSION=$1
            else
                print_error "Unknown argument: $1"
                exit 1
            fi
            shift
            ;;
    esac
done

# Check if version argument is provided
if [ -z "$VERSION" ]; then
    print_error "Usage: ./release.sh <version> [-y|--yes]"
    echo "Examples:"
    echo "  ./release.sh 0.1.0-beta           # Create a beta release (interactive)"
    echo "  ./release.sh 0.1.0-beta -y        # Create a beta release (auto-confirm)"
    echo "  ./release.sh 1.0.0 --yes          # Create a stable release (auto-confirm)"
    exit 1
fi

# Validate version format (semantic versioning with optional pre-release)
if ! [[ $VERSION =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?$ ]]; then
    print_error "Invalid version format: $VERSION"
    echo "Version must follow semantic versioning: X.Y.Z or X.Y.Z-prerelease"
    exit 1
fi

# Add 'v' prefix if not present
if [[ ! $VERSION =~ ^v ]]; then
    TAG="v$VERSION"
else
    TAG="$VERSION"
fi

print_info "Creating release $TAG"

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
    print_error "GitHub CLI (gh) is not installed"
    echo "Install it with: brew install gh"
    exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
    print_error "Not authenticated with GitHub"
    echo "Run: gh auth login"
    exit 1
fi

# Check if working directory is clean
if [ -n "$(git status --porcelain)" ]; then
    print_warn "Working directory has uncommitted changes"
    git status --short
    if [ "$AUTO_CONFIRM" = false ]; then
        read -p "Continue anyway? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        print_info "Auto-confirming (--yes flag)"
    fi
fi

# Check if tag already exists
if git rev-parse "$TAG" >/dev/null 2>&1; then
    print_error "Tag $TAG already exists"
    echo "Delete it first with: git tag -d $TAG && git push origin :refs/tags/$TAG"
    exit 1
fi

# Update version in waterfurnace-esp32-s3.yaml
print_info "Updating version in waterfurnace-esp32-s3.yaml"
sed -i.bak "s/version: \".*\"/version: \"$VERSION\"/" waterfurnace-esp32-s3.yaml
rm waterfurnace-esp32-s3.yaml.bak

# Show the diff
git diff waterfurnace-esp32-s3.yaml

# Ask for confirmation
if [ "$AUTO_CONFIRM" = false ]; then
    read -p "Create release $TAG? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        # Restore the file
        git checkout waterfurnace-esp32-s3.yaml
        print_info "Release cancelled"
        exit 0
    fi
else
    print_info "Auto-confirming release creation (--yes flag)"
fi

# Commit version bump
print_info "Committing version bump"
git add waterfurnace-esp32-s3.yaml
git commit -m "Bump version to $VERSION"

# Create and push tag
print_info "Creating tag $TAG"
git tag -a "$TAG" -m "Release $TAG"

print_info "Pushing commit and tag"
git push origin main
git push origin "$TAG"

# Determine if this is a pre-release
PRERELEASE_FLAG=""
if [[ $VERSION =~ - ]]; then
    PRERELEASE_FLAG="--prerelease"
    print_info "This is a pre-release (beta/alpha/rc)"
fi

# Create GitHub release
print_info "Creating GitHub release"
gh release create "$TAG" \
    --title "$TAG" \
    --notes "Release $TAG

This release will automatically build and publish firmware artifacts.

## Installation
1. Visit the [installation page](https://espforge.github.io/esphome-waterfurnace/)
2. Click \"Install\" and follow the instructions

## What's Changed
- See commit history for details" \
    $PRERELEASE_FLAG

print_info "✅ Release $TAG created successfully!"
print_info "The firmware will be built and attached to the release by GitHub Actions"
print_info "View the release: $(gh release view $TAG --json url -q .url)"
