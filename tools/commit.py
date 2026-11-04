#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""What is changed here, given to GitHub as one commit of its own.

Not `git commit` and `git push`, though both end in the same place: a
commit put through GitHub itself is written down as its author's own --
the account it was made with, which is MaurerAnton -- rather than as an
address no account answers to. That is the only reason to be here.

    tools/commit.py "what this is, and why"
    tools/commit.py --dry-run "..."

Everything the working tree holds differently from what is already up
goes into it: files new, changed and gone alike, asked of git rather
than said one by one. GitHub is spoken to through `gh`, which already
knows who you are.
"""
import argparse
import base64
import json
import os
import subprocess


def gh(*args: str, stdin: str | None = None) -> str:
    run = subprocess.run(('gh', 'api', *args), input=stdin, capture_output=True,
                         text=True)
    if run.returncode != 0:
        raise SystemExit(f'gh said no: {run.stderr.strip()}')
    return run.stdout


def gh_json(*args: str, stdin: str | None = None):
    said = gh(*args, stdin=stdin)
    return json.loads(said) if said.strip() else {}


def git(*args: str) -> str:
    return subprocess.run(('git', *args), capture_output=True, text=True,
                          check=True).stdout


parser = argparse.ArgumentParser()
parser.add_argument('message')
parser.add_argument('--dry-run', action='store_true')
args = parser.parse_args()

repo = json.loads(subprocess.run(('gh', 'repo', 'view', '--json', 'nameWithOwner'),
                                 capture_output=True, text=True,
                                 check=True).stdout)['nameWithOwner']
branch = git('rev-parse', '--abbrev-ref', 'HEAD').strip()

# What the working tree says that the branch does not. Nothing here is
# taken from what git staged: staged or not, what is in the files is what
# goes up.
changed = []
for line in git('status', '--porcelain').splitlines():
    state, path = line[:2], line[3:]
    if '->' in line:
        raise SystemExit(f'{path} was renamed: say it as two changes')
    changed.append((path, None if state.strip() == 'D' else 'file'))
if not changed:
    raise SystemExit('nothing here is different from what is up')

if args.dry_run:
    for path, what in changed:
        print(f'{"gone" if what is None else "sent"}  {path}')
    raise SystemExit(0)

head = git('rev-parse', 'HEAD').strip()
# The tree it is put on top of: without it, every file not named would be
# taken away.
base = gh_json(f'repos/{repo}/git/commits/{head}')['tree']['sha']

# Every file given to GitHub as a blob of its own first: a commit names a
# tree, and a tree names blobs.
tree = []
for path, what in changed:
    if what is None:
        tree.append({'path': path, 'mode': '100644', 'type': 'blob', 'sha': None})
        continue
    with open(path, 'rb') as held:
        body = base64.b64encode(held.read()).decode()
    tree.append({'path': path,
                 'mode': '100755' if os.access(path, os.X_OK) else '100644',
                 'type': 'blob',
                 'sha': gh_json(f'repos/{repo}/git/blobs', '--method', 'POST',
                                '-f', 'encoding=base64', '-f', f'content={body}')['sha']})

# base_tree goes in the same body as the tree, and not beside it as its
# own field: said beside it, it is not what GitHub is given, and what is
# made is a commit of nothing but these files -- every other one of them
# taken away.
made = gh_json(f'repos/{repo}/git/trees', '--method', 'POST', '--input', '-',
               stdin=json.dumps({'base_tree': base, 'tree': tree}))

# And said before the branch is moved: how many files it holds now against
# how many it held, which is the one thing that a wrong base_tree takes
# away and nothing else says.
def files_in(sha: str) -> int:
    said = gh_json(f'repos/{repo}/git/trees/{sha}?recursive=1')
    assert not said.get('truncated'), 'too many files to count'
    return sum(1 for one in said['tree'] if one['type'] == 'blob')


# Of what changed, only what GitHub has never seen is a file more; what
# it has seen changes in place.
def tracked(path: str) -> bool:
    return subprocess.run(('git', 'ls-files', '--error-unmatch', path),
                          capture_output=True).returncode == 0


gone = sum(1 for _, what in changed if what is None)
added = sum(1 for path, what in changed if what is not None and not tracked(path))
held = files_in(base)
now = files_in(made['sha'])
if now != held + added - gone:
    raise SystemExit(
        f'{now} files where {held + added - gone} belong: nothing has been '
        'committed, and nothing moved')
commit = gh_json(f'repos/{repo}/git/commits', '--method', 'POST',
                 '-f', f'message={args.message}', '-f', f'tree={made["sha"]}',
                 '-f', f'parents[]={head}')
gh(f'repos/{repo}/git/refs/heads/{branch}', '--method', 'PATCH',
   '-f', f'sha={commit["sha"]}')
print(f'[{branch} {commit["sha"][:12]}] {args.message.splitlines()[0]}')
# What is up and what is here are one now, and the branch is where it is.
# Kept mixed and not soft: soft moves what git thinks HEAD is and leaves
# what git thinks is staged behind it, so that what it says is next to be
# committed is what the commit before this one said -- a whole round of
# it seen as undone.
git('fetch', '-q', 'origin', branch)
git('reset', '-q', '--mixed', f'origin/{branch}')
