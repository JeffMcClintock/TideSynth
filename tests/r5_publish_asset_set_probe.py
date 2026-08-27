"""Extract the two changed run: blocks from release.yml and RUN them.

YAML validity is not shell validity (A33 shipped a jq program that parsed as
perfectly valid YAML and was broken shell). So this pulls the real strings out
of the workflow, substitutes only what GitHub would expand, and executes them
against a tree that reproduces what package-macos.sh and package-windows.ps1
actually leave in dist/.

It must FAIL on the v0.1.3 shape before its passes mean anything.

Note: run on Windows, Git Bash's sha256sum writes a '*' binary marker where
the ubuntu runner writes two spaces. That is this harness, not the workflow.
"""
import io, os, shutil, subprocess, sys, tempfile, yaml

ROOT = os.path.join('C:', os.sep, 'SE', '_scratch', 'e19b', 'probe')
_n = [0]

def workdir():
    _n[0] += 1
    d = os.path.join(ROOT, 'w%d' % _n[0])
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    return d

BS = chr(92)
BASH = os.path.join('C:', os.sep, 'Program Files', 'Git', 'bin', 'bash.exe')
WF = os.path.join('C:', os.sep, 'SE', 'TideSynth', '.github', 'workflows', 'release.yml')
doc = yaml.safe_load(io.open(WF, encoding='utf-8'))

verify = next(s['run'] for s in doc['jobs']['build']['steps']
              if s.get('name', '').startswith('Verify the expected asset'))
collect = next(s['run'] for s in doc['jobs']['publish']['steps']
               if s.get('name', '').startswith('Collect assets'))


def posix(p):
    p = os.path.abspath(p)
    return '/' + p[0].lower() + p[2:].replace(BS, '/')


def sh(script, cwd, matrix=None):
    # Write the script to a file: passing a multi-line script as an argv
    # element from Windows Python to MSYS bash mangles the quoting.
    script = script.replace('${{ matrix.name }}', matrix or '')
    f = os.path.join(cwd, '_step.sh')
    with io.open(f, 'w', encoding='utf-8', newline=chr(10)) as fh:
        fh.write(script)
    env = dict(os.environ, GITHUB_WORKSPACE=posix(cwd), MSYS_NO_PATHCONV='1')
    return subprocess.run([BASH, posix(f)], cwd=cwd, env=env,
                          capture_output=True, text=True)


def touch(path, body='x'):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as fh:
        fh.write(body)


def make_dist(ws, platform):
    """dist/ as the packaging scripts really leave it."""
    d = os.path.join(ws, 'dist')
    os.makedirs(d, exist_ok=True)
    if platform == 'macos':
        touch(os.path.join(d, 'TIDE-Rack-macOS.pkg'))
        touch(os.path.join(d, 'component.pkg'))
        touch(os.path.join(d, 'Distribution.xml'))
        app = os.path.join(d, 'root', 'Applications', 'TIDE-Rack-AUv3.app', 'Contents')
        touch(os.path.join(app, 'Info.plist'))
        touch(os.path.join(app, 'CodeResources'))
        touch(os.path.join(app, 'PlugIns', 'TIDE-Rack.appex', 'Contents', 'MacOS', 'TIDE-Rack'))
    elif platform == 'windows':
        touch(os.path.join(d, 'TIDE-Rack-Windows.exe'))
        touch(os.path.join(d, 'TIDE-Rack-Windows.zip'))
        touch(os.path.join(d, 'README.txt'))
    else:
        touch(os.path.join(d, 'TIDE-Rack-Linux.tar.gz'))


fails = 0


def report(name, ok, detail=''):
    global fails
    tail = ('  -- ' + detail) if (detail and not ok) else ''
    print(('  PASS  ' if ok else '  FAIL  ') + name + tail)
    if not ok:
        fails += 1


print('1. the build job stages ONLY the named assets')
staged = {}
for plat, want in (('macos', {'TIDE-Rack-macOS.pkg'}),
                   ('windows', {'TIDE-Rack-Windows.exe', 'TIDE-Rack-Windows.zip'}),
                   ('linux', {'TIDE-Rack-Linux.tar.gz'})):
    ws = workdir()
    make_dist(ws, plat)
    r = sh(verify, ws, plat)
    up = os.path.join(ws, 'upload')
    got = set(os.listdir(up)) if os.path.isdir(up) else set()
    report('%s: rc=%d upload=%s' % (plat, r.returncode, sorted(got)),
           r.returncode == 0 and got == want,
           '' if got == want else 'wanted %s; %s' % (sorted(want), r.stderr.strip()[:180]))
    staged[plat] = up


def make_staging(ws, extra=None, nested=False):
    st = os.path.join(ws, 'staging')
    for plat in ('macos', 'windows', 'linux'):
        d = os.path.join(st, 'dist-' + plat)
        os.makedirs(d, exist_ok=True)
        for f in sorted(os.listdir(staged[plat])):
            shutil.copy(os.path.join(staged[plat], f), d)
    if nested:
        # the v0.1.3 shape: a job that uploaded a whole directory tree
        touch(os.path.join(st, 'dist-macos', 'root', 'Contents', 'Info.plist'))
    if extra:
        touch(os.path.join(st, 'dist-linux', extra))
    return st


WANT = ['SHA256SUMS.txt', 'TIDE-Rack-Linux.tar.gz', 'TIDE-Rack-Windows.exe',
        'TIDE-Rack-Windows.zip', 'TIDE-Rack-macOS.pkg']


def assets(ws):
    out = os.path.join(ws, 'dist')
    if not os.path.isdir(out):
        return []
    return sorted(x for x in os.listdir(out) if x != '_step.sh')


print('2. publish accepts exactly the four expected assets')
ws = workdir()
make_staging(ws)
r = sh(collect, ws)
got = assets(ws)
report('rc=%d assets=%s' % (r.returncode, got), r.returncode == 0 and got == WANT,
       '' if got == WANT else (r.stdout + r.stderr).strip()[-220:])

print('3. NEGATIVE -- an unexpected asset must FAIL the release')
ws = workdir()
make_staging(ws, extra='component.pkg')
r = sh(collect, ws)
report('rc=%d (want non-zero)' % r.returncode, r.returncode != 0,
       'the gate did not fire' if r.returncode == 0 else '')
report('and it names the offender', 'component.pkg' in (r.stdout + r.stderr))

print('4. NEGATIVE -- the v0.1.3 shape: a nested tree must not be flattened in')
ws = workdir()
make_staging(ws, nested=True)
r = sh(collect, ws)
got = assets(ws)
report('rc=%d assets=%s' % (r.returncode, got), r.returncode == 0 and got == WANT,
       'Info.plist leaked in' if 'Info.plist' in got else (r.stdout + r.stderr).strip()[-220:])

print()
print('PROBE OK' if fails == 0 else 'PROBE FAILED (%d)' % fails)
sys.exit(1 if fails else 0)
