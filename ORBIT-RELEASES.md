# Orbit Launcher releases

Public repository: https://github.com/kazumhub-pixel/orbit-launcher

The initial public version is **1.0**, with semantic version/tag **1.0.0 / v1.0.0**.
Change `release.json` explicitly for later releases (for example 1.0.1 or 1.1.0).
Building, editing, committing and pushing do not publish an update.

## Build and package

Normal CMake builds default to `Orbit_OFFICIAL_RELEASE=OFF`, display `-dev`, and
never contact the Orbit update service. An official candidate is an explicit
Release build with `-DOrbit_OFFICIAL_RELEASE=ON`. CMake derives all version
components and repository settings from release.json, and installs orbit-build.json
as build provenance. No token or signing secret is compiled into the client.

Run the existing build/tests/install procedure with that option enabled. Then:

```text
python scripts/prepare-windows-release.py --runtime <installed-runtime> --output <new-candidate-folder> --makensis <makensis.exe>
```

The output folder must be new. The packager rejects development/mismatched build
provenance. It creates a per-user NSIS installer and a release-candidate.json with
the final SHA-256. Add the corresponding source archive named
OrbitLauncher-Source-<version>.zip and RELEASE-NOTES.md before release validation.
Never include player data, downloaded Minecraft content, accounts or local logs.
The installer contains the application's libraries, licenses and third-party sources.

## Deliberate publication

First push/review the matching source commit in the public repository. Existing
Prism workflows are archived under .github/upstream-workflows and cannot run;
they are preserved only as references. No workflow automatically publishes builds.

With GitHub CLI authenticated on the publisher's machine:

```text
python scripts/github-release.py --candidate <folder>
python scripts/github-release.py --candidate <folder> --action draft
```

The first command only validates local files; the second creates a **draft**.
Review the release notes, installer and source. Only when ready to release:

```text
python scripts/github-release.py --candidate <folder> --action publish
```

Publication verifies the uploaded asset digests against the reviewed local files,
then publishes the stable release and marks it latest. Use increasing versions;
published assets must not be replaced with different bytes under the same tag.
For a fix, publish a new higher version. Never label test builds as stable releases.

## Client behavior

Only official builds installed by this installer auto-update. Portable/development
copies remain separate. On each normal launch, the installed client requests the
public repository's /releases/latest endpoint asynchronously. Drafts, prereleases,
unpublished, older or equal versions are ignored. It expects exactly:

```text
OrbitLauncher-Setup-<major.minor.patch>-x64.exe
```

The HTTPS download must come from the configured repository or GitHub's release
asset CDN; size and GitHub-provided SHA-256 digest are verified before handover.
No update runs if metadata/digest is absent, networking fails or the file differs.
The current version continues running. No private GitHub login is needed by clients.

The launcher cancels update downloads when a game starts. Installation is deferred
if a game or another launcher window is in use; the next launch checks again.
Otherwise the installer waits for the requesting launcher process to exit, updates
the registered installation, and reopens with the same data directory. No taskkill.

Application location defaults to %LOCALAPPDATA%/Programs/OrbitLauncher. Player data
defaults to %APPDATA%/OrbitLauncher; --dir overrides it. The installer rejects known
portable-data roots, backs up overwritten app files, and attempts restoration on
installation failure. Uninstall removes only its packaged file list and shortcuts,
never recursively deletes user content, and leaves worlds/accounts untouched.

Offline and mocked updater/installer tests do not establish that a public release
has been uploaded or that GitHub delivery worked end to end. Confirm the first real
upgrade after an explicitly published higher version exists.
