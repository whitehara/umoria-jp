# Releasing umoria-jp

This describes how to cut a new Copr release of `umoria-jp` from the `main`
branch. `main` carries its own independent history, kept separate from the
maintainer's development history for this project — it is not a mirror of
any other branch.

## Steps

1. On `main`, bump the version/release in `umoria-jp.spec`:
   - Bump `Release:` (e.g. `1%{?dist}` -> `2%{?dist}`) for a packaging-only change,
     or bump `Version:` for a new upstream/translation snapshot (reset `Release:` to `1%{?dist}`).
   - Add a new entry at the top of `%changelog` describing the change.
2. Commit the spec change on `main`.
3. Tag the commit as `jp-v<Version>-<Release>` (the numeric release, without the
   `%{?dist}` macro), e.g. `jp-v5.7.15-2`. The tag **must** point at the commit
   that contains the matching spec `Version`/`Release`, or the workflow's tag
   consistency check will fail and no Copr build will be submitted.
4. Push both the commit and the tag to `whitehara/umoria-jp`'s `main` branch
   (the exact command depends on how your local clone's remotes are set up;
   the maintainer's clone uses a remote named `jp` for this repository, e.g.
   `git push jp main:refs/heads/main && git push jp jp-v5.7.15-2`).
5. This triggers `.github/workflows/copr.yml`, which builds the SRPM via
   `scripts/make-srpm.sh` and submits it to the `whitehara/umoria-jp` Copr
   project, then attaches the SRPM to a GitHub Release for the tag.
6. Watch the Actions run and the linked Copr build page to confirm success
   across all configured chroots.

## One-time setup (already done, kept here for reference)

- GitHub Actions must be enabled on `whitehara/umoria-jp` (disabled by default on forks).
- Repository secrets `COPR_LOGIN`, `COPR_USERNAME`, `COPR_TOKEN`, `COPR_URL` must
  be registered under the maintainer's Copr account. If the Copr API token is
  ever regenerated, update it here (and anywhere else it's registered).
- You can sanity-check a token locally before registering it as a secret with
  `copr-cli whoami` (using a local `~/.config/copr` pointed at the same token).

## Manual/dry-run trigger

The workflow also accepts `workflow_dispatch` (manual run from the Actions tab
or `gh workflow run copr.yml`). A manual run skips the tag consistency check
(there is no tag to check against) and skips creating a GitHub Release, but
still builds the SRPM and submits it to Copr.
