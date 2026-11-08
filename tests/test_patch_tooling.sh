#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }
expect_rejected() {
    local repo="$1" patch="$2"
    if "$repo/tools/apply_patch.sh" "$patch" >"$work/rejected.out" 2>&1; then
        fail "Unsafe patch accepted: $patch"
    fi
}

make_repo() {
    local path="$1"
    mkdir -p "$path/tools" "$path/docs" "$path/build-linux-gpu"
    cp -p "$root/tools/apply_patch.sh" "$root/tools/zip_vkeybd_project.sh" "$path/tools/"
    printf 'Original\n' > "$path/docs/network.txt"
    printf 'Remove this\n' > "$path/docs/obsolete.txt"
    printf 'This is a fake secret\n' > "$path/.env"
    printf 'Generated object\n' > "$path/build-linux-gpu/ignored.txt"
    printf '/build-*/\n.env\n' > "$path/.gitignore"
    git -C "$path" init -q
    git -C "$path" add .
    git -C "$path" -c user.name=ToolingTest -c user.email=test@example.com commit -qm fixture
}

repo1="$work/not the repo name with spaces"
repo2="$work/a different location/another-checkout"
make_repo "$repo1"
make_repo "$repo2"

for repo in "$repo1" "$repo2"; do
    "$repo/tools/zip_vkeybd_project.sh" >/dev/null
    archive="$(dirname "$repo")/vkeybd.zip"
    python3 - "$archive" <<'PY'
import sys,zipfile
with zipfile.ZipFile(sys.argv[1]) as z:
    names=set(z.namelist())
    assert 'vkeybd-qt/docs/network.txt' in names
    assert 'vkeybd-qt/tools/apply_patch.sh' in names
    assert 'vkeybd-qt/tools/zip_vkeybd_project.sh' in names
    assert not any('.git/' in n or '/build-' in n or '.env' in n for n in names)
    assert not any('not the repo name' in n or 'another-checkout' in n for n in names)
PY
done
printf 'PASS: fixed archive namespace, arbitrary paths, source-only ZIPs\n'

python3 - "$work" "$root/tools/apply_patch.sh" <<'PY'
import pathlib,stat,sys,warnings,zipfile
out=pathlib.Path(sys.argv[1])
root='vkeybd-qt/'
def mk(name,files):
    with zipfile.ZipFile(out/name,'w') as z:
        for filename,content in files.items():
            z.writestr(filename,content)
mk('vkeybd-valid.zip',{
    root+'docs/network.txt':'Updated\n',
    root+'docs/new-file.txt':'New\n',
    root+'.vkeybd-patch-delete.txt':'docs/obsolete.txt\n',
})
mk('vkeybd-self-update.zip',{
    root+'tools/apply_patch.sh':pathlib.Path(sys.argv[2]).read_bytes()+b'\n# updated marker\n',
})
mk('vkeybd-traversal.zip',{root+'../outside.txt':'malicious'})
mk('vkeybd-wrong-root.zip',{'wrong-project/docs/network.txt':'malicious'})
mk('vkeybd-git.zip',{root+'.git/config':'malicious'})
mk('vkeybd-conflict.zip',{
    root+'docs/network.txt':'bad\n',
    root+'.vkeybd-patch-delete.txt':'docs/network.txt\n',
})
mk('vkeybd-only-delete.zip',{
    root+'.vkeybd-patch-delete.txt':'docs/new-file.txt\n',
})
mk('vkeybd-dir-delete.zip',{
    root+'docs/new-file.txt':'bad\n',
    root+'.vkeybd-patch-delete.txt':'docs\n',
})
mk('vkeybd-duplicate.zip',{root+'docs/network.txt':'bad\n'})
with warnings.catch_warnings():
    warnings.simplefilter('ignore', UserWarning)
    with zipfile.ZipFile(out/'vkeybd-duplicate.zip','a') as z:
        z.writestr(root+'docs/network.txt','duplicate\n')
with zipfile.ZipFile(out/'vkeybd-symlink.zip','w') as z:
    info=zipfile.ZipInfo(root+'docs/link')
    info.create_system=3
    info.external_attr=(stat.S_IFLNK|0o777)<<16
    z.writestr(info,'../../../outside')
PY

for repo in "$repo1" "$repo2"; do
    "$repo/tools/apply_patch.sh" "$work/vkeybd-valid.zip" >/dev/null
    [[ "$(cat "$repo/docs/network.txt")" == Updated ]] || fail 'Update not applied'
    [[ -f "$repo/docs/new-file.txt" ]] || fail 'New file absent'
    [[ ! -e "$repo/docs/obsolete.txt" ]] || fail 'Deletion not applied'
    [[ ! -e "$repo/.vkeybd-patch-delete.txt" ]] || fail 'Manifest left in checkout'
    [[ "$(unzip -p "$(dirname "$repo")/vkeybd.zip" vkeybd-qt/docs/network.txt)" == Updated ]] || fail 'Snapshot stale'
done
printf 'PASS: patch applied on two unrelated checkouts, deletions, snapshot refreshed\n'

"$repo1/tools/apply_patch.sh" "$work/vkeybd-self-update.zip" >/dev/null
grep -F '# updated marker' "$repo1/tools/apply_patch.sh" >/dev/null || fail 'Self-update failed'
[[ -x "$repo1/tools/apply_patch.sh" ]] || fail 'Executable bit lost in self-update'
printf 'PASS: safe self-update\n'

"$repo2/tools/apply_patch.sh" "$work/vkeybd-only-delete.zip" >/dev/null
[[ ! -e "$repo2/docs/new-file.txt" ]] || fail 'Deletion-only patch failed'
printf 'PASS: deletion-only patch\n'

# Rejected patches must leave previously existing files intact.
for kind in traversal wrong-root git conflict dir-delete duplicate symlink; do
    expect_rejected "$repo1" "$work/vkeybd-$kind.zip"
    [[ "$(cat "$repo1/docs/network.txt")" == Updated ]] || fail "Data modified by $kind"
done
[[ ! -e "$work/outside.txt" ]] || fail 'Path traversal escaped repository'
printf 'PASS: traversal, foreign namespace, Git metadata, collision and symlink rejection\n'

ln -s "$work" "$repo1/docs/redirect"
python3 - "$work/vkeybd-through-local-symlink.zip" <<'PY'
import sys,zipfile
with zipfile.ZipFile(sys.argv[1],'w') as z:
    z.writestr('vkeybd-qt/docs/redirect/exploit.txt','BAD')
PY
expect_rejected "$repo1" "$work/vkeybd-through-local-symlink.zip"
[[ ! -e "$work/exploit.txt" ]] || fail 'Existing local symlink followed'
printf 'PASS: local symlink escape rejected\n'

# A custom output path works while default vkeybd.zip remains beside checkout.
"$repo1/tools/zip_vkeybd_project.sh" "$work/custom snapshot.zip" >/dev/null
unzip -tq "$work/custom snapshot.zip" >/dev/null
printf 'PASS: custom snapshot path and ZIP integrity\n'

echo 'All vkeybd-qt patch/snapshot tooling tests passed.'
