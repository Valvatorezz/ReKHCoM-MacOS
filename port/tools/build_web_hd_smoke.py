"""Build an isolated ROM-free font/SDL browser probe; never publish it as a game."""
from pathlib import Path
import subprocess
import shutil
import argparse

R=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--emsdk',type=Path,required=True)
parser.add_argument('--sdl-source',type=Path,required=True)
parser.add_argument('--sdl-build',type=Path,required=True)
args=parser.parse_args()
sdk=args.emsdk.resolve()
out=R/'build/web-hd-smoke';out.mkdir(exist_ok=True)
sdl=args.sdl_source.resolve()
subprocess.run([str(sdk/'upstream/emscripten/emcc.exe'),str(R/'port/tools/web_hd_smoke.c'),
    str(R/'port/web/native/hdtext_web.c'),'-I'+str(R/'port/include'),'-I'+str(sdl/'include'),
    str(args.sdl_build.resolve()/'libSDL3.a'),'-O2','-sASYNCIFY=1','-sMODULARIZE=1','-sEXPORT_NAME=createSmoke',
    '-sEXPORTED_FUNCTIONS=["_main","_SmokeResize","_PortWebSetHdReady"]',
    '-sEXPORTED_RUNTIME_METHODS=["HEAPU8","callMain"]','-o',str(out/'smoke.js')],check=True)
shutil.copy2(R/'port/web/runtime.js',out/'runtime.js')
shutil.copytree(R/'port/assets/fonts',out/'fonts',dirs_exist_ok=True)
(out/'index.html').write_text('''<!doctype html><html><meta charset="utf-8"><title>ROM-free HD font probe</title>
<style>body{background:#0b101a;color:white;font:16px system-ui}canvas{width:960px;max-width:100%;aspect-ratio:16/9}canvas:fullscreen{width:100vw;height:100vh}</style>
<h1>ROM-free HD font probe</h1><button id="start">Draw HD fonts</button><button id="full">Fullscreen</button><p id="stats">Not started</p><canvas id="canvas" width="960" height="540"></canvas>
<script src="runtime.js"></script><script src="smoke.js"></script><script>
let core;const canvas=document.getElementById('canvas');
document.getElementById('start').onclick=async()=>{core=await createSmoke({canvas,noInitialRun:true});await ReKHWeb.fonts(core);core._PortWebSetHdReady(1);core.callMain([]);};
document.getElementById('full').onclick=()=>canvas.requestFullscreen();
function resize(){if(!core)return;const full=document.fullscreenElement===canvas,w=full?innerWidth:canvas.getBoundingClientRect().width;const size=ReKHWeb.fitSize(w,full?innerHeight:w*9/16);core._SmokeResize(size.width,size.height);}
document.addEventListener('fullscreenchange',()=>requestAnimationFrame(resize));window.addEventListener('resize',()=>requestAnimationFrame(resize));
</script></html>''',encoding='utf-8')
print(out)
