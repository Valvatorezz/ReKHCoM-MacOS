/* Byte-for-byte parity with the launcher's real C++ save editor. */
const assert=require('node:assert/strict'),fs=require('node:fs');
const Ed=require('../web/save-editor.js');
const [input,golden,slot]=process.argv.slice(2);
const bytes=new Uint8Array(fs.readFileSync(input)),file=Number(slot),save=Ed.read(bytes,file);
assert(save,'Valid fixture');
save.level=50;save.hp=400;save.moogle=12345;save.setMap(0,5);
if(!save.riku){save.cp=1500;save.add(9,3);save.add(459,1);save.remove(9,1);}
const edited=save.export();assert.deepEqual(Buffer.from(edited),fs.readFileSync(golden));
assert.deepEqual(Buffer.from(bytes),fs.readFileSync(input),'Original input untouched');
for(let copy=0;copy<2;copy++)assert(Ed.valid(edited.subarray(Ed.at(file,copy),Ed.at(file,copy)+Ed.size(file))));
const corrupt=bytes.slice();corrupt[Ed.at(file,0)+30]^=1;assert(Ed.read(corrupt,file),'Second copy fallback');corrupt[Ed.at(file,1)+30]^=1;assert.equal(Ed.read(corrupt,file),null);
if(!save.riku){
  assert.equal(save.add(9,999),96,'99 per ID includes the protected in-deck card');
  assert.equal(save.remove(9,999),98,'Never remove the in-deck copy');
  for(let id=0;id<15;id++)save.add(id,99);
  assert.equal(save.total,999);assert.equal(save.add(20,1),0,'999 overall');
}
assert.equal(Ed.read(new Uint8Array(32),file),null);assert.equal(Ed.read(bytes,4),null);
console.log('PASS native editor parity, copies, limits, deck protection: slot '+file);
