#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""The page, and the client it loads: one directory, ready for a static host.

What is copied beside the linked .js and .wasm is named by what they are made
of (build.json), so a page left open does not keep a client that has been
replaced; .nojekyll keeps Pages from reading the tree as a Jekyll site.
"""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--program', default='progressive-chat')
args = parser.parse_args()
source = Path(__file__).resolve().parents[1]
args.output.mkdir(parents=True, exist_ok=True)
shutil.copytree(source / 'web', args.output, dirs_exist_ok=True)
for asset in args.build.glob(args.program + '.*'):
    if asset.suffix in {'.js', '.wasm', '.data', '.mem', '.map'}:
        shutil.copy2(asset, args.output / asset.name)
for name in [args.program + '.js', args.program + '.wasm']:
    if not (args.output / name).is_file():
        raise SystemExit(f'Missing linked web client: {name}')
digest = hashlib.sha256()
for asset in sorted(args.output.glob(args.program + '.*')):
    if asset.suffix not in {'.js', '.wasm', '.data', '.mem'}:
        continue
    digest.update(asset.name.encode())
    with asset.open('rb') as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
(args.output / 'build.json').write_text(
    json.dumps({'program': args.program, 'revision': digest.hexdigest()[:20]}) + '\n')
(args.output / '.nojekyll').touch()
