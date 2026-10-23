'use strict';
let editorBytes=null,editorSave=null,editorOriginal=null,editorFilename='save.sav';
const editorDrafts=new Map();
const editorStatSpecs=[['level','Level','Nivel',1,99],['hp','Max HP','HP máximo',1,560],['cp','CP','CP',0,9999],['ap','AP','AP',0,999],['dp','DP','DP',0,999],['moogle','Moogle points','Puntos moguri',0,9999999]];
function editorMessage(en,es){$('editor-status').textContent=optionWord(en,es);}
function editorDownload(bytes,name){const url=URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'}));const link=document.createElement('a');link.href=url;link.download=name;link.click();setTimeout(()=>URL.revokeObjectURL(url),30000);}
$('editor-enable').addEventListener('click',()=>$('editor-warning').showModal());
$('editor-cancel').addEventListener('click',()=>$('editor-warning').close());
$('editor-confirm').addEventListener('click',()=>{$('editor-warning').close();$('editor-workspace').hidden=false;$('editor-enable').hidden=true;$('editor-file').focus();});
$('editor-file').addEventListener('change',async()=>{
  const file=$('editor-file').files[0];if(!file)return;
  if(file.size!==65536){editorMessage('Choose a 64 KiB .sav, not a quick state.','Selecciona una partida .sav de 64 KiB, no un guardado rápido.');$('editor-file').value='';return;}
  editorBytes=new Uint8Array(await file.arrayBuffer());editorDrafts.clear();editorFilename=file.name;loadEditorSlot();$('editor-file').value='';
});
function loadEditorSlot(){
  const file=Number($('editor-slot').value);editorSave=editorBytes?ReKHSave.read(editorBytes,file):null;editorOriginal=editorBytes?ReKHSave.read(editorBytes,file):null;
  if(editorSave&&editorDrafts.has(file))editorSave.block=editorDrafts.get(file).slice();
  $('editor-fields').replaceChildren();$('editor-cards').hidden=!editorSave;
  if(!editorSave){editorMessage('This slot is empty or neither backup has a valid checksum.','La ranura está vacía o ninguna copia tiene un checksum válido.');return;}
  editorMessage('Valid save. Changes will be downloaded as a separate file.','Partida válida. Los cambios se descargarán en otro archivo.');
  for(const[key,en,es,min,max]of editorStatSpecs){
    if((editorSave.riku&&key==='cp')||(!editorSave.riku&&['ap','dp'].includes(key)))continue;
    const label=document.createElement('label');label.className='option-row';label.append(optionLabel(en,es));const input=document.createElement('input');input.type='number';input.min=min;input.max=max;input.value=editorSave[key];input.setAttribute('aria-label',optionWord(en,es));
    input.addEventListener('input',()=>{if(input.value!==''&&input.validity.valid){editorSave[key]=Number(input.value);renderEditorChanges();}});
    input.addEventListener('change',()=>{editorSave[key]=Number(input.value);input.value=editorSave[key];renderEditorChanges();});label.append(input);$('editor-fields').append(label);
  }
  $('battle-cards').hidden=editorSave.riku;refreshMapCard();refreshBattleCard();renderEditorChanges();
}
$('editor-slot').addEventListener('change',()=>{if(editorSave)editorDrafts.set(editorSave.file,editorSave.block.slice());loadEditorSlot();});
for(let i=0;i<ReKHCards.maps.length;i++){const o=document.createElement('option');o.value=i;o.textContent=ReKHCards.maps[i];$('editor-map-kind').append(o);}
for(const id of ['editor-map-value','editor-card-value'])for(let v=0;v<10;v++){const o=document.createElement('option');o.value=v;o.textContent=v;$(id).append(o);}
// Player kinds end at Ansem (97); Marluxia 2 (95) belongs to the boss deck.
function editableBattleCard(c){return c[0]<=97&&c[0]!==95&&c[3]<=3&&!(c[1]&0x18);}
const editableKinds=new Map();
ReKHCards.cards.forEach((card,id)=>{if(editableBattleCard(card)&&!editableKinds.has(card[0]))editableKinds.set(card[0],id);});
for(const kind of editableKinds.keys()){const o=document.createElement('option');o.value=kind;o.textContent=ReKHCards.kinds[kind];$('editor-card-kind').append(o);}
function refreshMapCard(){if(editorSave)$('editor-map-count').value=editorSave.getMap(Number($('editor-map-kind').value)*10+Number($('editor-map-value').value));}
$('editor-map-kind').addEventListener('change',refreshMapCard);$('editor-map-value').addEventListener('change',refreshMapCard);
$('editor-map-count').addEventListener('input',()=>{const input=$('editor-map-count');if(editorSave&&input.value!==''&&input.validity.valid){editorSave.setMap(Number($('editor-map-kind').value)*10+Number($('editor-map-value').value),Number(input.value));renderEditorChanges();}});
$('editor-map-count').addEventListener('change',()=>{if(!editorSave)return;editorSave.setMap(Number($('editor-map-kind').value)*10+Number($('editor-map-value').value),Number($('editor-map-count').value));refreshMapCard();renderEditorChanges();});
function currentBattleCard(){return ReKHCards.cards.findIndex(c=>c[0]===Number($('editor-card-kind').value)&&c[2]===Number($('editor-card-value').value)&&editableBattleCard(c));}
function refreshBattleCard(){
  const kind=Number($('editor-card-kind').value),values=ReKHCards.cards.filter(c=>c[0]===kind&&editableBattleCard(c)).map(c=>c[2]);
  for(const option of $('editor-card-value').options)option.disabled=!values.includes(Number(option.value));
  if(!values.includes(Number($('editor-card-value').value)))$('editor-card-value').value=values[0];
  const id=currentBattleCard(),canvas=$('card-sprite'),ctx=canvas.getContext('2d');ctx.clearRect(0,0,16,32);
  const image=ctx.createImageData(16,32),regionIndex=region?{eu:0,us:1,jp:2}[region.id]:-1;
  if(id>=0&&regionIndex>=0&&core){
    const sprite=ReKHCards.cards[id][5+regionIndex],info=ReKHCards.sprites[sprite];
    if(info){const tiles=0x08000000+info[0][regionIndex],palette=0x08000000+info[1][regionIndex],rom=core.HEAPU8;
      for(let tile=0;tile<8;tile++)for(let y=0;y<8;y++)for(let x=0;x<8;x++){
        const byte=rom[tiles+tile*32+y*4+(x>>1)],index=x&1?byte>>4:byte&15;if(!index)continue;
        const color=rom[palette+index*2]|rom[palette+index*2+1]<<8,p=(((tile>>1)*8+y)*16+(tile%2)*8+x)*4;
        image.data[p]=(color&31)*255/31;image.data[p+1]=((color>>5)&31)*255/31;image.data[p+2]=((color>>10)&31)*255/31;image.data[p+3]=255;
      }ctx.putImageData(image,0,0);
    }
  }
  canvas.title=region?optionWord('Original cartridge sprite, ×1','Sprite original del cartucho, ×1'):optionWord('Choose a ROM in Play to see the sprite.','Selecciona una ROM en Juego para ver el sprite.');
  const premium=id>=0&&ReKHCards.cards[id][3]===0;$('editor-premium').disabled=!premium;if(!premium)$('editor-premium').checked=false;
  $('editor-add').disabled=!editorSave||editorSave.riku||id<0;$('editor-remove').disabled=$('editor-add').disabled;
  if(editorSave&&!editorSave.riku)$('editor-total').textContent=editorSave.total+'/999 · '+(id>=0?editorSave.count(id):0)+'/99';
}
$('editor-card-kind').addEventListener('change',refreshBattleCard);$('editor-card-value').addEventListener('change',refreshBattleCard);
for(const[name,method]of [['editor-add','add'],['editor-remove','remove']])$(name).addEventListener('click',()=>{
  if(!editorSave)return;const id=currentBattleCard(),n=editorSave[method](id,Number($('editor-card-count').value),$('editor-premium').checked);editorMessage(n+' '+optionWord('cards changed.','cartas modificadas.'),n+' cartas modificadas.');refreshBattleCard();renderEditorChanges();
});
function cardCounts(file){const counts=new Map();if(file.riku)return counts;for(let i=0;i<999;i++){const c=file.card(i);if(c===0xfff)continue;const key=c&0x8fff;counts.set(key,(counts.get(key)||0)+1);}return counts;}
function editorChangeList(){
  if(!editorSave||!editorOriginal)return [];const changes=[];
  for(const[key,en,es]of editorStatSpecs){if((editorSave.riku&&key==='cp')||(!editorSave.riku&&['ap','dp'].includes(key)))continue;if(editorSave[key]!==editorOriginal[key])changes.push(optionWord(en,es)+': '+editorOriginal[key]+' → '+editorSave[key]);}
  for(let id=0;id<ReKHCards.maps.length*10;id++)if(editorSave.getMap(id)!==editorOriginal.getMap(id))changes.push(ReKHCards.maps[Math.floor(id/10)]+' '+id%10+': '+editorOriginal.getMap(id)+' → '+editorSave.getMap(id));
  const a=cardCounts(editorOriginal),b=cardCounts(editorSave);for(const code of new Set([...a.keys(),...b.keys()]))if((a.get(code)||0)!==(b.get(code)||0)){const id=code&0xfff,c=ReKHCards.cards[id];changes.push((c?ReKHCards.kinds[c[0]]+' '+c[2]:'Card '+id)+(code&0x8000?' Premium':'')+': '+(a.get(code)||0)+' → '+(b.get(code)||0));}
  return changes;
}
function renderEditorChanges(){const changes=editorChangeList();$('editor-changes').replaceChildren();for(const text of changes){const li=document.createElement('li');li.textContent=text;$('editor-changes').append(li);}$('editor-download').disabled=!changes.length;}
$('editor-backup').addEventListener('click',()=>{if(editorOriginal)editorDownload(editorOriginal.source,editorFilename.replace(/\.sav$/i,'')+'-original.sav');});
$('editor-download').addEventListener('click',()=>{if(editorSave)editorDownload(editorSave.export(),editorFilename.replace(/\.sav$/i,'')+'-edited.sav');});
$('interface-language').addEventListener('change',()=>{if(editorSave)renderEditorChanges();});
