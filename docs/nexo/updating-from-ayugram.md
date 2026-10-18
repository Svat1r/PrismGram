# Taking new AyuGram releases

PrismGram is never replaced by a new AyuGram version. New AyuGram code is
merged in, tested on its own branch, and only then reaches `main`.

1. Fetch: `git fetch upstream`
2. Read what changed: `git log --oneline main..upstream/dev` and the AyuGram
   release notes. Note anything touching files listed in `README.md`
   ("Changes outside nexo/"), the updater, proxy code or settings.
3. Merge on the sync branch:

   ```
   git checkout upstream-sync
   git merge main
   git merge upstream/dev
   ```

4. Resolve conflicts. Nexo code in `nexo/` rarely conflicts; hooks in
   upstream files may. Keep the Nexo hook and adapt it to the new code.
   Binary icon conflicts: keep ours, then rerun `make_icons.py`.
5. Update submodules: `git submodule update --init --recursive`
6. If `Telegram/build/prepare/prepare.py` changed, rerun the prepare step.
7. Build, run, go through `testing.md`.
8. Check plugins against the new version (once the plugin system exists).
9. Merge into main: `git checkout main && git merge upstream-sync`
10. Add a changelog entry listing AyuGram changes and Nexo changes
    separately, with the new base version.

Never release straight from `upstream-sync`, and never merge upstream into
`main` without steps 7 and 8.
