#!/usr/bin/env python3
"""Build a local arm64 macOS app using a wasm32 -> portable C intermediate.

WASI SDK supplies the freestanding LLVM backend only. The runtime is SDL3 and
w2c2's header; no WASI, JavaScript, browser, JIT, Rosetta, or GBA CPU emulation.
Dependencies are explicit paths; never downloads tools or ROMs automatically.
"""
import argparse
import concurrent.futures
import hashlib
import json
import inspect
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
import struct

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'port/tools'))
import baserom
from wasm_ir import rewrite_ir


def run(args, **kwargs):
    result = subprocess.run([str(a) for a in args], **kwargs)
    if result.returncode:
        if result.stderr: print(result.stderr.decode() if isinstance(result.stderr, bytes) else result.stderr, file=sys.stderr)
        raise RuntimeError('Command failed: ' + str(args[0]))



def verify_portable_binary(path):
    """Reject accidental host-only libraries or a raised OS/CPU requirement."""
    archs=subprocess.check_output(['lipo','-archs',str(path)],text=True).split()
    if 'arm64' not in archs:
        raise RuntimeError(f'{path}: missing Apple Silicon arm64 slice')
    info=subprocess.check_output(['otool','-l',str(path)],text=True)
    minimum=re.search(r'\bminos (\d+(?:\.\d+)*)',info)
    version=tuple(int(v) for v in minimum[1].split('.')) if minimum else ()
    if not minimum or (version+(0,0))[:2]>(15,0):
        raise RuntimeError(f'{path}: expected a macOS minimum of 15.0 or earlier')
    linked=subprocess.check_output(['otool','-L',str(path)],text=True)
    for line in linked.splitlines()[1:]:
        dependency=line.strip().split(' (')[0]
        if dependency=='@rpath/libSDL3.0.dylib':continue
        if dependency.startswith(('/System/Library/','/usr/lib/')):continue
        raise RuntimeError(f'{path}: non-bundled dependency {dependency}')


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('Upstream integration anchor changed: ' + old[:100])
    return text.replace(old, new)


def staged(source, out):
    text = source.read_text()
    if source.name == 'rom_relocs.c':
        text = re.sub(r'extern char (SrollBlit\w+)\[\];', r'extern void \1(void);', text)
        text = re.sub(r'(\{0x[\da-fA-F]+,\s*)(SrollBlit\w+)(\s*\})', r'\1(const void*)\2\3', text)
    if source.name == 'libc.c':
        text = replace_once(text, 'extern char __heap_start[], __heap_end[];',
                            'static char __heap_start[0x06000000];\n#define __heap_end (__heap_start + sizeof(__heap_start))')
    if source.name == 'port_bios.c':
        text = replace_once(text, '__builtin_return_address(0)', '(void*)0')
    if source.name == 'port_main.c':
        text = replace_once(text, '    setjmp(sResetJmp);', '    /* host reinstantiates the core on soft reset */')
        text = replace_once(text, '    longjmp(sResetJmp, 1);',
                            '    PortSramSyncIfDirty();\n    PortConfigSave();\n    exit(75);')
    if source.name == 'sdl_shim.c':
        text = '#include "keyboard.h"\n' + text
        text = text.replace('return "vulkan";', 'return "macOS SDL3 host";')
        text = replace_once(text, '(void)w; (void)f; return true; }', '(void)w; return h_fullscreen(f)!=0; }')
        text = replace_once(text, 'bool SDL_SetWindowSize(SDL_Window* w, int a, int b) { (void)w; (void)a; (void)b; return true; }', 'bool SDL_SetWindowSize(SDL_Window* w,int a,int b) { (void)w;return h_window_size(a,b)!=0; }')
        text = replace_once(text, 'r->vsync = v; return true;', 'r->vsync = v; return h_vsync(v)!=0;')
        text = replace_once(text, '    TouchpadHold(now);', '''    TouchpadHold(now);
    { uint32_t k[5]; while (h_key_event(k)) {
        SDL_Event event; memset(&event,0,sizeof(event));
        event.type=k[2]==2?SDL_EVENT_WINDOW_FOCUS_LOST:(k[2]?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP);
        event.key.key=k[0]; event.key.scancode=(int)k[1]; event.key.down=k[2];
        event.key.mod=k[3]; event.key.repeat=k[4]; Push(&event);
    }}''')
        text = replace_once(text,
            'const bool* SDL_GetKeyboardState(int* n) { static bool keys[512]; if (n) *n = 512; return keys; }',
            'const bool* SDL_GetKeyboardState(int* n) { static bool keys[512]; h_keyboard((uint8_t*)keys,512); if(n)*n=512;return keys; }')
        text = replace_once(text,
            'SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode k, SDL_Keymod* m) { (void)k; if (m) *m = 0; return 0; }',
            'SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode k, SDL_Keymod* m) { if(m)*m=0;return h_scancode(k); }')
    dest = out / 'staged' / source.name
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(text)
    return dest


def adapters(out):
    """Generate explicit offset conversions from the shared bridge declarations."""
    prototypes = (HERE / 'guest/include/bridge_calls.h').read_text() + (HERE / 'keyboard.h').read_text()
    scalar = {'void':'void','int':'U32','int32_t':'U32','uint32_t':'U32','uint64_t':'U64','double':'F64','float':'F32'}
    lines = ['/* generated from bridge_calls.h and keyboard.h */']
    for ret, name, params in re.findall(r'^(void|int|uint32_t|uint64_t|double|float) h_(\w+)\(([^)]*)\);', prototypes, re.M):
        args = [] if params.strip() in ('','void') else [p.strip() for p in params.split(',')]
        types=[]; calls=[]
        for j,p in enumerate(args):
            # Original declarations are mostly anonymous; a few give parameter names.
            if '*' in p:
                t = p[:p.index('*')+1].strip()
                types.append('U32')
                spans={('getenv',1):'a2',('fread',1):'a2',('fwrite',1):'a2',('dir_next',1):'a2',
                       ('gfx_size',0):'4',('gfx_size',1):'4',('tex_create',0):'(U64)a1*a2*4',
                       ('tex_update',1):'(a0<4096 ? (U64)sTexW[a0]*sTexH[a0]*4 : 0)',
                       ('gfx_draw',1):'(U64)a2*sizeof(HostVertex)',('audio_push',0):'(U64)a1*4',
                       ('pad_poll',0):'(U64)a1*sizeof(HostPad)',('touch_poll',0):'(U64)a1*sizeof(HostTouch)',
                       ('keyboard',0):'a1',('key_event',0):'20'}
                if t=='const char*': calls.append(f'GuestString(instance,a{j})')
                else: calls.append(f'({t})GuestSpan(instance,a{j},{spans.get((name,j), "1")})')
            else:
                t = next((s for s in scalar if p == s or p.startswith(s+' ')),None)
                if not t: raise RuntimeError('Unknown bridge type: '+p)
                types.append(scalar[t]); calls.append(f'({t})a{j}')
        ident='h_'+name
        encoded=''.join(c if c.isalnum() else 'X%02X'%ord(c) for c in ident)
        signature=','.join(['wasmModuleInstance* instance']+[f'{t} a{j}' for j,t in enumerate(types)])
        target = {'log_e':'log'}.get(name,name)
        math={'sin','cos','tan','atan2','pow','exp','log_e','fmod','sinf','cosf','powf'}
        fn=target if name in math else 'H_'+name
        expression=f'{fn}('+','.join(calls)+')'
        lines.append(f'{scalar[ret]} i3_env{len(ident)}_{encoded}({signature}) {{ (void)instance; '+
                     (expression+';' if ret=='void' else 'return '+expression+';')+' }')
    (out / 'adapters.inc').write_text('\n'.join(lines)+'\n')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--wasi-sdk',type=Path,required=True)
    p.add_argument('--w2c2',type=Path,required=True,help='w2c2 source checkout (CLI at w2c2/w2c2)')
    p.add_argument('--wasm-opt',type=Path,required=True)
    p.add_argument('--sdl',type=Path,default=ROOT/'build/macos/deps/sdl')
    p.add_argument('--jobs',type=int,default=4)
    p.add_argument('--release',action='store_true',help='US build with generated text read from the user-supplied ROM at runtime')
    args=p.parse_args()
    os.chdir(ROOT)
    rom=args.rom.expanduser().resolve()
    digest=hashlib.sha1(rom.read_bytes()).hexdigest()
    known=baserom.versions()
    region=next((r for r,(_,sha) in known.items() if sha==digest),None)
    if not region: sys.exit('Unsupported cartridge SHA-1: '+digest)
    code=known[region][0]
    local=ROOT/'roms'/f'{code}.gba'
    if local.exists() and hashlib.sha1(local.read_bytes()).hexdigest()!=digest:
        sys.exit(f'Refusing to overwrite a different local dump: {local}')
    if not local.exists(): shutil.copyfile(rom,local)
    out=ROOT/'build/macos'/('release/'+region if args.release else region)
    out.mkdir(parents=True,exist_ok=True)
    run([sys.executable,ROOT/'port/tools/gen_pc_assets.py','--version',region])
    sdk=args.wasi_sdk.resolve(); w2=args.w2c2.resolve(); opt=args.wasm_opt.resolve(); sdl=args.sdl.resolve()
    info=subprocess.check_output(['otool','-l',str(sdl/'lib/libSDL3.0.dylib')],text=True)
    minimum=re.search(r'\bminos (\d+(?:\.\d+)*)',info)
    if not minimum or int(minimum[1].split('.')[0])>15:
        sys.exit('SDL3 must be built for macOS 15 or earlier. See port/macos/README.md.')
    cc=sdk/'bin/clang'
    guest_support=HERE
    gen=ROOT/'build'/region/'gen'; glue=ROOT/'build'/region/'pc'
    symbols={}
    for source in [glue/'rom_symbols.s',ROOT/'port/src/port_symbols.s']:
        for name,value in re.findall(r'\.set\s+_(\w+),\s*(0x[0-9a-fA-F]+|\d+)',source.read_text()):
            symbols[name]=int(value,0)
    if len(symbols)<1000: raise RuntimeError('Missing fixed-address ROM symbols')
    release_source = None
    release_samples = {}
    if args.release:
        from release_assets import prepare as prepare_release
        gen, release_source, release_samples = prepare_release(ROOT,out,region,rom.read_bytes(),symbols)
    includes=[ROOT/'include']+sorted(d for d in (ROOT/'include').iterdir() if d.is_dir() and d.name!='gba')+[gen,ROOT/'port/include']
    flags=['--target=wasm32','-ffreestanding','-nostdinc','-isystem',str(sdk/'lib/clang/23/include'),
           '-I'+str(guest_support/'guest/include'),'-I'+str(guest_support/'shared'),'-I'+str(HERE),
           '-I'+str(guest_support/'third_party/stb'),'-I'+str(ROOT/'port/src'),
           '-O2','-funsigned-char','-fno-strict-aliasing','-fwrapv','-fno-common',
           '-fno-stack-protector','-fno-builtin','-DPLATFORM_PC','-DPORT_MACOS','-DVERSION_'+region.upper(),'-w','-include',str(HERE/'guest_sdl.h'),
           '-mno-sign-ext','-mno-bulk-memory','-mno-nontrapping-fptoint']
    resource=subprocess.check_output([str(cc),'-print-resource-dir'],text=True).strip()
    flags[flags.index('-isystem')+1]=resource+'/include'
    for inc in includes: flags+=['-iquote',str(inc)]
    game=sorted((ROOT/'src').rglob('*.c'))
    skip={'port_memory','port_sections','port_state','port_hdtext','port_watchdog','port_biosfault','port_hdmusic'}
    port=[q for q in sorted((ROOT/'port/src').glob('*.c')) if q.stem not in skip]
    port += [guest_support/'guest/src'/f for f in ['libc.c','sdl_shim.c','hdtext_ps5.c','port_memory_ps5.c']]
    port += [glue/'rom_relocs.c',HERE/'guest.c']
    config=repr(flags)+json.dumps(symbols,sort_keys=True)+inspect.getsource(staged)+inspect.getsource(rewrite_ir)
    # Header changes invalidate every object; include generated asset fragments too.
    h=hashlib.sha256(config.encode())
    for folder in [ROOT/'include',ROOT/'port/include',gen,guest_support/'guest/include',guest_support/'shared',HERE]:
        for file in sorted(folder.rglob('*')):
            if file.is_file() and file.suffix in ('.h','.inc'): h.update(file.read_bytes())
    stamp=h.hexdigest()
    def compile_one(item):
        source,is_game=item
        key=source.stem+'-'+hashlib.sha256(str(source).encode()).hexdigest()[:8]
        obj=out/(key+'.o'); fingerprint=hashlib.sha256(source.read_bytes()+stamp.encode()).hexdigest()
        if release_source is not None and source.name == 'card_battle_riku.c':
            fingerprint=hashlib.sha256(source.read_bytes()+release_source.read_bytes()+stamp.encode()).hexdigest()
        marker=out/(key+'.sha256')
        if obj.exists() and marker.exists() and marker.read_text()==fingerprint:return obj
        original=source
        if release_source is not None and source.name == 'card_battle_riku.c':source=release_source
        elif source.name in ('libc.c','sdl_shim.c','port_main.c','port_config.c','port_bios.c','rom_relocs.c'):source=staged(source,out)
        extra=['-std=gnu89' if is_game else '-std=gnu11']
        ir=out/(key+'.ll')
        command=[cc,*flags,*extra,'-S','-emit-llvm',source,'-o',ir]
        result=subprocess.run([str(a) for a in command],capture_output=True,text=True)
        if result.returncode:raise RuntimeError(str(original)+'\n'+result.stderr)
        ir.write_text(rewrite_ir(ir.read_text(),symbols))
        run([cc,'--target=wasm32','-O2','-mno-sign-ext','-mno-bulk-memory','-mno-nontrapping-fptoint','-c',ir,'-o',obj],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        marker.write_text(fingerprint)
        return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        objects=[]
        for j,obj in enumerate(pool.map(compile_one,[(q,True) for q in game]+[(q,False) for q in port]),1):
            objects.append(obj)
            if j%40==0:print('Compiled',j,flush=True)
    run([sdk/'bin/wasm-ld','--no-entry','--export=guest_main','--export=macos_sram_test','--export=macos_input_test','--export-memory','--allow-undefined',
         '--global-base=234946560','--initial-memory=402653184','--max-memory=402653184','-z','stack-size=8388608',
         *objects,'-o',out/'raw.wasm'])
    run([opt,out/'raw.wasm','--fpcast-emu','--pass-arg=fpcast-emu-max-params@16','--disable-reference-types','--disable-multivalue','-o',out/'core.wasm'])
    run([w2/'w2c2/w2c2','-c','-f','100',out/'core.wasm',out/'core.c'])
    adapters(out)
    native_flags=['-arch','arm64','-mmacosx-version-min=15.0','-O2','-DW2C2_RUNTIME_CHECKS=1',
                  '-I'+str(HERE),'-I'+str(w2/'w2c2'),'-I'+str(guest_support/'shared'),'-I'+str(out),'-I'+str(sdl/'include'),
                  '-Wno-deprecated-declarations','-DMACOS_REGION="'+region+'"','-DMACOS_ROM_SHA1="'+digest+'"']
    def native(source):
        obj=source.with_suffix('.arm64.o')
        run(['clang',*native_flags,'-c',source,'-o',obj],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        return obj
    sources=[out/'core.c']+sorted(out.glob('m4_core.*.c'))
    sources += [HERE/'host.c']
    # Host output must stay under build/, too.
    host=out/'host.c';host.write_text((HERE/'host.c').read_text());sources[-1]=host
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool: native_objects=list(pool.map(native,sources))
    # Sign outside cloud-sync folders, which can reattach forbidden Finder xattrs.
    stage=Path(tempfile.mkdtemp(prefix='rekhcom-macos-'))
    app=stage/'ReKHCoM.app'; contents=app/'Contents'; mac=contents/'MacOS';fw=contents/'Frameworks'
    mac.mkdir(parents=True,exist_ok=True);fw.mkdir(exist_ok=True)
    exe=mac/('khcom_'+region)
    launcher_exe=mac/'ReKHCoM'
    run(['clang','-arch','arm64','-mmacosx-version-min=15.0',*native_objects,'-L'+str(sdl/'lib'),'-lSDL3',
         '-Wl,-rpath,@executable_path/../Frameworks','-o',exe])
    from build_launcher import build as build_launcher
    build_launcher(out,launcher_exe,sdl,args.jobs,run)
    library=sdl/'lib/libSDL3.0.dylib';shutil.copyfile(library,fw/library.name)
    linked=subprocess.check_output(['otool','-L',str(exe)],text=True)
    dep=next(line.strip().split(' (')[0] for line in linked.splitlines() if 'libSDL3' in line)
    run(['install_name_tool','-change',dep,'@rpath/'+library.name,exe])
    linked=subprocess.check_output(['otool','-L',str(launcher_exe)],text=True)
    launcher_dep=next(line.strip().split(' (')[0] for line in linked.splitlines() if 'libSDL3' in line)
    run(['install_name_tool','-change',launcher_dep,'@rpath/'+library.name,launcher_exe])
    run(['install_name_tool','-id','@rpath/'+library.name,fw/library.name])
    plist={'CFBundleIdentifier':'org.rekhcom.macos','CFBundleName':'ReKHCoM','CFBundleExecutable':'ReKHCoM',
           'CFBundlePackageType':'APPL','CFBundleVersion':'2','CFBundleShortVersionString':'0.2',
           'LSMinimumSystemVersion':'15.0','NSHighResolutionCapable':True,'LSApplicationCategoryType':'public.app-category.games'}
    assets=contents/'Resources/assets';assets.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'port/assets/fonts',assets/'fonts')
    for name in ['boxart.jpg','logo.png','NOTICE.md']:
        shutil.copyfile(ROOT/'port/decompui/assets'/name,assets/name)
    (assets/'achievements').mkdir()
    shutil.copyfile(ROOT/'port/assets/achievements/badges.png',assets/'achievements/badges.png')
    # Use the upstream icon in macOS's standard icon container.
    iconset=out/'ReKHCoM.iconset';iconset.mkdir(exist_ok=True)
    for size in [16,32,128,256,512]:
        for density in [1,2]:
            name=f'icon_{size}x{size}'+('@2x' if density==2 else '')+'.png'
            run(['sips','-s','format','png','-z',str(size*density),str(size*density),ROOT/'port/decompui/assets/rekhcom.ico','--out',iconset/name],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    # ICNS PNG chunks avoid iconutil's dependence on unavailable desktop services in build sandboxes.
    chunks=[]
    for kind,name in [('icp4','icon_16x16.png'),('icp5','icon_32x32.png'),('ic07','icon_128x128.png'),
                      ('ic08','icon_256x256.png'),('ic09','icon_512x512.png'),('ic10','icon_512x512@2x.png')]:
        png=(iconset/name).read_bytes();chunks.append(kind.encode()+struct.pack('>I',len(png)+8)+png)
    body=b''.join(chunks)
    (contents/'Resources/ReKHCoM.icns').write_bytes(b'icns'+struct.pack('>I',len(body)+8)+body)
    plist['CFBundleIconFile']='ReKHCoM.icns'
    (contents/'Info.plist').write_bytes(plistlib.dumps(plist))
    notices=contents/'Resources'/'Licenses';notices.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'LICENSE.md',notices/'ReKHCoM-CC0.txt')
    shutil.copyfile(w2/'LICENSE',notices/'w2c2-MIT.txt')
    shutil.copyfile(ROOT/'port/decompui/third_party/imgui/LICENSE.txt',notices/'Dear-ImGui-MIT.txt')
    sdl_license=list(sdl.glob('share/licenses/*/*'))
    stb=(guest_support/'third_party/stb/stb_truetype.h').read_text()
    (notices/'stb-LICENSE.txt').write_text(stb[stb.rfind('ALTERNATIVE A - MIT License'):].rstrip('*/\n'))
    if not sdl_license: raise RuntimeError('SDL3 license missing from dependency prefix')
    for file in sdl_license:
        if file.is_file():shutil.copyfile(file,notices/('SDL3-'+file.name))
    for binary in [exe,launcher_exe,fw/library.name]:verify_portable_binary(binary)
    release_audit = None
    if args.release:
        from release_assets import audit
        release_audit = audit(exe,release_samples)
        release_audit['rom_files_in_bundle'] = [str(f.relative_to(app)) for f in app.rglob('*') if f.is_file() and f.suffix.lower() in ('.gba','.sav','.state')]
        if release_audit['rom_files_in_bundle']:
            raise RuntimeError('Private game data found in release bundle')
        (out/'release-audit.json').write_text(json.dumps(release_audit,indent=2)+'\n')
    for file in [fw/library.name,exe,app]:run(['codesign','--force','--sign','-',file])
    run(['codesign','--verify','--deep','--strict',app])
    archive=out/f'ReKHCoM-macos-arm64-{region}.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for file in sorted(app.rglob('*')):
            if file.is_file(): z.write(file,file.relative_to(stage))
    (out/'app-path.txt').write_text(str(app)+'\n')
    (out/'native-build.json').write_text(json.dumps({'flags':native_flags,'objects':[str(o) for o in native_objects if o.name!='host.arm64.o'],'sdl':str(sdl),'app':str(app),'game_executable':str(exe)},indent=2))
    (out/'SHA256SUMS').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
    print('Local build:',app,flush=True)
    print('Install archive:',archive,flush=True)
    print('Upstream launcher artwork, badges and optional fonts restored with notices. No ROM is bundled.')
    if args.release:
        print('Release build: generated text is read from the user-supplied ROM; payload audit passed.')
    else:
        print('Generated game text may be present in the executable. This is a private local build, not a release package.')

if __name__=='__main__':main()
