"""ROM-free browser/launcher save editor parity, all four slots and regions."""
from pathlib import Path
import os,struct,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'build/web55-editor-test'
OUT.mkdir(parents=True,exist_ok=True)
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang++.exe'))
exe=OUT/'native.exe'
subprocess.run([str(cc),'-O1','-std=c++17','-static','-o',str(exe),str(ROOT/'port/tools/save_editor_probe.cpp')],check=True)
def seal(b):
    struct.pack_into('<H',b,24,0)
    total=sum(struct.unpack('<'+'H'*(len(b)//2),b));total=(total&65535)+(total>>16)
    struct.pack_into('<H',b,24,~(total+(total>>16))&65535)
def at(f,c):return 0x2f20+f*2*0xf14+c*0xf14 if f<2 else 0x1ec0+(f-2)*2*0x418+c*0x418
for region in ['eu','us','jp']:
    sram=bytearray([255])*65536
    signature=b'KHCOM_BACKUP_VER000000'+(b'13' if region=='eu' else b'12')
    for f in range(4):
        size=0xf14 if f<2 else 0x418;b=bytearray(size);b[:24]=signature
        b[28+4+16]=1;struct.pack_into('<I',b,28+4+8,20)
        if f<2:
            for i in range(999):struct.pack_into('<H',b,28+0x3fc+i*2,0xfff)
            struct.pack_into('<H',b,28+0x3fc,0x1009);struct.pack_into('<H',b,28+0xbca,999)
        seal(b)
        for c in range(2):sram[at(f,c):at(f,c)+size]=b
    for c in range(2):
        b=bytearray(64);b[:24]=signature;seal(b);sram[0x10+c*64:0x50+c*64]=b
    source=OUT/f'{region}.sav';source.write_bytes(sram)
    for f in range(4):
        golden=OUT/f'{region}-{f}.sav'
        args=['level','50','hp','400','moogle','12345','map','0','5']
        if f<2:args+=['cp','1500','add','9','3','add','459','1','remove','9','1']
        subprocess.run([str(exe),str(source),str(golden),str(f),*args],check=True,stdout=subprocess.DEVNULL)
        subprocess.run(['node',str(ROOT/'port/tools/test_web_save_editor.cjs'),str(source),str(golden),str(f)],check=True)
print('PASS 12 browser/native save editor comparisons; no ROM or player saves used')
