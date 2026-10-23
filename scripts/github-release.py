#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit release operations. Default is offline validation, never publication."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument('--candidate',required=True,type=Path)
parser.add_argument('--action',choices=['validate','draft','publish'],default='validate')
args=parser.parse_args()
folder=args.candidate.resolve()
candidate=json.loads((folder/'release-candidate.json').read_text(encoding='utf-8'))
source=Path(__file__).resolve().parents[1]
config=json.loads((source/'release.json').read_text(encoding='utf-8'))
if candidate['channel']!='stable' or candidate['version']!=config['version'] or candidate['repository']!=config['repository']:
    raise SystemExit('Candidate does not match the configured stable release')
version=candidate['version'];repo=candidate['repository'];tag='v'+version
if not re.fullmatch(r'\d+\.\d+\.\d+',version) or not re.fullmatch(r'[A-Za-z0-9-]+/[A-Za-z0-9_.-]+',repo):raise SystemExit('Invalid release identity')
expected=f'OrbitLauncher-Setup-{version}-x64.exe'
installer=folder/expected
if candidate['installer']!=expected or hashlib.sha256(installer.read_bytes()).hexdigest()!=candidate['sha256']:
    raise SystemExit('Installer checksum mismatch')
source_archive=folder/f'OrbitLauncher-Source-{version}.zip'
if not source_archive.is_file():raise SystemExit('Corresponding source archive is required')
if args.action=='validate':
    print('Validated locally. No network operation or publication performed.')
    raise SystemExit(0)
gh=shutil.which('gh')
if not gh:raise SystemExit('GitHub CLI is required. Authenticate locally with gh auth login; never put tokens in the package.')
def run(arguments):
    result=subprocess.run([gh]+arguments,check=True,text=True,capture_output=True)
    return result.stdout
run(['auth','status'])
remote=json.loads(run(['api',f'repos/{repo}/contents/release.json']))
import base64
remote_config=json.loads(base64.b64decode(remote['content']))
if remote_config!=config:raise SystemExit('Push and review the matching source commit before preparing a release')
if args.action=='draft':
    run(['release','create',tag,str(installer),str(source_archive),'--repo',repo,'--draft','--title',f'Orbit Launcher {version}',
         '--notes-file',str(folder/'RELEASE-NOTES.md')])
    print('Draft created. Clients will not install this draft.')
else:
    release=json.loads(run(['api',f'repos/{repo}/releases/tags/{tag}']))
    if release.get('draft') is not True or release.get('prerelease') is not False:raise SystemExit('Only the reviewed stable draft can be published')
    assets={asset['name']:asset for asset in release['assets']}
    for path in [installer,source_archive]:
        asset=assets.get(path.name,{})
        digest='sha256:'+hashlib.sha256(path.read_bytes()).hexdigest()
        if asset.get('state')!='uploaded' or asset.get('digest')!=digest:raise SystemExit('Uploaded draft asset does not match the local candidate')
    run(['release','edit',tag,'--repo',repo,'--draft=false','--latest'])
    print(f'Published https://github.com/{repo}/releases/tag/{tag}')
