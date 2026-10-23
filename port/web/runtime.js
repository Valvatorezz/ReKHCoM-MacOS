/* Pure browser services, also exercised by the Node regression suite. */
(function(root){
  function fitSize(width,height){
    width=Math.max(1,width);height=Math.max(1,height);
    const factor=Math.min(1,1920/width,1080/height);
    return {width:Math.max(240,Math.round(width*factor)),height:Math.max(160,Math.round(height*factor))};
  }
  function saveName(base,bank){return bank>1?base.replace(/\.sav$/,`_bank${bank}.sav`):base;}
  function persistence(sync,onFailure){
    let timer, running=null, requested=false, enabled=true;
    async function flush(){
      clearTimeout(timer);timer=null;requested=true;
      if(!enabled)return false;
      if(running)return running;
      running=(async()=>{
        try{while(requested){requested=false;await sync(false);}return true;}
        catch(error){enabled=false;onFailure(error);return false;}
        finally{running=null;}
      })();
      return running;
    }
    return {request(){if(enabled){clearTimeout(timer);timer=setTimeout(flush,250);}},flush,
      disable(){enabled=false;clearTimeout(timer);timer=null;},get enabled(){return enabled;},
      get pending(){return enabled&&(requested||running!==null||timer!=null);}};
  }
  async function fonts(module){
    const fontUrl=name=>`url("fonts/${name}?v=${module.webFiles?.['fonts/'+name]?.sha256||'alpha2'}")`;
    const faces=[new FontFace('ReKHMenu',fontUrl('KHMenu.otf')),new FontFace('ReKHSubtitle',fontUrl('ComicHearts.otf'))];
    await Promise.all(faces.map(async face=>document.fonts.add(await face.load())));
    const canvas=document.createElement('canvas'),ctx=canvas.getContext('2d',{willReadFrequently:true});
    const latin=new TextDecoder('windows-1252'),japanese=new TextDecoder('shift_jis');
    const sample='abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    const sizes=new Map();
    module.rasterGlyph=(style,cap,gameWidth,code,jp,pixels,metrics)=>{
      try{
        const key=`${style}:${cap}:${jp}`,family=jp?'"Yu Gothic UI","Noto Sans JP",sans-serif':style===1?'ReKHSubtitle,sans-serif':'ReKHMenu,sans-serif';
        let config=sizes.get(key);
        if(!config){
          ctx.font=`100px ${family}`;
          let px=Math.max(1,Math.round(cap*100/(ctx.measureText('H').actualBoundingBoxAscent||70)));
          if(jp)px=Math.max(1,Math.round(cap*11/7));
          ctx.font=`${px}px ${family}`;
          const natural=ctx.measureText(sample).width;
          config={font:ctx.font,condense:!jp&&natural>gameWidth*1.02?gameWidth/natural:1};
          if(sizes.size>=6)sizes.delete(sizes.keys().next().value);sizes.set(key,config);
        }
        ctx.font=config.font;ctx.textBaseline='alphabetic';
        const char=jp?japanese.decode(new Uint8Array([code>>8,code&255])):latin.decode(new Uint8Array([code]));
        const measure=ctx.measureText(char),sx=config.condense;
        const left=-Math.ceil(measure.actualBoundingBoxLeft*sx),top=Math.ceil(measure.actualBoundingBoxAscent);
        const width=Math.max(0,Math.ceil((measure.actualBoundingBoxLeft+measure.actualBoundingBoxRight)*sx));
        const height=Math.max(0,top+Math.ceil(measure.actualBoundingBoxDescent));
        if(width>512||height>512)return 0;
        new Int32Array(module.HEAPU8.buffer,metrics,5).set([width,height,Math.round(measure.width*sx),left,top]);
        if(width&&height){
          canvas.width=width;canvas.height=height;ctx.font=config.font;ctx.textBaseline='alphabetic';ctx.fillStyle='#fff';
          ctx.setTransform(sx,0,0,1,-left,top);ctx.fillText(char,0,0);
          const rgba=ctx.getImageData(0,0,width,height).data,heap=module.HEAPU8;
          for(let i=0;i<width*height;i++)heap[pixels+i]=rgba[i*4+3];
        }
        return 1;
      }catch(error){console.error('HD glyph',error);return 0;}
    };
  }
  // SDL's canonical button/axis encoding, shared with the native controller art.
  function controllerBinding(binding){
    if(!Number.isInteger(binding)||binding<0)return {region:-1,directions:0};
    if(binding<100)return {region:binding<=14?binding:-1,directions:binding<=14?15:0};
    const axis=Math.floor((binding-100)/2),negative=(binding-100)%2;
    if(axis>=6)return {region:-1,directions:0};
    if(axis>=4)return {region:15+axis-4,directions:15};
    return {region:axis<2?7:8,directions:axis%2?(negative?1:2):(negative?4:8)};
  }
  const keyboardCodes=new Map([
    ['Enter',[13,'Enter']],['Backspace',[8,'Backspace']],['Tab',[9,'Tab']],['Space',[32,'Space']],
    ['Escape',[27,'Escape']],['Delete',[127,'Delete']],['Insert',[0x40000049,'Insert']],
    ['Home',[0x4000004a,'Home']],['PageUp',[0x4000004b,'Page Up']],['End',[0x4000004d,'End']],['PageDown',[0x4000004e,'Page Down']],
    ['ArrowRight',[0x4000004f,'→']],['ArrowLeft',[0x40000050,'←']],['ArrowDown',[0x40000051,'↓']],['ArrowUp',[0x40000052,'↑']]
  ]);
  ['ControlLeft','ShiftLeft','AltLeft','MetaLeft','ControlRight','ShiftRight','AltRight','MetaRight'].forEach((code,n)=>keyboardCodes.set(code,[0x400000e0+n,code.replace('Control','Ctrl ').replace('Shift','Shift ').replace('Alt','Alt ').replace('Meta','Meta ')]));
  for(let n=1;n<=24;n++)keyboardCodes.set('F'+n,[(n<=12?0x4000003a+n-1:0x40000068+n-13),'F'+n]);
  for(let n=1;n<=9;n++)keyboardCodes.set('Numpad'+n,[0x40000059+n-1,'Numpad '+n]);
  keyboardCodes.set('Numpad0',[0x40000062,'Numpad 0']);keyboardCodes.set('NumpadDecimal',[0x40000063,'Numpad .']);
  ['Divide','Multiply','Subtract','Add','Enter'].forEach((code,n)=>keyboardCodes.set('Numpad'+code,[0x40000054+n,'Numpad '+['/','*','-','+','Enter'][n]]));
  function keyboardBinding(event){
    const code=keyboardCodes.get(event.code)||keyboardCodes.get(event.key)||keyboardCodes.get({Shift:'ShiftLeft',Control:'ControlLeft',Alt:'AltLeft',Meta:'MetaLeft',' ':'Space'}[event.key]);
    if(code)return code[0];
    return typeof event.key==='string'&&event.key.length===1?event.key.toLowerCase().codePointAt(0):null;
  }
  function keyboardName(value){
    if(!value)return '—';
    for(const [code,name]of keyboardCodes.values())if(code===value)return name;
    return value>=32&&value<=0x10ffff?(value<127?String.fromCodePoint(value).toUpperCase():String.fromCodePoint(value)):null;
  }
  const api={fitSize,saveName,persistence,fonts,controllerBinding,keyboardBinding,keyboardName};root.ReKHWeb=api;
  if(typeof module!=='undefined'&&module.exports)module.exports=api;
})(typeof globalThis!=='undefined'?globalThis:this);
