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
# -uall: a new directory is said as the files in it and not as itself, so that
# nothing here has to know how to walk one.
for line in git('status', '--porcelain', '-uall').splitlines():
    state, path = line[:2], line[3:]
    if '->' in line:
        raise SystemExit(f'{path} was renamed: say it as two changes')
    changed.append((path, None if state.strip() == 'D' else 'file'))
if not changed:
    raise SystemExit('nothing here is different from what is up')

# How a name is written, which CODE_STYLE.md says at length: it finishes the
# sentence "After this commit, ...". What cannot finish that sentence is said
# here and the tree is left as it is -- a name is read far more often than it
# is written, and it is put into the names of builds besides.
said = args.message.splitlines()[0]
wrong = []
if said[:1].isupper():
    wrong.append('it begins with a capital letter, and it is not a sentence '
                 'of its own')
if said.endswith('.'):
    wrong.append('it ends with a full stop, and it does not end the sentence '
                 'it finishes')
for prefix in ('feat', 'fix', 'chore', 'docs', 'refactor', 'test'):
    if said.lower().startswith(prefix + ':') or said.lower().startswith(prefix + '('):
        wrong.append(f'it begins with "{prefix}:", and nothing comes in front '
                     'of a name')
# What was done, said as an instruction: an imperative answers what someone
# was told to do, and reads in a log as a thing still to be done.
for imperative in ('add', 'fix', 'update', 'remove', 'improve', 'implement',
                   'bump', 'revert', 'change', 'make', 'let', 'try'):
    if said.lower().startswith(imperative + ' '):
        wrong.append(f'it begins with "{imperative}", which is an '
                     'instruction and not what is true afterwards')
if len(said) > 96:
    wrong.append(f'it is {len(said)} characters, and these names are read in '
                 'the names of builds')
if wrong:
    raise SystemExit('this name finishes no sentence:\n  ' +
                     '\n  '.join(wrong) +
                     '\nA name finishes "After this commit, ..." '
                     '(CODE_STYLE.md).')
if len(said) > 72:
    print(f'{len(said)} characters: short names are asked for, and 72 is '
          'asked for rather than 96')

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
