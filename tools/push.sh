#!/usr/bin/env bash
# push.sh <node> - upload the source tree (src/, vms/, probes/) to <node>'s work directory.
#
# The tree is uploaded as is to <workdir>/{src,vms,probes} - there is no
# upstream tarball, so no staging step.  Only files whose content changed
# since the last push to this node are uploaded, so MMS sees new timestamps
# only on what really changed.  PUSH_ALL=1 uploads everything.
# Re-pushing creates new file versions; build.sh purges the old ones on VMS.
set -euo pipefail
export LC_ALL=C   # sort and comm must agree on collation

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: push.sh <node>}
read -r _ _ HOST PORT USER WORKDIR SFTPDIR < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf") || true
[ -n "${HOST:-}" ] || { echo "push: unknown node '$node' (see tools/nodes.conf)" >&2; exit 2; }

dirs=(src vms probes)
mkdir -p "$top/cache"
manifest=$top/cache/pushed-$node.sha
[ "${PUSH_ALL:-}" = 1 ] && rm -f "$manifest"
touch "$manifest"
now=$top/cache/push-$node.sha
(cd "$top" && find "${dirs[@]}" -type f ! -name '*~' -print0 | sort -z | xargs -0 sha256sum) > "$now"
changed=$(comm -23 <(awk '{print $2" "$1}' "$now" | sort) \
                   <(awk '{print $2" "$1}' "$manifest" | sort) | awk '{print $1}')
echo "push: $(echo "$changed" | grep -c . || true) changed of $(wc -l < "$now") files -> $node"
batch=$top/cache/push-$node.batch
{
    echo "-mkdir $SFTPDIR"
    echo "cd $SFTPDIR"
    (cd "$top" && find "${dirs[@]}" -type d) | sed 's/^/-mkdir /'
    for f in $changed; do echo "put $top/$f $f"; done
} > "$batch"
# Explicit put per file: 'put -r' into an existing directory nests a copy.
sftp -P "$PORT" -i "${VMS_SSH_KEY:-$HOME/.ssh/vms_ed25519}" -o BatchMode=yes -b "$batch" "$USER@$HOST" \
    2>&1 >/dev/null | grep -vE '^ *Welcome to|^ *$|^remote mkdir .*Failure' >&2 || true
cp "$now" "$manifest"
echo "push: done"
