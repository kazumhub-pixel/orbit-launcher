#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage a Windows installer; compiling/testing never uploads or publishes a release."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument('--runtime',required=True,type=Path)
parser.add_argument('--output',required=True,type=Path)
parser.add_argument('--makensis',required=True,type=Path)
args=parser.parse_args()
source=Path(__file__).resolve().parents[1]
release=json.loads((source/'release.json').read_text(encoding='utf-8'))
version=release['version']
runtime=args.runtime.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
build=json.loads((runtime/'orbit-build.json').read_text(encoding='utf-8'))
if build.get('official')!=1 or build.get('version')!=version or build.get('repository')!=release['repository']:
    raise ValueError('Only an explicit official build matching release.json can become a release candidate')
payload=out/'app';payload.mkdir(exist_ok=False)
for entry in runtime.iterdir():
    if entry.name in ['portable.txt','UserData','instances','accounts.json']:
        continue
    if entry.is_dir():shutil.copytree(entry,payload/entry.name)
    else:shutil.copy2(entry,payload/entry.name)
shutil.copy2(payload/'AuroraLauncher.exe',payload/'OrbitLauncher.exe')
(payload/'AuroraLauncher.exe').unlink()
(payload/'orbit-installed.json').write_text(json.dumps({'version':version,'repository':release['repository']},indent=2),encoding='utf-8')
shutil.copy2(source/'LICENSE',payload/'LICENSE')
shutil.copy2(source/'HELIOS-LICENSE.txt',payload/'HELIOS-LICENSE.txt')
generated=out/'installer-includes';generated.mkdir()
files=sorted(p for p in payload.rglob('*') if p.is_file())
install=[];backup=[];restore=[];uninstall=[]
for file in files:
    rel=file.relative_to(payload)
    if any('"' in p or '$' in p for p in rel.parts):raise ValueError('Unsupported payload filename')
    windows=str(rel).replace('/','\\');parent=str(rel.parent).replace('/','\\')
    install += [f'SetOutPath "$INSTDIR\\{parent}"',f'File "${{PAYLOAD}}\\{windows}"']
    backup += [f'CreateDirectory "$PLUGINSDIR\\rollback\\{parent}"',f'IfFileExists "$INSTDIR\\{windows}" 0 +2',f'CopyFiles /SILENT /FILESONLY "$INSTDIR\\{windows}" "$PLUGINSDIR\\rollback\\{parent}"']
    restore += [f'IfFileExists "$PLUGINSDIR\\rollback\\{windows}" 0 +2',f'CopyFiles /SILENT /FILESONLY "$PLUGINSDIR\\rollback\\{windows}" "$INSTDIR\\{parent}"']
    uninstall += [f'Delete "$INSTDIR\\{windows}"']
directories=sorted({p.parent.relative_to(payload) for p in files},key=lambda p:len(p.parts),reverse=True)
for directory in directories:
    if str(directory)!='.':uninstall.append(f'RMDir "$INSTDIR\\{str(directory)}"')
for name,lines in [('install',install),('backup',backup),('restore',restore),('uninstall',uninstall)]:
    (generated/f'{name}.nsh').write_text('\n'.join(lines)+'\n',encoding='utf-8')
installer=out/f'OrbitLauncher-Setup-{version}-x64.exe'
command=[str(args.makensis.resolve()),f'/DVERSION={version}',f'/DPAYLOAD={payload}',f'/DGENERATED={generated}',f'/DOUTPUT={installer}',str(source/'scripts/windows/orbit-installer.nsi')]
subprocess.run(command,check=True)
manifest={'version':version,'repository':release['repository'],'channel':'stable','installer':installer.name,
          'sha256':hashlib.sha256(installer.read_bytes()).hexdigest(),'size':installer.stat().st_size,
          'published':False}
(out/'release-candidate.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('Prepared locally:',installer)
