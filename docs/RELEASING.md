# Releasing umoria-jp

This describes how to cut a new release of `umoria-jp` (Copr SRPM and .deb)
from the `main` branch. `main` carries its own independent history, kept
separate from the maintainer's development history for this project — it
is not a mirror of any other branch.

## Steps

1. On `main`, bump the version/release in **both** `umoria-jp.spec` and
   `debian/changelog` — they must always agree, and `deb.yml` runs
   `scripts/check-versions.sh` to enforce this before building (`copr.yml`
   runs its own inline spec-only check instead; see the D2 note in
   `scripts/check-versions.sh`'s header comment):
   - `umoria-jp.spec`: bump `Release:` (e.g. `2%{?dist}` -> `3%{?dist}`) for a
     packaging-only change, or bump `Version:` for a new upstream/translation
     snapshot (reset `Release:` to `1%{?dist}`). Add a new entry at the top of
     `%changelog` describing the change.
   - `debian/changelog`: add a new entry at the top with the **same**
     `<Version>-<Release>` as the spec, e.g. `umoria-jp (5.7.15-3) unstable;
     urgency=medium`. Use `date -R` for the date line (not `dch`, which isn't
     available on the Fedora host used for the Copr side of this workflow).
   - Sanity-check the two files agree before committing:
     `bash scripts/check-versions.sh`.
2. Commit the spec and debian/changelog changes together on `main`.
3. Tag the commit as `jp-v<Version>-<Release>` (the numeric release, without the
   `%{?dist}` macro), e.g. `jp-v5.7.15-3`. The tag **must** point at the commit
   that contains the matching `Version`/`Release`, or `scripts/check-versions.sh`
   will fail in CI and no build will be submitted (for either package format).
4. Push both the commit and the tag to `whitehara/umoria-jp`'s `main` branch
   (the exact command depends on how your local clone's remotes are set up;
   the maintainer's clone uses a remote named `jp` for this repository, e.g.
   `git push jp main:refs/heads/main && git push jp jp-v5.7.15-3`).
5. This triggers two independent workflows on the same tag:
   - `.github/workflows/copr.yml` builds the SRPM via `scripts/make-srpm.sh`
     and submits it to the `whitehara/umoria-jp` Copr project, then attaches
     the SRPM to a GitHub Release for the tag.
   - `.github/workflows/deb.yml` builds the `.deb` via `scripts/make-deb.sh`
     and attaches it to the **same** GitHub Release.
   Both workflows share a `concurrency` group keyed on the tag, so they run
   one after the other rather than racing to create the Release at the same
   time.
6. Watch both Actions runs and the linked Copr build page to confirm success
   across all configured chroots, and that the Release ends up with both a
   `.src.rpm` and a `.deb` attached.
7. If only one of the two workflows fails, fix the underlying issue and
   re-run just that workflow (`gh run rerun <run-id>`) — it will attach its
   artifact to the same, already-existing Release. There's no need to
   re-tag.

## One-time setup (already done, kept here for reference)

- GitHub Actions must be enabled on `whitehara/umoria-jp` (disabled by default on forks).
- Repository secrets `COPR_LOGIN`, `COPR_USERNAME`, `COPR_TOKEN`, `COPR_URL` must
  be registered under the maintainer's Copr account. If the Copr API token is
  ever regenerated, update it here (and anywhere else it's registered). These
  are only used by `copr.yml`; `deb.yml` needs no secrets.
- You can sanity-check a token locally before registering it as a secret with
  `copr-cli whoami` (using a local `~/.config/copr` pointed at the same token).

## Manual/dry-run trigger

Both workflows also accept `workflow_dispatch` (manual run from the Actions
tab, or `gh workflow run copr.yml` / `gh workflow run deb.yml`). A manual run
skips the tag consistency check (there is no tag to check against) and skips
creating a GitHub Release, but still builds and submits/uploads the package:
`copr.yml` still submits to Copr, and `deb.yml` uploads the `.deb` as a
workflow artifact (since there's no Release to attach it to).
