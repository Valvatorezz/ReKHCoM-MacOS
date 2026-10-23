'use strict';
const $ = id => document.getElementById(id);
const regions = {
  B8CP: {id:'eu', sha:'8db73586cdb11b3795907edebf43228dbcd3e6b2', save:'kh_com.sav'},
  B8CE: {id:'us', sha:'10729bd884f8fdca7a310b6d606c52e46657aa48', save:'kh_com_us.sav'},
  B8CJ: {id:'jp', sha:'59ec0a0a4ccd1e6acb3bbd7bfb21d63988958cfa', save:'kh_com_jp.sav'}
};
let core,region,store,started=false,loading=false,paused=false,resizeFrame=0,pendingSave=false;
let backupFile,backupUrl,expanded=false;
const lines=[];
const warningKeys=new Set();
function log(message){console.log(message);lines.push(String(message));if(lines.length>60)lines.shift();$('log').textContent=lines.join('\n');}
function status(key){$('status').textContent=key?t(key):'';}
function warning(key){warningKeys.add(key);$('storage-status').textContent=Array.from(warningKeys,t).join(' ');$('storage-status').hidden=false;}
function fail(error){log(error.stack||error);$('status').textContent=t('error')+error.message;}
function sync(populate=false){return new Promise((resolve,reject)=>core.FS.syncfs(populate,error=>error?reject(error):resolve()));}
function canvasSize(){
  if($('panel-game')?.hidden)return;
  const canvas=$('canvas'),full=document.fullscreenElement===canvas,rect=canvas.getBoundingClientRect();
  const width=full?innerWidth:rect.width||Math.min(innerWidth-28,1052);
  const size=ReKHWeb.fitSize(width,full?innerHeight:expanded?rect.height:width*9/16);
  if(started)core._PortWebResize(size.width,size.height);
  else{canvas.width=size.width;canvas.height=size.height;}
  $('fullscreen').textContent=$('fullscreen-bar').textContent=t(full||expanded?'exitFullscreen':'fullscreen');
}
function scheduleResize(){
  cancelAnimationFrame(resizeFrame);resizeFrame=requestAnimationFrame(canvasSize);
}
function visibility(){
  if(!started)return;
  core._PortWebSetVisible(!document.hidden&&!paused&&!window.ReKHOptions?.paused?1:0);
  if(document.hidden||paused||window.ReKHOptions?.paused)store.flush();
}
function clearInput(){if(started)core._PortWebClearInput();}
function canShareFile(file){
  try{return !!(navigator.share&&navigator.canShare?.({files:[file]}));}
  catch{return false;}
}
async function flushSave(){
  pendingSave=true;
  try{if(started)core._PortWebFlushSave();return await store.flush();}
  finally{pendingSave=false;}
}
async function loadEngine(){
  const manifest=await fetch('manifest.json',{cache:'no-cache'}).then(response=>{if(!response.ok)throw Error(t('offline'));return response.json();});
  const name=`rekhcom-${region.id}.js`,wasm=`rekhcom-${region.id}.wasm`;
  const jsVersion=manifest.files[name].sha256,wasmVersion=manifest.files[wasm].sha256;
  await new Promise((resolve,reject)=>{
    const script=document.createElement('script');script.src=`${name}?v=${jsVersion}`;
    script.onload=resolve;script.onerror=()=>reject(Error(t('engine')));document.head.append(script);
  });
  canvasSize();
  return createReKHCoM({canvas:$('canvas'),webFiles:manifest.files,noInitialRun:true,print:log,printErr:log,
    locateFile:path=>path.endsWith('.wasm')?`${wasm}?v=${wasmVersion}`:path,
    onAbort:message=>fail(Error(String(message)))});
}
$('rom').addEventListener('change',async()=>{
  if(loading||core)return;
  const file=$('rom').files[0];if(!file)return;
  loading=true;$('rom').disabled=true;
  try{
    if(file.size!==33554432)throw Error(t('romSize'));
    status('checking');let buffer=await file.arrayBuffer();
    const code=new TextDecoder().decode(new Uint8Array(buffer,0xac,4));region=regions[code];
    if(!region)throw Error(t('region'));
    const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-1',buffer)),n=>n.toString(16).padStart(2,'0')).join('');
    if(hash!==region.sha)throw Error(t('romHash'));
    $('game-language-row').hidden=region.id!=='eu'; /* only the European cartridge has the language screen */
    for(const row of document.querySelectorAll('.version')){const on=row.dataset.region===region.id;row.classList.toggle('loaded',on);
      const state=row.querySelector('.version-state');state.dataset.en=on?'Loaded · verified':'No ROM';state.dataset.es=on?'Cargada · verificada':'Sin ROM';
      state.textContent=document.documentElement.lang==='es'?state.dataset.es:state.dataset.en;}
    status('loading');core=await loadEngine();
    core.HEAPU8.set(new Uint8Array(buffer),0x08000000);buffer=null;
    if(!core._PortWebRomReady(file.size))throw Error(t('romHash'));
    core.FS.mkdir('/saves');core.FS.mount(core.IDBFS,{},'/saves');
    store=ReKHWeb.persistence(sync,error=>{log(error);warning('storage');});
    try{await sync(true);}catch(error){store.disable();log(error);warning('storage');}
    let savedOptions='';
    try{savedOptions=core.FS.readFile('/saves/kh_com.ini',{encoding:'utf8'});}catch{}
    window.ReKHOptions?.load(savedOptions);
    syncGameLanguage(savedOptions);
    core.persistSoon=()=>store.request();
    core.onAchievementsChanged=()=>window.ReKHAchievements?.render();
    window.ReKHAchievements?.render();
    core.saveBeforeReset=async()=>{
      status('reset');pendingSave=true;
      try{const ok=await store.flush();if(!ok)status('resetFailed');return ok?1:0;}
      finally{pendingSave=false;}
    };
    try{await ReKHWeb.fonts(core);core.hdReady=true;core._PortWebSetHdReady(1);}
    catch(error){log(error);core.hdReady=false;core._PortWebSetHdReady(0);warning('fonts');}
    core.onTouchPanelsChanged=enabled=>{$('touch').checked=!!enabled;};
    let touchPreference=matchMedia('(pointer:coarse)').matches;
    try {
      const ini=core.FS.readFile('/saves/kh_com.ini',{encoding:'utf8'});
      const hidden=ini.match(/^experimental_hide_touch_controls=(\d+)$/m);
      if(hidden)touchPreference=hidden[1]==='0';
    }catch{}
    $('touch').checked=touchPreference;
    core._PortWebTouchEnable($('touch').checked?1:0);
    $('play').disabled=false;$('save').disabled=false;status('ready');
  }catch(error){fail(error);core=null;region=null;$('rom').disabled=false;$('rom').value='';}
  finally{loading=false;}
});
$('save').addEventListener('change',async()=>{
  try{
    if(started)throw Error(t('beforePlay'));
    const file=$('save').files[0];if(!file)return;
    if(file.size!==65536)throw Error(t('saveSize'));
    // Use the configured bank, so importing does not silently target another save.
    let bank=1;
    try{const ini=core.FS.readFile('/saves/kh_com.ini',{encoding:'utf8'});bank=Number(ini.match(/^save_bank=(\d+)$/m)?.[1]||1);}catch{}
    bank=Math.max(1,Math.min(5,bank));
    core.FS.writeFile('/saves/'+ReKHWeb.saveName(region.save,bank),new Uint8Array(await file.arrayBuffer()));
    if(await store.flush())status('imported');
    else warning('storage');
  }catch(error){fail(error);}
  finally{$('save').value='';}
});
$('play').addEventListener('click',()=>{
  if(!core||started)return;
  window.ReKHOptions?.beforeStart();
  $('setup').hidden=true;$('game').hidden=false;canvasSize();started=true;$('canvas').focus();status('');
  window.ReKHOptions?.refresh();
  const args=['--data-dir','/saves','--rom',':browser:'];
  if(region.id==='eu'&&$('game-language').value)args.push('--language',$('game-language').value);
  try{core.callMain(args);scheduleResize();}catch(error){fail(error);}
});
$('touch').addEventListener('change',()=>{
  core?._PortWebTouchEnable($('touch').checked?1:0);
  if(started)core?._PortWebTouchPersist();
  $('canvas').focus();
});
async function toggleFullscreen(){
  try{
    if(document.fullscreenElement)await document.exitFullscreen();
    else if(expanded)setExpanded(false);
    else if($('canvas').requestFullscreen)await $('canvas').requestFullscreen();
    else setExpanded(true); // iPhone Safari: use the available viewport, keeping an exit button.
  }
  catch(error){
    if(error.name==='NotSupportedError')setExpanded(true);
    else fail(error);
  }
}
function setExpanded(on){
  expanded=on;$('game').classList.toggle('expanded',on);
  document.documentElement.classList.toggle('game-expanded',on);
  clearInput();scheduleResize();$('canvas').focus();
}
$('fullscreen').addEventListener('click',toggleFullscreen);
$('fullscreen-bar').addEventListener('click',toggleFullscreen); /* the same, right under the picture */
$('pause').addEventListener('click',()=>{paused=!paused;$('pause').textContent=t(paused?'resume':'pause');visibility();if(!paused)$('canvas').focus();});
$('export').addEventListener('click',async()=>{
  $('export').disabled=true;
  try{
    await flushSave();const name=ReKHWeb.saveName(region.save,core._PortWebActiveBank());
    const bytes=core.FS.readFile('/saves/'+name);
    backupFile=new File([bytes],name,{type:'application/octet-stream'});
    if(backupUrl)URL.revokeObjectURL(backupUrl);
    backupUrl=URL.createObjectURL(backupFile);
    const link=$('save-download');link.href=backupUrl;link.download=name;
    $('save-share').hidden=!canShareFile(backupFile);
    $('save-export').hidden=false;link.focus();
  }catch(error){status('noSave');log(error);}
  finally{$('export').disabled=false;}
});
$('save-share').addEventListener('click',async()=>{
  // A fresh click preserves Safari's transient user activation. Preparing the
  // backup (including async SRAM/IDBFS sync) happens in the Export handler.
  if(!backupFile)return;
  try{await navigator.share({files:[backupFile]});}
  catch(error){if(error.name!=='AbortError'){log(error);status('shareFallback');}}
});
$('interface-language').addEventListener('change',()=>{translateWeb($('interface-language').value);$('pause').textContent=t(paused?'resume':'pause');if(warningKeys.size)warning(Array.from(warningKeys)[0]);scheduleResize();if(!core)status('choose');});
document.addEventListener('fullscreenchange',()=>{clearInput();scheduleResize();if(started)$('canvas').focus();});
window.addEventListener('blur',clearInput);
window.addEventListener('resize',scheduleResize);
window.visualViewport?.addEventListener('resize',scheduleResize);
new ResizeObserver(scheduleResize).observe($('canvas'));
document.addEventListener('visibilitychange',visibility);
window.addEventListener('pagehide',()=>{if(started){core._PortWebFlushSave();store.flush();}});
window.addEventListener('beforeunload',event=>{if(pendingSave||store?.pending){event.preventDefault();event.returnValue=t('leave');}});
$('canvas').addEventListener('keydown',event=>{
  if(expanded&&event.key==='Escape'){
    event.preventDefault();event.stopImmediatePropagation();setExpanded(false);return;
  }
  if(event.key==='F11'||(event.key==='Enter'&&event.altKey)){
    event.preventDefault();event.stopImmediatePropagation();if(!event.repeat)toggleFullscreen();return;
  }
  if(['ArrowUp','ArrowDown','ArrowLeft','ArrowRight',' ','Backspace'].includes(event.key))event.preventDefault();
});
$('canvas').addEventListener('blur',clearInput);
$('canvas').addEventListener('pointercancel',clearInput);
$('canvas').addEventListener('pointerdown',()=>$('canvas').focus());
$('canvas').addEventListener('contextmenu',event=>event.preventDefault());
translateWeb('en');status('choose');

/* The European game's language lives in kh_com.ini (game_language, as on the desktop):
   the game remembers the choice made on its language screen there, and this selector
   shows and changes the same value. A choice made before the ROM is loaded is written
   once the save folder is mounted. */
const gameLanguageIds=['','en','fr','de','it','es'];
let gameLanguageChoice=null;
try{const saved=localStorage.getItem('rekhcom-game-language');if(saved)$('game-language').value=saved;}catch{}
function writeGameLanguage(){
  if(!core||region?.id!=='eu')return;
  const value=Math.max(0,gameLanguageIds.indexOf($('game-language').value));
  let ini='';try{ini=core.FS.readFile('/saves/kh_com.ini',{encoding:'utf8'});}catch{}
  if(/^game_language=.*$/m.test(ini))ini=ini.replace(/^game_language=.*$/m,'game_language='+value);
  else ini=(ini||'config_version=11').replace(/\n?$/,'\n')+'game_language='+value+'\n';
  core.FS.writeFile('/saves/kh_com.ini',ini);store?.request();
}
function syncGameLanguage(ini){
  const remembered=ini.match(/^game_language=(\d+)$/m);
  if(gameLanguageChoice===null&&remembered)$('game-language').value=gameLanguageIds[Number(remembered[1])]||'';
  else if(gameLanguageChoice!==null||$('game-language').value)writeGameLanguage(); /* older pages kept it in this browser */
  try{localStorage.removeItem('rekhcom-game-language');}catch{}
}
$('game-language').addEventListener('change',()=>{gameLanguageChoice=$('game-language').value;writeGameLanguage();});
