"""ROM-free production C control regressions, using the pinned desktop SDL.

This does not simulate Safari/Android hardware or the browser's Gamepad API.
"""
from pathlib import Path
import subprocess
import shutil

ROOT = Path(__file__).resolve().parents[2]
cc = next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
sdl = ROOT.parent/'toolchain/SDL3-3.4.18/i686-w64-mingw32'
out = ROOT/'build/web-controls-test'
out.mkdir(exist_ok=True)
shutil.copy2(sdl/'bin/SDL3.dll', out/'SDL3.dll')
for name in ['web_touch_conformance', 'input_controls_conformance']:
    exe = out/(name+'.exe')
    subprocess.run([str(cc), '-O2', '-std=gnu11', '-I'+str(ROOT/'port/include'),
                    '-I'+str(sdl/'include'), str(ROOT/'port/tools'/(name+'.c')),
                    '-L'+str(sdl/'lib'), '-lSDL3', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], cwd=out, check=True)
