/* Browser counterpart of decompui/save_editor.h. Operates on copies only. */
(function(root){
  'use strict';
  const DATA=0x1c, PROGRESS=4, COUNT=999, EMPTY=0xfff;
  const u16=(b,p)=>b[p]|b[p+1]<<8;
  const u32=(b,p)=>(u16(b,p)|(u16(b,p+2)<<16))>>>0;
  const put16=(b,p,v)=>{b[p]=v;b[p+1]=v>>>8;};
  const put32=(b,p,v)=>{put16(b,p,v);put16(b,p+2,v>>>16);};
  const clamp=(v,a,z)=>Math.max(a,Math.min(z,Math.trunc(Number(v)||0)));
  function checksum(b){let sum=0;for(let i=0;i+1<b.length;i+=2)sum+=u16(b,i);if(b.length&1)sum+=b[b.length-1];sum=(sum&65535)+(sum>>>16);return ~(sum+(sum>>>16))&65535;}
  function valid(b){return new TextDecoder().decode(b.subarray(0,22))==='KHCOM_BACKUP_VER000000'&&checksum(b)===0;}
  function seal(b){put16(b,24,0);put16(b,24,checksum(b));}
  const size=file=>file<2?0xf14:0x418;
  const at=(file,copy)=>file<2?0x2f20+file*2*0xf14+copy*0xf14:0x1ec0+(file-2)*2*0x418+copy*0x418;
  class SaveFile{
    constructor(bytes,file,block){this.source=bytes.slice();this.file=file;this.block=block.slice();this.original=block.slice();}
    get riku(){return this.file>=2;}
    get data(){return this.block.subarray(DATA);}
    get level(){return this.data[PROGRESS+0x10];}
    get hp(){return u16(this.data,PROGRESS);}
    get cp(){return u16(this.data,PROGRESS+2);}
    get ap(){return u16(this.data,PROGRESS+6);}
    get dp(){return u16(this.data,PROGRESS+4);}
    get moogle(){return u32(this.data,PROGRESS+0x7c);}
    set level(v){v=clamp(v,1,99);let next=25,reached=0;for(let l=1;l<v;l++){reached=next;next+=(l+1)*(l+1)*3;}this.data[PROGRESS+0x10]=v;const exp=u32(this.data,PROGRESS+8);if(exp<reached||exp>=next)put32(this.data,PROGRESS+8,reached);put32(this.data,PROGRESS+12,next);}
    set hp(v){put16(this.data,PROGRESS,clamp(v,1,560));}
    set cp(v){put16(this.data,PROGRESS+2,clamp(v,0,9999));}
    set ap(v){put16(this.data,PROGRESS+6,clamp(v,0,999));}
    set dp(v){put16(this.data,PROGRESS+4,clamp(v,0,999));}
    set moogle(v){put32(this.data,PROGRESS+0x7c,clamp(v,0,9999999));}
    getMap(id){if(id<0||id>=270)throw Error('Invalid map card');return this.data[(this.riku?0x2ec:0x2ed)+id];}
    setMap(id,v){if(id<0||id>=270)throw Error('Invalid map card');this.data[(this.riku?0x2ec:0x2ed)+id]=clamp(v,0,9);}
    card(i){return u16(this.data,0x3fc+i*2);}
    count(id){if(this.riku)return 0;let n=0;for(let i=0;i<COUNT;i++){const c=this.card(i);if(c!==EMPTY&&(c&0xfff)===id)n++;}return n;}
    get total(){if(this.riku)return 0;let n=0;for(let i=0;i<COUNT;i++)if(this.card(i)!==EMPTY)n++;return n;}
    add(id,copies,premium=false){if(this.riku||id<0||id>=950)throw Error('Invalid battle card');copies=clamp(copies,0,99);let n=0,owned=this.count(id);if(u16(this.data,0xbca)<COUNT)put16(this.data,0xbca,COUNT);for(let i=0;i<COUNT&&n<copies&&owned+n<99;i++)if(this.card(i)===EMPTY){put16(this.data,0x3fc+i*2,id|(premium?0x8000:0));n++;}return n;}
    remove(id,copies){if(this.riku)return 0;let n=0;copies=clamp(copies,0,99);for(let i=COUNT-1;i>=0&&n<copies;i--){const c=this.card(i);if(c===EMPTY||(c&0xfff)!==id||(c&0x7000))continue;put16(this.data,0x3fc+i*2,EMPTY);n++;}return n;}
    export(){const bytes=this.source.slice(),b=this.block.slice();seal(b);for(let copy=0;copy<2;copy++)bytes.set(b,at(this.file,copy));for(let copy=0;copy<2;copy++){const h=bytes.subarray(0x10+copy*0x40,0x50+copy*0x40);if(!valid(h))continue;const p=DATA+4+this.file*8;h[p]=this.data[0x90];h[p+1]=this.data[0x91];h[p+2]=this.level;put32(h,p+4,u32(this.data,0x94));seal(h);}return bytes;}
  }
  function read(bytes,file){if(bytes.length!==65536||file<0||file>3)return null;for(let copy=0;copy<2;copy++){const block=bytes.subarray(at(file,copy),at(file,copy)+size(file));if(valid(block))return new SaveFile(bytes,file,block);}return null;}
  const api={read,valid,checksum,seal,at,size};
  if(typeof module==='object'&&module.exports)module.exports=api;else root.ReKHSave=api;
})(typeof globalThis!=='undefined'?globalThis:this);
