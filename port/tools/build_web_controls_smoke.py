"""ROM-free browser input probe, isolated from the playable web output."""
import argparse
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--emsdk',type=Path,required=True)
p.add_argument('--sdl-source',type=Path,required=True)
p.add_argument('--sdl-build',type=Path,required=True)
a=p.parse_args()
out=ROOT/'build/web-controls-smoke';out.mkdir(exist_ok=True)
subprocess.run([str(a.emsdk.resolve()/'upstream/emscripten/emcc.exe'),
    str(ROOT/'port/tools/web_controls_smoke.c'),'-I'+str(ROOT/'port/include'),
    '-I'+str(a.sdl_source.resolve()/'include'),str(a.sdl_build.resolve()/'libSDL3.a'),
    '-O2','-sASYNCIFY=1','-sMODULARIZE=1','-sEXPORT_NAME=createControls',
    '-sEXPORTED_FUNCTIONS=["_main","_SmokeClear"]','-sEXPORTED_RUNTIME_METHODS=["callMain"]',
    '-o',str(out/'controls.js')],check=True)
(out/'index.html').write_text('''<!doctype html><meta charset="utf-8"><title>ROM-free controls probe</title>
<style>body{font:16px system-ui;background:#0b101a;color:white}canvas{width:960px;max-width:100%;aspect-ratio:16/9;touch-action:none;border:1px solid #345}button,input{padding:12px}</style>
<h1>ROM-free controls probe</h1><p>Actual SDL input backend. This is a test fixture, not the game.</p>
<button id="start">Start controls probe</button><input aria-label="Outside-game typing" placeholder="Typing here must not control the canvas">
<pre id="stats">Not started</pre><canvas id="canvas" tabindex="0" width="960" height="540"></canvas>
<script src="controls.js"></script><script>
let core;const canvas=document.getElementById('canvas');
document.getElementById('start').onclick=async()=>{core=await createControls({canvas,noInitialRun:true});canvas.focus();core.callMain([]);};
canvas.addEventListener('blur',()=>core?._SmokeClear());
canvas.addEventListener('pointercancel',()=>core?._SmokeClear());
canvas.addEventListener('pointerdown',()=>canvas.focus());
canvas.addEventListener('keydown',e=>{if(e.key.startsWith('Arrow')||e.key==='Backspace')e.preventDefault();});
</script>''',encoding='utf8')
print(out)
