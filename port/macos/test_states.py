#!/usr/bin/env python3
"""Save/load regression with isolated synthetic SRAM and real F5/F9 events.

No installed application or live player folder is used. Requires an existing
--experimental-states build and the user's locally supplied matching ROM.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import struct
import tempfile
import time

ROOT=Path(__file__).resolve().parents[2]


def different_identity(original,offset):
    # Native arm64 StateHeader v1 prefix, through the volatile-region list.
    # Repair its checksum so identity validation is tested independently of
    # damaged-file detection. Fail if the format changed under this test.
    prefix=struct.calcsize('@8s65s41s4I2i2I2Q32s32s16I')
    assert original[prefix:prefix+32]==hashlib.sha256(original[:prefix]).digest()
    changed=bytearray(original);changed[offset]^=1
    changed[prefix:prefix+32]=hashlib.sha256(changed[:prefix]).digest()
    return changed


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom',type=Path,required=True)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/macos/states-release/us')
    args=parser.parse_args()
    meta=json.loads((args.build_dir/'native-build.json').read_text())
    exe=Path(meta['game_executable'])
    output=ROOT/'build/macos/tests'
    output.mkdir(parents=True,exist_ok=True)
    run=Path(tempfile.mkdtemp(prefix='states-',dir=output))
    print('Evidence:',run,flush=True)
    env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',KHCOM_FAST='1',KHCOM_QUIT='650',KHCOM_MENU_FRAMECLOCK='1')
    scenes={
        'title':{'KHCOM_MODE':'30:mode_title'},
        'battle':{'KHCOM_BATTLE':'1:0','KHCOM_KEYS':','.join(f'{f}-{f+3}:A' for f in range(60,650,45))},
        'event':{'KHCOM_EVENT':'0:0','KHCOM_KEYS':','.join(f'{f}-{f+3}:A' for f in range(30,650,40))},
        'deck':{'KHCOM_BATTLE':'1:0','KHCOM_KEYS':'60-63:A','KHCOM_MODE':'180:Mode_Deck'},
    }
    reports=[]

    def execute(folder,extra,label):
        folder.mkdir(parents=True,exist_ok=True)
        (folder/'states').mkdir(exist_ok=True)
        save=folder/'kh_com_us.sav'
        if not save.exists():save.write_bytes(b'\xff'*65536)
        before=hashlib.sha256(save.read_bytes()).hexdigest()
        started=time.monotonic()
        result=subprocess.run([str(exe),'--rom',str(args.rom.resolve()),'--data-dir',str(folder)],
            cwd=folder,env={**env,**extra},capture_output=True,text=True,timeout=90)
        text=result.stdout+result.stderr
        (folder/(label+'.log')).write_text(text)
        if result.returncode:raise RuntimeError(f'{label}: exit {result.returncode}: {text[-2000:]}')
        assert 'game trap' not in text,text[-2000:]
        assert hashlib.sha256(save.read_bytes()).hexdigest()==before,f'{label}: SRAM file changed'
        return text,round(time.monotonic()-started,3)

    for scene,settings in scenes.items():
        base=run/scene/'baseline'; restored=run/scene/'restore'
        picture=base/'frame.bmp'
        _,seconds=execute(base,{**settings,'KHCOM_HOST_SHOT':str(picture),'KHCOM_HOST_SHOT_FRAME':'540'},'baseline')
        image=restored/'frame.bmp'
        text,elapsed=execute(restored,{**settings,'KHCOM_STATE_KEYS':'120:F5,220:F9',
            'KHCOM_HOST_SHOT':str(image),'KHCOM_HOST_SHOT_FRAME':'640'},'roundtrip')
        assert 'state: save ' in text and '-> ok' in text,text[-2000:]
        assert 'state: restored;' in text,text[-2000:]
        saved=int(re.search(r'state: save .*host frame (\d+)',text)[1])
        loaded=int(re.search(r'state: load validated .*host frame (\d+)',text)[1])
        assert loaded-saved==100,(saved,loaded)
        equal=picture.read_bytes()==image.read_bytes()
        reports.append({'scene':scene,'test':'same-process','framebuffer_identical':equal,
            'synthetic_sram_unchanged':True,'seconds':elapsed})
        print(reports[-1],flush=True)
        if not equal:raise AssertionError(f'{scene}: different picture after save/load')

        fresh=restored/'fresh.bmp'
        text,elapsed=execute(restored,{**settings,'KHCOM_LOADSTATE':'states/slot1.macstate',
            'KHCOM_HOST_SHOT':str(fresh),'KHCOM_HOST_SHOT_FRAME':str(540-saved)},'fresh-process')
        assert 'state: restored;' in text,text[-2000:]
        equal=picture.read_bytes()==fresh.read_bytes()
        reports.append({'scene':scene,'test':'fresh-process','framebuffer_identical':equal,
            'synthetic_sram_unchanged':True,'seconds':elapsed})
        print(reports[-1],flush=True)
        if not equal:raise AssertionError(f'{scene}: different picture after restarting and loading')

    # Reloading must work repeatedly, and bad files must leave gameplay intact.
    folder=run/'title'/'restore'
    picture=run/'title'/'baseline'/'frame.bmp'
    repeated=folder/'repeated.bmp'
    text,elapsed=execute(folder,{**scenes['title'],'KHCOM_STATE_KEYS':'120:F5,220:F9,320:F9',
        'KHCOM_HOST_SHOT':str(repeated),'KHCOM_HOST_SHOT_FRAME':'740'},'repeated')
    assert text.count('state: restored;')==2,text[-2000:]
    assert picture.read_bytes()==repeated.read_bytes(),'Repeated loads changed gameplay'
    reports.append({'test':'repeated-loads','framebuffer_identical':True,'synthetic_sram_unchanged':True})
    print(reports[-1],flush=True)
    original=(folder/'states/slot1.macstate').read_bytes()
    bad_build=different_identity(original,8)
    bad_rom=different_identity(original,8+65)
    bad_payload=bytearray(original);bad_payload[-1]^=1
    invalid={'missing':None,'truncated':original[:100],'wrong-build':bad_build,'wrong-rom':bad_rom,'corrupt-payload':bad_payload}
    for label,data in invalid.items():
        path=folder/'states'/(label+'.macstate')
        if data is not None:path.write_bytes(data)
        shot=folder/(label+'.bmp')
        text,_=execute(folder,{**scenes['title'],'KHCOM_LOADSTATE':'states/'+path.name,
            'KHCOM_HOST_SHOT':str(shot),'KHCOM_HOST_SHOT_FRAME':'540'},label)
        assert 'state: rejected ' in text and 'state: restored;' not in text,text[-2000:]
        assert picture.read_bytes()==shot.read_bytes(),label+': rejection altered gameplay'
        reports.append({'test':label,'rejected_without_changing_gameplay':True,'synthetic_sram_unchanged':True})
        print(reports[-1],flush=True)

    (run/'report.json').write_text(json.dumps(reports,indent=2)+'\n')
    print('Evidence:',run)


if __name__=='__main__':main()
