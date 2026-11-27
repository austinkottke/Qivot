# Submitting Qivot to vcpkg and ConanCenter

Both registries build from a release tag's tarball, so each submission starts
with a release. Everything below runs on your machine; nothing is pushed until you
push it.

## 1. Release

```sh
# project(VERSION) in CMakeLists.txt must already say the new version;
# the release workflow fails the tag otherwise.
git tag v1.0.6 && git push origin v1.0.6
```

Then fill in the checksums of that tag's tarball:

```sh
tools/update-package-recipes.sh 1.0.6
```

That writes the version and SHA512 into `vcpkg/ports/qivot/` and the version
and SHA256 into `conan/center/qivot/`. Commit those files.

## 2. Clone the registries

Fork both on GitHub first, then:

```sh
git clone https://github.com/<you>/vcpkg ~/src/vcpkg
~/src/vcpkg/bootstrap-vcpkg.sh -disableMetrics
git -C ~/src/vcpkg switch -c qivot

git clone https://github.com/<you>/conan-center-index ~/src/conan-center-index
git -C ~/src/conan-center-index switch -c qivot
```

## 3. Copy the recipes in

```sh
tools/update-package-recipes.sh 1.0.6 --submit \
    --vcpkg ~/src/vcpkg --conan-center ~/src/conan-center-index
```

This writes `ports/qivot/` in the vcpkg clone (and formats its manifest) and
`recipes/qivot/` in the ConanCenter clone. It doesn't commit anything.

## 4. vcpkg

vcpkg records each version by the port's git tree, so commit first, then add the
version and fold it into the same commit:

```sh
cd ~/src/vcpkg
git add ports/qivot && git commit -m "[qivot] new port"
./vcpkg x-add-version qivot
git add versions && git commit --amend --no-edit
./vcpkg install qivot          # optional: build it once locally
git push -u origin qivot
```

Open the pull request against `microsoft/vcpkg`. Its title is `[qivot] new port`.
The PR contains five files:
`ports/qivot/{portfile.cmake,usage,vcpkg.json}`, `versions/q-/qivot.json` and
the `qivot` line in `versions/baseline.json`.

## 5. ConanCenter

```sh
cd ~/src/conan-center-index/recipes/qivot
conan create all --version 1.0.6 --build=missing      # optional: build it once locally
cd ~/src/conan-center-index
git add recipes/qivot && git commit -m "qivot: new recipe"
git push -u origin qivot
```

Open the pull request against `conan-io/conan-center-index`. Its title is
`qivot/1.0.6: new recipe`. ConanCenter asks first-time contributors to sign its
CLA, through a bot on the pull request.

## Later releases

vcpkg: rerun steps 1 and 3, then commit the port and run
`./vcpkg x-add-version qivot` as in step 4. The PR title is
`[qivot] Update to 1.0.7`.

ConanCenter: add the new version to `config.yml` and `all/conandata.yml`. Keep
the old versions there, because `--submit` writes only the newest. The PR title
is `qivot/1.0.7: new version`.

## What was tested

Tested on macOS (Apple Silicon) against the v1.0.5 tag:
- Both recipes build from the tag tarball.
- The vcpkg port passes vcpkg's post-build validation.
- `x-add-version` produces the version files.
- The Conan test package saves and reads back a row.

Not tested here: the Windows (MSVC) and Linux builds. Both registries' CI
builds those, and they're the most likely place for review feedback.
