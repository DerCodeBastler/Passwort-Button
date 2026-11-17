/* Reale Chromium-Seite, USB als Web-Serial-Testdouble. Kein Hardwaretest. */
const {chromium}=require('playwright');
const path=require('path'),assert=require('assert'),fs=require('fs');
(async()=>{
 const browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox']});
 const page=await browser.newPage({viewport:{width:1100,height:1000}}),errors=[],network=[];
 page.on('pageerror',e=>errors.push(e.message));page.on('request',r=>{if(!r.url().startsWith('file:'))network.push(r.url());});
 await page.addInitScript(()=>{
  window.mock={delay:3,enter:1,ready:true,locked:true,writes:[]};
  let readerResolve;
  const reader={read:()=>new Promise(r=>readerResolve=r),cancel:async()=>readerResolve?.({done:true}),releaseLock(){}};
  const writer={write:async data=>{
   const cmd=new TextDecoder().decode(data).trim(),s=window.mock;s.writes.push(cmd);
   let reply;
   if(cmd==='STATUS')reply=`${s.ready?'READY':'EMPTY'} DE V2 DELAY=${s.delay} ENTER=${s.enter} ${s.locked?'LOCKED':'OPEN'}`;
   else if(s.locked)reply='ERR LOCKED';
   else if(cmd==='CLEAR'){s.ready=false;s.enter=0;s.locked=true;reply='OK CLEARED';}
   else {const m=/^(CONFIG|SET) (\d+) ([01])(?: (.*))?$/.exec(cmd);if(!m)throw Error(cmd);
    s.delay=+m[2];s.enter=+m[3];s.locked=true;reply=m[1]==='SET'?'OK SAVED':'OK CONFIG';if(m[1]==='SET')s.ready=true;}
   readerResolve({done:false,value:new TextEncoder().encode(reply+'\n')});
  },abort:async()=>{},releaseLock(){}};
  const port={open:async()=>{},close:async()=>{},readable:{getReader:()=>reader},writable:{getWriter:()=>writer}};
  Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>port},configurable:true});
 });
 await page.goto('file://'+path.resolve(__dirname,'../../Passwort_einrichten.html'));
 await page.locator('#connect').click();await page.waitForFunction(()=>document.getElementById('connect').dataset.state==='connected');
 assert.equal(await page.locator('#speedValue').textContent(),'3 ms');assert(await page.locator('#enter').isChecked());
 await page.locator('#speed').fill('50');assert.equal(await page.locator('#speedValue').textContent(),'50 ms');
 await page.locator('#settings').click();await page.waitForFunction(()=>document.getElementById('status').textContent.startsWith('Noch gesperrt'));
 await page.evaluate(()=>window.mock.locked=false);await page.locator('#settings').click();
 await page.waitForFunction(()=>document.getElementById('status').textContent.startsWith('Tempo und Enter dauerhaft'));
 await page.evaluate(()=>window.mock.locked=false);await page.locator('#speed').fill('3');
 await page.locator('#password').fill('Testä!@123');await page.locator('#repeat').fill('Testä!@123');await page.locator('#save').click();
 await page.waitForFunction(()=>document.getElementById('status').textContent.startsWith('Passwort, Tempo und Enter dauerhaft'));
 assert.equal(await page.locator('#password').inputValue(),'');assert.equal(await page.locator('#repeat').inputValue(),'');
 assert((await page.evaluate(()=>window.mock.writes.at(-1))).startsWith('SET 3 1 '));
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>window.innerWidth),false);
 const output=process.env.SCREENSHOT_DIR;if(output){fs.mkdirSync(output,{recursive:true});await page.screenshot({path:path.join(output,'setup_desktop.png'),fullPage:true});}
 await page.setViewportSize({width:390,height:844});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>window.innerWidth),false);
 if(output)await page.screenshot({path:path.join(output,'setup_mobile.png'),fullPage:true});
 assert.deepEqual(errors,[]);assert.deepEqual(network,[]);
 await browser.close();
 console.log('PASS: Chromium lädt lokale Datei; Verbindung, Schieber, Sperrhinweis, Speicheraktionen, Felder leeren; keine JS-Fehler/Netzaufrufe; Desktop/Mobil ohne Überlauf');
})().catch(e=>{console.error(e);process.exitCode=1;});
