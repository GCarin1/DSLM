# Tasks — Change 0014-v1-0-0-release

- [x] CHANGELOG.md (Keep a Changelog) with the full 1.0.0 entry
- [x] docs/releases/v1.0.0.md: What's New, changes and defaults, bug fixes, technical, full changelog, assets
- [x] docs/compatibility.md: carts, consoles, emulators
- [x] README: play it / build it, status, roadmap
- [x] .github/workflows/release.yml (tag -> tests, build, soft-float check, assets, release); steps simulated in the pinned image
- [x] Specs ds-interface, inference-engine, model-loading, pc-toolchain marked verified
- [x] Hardware bench run of the v1.0.0 build on the reference R4 (heap peak) imported with `tools/dslm.py bench`
- [x] Hardware run of the final build (model picker, change 0015) switching to tiny1m on the reference R4
- [x] Merge into main and tag v1.0.0; the release workflow publishes the three assets
