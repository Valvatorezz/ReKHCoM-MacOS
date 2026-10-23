#!/usr/bin/env python3
"""Run isolated bridge and gameplay tests of an existing macOS build.

Dummy video/audio are intentional here. This verifies output and audio queues,
not hardware sound quality, Bluetooth controllers, or display pacing.
"""
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--region',choices=['eu','us','jp'],default='us')
    p.add_argument('--build-dir',type=Path,help='Existing build output directory, e.g. build/macos/release/us')
    a=p.parse_args()
    build=a.build_dir.resolve() if a.build_dir else ROOT/'build/macos'/a.region
    meta=json.loads((build/'native-build.json').read_text())
    test_root=ROOT/'build/macos/tests';test_root.mkdir(parents=True,exist_ok=True)
    out=Path(tempfile.mkdtemp(prefix='run-',dir=test_root))
    exe=out/'integration-test'
    command=['clang',*meta['flags'],'-I'+str(HERE),str(HERE/'integration_test.c'),*meta['objects'],
             '-L'+meta['sdl']+'/lib','-lSDL3','-Wl,-rpath,'+meta['sdl']+'/lib','-o',str(exe)]
    subprocess.run(command,check=True)
    env={k:v for k,v in os.environ.items() if not k.startswith(('SDL_','KHCOM_'))}
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    integration=out/'integration';integration.mkdir()
    result=subprocess.run([str(exe)],cwd=integration,env=env,capture_output=True,text=True,timeout=30)
    (integration/'run.log').write_text(result.stdout+result.stderr)
    print(result.stdout,flush=True)
    if result.returncode:raise RuntimeError('Integration test failed: '+result.stderr)
    scenes={
        'title':{'KHCOM_MODE':'30:mode_title'},
        'deck':{'KHCOM_BATTLE':'1:0','KHCOM_KEYS':'60-63:A','KHCOM_MODE':'180:Mode_Deck'},
        'battle':{'KHCOM_BATTLE':'1:0','KHCOM_KEYS':','.join(f'{f}-{f+3}:A' for f in range(60,900,45))},
        'hd-dialogue':{'TEST_HD':'1','KHCOM_EVENT':'0:0','KHCOM_KEYS':','.join(f'{f}-{f+3}:A' for f in range(30,900,40))},
        'event':{'KHCOM_EVENT':'0:0','KHCOM_KEYS':','.join(f'{f}-{f+3}:A' for f in range(30,900,40))},
    }
    def run_scene(item):
        name,extra=item;extra=dict(extra);folder=out/name;folder.mkdir()
        hd=extra.pop('TEST_HD',None)
        if hd:(folder/'kh_com.ini').write_text('hd_text=1\nview_mode=0\naspect=6\n')
        child={**env,'KHCOM_FAST':'1','KHCOM_QUIT':'905','KHCOM_HOST_SHOT':str(folder/'frame.bmp'),
               'KHCOM_HOST_SHOT_FRAME':'900',**extra}
        with (folder/'run.log').open('w') as log:
            r=subprocess.run([meta.get('game_executable',str(Path(meta['app'])/'Contents/MacOS/ReKHCoM')),'--rom',str(a.rom.resolve()),
                              '--data-dir',str(folder)],env=child,cwd=folder,stdout=log,stderr=log,timeout=90)
        log=(folder/'run.log').read_text()
        ok=r.returncode==0 and (folder/'frame.bmp').is_file() and 'harness: no mode' not in log and 'game trap' not in log
        if hd:
            game_log=(folder/'kh_com.log').read_text()
            ok=ok and 'HD text: font ' in game_log and 'unable to load' not in game_log
        return {'scene':name,'passed':ok,'exit':r.returncode}
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(run_scene,scenes.items()))
    (out/'report.json').write_text(json.dumps(results,indent=2))
    for item in results:print(item)
    print('Evidence:',out)
    if not all(item['passed'] for item in results):raise SystemExit(1)

if __name__=='__main__':main()
