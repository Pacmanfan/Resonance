// The web interface's single page: HTML, style and script in one string,
// kept in flash. No framework and nothing loaded from outside the device.
#pragma once

#include <pgmspace.h>

namespace webui {

const char kPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Resonance</title>
<style>
:root{color-scheme:dark}
body{margin:0;background:#02040c;color:#fff;font:16px/1.4 system-ui,sans-serif}
main{max-width:720px;margin:0 auto;padding:12px}
h1{color:#40f0ff;font-size:22px;letter-spacing:.15em;margin:8px 0}
h2{color:#2df0c8;font-size:17px;margin:22px 0 8px;border-bottom:2px solid #5ce1f5;padding-bottom:4px}
.card{background:#0b1a3a;border:1px solid #5ce1f5;border-radius:8px;padding:10px 12px;margin:8px 0}
button,.btn{background:#122a5c;color:#fff;border:2px solid #5ce1f5;border-radius:7px;padding:9px 14px;font-size:16px;cursor:pointer}
button:active{background:#22d3ee;color:#02040c}
button.warn{border-color:#ffc020}
input[type=text],input[type=password]{background:#02040c;color:#fff;border:1px solid #5ce1f5;border-radius:6px;padding:8px;font-size:16px;width:100%;box-sizing:border-box;margin:4px 0 10px}
table{width:100%;border-collapse:collapse}
td{padding:7px 4px;border-bottom:1px solid #1d3466;vertical-align:middle}
td.n{word-break:break-all}
td.s{text-align:right;color:#d0d8e8;white-space:nowrap}
a{color:#40f0ff;cursor:pointer;text-decoration:none}
.dim{color:#d0d8e8;font-size:14px}
.err{color:#ffc020}
#drop{border:2px dashed #5ce1f5;border-radius:8px;padding:18px;text-align:center;margin:8px 0}
#drop.over{background:#122a5c}
progress{width:100%;height:14px}
</style></head><body><main>
<h1>RESONANCE</h1>
<div class="card" id="status">Loading...</div>

<h2>Sound files</h2>
<div class="card">
<div id="crumbs"></div>
<table id="files"></table>
<div id="drop">Drop .mp3 or .wav files here, or
<input type="file" id="pick" multiple accept=".mp3,.wav,audio/mpeg,audio/wav"></div>
<div id="uploads"></div>
<p><input type="text" id="newdir" placeholder="New folder name" maxlength="60">
<button onclick="mkdir()">Create folder</button></p>
</div>

<h2>Wi-Fi</h2>
<div class="card">
<p class="dim">Give the device your home network. It restarts and joins it; if it cannot, it opens its own network again. The password is stored on the device and never shown.</p>
<label>Network name<input type="text" id="ssid" maxlength="32"></label>
<label>Password<input type="password" id="wpass" maxlength="64"></label>
<label>Device name (address becomes name.local)<input type="text" id="host" maxlength="23"></label>
<button onclick="saveWifi()">Save and restart</button>
<button onclick="forgetWifi()">Forget network</button>
</div>

<h2>Device</h2>
<div class="card">
<button class="warn" onclick="restart('play')">Restart in PLAY mode</button>
<button onclick="restart('setup')">Restart in SETUP mode</button>
<p class="dim" id="msg"></p>
</div>
</main>
<script>
let dir='';
const $=id=>document.getElementById(id);
const esc=s=>s.replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const enc=s=>encodeURIComponent(s).replace(/'/g,'%27');
const size=n=>n>1048576?(n/1048576).toFixed(1)+' MB':Math.ceil(n/1024)+' kB';
function say(t,bad){$('msg').textContent=t;$('msg').className=bad?'err':'dim'}
async function api(path,params){
  const body=params?new URLSearchParams(params):null;
  const r=await fetch(path,{method:params?'POST':'GET',body});
  const j=await r.json().catch(()=>({error:'bad answer'}));
  if(!r.ok||j.error)throw new Error(j.error||('HTTP '+r.status));
  return j}
async function status(){
  try{const s=await api('/api/status');
    $('status').innerHTML='<b>'+esc(s.name)+'</b> &middot; firmware '+esc(s.version)+
      '<br><span class="dim">'+(s.sd?('SD card: '+s.freeMb+' MB free of '+s.totalMb+' MB'):'<span class="err">No SD card</span>')+
      ' &middot; network: '+esc(s.network)+' &middot; free memory '+Math.round(s.heap/1024)+' kB</span>';
    if(!$('ssid').value)$('ssid').value=s.ssid;if(!$('host').value)$('host').value=s.name;
  }catch(e){$('status').innerHTML='<span class="err">'+esc(e.message)+'</span>'}}
async function list(){
  let h='';const parts=dir?dir.split('/'):[];
  let c='<a onclick="go(\'\')">Sound library</a>';let p='';
  parts.forEach(x=>{p=p?p+'/'+x:x;c+=' / <a onclick="go(\''+enc(p)+'\')">'+esc(x)+'</a>'});
  $('crumbs').innerHTML=c;
  try{const l=await api('/api/files?dir='+encodeURIComponent(dir));
    l.entries.sort((a,b)=>(b.d-a.d)||a.n.localeCompare(b.n));
    l.entries.forEach(e=>{const full=dir?dir+'/'+e.n:e.n;
      h+='<tr><td class="n">'+(e.d?'<a onclick="go(\''+enc(full)+'\')">&#128193; '+esc(e.n)+'</a>':esc(e.n))+
         '</td><td class="s">'+(e.d?'':size(e.s))+'</td><td class="s"><a onclick="del(\''+enc(e.n)+'\','+e.d+')">delete</a></td></tr>'});
    if(!l.entries.length)h='<tr><td class="dim">Empty</td></tr>';
  }catch(e){h='<tr><td class="err">'+esc(e.message)+'</td></tr>'}
  $('files').innerHTML=h}
function go(d){dir=decodeURIComponent(d);list()}
async function del(n,isDir){n=decodeURIComponent(n);
  if(!confirm('Delete '+n+(isDir?' (folder, must be empty)':'')+'?'))return;
  try{await api('/api/delete',{dir,name:n});say('Deleted '+n)}catch(e){say(e.message,1)}
  list();status()}
async function mkdir(){const n=$('newdir').value.trim();if(!n)return;
  try{await api('/api/mkdir',{dir,name:n});$('newdir').value='';say('Created '+n)}catch(e){say(e.message,1)}
  list()}
function sendOne(f,overwrite){return new Promise(res=>{
  const row=document.createElement('div');row.className='dim';
  row.innerHTML=esc(f.name)+' ('+size(f.size)+') <progress max="1" value="0"></progress> <a>cancel</a>';
  $('uploads').appendChild(row);
  const x=new XMLHttpRequest();const bar=row.querySelector('progress');
  row.querySelector('a').onclick=()=>x.abort();
  x.upload.onprogress=e=>{if(e.lengthComputable)bar.value=e.loaded/e.total};
  const done=(t,bad)=>{row.innerHTML=esc(f.name)+': '+esc(t);row.className=bad?'err':'dim';res()};
  x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}
    if(x.status==409&&!overwrite){if(confirm(f.name+' already exists. Replace it?')){row.remove();sendOne(f,1).then(res)}else done('kept the existing file',1)}
    else if(x.status==200&&!j.error)done('uploaded');else done(j.error||('failed, HTTP '+x.status),1)};
  x.onerror=()=>done('connection lost',1);x.onabort=()=>done('cancelled',1);
  const d=new FormData();d.append('file',f,f.name);
  x.open('POST','/api/upload?dir='+encodeURIComponent(dir)+'&size='+f.size+(overwrite?'&overwrite=1':''));x.send(d)})}
async function sendAll(files){for(const f of files)await sendOne(f,0);list();status()}
$('pick').onchange=e=>{sendAll([...e.target.files]);e.target.value=''};
const drop=$('drop');
drop.ondragover=e=>{e.preventDefault();drop.className='over'};
drop.ondragleave=()=>drop.className='';
drop.ondrop=e=>{e.preventDefault();drop.className='';sendAll([...e.dataTransfer.files])};
async function saveWifi(){
  try{await api('/api/wifi',{ssid:$('ssid').value,password:$('wpass').value,hostname:$('host').value});
    say('Saved. The device is restarting and will try to join '+$('ssid').value+'.')}catch(e){say(e.message,1)}}
async function forgetWifi(){if(!confirm('Forget the saved network?'))return;
  try{await api('/api/wifi',{ssid:'',password:'',hostname:$('host').value});say('Forgotten. The device is restarting with its own network.')}catch(e){say(e.message,1)}}
async function restart(m){try{await api('/api/restart',{mode:m});say('Restarting in '+m+' mode...')}catch(e){say(e.message,1)}}
status();list();setInterval(status,10000);
</script></body></html>
)HTML";

}  // namespace webui
