const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const html=fs.readFileSync(path.join(__dirname,'../../Passwort_einrichten.html'),'utf8');
const js=html.match(/<script>([\s\S]*)<\/script>/)[1];
const ids=[...html.matchAll(/id="([^"]+)"/g)].map(m=>m[1]);
function fixture({legacy=false,hasSerial=true}={}){
 const elements=Object.fromEntries(ids.map(id=>[id,{value:id==='speed'?'3':'',textContent:'',disabled:false,checked:false,hidden:id==='clear-confirm',dataset:{}}]));
 let readerResolve,writes=[],closed=0,writeFail=false;
 const stored={delay:23,enter:1,ready:true,locked:true,hex:'0400'};
 const respond=cmd=>{
  if(cmd==='STATUS')return legacy?'READY DE':`${stored.ready?'READY':'EMPTY'} DE V2 DELAY=${stored.delay} ENTER=${stored.enter} ${stored.locked?'LOCKED':'OPEN'}`;
  if(stored.locked)return 'ERR LOCKED';
  if(cmd==='CLEAR'){stored.ready=false;stored.enter=0;stored.hex='';stored.locked=true;return 'OK CLEARED';}
  const m=/^(SET|CONFIG) (\d+) ([01])(?: ([a-f0-9]+))?$/.exec(cmd);assert(m,cmd);
  stored.delay=+m[2];stored.enter=+m[3];stored.locked=true;
  if(m[1]==='SET'){stored.ready=true;stored.hex=m[4];return 'OK SAVED';}return 'OK CONFIG';
 };
 const reader={read:()=>new Promise(r=>readerResolve=r),cancel:async()=>{readerResolve?.({done:true});},releaseLock(){}};
 const writer={write:async data=>{
  if(writeFail)throw Error('Test: USB abgezogen');
  const cmd=new TextDecoder().decode(data).trim();writes.push(cmd);
  readerResolve({done:false,value:new TextEncoder().encode(respond(cmd)+'\n')});
 },abort:async()=>{},releaseLock(){}};
 const port={open:async()=>{},close:async()=>{closed++;},readable:{getReader:()=>reader},writable:{getWriter:()=>writer}};
 const context=vm.createContext({document:{getElementById:id=>elements[id]},navigator:hasSerial?{serial:{requestPort:async()=>port}}:{},TextEncoder,TextDecoder,setTimeout,clearTimeout});
 vm.runInContext(js,context);
 return {elements,writes,stored,context,getClosed:()=>closed,disconnect:()=>readerResolve({done:true}),fail:()=>{writeFail=true;}};
}
const pause=()=>new Promise(r=>setTimeout(r,0));
(async()=>{
 const f=fixture(),e=f.elements;
 await e.connect.onclick();assert.equal(e.connect.dataset.state,'connected');assert.equal(e.speed.value,'23');assert.equal(e.speedValue.textContent,'23 ms');assert(e.enter.checked);assert(e.password.value==='');
 e.password.value='x';e.repeat.value='y';await e.save.onclick();assert.match(e.status.textContent,/stimmen/);assert.equal(f.writes.length,1);
 e.password.value=e.repeat.value='🔑';await e.save.onclick();assert.match(e.status.textContent,/Nicht unterstütztes/);assert.equal(f.writes.length,1);
 e.password.value=e.repeat.value='Ab1!';e.speed.value='3';e.speed.oninput();assert.equal(e.speedValue.textContent,'3 ms');
 await e.save.onclick();assert.match(e.status.textContent,/Noch gesperrt/);assert.equal(e.password.value,'Ab1!');assert.equal(f.stored.hex,'0400');
 f.stored.locked=false;await e.save.onclick();assert.equal(f.writes.at(-1),'SET 3 1 040205001e001e02');assert.equal(e.password.value,'');assert.equal(e.repeat.value,'');assert(f.stored.locked);
 f.stored.locked=false;e.speed.value='50';e.enter.checked=false;await e.settings.onclick();assert.equal(f.writes.at(-1),'CONFIG 50 0');assert.equal(f.stored.hex,'040205001e001e02');
 assert.equal(f.stored.delay,50);assert.equal(f.stored.enter,0);
 f.stored.locked=false;e.password.value=e.repeat.value='AaYyZz09äöüÄÖÜß@€\\|{}[]^´`';await e.save.onclick();
 assert.equal(f.writes.at(-1),'SET 50 0 040204001d021d001c021c0027002600340033002f00340233022f022d00144008402d406440244027402540264035002c002e002c002e022c00');
 f.stored.locked=false;e.enter.checked=true;e.password.value=e.repeat.value='^'.repeat(128);await e.save.onclick();assert.equal(f.stored.hex.length,1024);assert.equal(f.stored.enter,1);
 e.password.value=e.repeat.value='x'.repeat(129);const n=f.writes.length;await e.save.onclick();assert.match(e.status.textContent,/Maximal/);assert.equal(f.writes.length,n);
 for(const speed of ['2','51','3.1','NaN']){e.speed.value=speed;await e.settings.onclick();assert.match(e.status.textContent,/zwischen 3 und 50/);assert.equal(f.writes.length,n);}
 e.speed.value='3';e.clear.onclick();assert(!e['clear-confirm'].hidden);e['clear-no'].onclick();assert(e['clear-confirm'].hidden);assert.equal(f.writes.length,n);
 e.clear.onclick();await e['clear-yes'].onclick();assert.match(e.status.textContent,/Noch gesperrt/);assert(f.stored.ready);
 f.stored.locked=false;await e['clear-yes'].onclick();assert.equal(f.writes.at(-1),'CLEAR');assert(!f.stored.ready&&!e.enter.checked&&e['clear-confirm'].hidden);
 const layout=vm.runInContext('layout',f.context);for(let c=32;c<127;c++)assert(Object.hasOwn(layout,String.fromCharCode(c)));
 assert.deepEqual(JSON.parse(JSON.stringify(layout)),JSON.parse(fs.readFileSync(path.join(__dirname,'../firmware/layout-de.json'),'utf8')));
 assert(!/<script[^>]+src=|fetch\(|XMLHttpRequest|WebSocket\(|localStorage|sessionStorage|document\.cookie/.test(html));
 assert(/connect-src 'none'/.test(html));assert(/id="speed" type="range" min="3" max="50" step="1" value="3"/.test(html));
 f.disconnect();await pause();await pause();assert.equal(e.connect.dataset.state,'disconnected');assert(!e.connect.disabled&&e.save.disabled);assert(f.getClosed()>=1);
 await e.connect.onclick();assert.equal(e.connect.dataset.state,'connected');assert.equal(e.speed.value,'50');assert(!e.enter.checked);
 f.fail();await e.settings.onclick();await pause();assert.equal(e.connect.dataset.state,'disconnected');assert(e.save.disabled&&!e.connect.disabled);
 const old=fixture({legacy:true});await old.elements.connect.onclick();assert.match(old.elements.status.textContent,/zuerst die neue/);assert(old.elements.save.disabled);assert(old.getClosed()>=1);
 const unsupported=fixture({hasSerial:false});await unsupported.elements.connect.onclick();assert.match(unsupported.elements.status.textContent,/Chrome oder Edge/);assert(!unsupported.elements.connect.disabled);
 console.log('PASS: Web-Serial-Handshake, alter Firmwarestand abgewiesen, persistente Werte geladen, 3–50-ms-Schieber, Änderungen ohne Passwort');
 console.log('PASS: Tastensperre, deutsches Layout unverändert, 128 Akzentzeichen + Enter, Unicode/Längenfehler, Löschbestätigung');
 console.log('PASS: Disconnect/Reconnect/Schreibfehler, Passwortfelder geleert, keine Netzaufrufe/Browser-Persistenz');
})().catch(e=>{console.error(e);process.exitCode=1;});
