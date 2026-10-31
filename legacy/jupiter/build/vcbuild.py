#!/usr/bin/env python3
"""Build Visual Studio .NET 2003 solutions under Wine, without devenv.

    vcbuild.py [options] <solution.sln> [project ...]

Reads the solution and its .vcproj files and runs cl, rc, lib and link
13.10/7.10 (VC .NET 2003) the way devenv would for one solution
configuration: each project in the configuration the solution maps it to,
after the projects it depends on, objects in the project's
IntermediateDirectory and the target where the project says. Projects
named on the command line are built with their dependencies; with none,
every project the configuration builds is.

Options:
  --config NAME          solution configuration (default: Release)
  --set PROJECT=CONFIG   build PROJECT in project configuration CONFIG
                         instead of the one the solution maps it to
  --skip PROJECT         leave PROJECT out (repeatable)
  --jobs N               parallel compiles (default: CPU count)
  --force                rebuild everything (default: an object is
                         rebuilt when its source is newer; headers are
                         not tracked)
  --keep-going           carry on with other projects after a failure
  --dry-run              print the commands only

Environment: VC71 (the msvc-7.10-3077 tree), DXSDK (a DirectX SDK with
Include/ and Lib/x86/), and a working `wine`. Every command's response file
is kept next to its output (<obj>.rsp, <target>.link.rsp) so it can be rerun
by hand. What devenv adds implicitly is added here too: _AFXDLL / _MBCS /
_UNICODE / _WINDLL from the configuration, /O2 unless Optimization says
otherwise, /EHsc unless ExceptionHandling is FALSE, the default import
libraries, and the targets of library projects a project depends on.
Makefile projects whose build command is `devenv X.sln /build "CONFIG"`
(MfcDlls, the plug-in projects) are built by building that solution
configuration here.
"""
import argparse
import concurrent.futures
import os
import re
import shlex
import subprocess
import sys
import threading
import xml.etree.ElementTree as ET

DEFAULT_LIBS = ('kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib '
                'advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib '
                'odbc32.lib odbccp32.lib').split()
PRINT_LOCK = threading.Lock()


def log(*a):
    with PRINT_LOCK:
        print(*a, flush=True)


# ---------------------------------------------------------------- paths ---

def ci_resolve(base, rel):
    """Resolve a Windows-style relative path case-insensitively, as the
    tools on Windows would see it. Missing components are kept as given."""
    rel = rel.replace('\\', '/')
    if re.match(r'^[A-Za-z]:', rel):
        return rel
    cur = base if not rel.startswith('/') else '/'
    for part in rel.split('/'):
        if part in ('', '.'):
            continue
        if part == '..':
            cur = os.path.dirname(cur)
            continue
        nxt = os.path.join(cur, part)
        if not os.path.exists(nxt) and os.path.isdir(cur):
            m = [x for x in os.listdir(cur) if x.lower() == part.lower()]
            if m:
                nxt = os.path.join(cur, m[0])
        cur = nxt
    return cur


def winpath(p):
    """The Windows path Wine's Z: drive gives an absolute Unix path."""
    return 'Z:' + os.path.abspath(p).replace('/', '\\')


def split_list(s, sep=r'[;,]'):
    return [x.strip() for x in re.split(sep, s or '') if x.strip()]


# ------------------------------------------------------------- solution ---

class Solution:
    def __init__(self, path):
        self.path = os.path.abspath(path)
        self.dir = os.path.dirname(self.path)
        text = open(path, encoding='latin-1').read().replace('\r', '')
        self.projects = {}          # guid -> (name, vcproj path, deps)
        for m in re.finditer(
                r'Project\("[^"]*"\) = "([^"]+)", "([^"]+)", "(\{[^}]+\})"(.*?)EndProject\n',
                text, re.S):
            name, rel, guid, body = m.groups()
            deps = re.findall(r'(\{[0-9A-F-]+\}) = \1', body, re.I)
            self.projects[guid.upper()] = (name, ci_resolve(self.dir, rel),
                                           [d.upper() for d in deps])
        self.map = {}               # (guid, sln config) -> (project config, build?)
        for g, cfg, kind, val in re.findall(
                r'(\{[0-9A-F-]+\})\.([^.\n=]+)\.(ActiveCfg|Build\.0) = ([^\n]+)', text, re.I):
            key = (g.upper(), cfg.strip())
            act, build = self.map.get(key, (None, False))
            if kind == 'ActiveCfg':
                act = val.strip()
            else:
                build = True
            self.map[key] = (act, build)
        self.by_name = {v[0].lower(): k for k, v in self.projects.items()}


# -------------------------------------------------------------- project ---

class Project:
    def __init__(self, name, path, config):
        self.name = name
        self.path = path
        self.dir = os.path.dirname(path)
        # XML turns line breaks inside attribute values into spaces; custom
        # build commands are one command per line, so keep them as &#10;
        raw = open(path, encoding='latin-1').read().replace('\r\n', '\n')
        raw = re.sub(r'="([^"]*)"', lambda m: '="' + m.group(1).replace('\n', '&#10;') + '"', raw)
        raw = raw.replace('encoding="Windows-1252"', 'encoding="latin-1"', 1)
        root = ET.fromstring(raw.encode('latin-1'))
        self.cfg = None
        for c in root.iter('Configuration'):
            if c.get('Name') == config:
                self.cfg = c
        if self.cfg is None:
            raise SystemExit(f'{name}: no configuration {config!r} in {path}')
        self.config = config
        self.tools = {t.get('Name'): dict(t.attrib) for t in self.cfg.iter('Tool')}
        self.type = int(self.cfg.get('ConfigurationType', '1'))
        self.files = []             # (path, per-file tool overrides)
        for f in root.iter('File'):
            over, excluded = {}, False
            for fc in f.iter('FileConfiguration'):
                if fc.get('Name') != config:
                    continue
                excluded = fc.get('ExcludedFromBuild', '').upper() == 'TRUE'
                for t in fc.iter('Tool'):
                    over[t.get('Name')] = dict(t.attrib)
            if not excluded:
                self.files.append((f.get('RelativePath'), over))
        self.macros = {'ProjectName': name, 'ConfigurationName': config.split('|')[0],
                       'PlatformName': 'Win32', 'ProjectDir': self.dir + '/',
                       'ProjectPath': path}
        self.macros['OutDir'] = self.expand(self.cfg.get('OutputDirectory', '.'))
        self.macros['IntDir'] = self.expand(self.cfg.get('IntermediateDirectory', '.'))
        ext = {1: '.exe', 2: '.dll', 4: '.lib'}.get(self.type, '')
        tool = 'VCLibrarianTool' if self.type == 4 else 'VCLinkerTool'
        out = self.tools.get(tool, {}).get('OutputFile') or f'$(OutDir)/{name}{ext}'
        self.target = ci_resolve(self.dir, self.expand(out))
        self.macros['TargetPath'] = self.target
        self.macros['TargetName'] = os.path.splitext(os.path.basename(self.target))[0]
        self.intdir = ci_resolve(self.dir, self.macros['IntDir'])
        self.outdir = ci_resolve(self.dir, self.macros['OutDir'])

    def expand(self, s, extra=None):
        m = dict(self.macros)
        m.update(extra or {})

        def rep(mo):
            return m.get(mo.group(1), m.get(mo.group(1).lower(), ''))
        return re.sub(r'\$\(([A-Za-z]+)\)', rep, s or '')

    def tool(self, name, over=None):
        t = dict(self.tools.get(name, {}))
        if over and name in over:
            for k, v in over[name].items():
                if k in ('AdditionalIncludeDirectories', 'PreprocessorDefinitions',
                         'ForcedIncludeFiles'):
                    t[k] = v + ';' + t.get(k, '')
                else:
                    t[k] = v
        return t

    # --- the compiler ------------------------------------------------------
    def cl_flags(self, t):
        f = ['/nologo', '/c']
        # devenv's default when the attribute is absent is Maximize Speed
        # (Monolith's binaries are /O2 code where their projects say nothing)
        opt = {'0': '/Od', '1': '/O1', '2': '/O2', '3': '/Ox'}
        f.append(opt.get(t.get('Optimization', '2'), '/O2'))
        f += {'1': ['/Ob1'], '2': ['/Ob2']}.get(t.get('InlineFunctionExpansion'), [])
        flag = lambda k, v: t.get(k, '').upper() == 'TRUE' and f.append(v)
        flag('GlobalOptimizations', '/Og')
        flag('EnableIntrinsicFunctions', '/Oi')
        flag('ImproveFloatingPointConsistency', '/Op')
        flag('OmitFramePointers', '/Oy')
        flag('StringPooling', '/GF')
        flag('EnableFunctionLevelLinking', '/Gy')
        flag('RuntimeTypeInfo', '/GR')
        flag('BufferSecurityCheck', '/GS')
        flag('DefaultCharIsUnsigned', '/J')
        flag('TreatWChar_tAsBuiltInType', '/Zc:wchar_t')
        flag('ForceConformanceInForLoopScope', '/Zc:forScope')
        flag('IgnoreStandardIncludePath', '/X')
        f += {'1': ['/Ot'], '2': ['/Os']}.get(t.get('FavorSizeOrSpeed'), [])
        f += {'1': ['/G5'], '2': ['/G6'], '3': ['/G7']}.get(t.get('OptimizeForProcessor'), [])
        f += {'1': ['/arch:SSE'], '2': ['/arch:SSE2']}.get(t.get('EnableEnhancedInstructionSet'), [])
        f += {'1': ['/Gr'], '2': ['/Gz']}.get(t.get('CallingConvention'), [])
        f += {'1': ['/Zp1'], '2': ['/Zp2'], '3': ['/Zp4'], '4': ['/Zp8'],
              '5': ['/Zp16']}.get(t.get('StructMemberAlignment'), [])
        f += {'1': ['/TC'], '2': ['/TP']}.get(t.get('CompileAs'), [])
        if t.get('ExceptionHandling', 'TRUE').upper() != 'FALSE':
            f.append('/EHsc')
        rt = {'0': '/MT', '1': '/MTd', '2': '/MD', '3': '/MDd', '4': '/ML', '5': '/MLd'}
        if t.get('RuntimeLibrary') in rt:
            f.append(rt[t['RuntimeLibrary']])
        if t.get('WarningLevel', '') in '01234' and t.get('WarningLevel'):
            f.append('/W' + t['WarningLevel'])
        if t.get('DebugInformationFormat') in ('1', '3', '4'):
            f.append('/Z7')         # no PDB server under Wine
        for d in split_list(self.expand(t.get('AdditionalIncludeDirectories'))):
            f.append('/I' + d)
        defs = split_list(self.expand(t.get('PreprocessorDefinitions')))
        use_mfc = self.cfg.get('UseOfMFC', '0')
        if use_mfc == '2':
            defs.append('_AFXDLL')
        cs = self.cfg.get('CharacterSet', '0')
        defs += {'1': ['_UNICODE', 'UNICODE'], '2': ['_MBCS']}.get(cs, [])
        if self.type == 2:
            defs.append('_WINDLL')
        for d in dict.fromkeys(defs):
            f.append('/D' + d)
        for u in split_list(self.expand(t.get('UndefinePreprocessorDefinitions')), r';'):
            f.append('/U' + u)
        for fi in split_list(self.expand(t.get('ForcedIncludeFiles')), r';'):
            f.append('/FI' + fi)
        f += shlex.split(self.expand(t.get('AdditionalOptions', '')), posix=False)
        return f

    def pch(self, t):
        mode = t.get('UsePrecompiledHeader', '0')
        through = t.get('PrecompiledHeaderThrough') or 'stdafx.h'
        pchfile = self.expand(t.get('PrecompiledHeaderFile') or f'$(IntDir)/{self.name}.pch')
        if mode == '1':
            return ['/Yc' + through, '/Fp' + pchfile], True
        if mode == '3':
            return ['/Yu' + through, '/Fp' + pchfile], False
        # mode 2 (/YX, "automatic"): every compile may rewrite the one .pch,
        # which parallel compiles cannot share; it only saves time, so it
        # is left out
        return [], False

    def custom_steps(self):
        """Per-file custom build steps: (source, outputs, batch text)."""
        steps = []
        for rel, over in self.files:
            t = over.get('VCCustomBuildTool')
            if not t or not t.get('CommandLine'):
                continue
            src = ci_resolve(self.dir, rel)
            name = os.path.basename(rel)
            m = {'InputPath': rel, 'InputName': os.path.splitext(name)[0],
                 'InputFileName': name, 'InputExt': os.path.splitext(name)[1],
                 'InputDir': os.path.dirname(rel) or '.'}
            cmd = self.expand(t['CommandLine'], m)
            outs = [ci_resolve(self.dir, o) for o in
                    split_list(self.expand(t.get('Outputs', ''), m), r';')]
            steps.append((src, outs, cmd))
        return steps

    def jobs(self):
        """(creates-pch compiles, other compiles, rc compiles, extra inputs)"""
        first, rest, rcs, inputs = [], [], [], []
        seen = set()
        for rel, over in self.files:
            src = ci_resolve(self.dir, rel)
            ext = os.path.splitext(rel)[1].lower()
            if 'VCCustomBuildTool' in over and over['VCCustomBuildTool'].get('CommandLine'):
                continue                # built by its custom step instead
            if ext in ('.c', '.cpp', '.cxx', '.cc'):
                t = self.tool('VCCLCompilerTool', over)
                base = os.path.splitext(os.path.basename(src))[0]
                obj = os.path.join(self.intdir, base + '.obj')
                if obj.lower() in seen:     # two sources, one name: keep apart
                    obj = os.path.join(self.intdir, base + f'_{len(seen)}.obj')
                seen.add(obj.lower())
                pflags, creates = self.pch(t)
                flags = self.cl_flags(t) + pflags + ['/Fo' + obj, src]
                (first if creates else rest).append((src, obj, flags))
            elif ext == '.rc':
                t = self.tool('VCResourceCompilerTool', over)
                res = os.path.join(self.intdir,
                                   os.path.splitext(os.path.basename(src))[0] + '.res')
                flags = ['/l', '0x%x' % int(t.get('Culture', '1033'))]
                for d in split_list(self.expand(t.get('PreprocessorDefinitions'))):
                    flags += ['/d', d]
                for d in [os.path.dirname(src)] + split_list(
                        self.expand(t.get('AdditionalIncludeDirectories'))):
                    flags += ['/i', d]
                flags += ['/fo', res, src]
                rcs.append((src, res, flags))
            elif ext in ('.lib', '.obj', '.res'):
                inputs.append(src)
        return first, rest, rcs, inputs


# -------------------------------------------------------------- running ---

class Runner:
    def __init__(self, args):
        self.a = args
        self.vc = os.environ['VC71']
        self.dx = os.environ['DXSDK']
        self.env = dict(os.environ)
        inc = [f'{self.dx}/Include', f'{self.vc}/include', f'{self.vc}/atlmfc/include',
               f'{self.vc}/PlatformSDK/include']
        lib = [f'{self.dx}/Lib/x86', f'{self.vc}/lib', f'{self.vc}/atlmfc/lib',
               f'{self.vc}/PlatformSDK/lib']
        self.env['INCLUDE'] = ';'.join(winpath(p) for p in inc)
        self.env['LIB'] = ';'.join(winpath(p) for p in lib)
        self.env['WINEPATH'] = winpath(f'{self.vc}/bin')

    def run(self, tool, flags, cwd, rsp, label):
        """Run a tool with its arguments in a response file next to its output.
        Windows tools take the paths: everything absolute becomes Z:\\..."""
        roots = ('/repo/', '/tc/', '/tmp/', '/home/')

        def conv(x):
            if x.startswith(roots):
                return winpath(x)
            for pre in ('/Fo', '/Fp', '/I', '/FI', '/out:', '/implib:', '/def:',
                        '/libpath:'):
                if x.startswith(pre) and x[len(pre):].startswith(roots):
                    return pre + winpath(x[len(pre):])
            return x
        args = [conv(x) for x in flags]
        os.makedirs(os.path.dirname(rsp), exist_ok=True)
        with open(rsp, 'w', newline='\r\n') as fh:
            fh.write('\n'.join(f'"{x}"' if ' ' in x and not x.startswith('"') else x
                               for x in args) + '\n')
        # rc 5.2 has no response files: its arguments go on the command line
        # (the .rsp is still written, as the record of the command)
        cmd = ['wine', f'{self.vc}/bin/{tool}'] + (
            args if tool == 'rc.exe' else ['@' + winpath(rsp)])
        if self.a.dry_run:
            log(f'[{label}] (cd {cwd}; {" ".join(cmd)})')
            return True
        p = subprocess.run(cmd, cwd=cwd, env=self.env, capture_output=True,
                           text=True, errors='replace')
        out = '\n'.join(l for l in (p.stdout + p.stderr).replace('\r', '').splitlines()
                        if l.strip() and not l.lower().endswith(('.cpp', '.c'))
                        and 'Creating library' not in l)
        warnings = [l for l in out.splitlines() if ': warning' in l]
        errors = [l for l in out.splitlines() if 'error' in l.lower() and ': warning' not in l]
        if p.returncode != 0:
            log(f'[{label}] FAILED ({tool} exit {p.returncode})\n{out}')
            return False
        if errors:
            log(f'[{label}]\n' + '\n'.join(errors))
        self.warnings += len(warnings)
        return True

    def stale(self, src, out):
        return (self.a.force or not os.path.exists(out)
                or os.path.getmtime(src) > os.path.getmtime(out))

    def build(self, proj, dep_libs):
        log(f'== {proj.name} [{proj.config}]')
        os.makedirs(proj.intdir, exist_ok=True)
        os.makedirs(os.path.dirname(proj.target), exist_ok=True)
        for src, outs, cmd in proj.custom_steps():
            if outs and all(os.path.exists(o) for o in outs) and not any(
                    self.stale(src, o) for o in outs):
                continue
            bat = os.path.join(proj.intdir, os.path.basename(src) + '.custom.bat')
            os.makedirs(proj.intdir, exist_ok=True)
            with open(bat, 'w', newline='\r\n') as fh:
                fh.write('@echo off\n' + cmd.replace('\r', '') + '\n')
            if self.a.dry_run:
                log(f'[{proj.name}] custom step: {bat}')
                continue
            p = subprocess.run(['wine', 'cmd', '/c', winpath(bat)], cwd=proj.dir,
                               env=self.env, capture_output=True, text=True,
                               errors='replace')
            if p.returncode != 0:
                log(f'[{proj.name}] custom step for {os.path.basename(src)} FAILED'
                    f' (exit {p.returncode})\n' + (p.stdout + p.stderr).replace('\r', ''))
                return False
            # some steps write somewhere other than their declared Outputs
            # (DTXFormat.r's .pipl lands beside it, where the .rc wants it);
            # such a step simply reruns next time
            missing = [o for o in outs if not os.path.exists(o)]
            if missing:
                log(f'[{proj.name}] note: custom step for {os.path.basename(src)} '
                    f'did not write its declared output {missing}')
        first, rest, rcs, inputs = proj.jobs()
        ok = True
        for src, obj, flags in first:
            if self.stale(src, obj) or self.a.force:
                ok &= self.run('cl.exe', flags, proj.dir, obj + '.rsp', proj.name)
        if not ok:
            return False
        todo = [(s, o, f) for s, o, f in rest if self.stale(s, o)]
        with concurrent.futures.ThreadPoolExecutor(self.a.jobs) as ex:
            futs = [ex.submit(self.run, 'cl.exe', f, proj.dir, o + '.rsp', proj.name)
                    for s, o, f in todo]
            futs += [ex.submit(self.run, 'rc.exe', f, proj.dir, o + '.rsp', proj.name)
                     for s, o, f in rcs if self.stale(s, o)]
            ok = all(f.result() for f in futs)
        if not ok:
            return False
        objs = [o for _, o, _ in first + rest] + [o for _, o, _ in rcs] + inputs
        if proj.type == 4:
            t = proj.tool('VCLibrarianTool')
            flags = ['/nologo', '/out:' + proj.target] + objs + split_list(
                proj.expand(t.get('AdditionalDependencies')), r'\s+')
            return self.run('lib.exe', flags, proj.dir, proj.target + '.lib.rsp', proj.name)
        if proj.type in (1, 2):
            t = proj.tool('VCLinkerTool')
            flags = ['/nologo', '/out:' + proj.target, '/machine:I386', '/incremental:no',
                     '/map:' + os.path.splitext(proj.target)[0] + '.map']
            if proj.type == 2:
                flags.append('/dll')
                imp = t.get('ImportLibrary') or f'$(OutDir)/{proj.macros["TargetName"]}.lib'
                flags.append('/implib:' + ci_resolve(proj.dir, proj.expand(imp)))
            flags.append({'1': '/subsystem:console', '2': '/subsystem:windows'}.get(
                t.get('SubSystem'), '/subsystem:windows'))
            if t.get('GenerateDebugInformation', '').upper() == 'TRUE':
                flags.append('/debug')
            if t.get('OptimizeReferences') == '2':
                flags.append('/opt:ref')
            if t.get('EnableCOMDATFolding') == '2':
                flags.append('/opt:icf')
            if t.get('ModuleDefinitionFile'):
                flags.append('/def:' + ci_resolve(proj.dir, proj.expand(t['ModuleDefinitionFile'])))
            if t.get('EntryPointSymbol'):
                flags.append('/entry:' + t['EntryPointSymbol'])
            if t.get('BaseAddress'):
                flags.append('/base:' + t['BaseAddress'])
            for d in split_list(proj.expand(t.get('AdditionalLibraryDirectories'))):
                flags.append('/libpath:' + ci_resolve(proj.dir, d))
            if t.get('IgnoreAllDefaultLibraries', '').upper() == 'TRUE':
                flags.append('/nodefaultlib')
            for n in split_list(proj.expand(t.get('IgnoreDefaultLibraryNames'))):
                flags.append('/nodefaultlib:' + n)
            for d in split_list(proj.expand(t.get('DelayLoadDLLs'))):
                flags.append('/delayload:' + d)
            flags += shlex.split(proj.expand(t.get('AdditionalOptions', '')), posix=False)
            deps = split_list(proj.expand(t.get('AdditionalDependencies')), r'\s+')
            flags += objs + deps + dep_libs + DEFAULT_LIBS
            return self.run('link.exe', flags, proj.dir, proj.target + '.link.rsp', proj.name)
        cmd = proj.tool('VCNMakeTool').get('BuildCommandLine', '')
        m = re.match(r'\s*devenv\s+("?)([^"\s]+\.sln)\1\s+/build\s+"([^"]+)"', cmd, re.I)
        if m:
            # a makefile project that builds another solution configuration
            # (MfcDlls, the Plugins_* projects): build it here instead
            return build_solution(self, ci_resolve(proj.dir, m.group(2)), m.group(3))
        log(f'[{proj.name}] configuration type {proj.type}: nothing to do'
            + (f' (cannot run {cmd!r})' if cmd else ''))
        return not cmd


def main():
    ap = argparse.ArgumentParser(add_help=False)
    ap.add_argument('sln')
    ap.add_argument('projects', nargs='*')
    ap.add_argument('--config', default='Release')
    ap.add_argument('--set', action='append', default=[])
    ap.add_argument('--skip', action='append', default=[])
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--keep-going', action='store_true')
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('-h', '--help', action='store_true')
    a = ap.parse_intermixed_args()
    if a.help:
        sys.exit(__doc__)
    r = Runner(a)
    r.warnings = 0
    r.overrides = {k.lower(): v for k, v in (s.split('=', 1) for s in a.set)}
    r.skip = {s.lower() for s in a.skip}
    r.done = {}                 # (vcproj, config) -> Project, across solutions
    r.failed = []
    build_solution(r, a.sln, a.config, a.projects, top=True)
    log(f'built {len(r.done)} project(s), {r.warnings} warning(s)'
        + (f'; FAILED: {", ".join(r.failed)}' if r.failed else ''))
    sys.exit(1 if r.failed else 0)


def build_solution(r, path, config, names=(), top=False):
    """Build one solution configuration (with --set overrides only at the top
    level). Returns True when everything it built succeeded."""
    sln = Solution(path)
    if not top:
        log(f'-- {os.path.basename(path)} [{config}]')

    def cfg_of(g):
        name = sln.projects[g][0]
        if top and name.lower() in r.overrides:
            c = r.overrides[name.lower()]
            return c if '|' in c else c + '|Win32', True
        return sln.map.get((g, config), (None, False))

    wanted = ([sln.by_name[p.lower()] for p in names] if names
              else [g for g in sln.projects if cfg_of(g)[1]])
    order, seen = [], set()

    def visit(g):
        if g in seen or g not in sln.projects:
            return
        seen.add(g)
        for d in sln.projects[g][2]:
            visit(d)
        order.append(g)
    for g in wanted:
        visit(g)

    local, failed = {}, set()
    for g in order:
        name, vcproj, deps = sln.projects[g]
        cfg, _ = cfg_of(g)
        if name.lower() in r.skip or not cfg:
            continue
        if any(d in failed for d in deps):
            log(f'== {name}: skipped (a dependency failed)')
            failed.add(g)
            r.failed.append(name)
            continue
        key = (vcproj.lower(), cfg)
        if key in r.done:
            local[g] = r.done[key]
            continue
        proj = Project(name, vcproj, cfg)
        dep_libs = [local[d].target for d in deps if d in local and local[d].type == 4]
        if r.build(proj, dep_libs):
            r.done[key] = local[g] = proj
        else:
            failed.add(g)
            r.failed.append(name)
            if not r.a.keep_going:
                raise SystemExit(f'FAILED: {name}')
    return not failed


if __name__ == '__main__':
    main()
