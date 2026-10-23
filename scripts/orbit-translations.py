#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate Orbit's six-column UTF-8 catalog and generate native Qt TS files."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
folder = root / 'program_info/orbit-i18n'
languages = ['pt_BR', 'en_US', 'ru', 'zh', 'es', 'ar']
rows = [line.split('\t') for line in (folder/'catalog.tsv').read_text(encoding='utf-8').splitlines()]
seen = set()
for row in rows:
    if len(row) != len(languages) or any(not cell.strip() for cell in row):
        raise ValueError(f'Invalid translation row: {row[0]}')
    if row[0] in seen:
        raise ValueError(f'Duplicate source: {row[0]}')
    seen.add(row[0])
    source_tokens = sorted(re.findall(r'%\d+|%n|\*\.\w+', row[0]))
    for cell in row:
        if sorted(re.findall(r'%\d+|%n|\*\.\w+', cell)) != source_tokens:
            raise ValueError(f'Placeholder or file filter mismatch: {row[0]}')
for index, language in enumerate(languages):
    tree = ET.Element('TS', version='2.1', language=language, sourcelanguage='pt_BR')
    context = ET.SubElement(tree, 'context')
    ET.SubElement(context, 'name').text = 'Orbit'
    for row in rows:
        message = ET.SubElement(context, 'message')
        ET.SubElement(message, 'source').text = row[0].replace('\\n', '\n')
        ET.SubElement(message, 'translation').text = row[index].replace('\\n', '\n')
    plurals = {
        'pt_BR': ['%n instância', '%n instâncias'],
        'en_US': ['%n instance', '%n instances'],
        'ru': ['%n сборка', '%n сборки', '%n сборок'],
        'zh': ['%n 个实例'],
        'es': ['%n instancia', '%n instancias'],
        'ar': ['%n نسخة', '%n نسخة', '%n نسختان', '%n نسخ', '%n نسخة', '%n نسخة']
    }
    message=ET.SubElement(context,'message',numerus='yes')
    ET.SubElement(message,'source').text='%n instância(s)'
    translation=ET.SubElement(message,'translation')
    for text in plurals[language]: ET.SubElement(translation,'numerusform').text=text
    ET.indent(tree)
    ET.ElementTree(tree).write(folder/f'orbit_{language}.ts', encoding='utf-8', xml_declaration=True)
print(f'Validated {len(rows)} messages in {len(languages)} languages.')
