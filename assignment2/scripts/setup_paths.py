#!/usr/bin/env python3
"""Rebase copied Vivado/Vitis configuration after moving this package.

Run before opening the tools. Existing bitstreams, checkpoints, libraries and
reports are never rewritten. Pass --tools-root if AMD 2026.1 is installed elsewhere.
"""
import argparse
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / '.project_paths.json'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tools-root', type=Path,
                        help='AMD version directory containing Vivado/ and Vitis/')
    args = parser.parse_args()
    previous = json.loads(STATE.read_text())
    candidates = [args.tools_root]
    for variable in ('XILINX_VITIS', 'XILINX_VIVADO'):
        value = os.environ.get(variable)
        if value:
            candidates.append(Path(value).parent)
    candidates.append(Path(previous['tools']))
    tools = next((p.resolve() for p in candidates if p and (p / 'Vitis').is_dir()
                  and (p / 'Vivado').is_dir()), None)
    if tools is None:
        parser.error('Specify --tools-root /path/to/2026.1 (containing Vitis and Vivado).')

    old_external = '/home/dodo/Projects/embeded/ai/SNN_guest_lecture'
    replacements = [
        (old_external + '/snn_mlp_model_assignment/models/fpga_export', str(ROOT / 'snn_ip/weights')),
        (old_external + '/snn_mlp_model/models/fpga_export', str(ROOT / 'snn_ip/weights')),
        (old_external + '/code/snn_mlp.cpp', str(ROOT / 'snn_ip/src/snn_mlp.cpp')),
        (old_external + '/code/snn_mlp_tb.cpp', str(ROOT / 'snn_ip/tb/snn_mlp_tb.cpp')),
        (previous['root'], str(ROOT)),
        ('/home/dodo/Work/vivado/projects/snn', str(ROOT)),
        ('/home/dodo/Work/vivado/projects/ip', str(ROOT / 'ip_repo/cnn')),
        (previous['tools'], str(tools)),
    ]
    extensions = {'.json', '.cfg', '.cmake', '.xml', '.xpr', '.bd', '.xci', '.xpfm',
                  '.spfm', '.yaml', '.yml', '.spec', '.tcl', '.prj', '.ninja'}
    changed = 0
    for path in ROOT.rglob('*'):
        if not path.is_file() or path.is_symlink():
            continue
        rel = path.relative_to(ROOT)
        if rel.parts[0] in {'dfx_debug', 'dataset_audit', 'scripts', 'artifacts'}:
            continue
        if 'logs' in rel.parts:
            continue
        if path.suffix.lower() not in extensions and path.name not in {'CMakeLists.txt', 'CMakeCache.txt', 'Makefile', '.clangd'}:
            continue
        try:
            text = path.read_text()
        except (UnicodeError, OSError):
            continue
        updated = text
        for old, new in replacements:
            updated = updated.replace(old, new)
        if updated != text:
            path.write_text(updated)
            changed += 1

    # Keep hardware dependencies local, including the PYNQ-Z2 board definition.
    project = ROOT / 'assignment2.xpr'
    import re
    text = project.read_text()
    text = re.sub(r'(<Option Name="BoardPartRepoPaths" Val=")[^"]*("/>)',
                  r'\g<1>$PPRDIR/board_files\2', text)
    repo_options = ('    <Option Name="IPRepoPath" Val="$PPRDIR/snn_ip"/>\n'
                    '    <Option Name="IPRepoPath" Val="$PPRDIR/ip_repo/cnn"/>')
    text = re.sub(r'(?m)^[ \t]*<Option Name="IPRepoPath" Val="[^"]*"/>[ \t]*\n?', '', text)
    text = text.replace('  <Configuration>', '  <Configuration>\n' + repo_options, 1)
    if project.read_text() != text:
        project.write_text(text)

    # Vitis rebuilds use the matching isolated DFX handoff.
    component = ROOT / 'platform/vitis-comp.json'
    if component.exists():  # the platform is created by scripts/update_platform.py
        data = json.loads(component.read_text())
        data['configuration']['xsa'] = str(ROOT / 'artifacts/dfx/main_wrapper.xsa')
        component.write_text(json.dumps(data, indent=2) + '\n')

    # Only the testbench's default data path changes; accelerator calculations do not.
    tb = ROOT / 'snn_ip/tb/snn_mlp_tb.cpp'
    text = tb.read_text()
    updated = text.replace(previous['root'] + '/snn_ip/test_features',
                           str(ROOT / 'snn_ip/test_features'))
    updated = updated.replace('/home/dodo/Work/vivado/projects/snn/snn_ip/test_features',
                              str(ROOT / 'snn_ip/test_features'))
    if updated != text:
        tb.write_text(updated)
    STATE.write_text(json.dumps({'root': str(ROOT), 'tools': str(tools)}, indent=2) + '\n')
    print(f'Configured {ROOT}; rebased {changed} text files. Existing hardware binaries retained.')


if __name__ == '__main__':
    main()
