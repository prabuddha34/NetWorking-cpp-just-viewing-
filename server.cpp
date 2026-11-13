// PraxChat - single-file LAN chat server (C++17)
// Features: browser UI, group chat, private messages, online users, typing indicators,
// unread counts, in-memory recent history, reconnect support, and bounded WebSocket handling.
//
// Linux / macOS / WSL:
//   g++ -std=c++17 -O2 -Wall -Wextra praxchat.cpp -o praxchat
//   ./praxchat 8080
//
// Windows MinGW:
//   g++ -std=c++17 -O2 -Wall -Wextra praxchat.cpp -o praxchat.exe -lws2_32
//   .\praxchat.exe 8080
//
// Open http://localhost:8080. Other devices on the same Wi-Fi can use the LAN URL printed below.
// This is a learning/LAN project, not a production-secure messaging service: no TLS,
// account authentication, durable storage, or end-to-end encryption is provided.

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
typedef WSAPOLLFD pollfd_t;
#define CLOSESOCK closesocket
#define POLL_CALL(a, n, t) WSAPoll((a), (ULONG)(n), (t))
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
static bool socket_would_block() { return WSAGetLastError() == WSAEWOULDBLOCK; }
static void set_nonblocking(socket_t s) { u_long mode = 1; ioctlsocket(s, FIONBIO, &mode); }
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
typedef struct pollfd pollfd_t;
#define CLOSESOCK close
#define POLL_CALL(a, n, t) poll((a), (nfds_t)(n), (t))
#define INVALID_SOCKET_VALUE (-1)
static bool socket_would_block() { return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR; }
static void set_nonblocking(socket_t s) { fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); }
#endif

static constexpr size_t MAX_FRAME = 8192;
static constexpr size_t MAX_HTTP_HEADER = 16384;
static constexpr size_t MAX_PENDING_OUTPUT = 1024 * 1024;
static constexpr size_t MAX_HISTORY = 100;
static constexpr size_t MAX_CLIENTS = 128;
static constexpr size_t MAX_MESSAGE = 1000;

static const char* PAGE = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#075e54"><title>PraxChat</title>
<style>
:root{color-scheme:light;--bg:#e9edef;--panel:#fff;--side:#fff;--line:#e8edef;--text:#111b21;--muted:#667781;--green:#008069;--green2:#d9fdd3;--bubble:#fff;--mine:#d9fdd3;--accent:#00a884;--soft:#f0f2f5;--shadow:0 8px 30px #102a3612}
:root.dark{color-scheme:dark;--bg:#0b141a;--panel:#202c33;--side:#111b21;--line:#2a3942;--text:#e9edef;--muted:#8696a0;--green:#00a884;--green2:#005c4b;--bubble:#202c33;--mine:#005c4b;--accent:#00a884;--soft:#182229;--shadow:0 8px 30px #0005}
*{box-sizing:border-box}html,body{height:100%;margin:0}body{font:15px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif;background:var(--bg);color:var(--text)}button,input{font:inherit}button{cursor:pointer}button:focus-visible,input:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
#shell{height:100dvh;max-width:1600px;margin:auto;padding:18px;display:grid;grid-template-columns:340px minmax(0,1fr);gap:0}
#sidebar,#chat{min-height:0;background:var(--panel);box-shadow:var(--shadow)}#sidebar{border-radius:12px 0 0 12px;border-right:1px solid var(--line);display:flex;flex-direction:column;overflow:hidden}#chat{border-radius:0 12px 12px 0;display:flex;flex-direction:column;min-width:0;overflow:hidden}
.brand{height:72px;padding:14px 18px;display:flex;align-items:center;justify-content:space-between;border-bottom:1px solid var(--line)}.brand-left{display:flex;align-items:center;gap:11px}.logo{width:42px;height:42px;border-radius:14px;background:var(--green);color:white;display:grid;place-items:center;font-size:21px;font-weight:800}.brand h1{font-size:19px;margin:0;letter-spacing:-.5px}.brand small{display:block;color:var(--muted);font-size:12px}.icon-btn{width:38px;height:38px;border:0;border-radius:50%;background:transparent;color:var(--muted);font-size:19px}.icon-btn:hover{background:var(--soft);color:var(--text)}
.profile{padding:12px 16px;display:flex;align-items:center;gap:10px;border-bottom:1px solid var(--line)}.avatar{width:42px;height:42px;border-radius:50%;background:#d7eee8;color:#075e54;display:grid;place-items:center;font-weight:800;flex:none}.profile-info{min-width:0;flex:1}.profile-info strong{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.profile-info span{font-size:12px;color:var(--muted)}
.search-wrap{padding:12px 14px}.search{width:100%;border:0;border-radius:9px;background:var(--soft);padding:11px 14px;color:var(--text)}.section-label{padding:5px 18px 9px;color:var(--muted);font-size:11px;font-weight:800;letter-spacing:1px;text-transform:uppercase}
#contacts{list-style:none;margin:0;padding:0 8px 12px;overflow:auto;flex:1}.contact{width:100%;border:0;background:transparent;color:var(--text);display:flex;align-items:center;gap:11px;padding:11px 10px;border-radius:10px;text-align:left}.contact:hover{background:var(--soft)}.contact.active{background:var(--green2)}.contact .avatar{width:46px;height:46px}.contact-main{min-width:0;flex:1}.contact-top{display:flex;justify-content:space-between;gap:8px}.contact-name{font-weight:650;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.contact-time{font-size:11px;color:var(--muted);white-space:nowrap}.contact-sub{display:flex;justify-content:space-between;gap:8px;color:var(--muted);font-size:12px;margin-top:3px}.preview{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.badge{min-width:19px;height:19px;border-radius:20px;background:var(--accent);color:#fff;font-size:11px;display:grid;place-items:center;padding:0 5px}
.chat-head{height:72px;border-bottom:1px solid var(--line);display:flex;align-items:center;gap:12px;padding:12px 20px}.chat-head .avatar{width:44px;height:44px}.chat-title{min-width:0;flex:1}.chat-title strong{display:block}.chat-title span{font-size:12px;color:var(--muted)}.head-actions{display:flex;gap:4px}.mobile-back{display:none}
#messages{flex:1;overflow:auto;padding:22px clamp(14px,5vw,70px);display:flex;flex-direction:column;gap:8px;background-color:var(--bg);background-image:radial-gradient(#aab7bd22 .7px,transparent .7px);background-size:18px 18px}.day-label{align-self:center;background:var(--panel);color:var(--muted);font-size:11px;padding:6px 12px;border-radius:20px;margin:5px 0 10px;box-shadow:0 1px 2px #0001}.msg{max-width:min(72%,620px);padding:9px 11px 7px;border-radius:10px;background:var(--bubble);box-shadow:0 1px 1px #0001;align-self:flex-start;overflow-wrap:anywhere;white-space:pre-wrap}.msg.mine{align-self:flex-end;background:var(--mine)}.msg.private{border-left:3px solid #e5a93d}.msg .sender{font-size:12px;font-weight:750;color:var(--green);margin-bottom:3px}.msg .body{font-size:14px}.msg .meta{font-size:10px;color:var(--muted);text-align:right;margin-top:4px}.system{align-self:center;color:var(--muted);background:var(--panel);padding:5px 12px;border-radius:20px;font-size:11px;margin:5px 0}.typing{height:22px;padding:0 22px;color:var(--muted);font-size:12px;background:var(--panel)}
#composer{display:flex;align-items:flex-end;gap:10px;padding:13px 18px;background:var(--panel);border-top:1px solid var(--line)}#messageInput{resize:none;max-height:120px;min-height:44px;flex:1;border:0;border-radius:12px;background:var(--soft);padding:12px 15px;color:var(--text);outline:none}#send{height:44px;min-width:48px;border:0;border-radius:12px;background:var(--green);color:#fff;font-size:18px;font-weight:800}#send:disabled{opacity:.5;cursor:not-allowed}.empty{margin:auto;text-align:center;color:var(--muted);max-width:360px;padding:30px}.empty .big{font-size:54px;margin-bottom:12px}.empty h2{color:var(--text);font-size:21px;margin:0 0 8px}.empty p{margin:0;font-size:13px}
#login{position:fixed;inset:0;background:linear-gradient(135deg,#075e54,#00a884);display:grid;place-items:center;padding:20px;z-index:5}.login-card{width:min(420px,100%);background:var(--panel);border-radius:20px;padding:32px;box-shadow:0 20px 70px #0018}.login-card .logo{margin-bottom:18px}.login-card h2{margin:0 0 7px;font-size:25px}.login-card p{margin:0 0 22px;color:var(--muted);font-size:14px}.login-card input{width:100%;padding:13px;border:1px solid var(--line);border-radius:10px;background:var(--soft);color:var(--text);margin-bottom:12px}.primary{width:100%;border:0;border-radius:10px;background:var(--green);color:#fff;padding:13px;font-weight:750}.hint{font-size:11px;color:var(--muted);margin-top:14px;text-align:center}#connection{font-size:11px;color:var(--muted);padding:0 18px 10px}
@media(max-width:760px){#shell{padding:0;display:block}#sidebar,#chat{height:100dvh;border-radius:0}#chat{display:none}body.chat-open #sidebar{display:none}body.chat-open #chat{display:flex}.mobile-back{display:inline-grid;place-items:center}#messages{padding:16px 12px}.msg{max-width:88%}.chat-head{padding:10px 12px}}
</style></head><body>
<div id="login"><form class="login-card" id="loginForm"><div class="logo">P</div><h2>Welcome to PraxChat</h2><p>Simple conversations. One local network. Pick a name to get started.</p><input id="nameInput" maxlength="20" placeholder="Your display name" autocomplete="nickname" required><button class="primary" type="submit">Join chat</button><div class="hint">Anyone connected to this server may be able to read messages.</div></form></div>
<div id="shell">
<aside id="sidebar"><div class="brand"><div class="brand-left"><div class="logo">P</div><div><h1>PraxChat</h1><small>Local network messenger</small></div></div><button class="icon-btn" id="themeBtn" title="Toggle theme">☾</button></div>
<div class="profile"><div class="avatar" id="myAvatar">?</div><div class="profile-info"><strong id="myName">You</strong><span>Available</span></div><button class="icon-btn" id="logoutBtn" title="Change name">↪</button></div>
<div class="search-wrap"><input class="search" id="searchInput" placeholder="⌕  Search people" autocomplete="off"></div><div class="section-label">Conversations</div><ul id="contacts"></ul><div id="connection">Connecting…</div></aside>
<main id="chat"><header class="chat-head"><button class="icon-btn mobile-back" id="backBtn">‹</button><div class="avatar" id="chatAvatar">#</div><div class="chat-title"><strong id="chatName">Everyone</strong><span id="chatStatus">Group conversation</span></div><div class="head-actions"><button class="icon-btn" id="clearBtn" title="Clear messages on this screen">⌫</button></div></header>
<div id="messages"><div class="empty"><div class="big">💬</div><h2>Welcome to PraxChat</h2><p>Choose a conversation and say hello.</p></div></div><div class="typing" id="typing"></div>
<form id="composer"><textarea id="messageInput" rows="1" maxlength="1000" placeholder="Message everyone…"></textarea><button id="send" type="submit" disabled>➤</button></form></main>
</div>
<script>
const $=id=>document.getElementById(id);let ws=null,myName='',target='*',users=[],history=[],unread={},typingTimer=null,reconnectDelay=1000,manualClose=false,lastTypingSent=0;
const esc=s=>String(s??'');const initials=n=>n.split(/\s+/).slice(0,2).map(x=>x[0]||'').join('').toUpperCase()||'?';
function time(ts){return new Date(Number(ts)*1000).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'})}
function setConnection(s){$('connection').textContent=s;$('chatStatus').dataset.connection=s}
function sendRaw(s){if(ws&&ws.readyState===WebSocket.OPEN)ws.send(s)}
function connect(){if(!myName)return;manualClose=false;const scheme=location.protocol==='https:'?'wss://':'ws://';ws=new WebSocket(scheme+location.host+'/ws');ws.onopen=()=>{reconnectDelay=1000;setConnection('Connected');sendRaw('join|'+myName)};ws.onclose=()=>{setConnection(manualClose?'Disconnected':'Reconnecting…');if(!manualClose)setTimeout(connect,reconnectDelay);reconnectDelay=Math.min(reconnectDelay*2,10000)};ws.onerror=()=>setConnection('Connection issue');ws.onmessage=e=>handlePacket(String(e.data))}
function handlePacket(s){const p=s.indexOf('|'),type=p<0?s:s.slice(0,p),r=p<0?'':s.slice(p+1);
 if(type==='welcome'){myName=r;$('myName').textContent=myName;$('myAvatar').textContent=initials(myName);$('login').style.display='none';renderContacts();renderMessages();$('messageInput').focus()}
 else if(type==='users'){users=r?r.split(',').filter(Boolean):[];renderContacts()}
 else if(type==='msg'){const a=r.indexOf('|'),b=r.indexOf('|',a+1);if(a<0||b<0)return;const m={kind:'group',from:r.slice(0,a),time:r.slice(a+1,b),text:r.slice(b+1)};history.push(m);if(target!=='*'&&m.from!==myName)unread['*']=(unread['*']||0)+1;renderContacts();if(target==='*')appendMessage(m)}
 else if(type==='dm'){const a=r.indexOf('|'),b=r.indexOf('|',a+1),c=r.indexOf('|',b+1);if(a<0||b<0||c<0)return;const m={kind:'dm',from:r.slice(0,a),to:r.slice(a+1,b),time:r.slice(b+1,c),text:r.slice(c+1)};history.push(m);const other=m.from===myName?m.to:m.from;if(target!==other&&m.from!==myName)unread[other]=(unread[other]||0)+1;renderContacts();if(target===other)appendMessage(m)}
 else if(type==='sys'){const a=r.indexOf('|');if(a>=0){const ts=r.slice(0,a),text=r.slice(a+1);if(target==='*')appendSystem(text,ts)}}
 else if(type==='typing'){const a=r.indexOf('|'),b=r.indexOf('|',a+1);if(a>=0&&b>=0){const who=r.slice(0,a),to=r.slice(a+1,b),on=r.slice(b+1)==='1';if(who!==myName&&(to==='*'||to===myName)&&((target==='*'&&to==='*')||(target===who&&to===myName)))$('typing').textContent=on?who+' is typing…':''}}
}
function relevant(m){if(target==='*')return m.kind==='group';return m.kind==='dm'&&((m.from===myName&&m.to===target)||(m.from===target&&m.to===myName))}
function renderMessages(){const box=$('messages');box.textContent='';const list=history.filter(relevant);if(!list.length){const d=document.createElement('div');d.className='empty';d.innerHTML='<div class="big">💬</div><h2>Start a conversation</h2><p>Your messages will appear here.</p>';box.appendChild(d);return}list.forEach(m=>appendMessage(m,false));box.scrollTop=box.scrollHeight}
function appendMessage(m,scroll=true){if(!relevant(m))return;const box=$('messages');const empty=box.querySelector('.empty');if(empty)empty.remove();const d=document.createElement('div');d.className='msg'+(m.from===myName?' mine':'')+(m.kind==='dm'?' private':'');if(m.kind==='group'&&m.from!==myName){const n=document.createElement('div');n.className='sender';n.textContent=m.from;d.appendChild(n)}const body=document.createElement('div');body.className='body';body.textContent=m.text;const meta=document.createElement('div');meta.className='meta';meta.textContent=time(m.time)+(m.kind==='dm'?' · private':'');d.append(body,meta);box.appendChild(d);if(scroll)box.scrollTop=box.scrollHeight}
function appendSystem(text,ts){const box=$('messages');const d=document.createElement('div');d.className='system';d.textContent=text+' · '+time(ts);box.appendChild(d);box.scrollTop=box.scrollHeight}
function renderContacts(){const list=$('contacts');list.textContent='';const query=$('searchInput').value.toLowerCase();const all=[{name:'*',label:'Everyone',sub:'Group chat',avatar:'#'} ,...users.filter(n=>n!==myName).map(n=>({name:n,label:n,sub:'Direct message',avatar:initials(n)}))];$('myName').textContent=myName||'You';all.filter(x=>(x.label+' '+x.sub).toLowerCase().includes(query)).forEach(x=>{const li=document.createElement('li'),b=document.createElement('button');b.className='contact'+(target===x.name?' active':'');b.type='button';const av=document.createElement('div');av.className='avatar';av.textContent=x.avatar;const main=document.createElement('div');main.className='contact-main';const top=document.createElement('div');top.className='contact-top';const nm=document.createElement('span');nm.className='contact-name';nm.textContent=x.label;const tm=document.createElement('span');tm.className='contact-time';const sub=document.createElement('div');sub.className='contact-sub';const preview=document.createElement('span');preview.className='preview';preview.textContent=x.sub;const count=document.createElement('span');if(unread[x.name]){count.className='badge';count.textContent=unread[x.name]>99?'99+':unread[x.name]}top.append(nm,tm);sub.append(preview,count);main.append(top,sub);b.append(av,main);b.onclick=()=>selectTarget(x.name);li.appendChild(b);list.appendChild(li)})}
function selectTarget(n){target=n;unread[n]=0;$('chatName').textContent=n==='*'?'Everyone':n;$('chatAvatar').textContent=n==='*'?'#':initials(n);$('chatStatus').textContent=n==='*'?users.length+' people online':'Direct conversation';$('messageInput').placeholder=n==='*'?'Message everyone…':'Message '+n+'…';$('typing').textContent='';document.body.classList.add('chat-open');renderContacts();renderMessages();$('messageInput').focus()}
$('loginForm').onsubmit=e=>{e.preventDefault();const n=$('nameInput').value.trim();if(!n)return;myName=n.slice(0,20);localStorage.setItem('praxchat-name',myName);$('login').style.display='none';connect()};
$('composer').onsubmit=e=>{e.preventDefault();const text=$('messageInput').value.trim();if(!text||!ws||ws.readyState!==WebSocket.OPEN)return;sendRaw(target==='*'?'msg|'+text:'dm|'+target+'|'+text);$('messageInput').value='';$('messageInput').style.height='auto';$('send').disabled=true;sendTyping(false)};
$('messageInput').addEventListener('input',()=>{const el=$('messageInput');el.style.height='auto';el.style.height=Math.min(el.scrollHeight,120)+'px';$('send').disabled=!el.value.trim();const now=Date.now();if(now-lastTypingSent>900){sendTyping(true);lastTypingSent=now}clearTimeout(typingTimer);typingTimer=setTimeout(()=>sendTyping(false),1300)});
$('messageInput').addEventListener('keydown',e=>{if(e.key==='Enter'&&!e.shiftKey){e.preventDefault();$('composer').requestSubmit()}});
function sendTyping(on){sendRaw('typing|'+(target==='*'?'*':target)+'|'+(on?'1':'0'))}
$('searchInput').oninput=renderContacts;$('themeBtn').onclick=()=>{document.documentElement.classList.toggle('dark');localStorage.setItem('praxchat-dark',document.documentElement.classList.contains('dark')?'1':'0')};$('clearBtn').onclick=()=>{$('messages').textContent='';const d=document.createElement('div');d.className='empty';d.textContent='Messages hidden on this screen. New messages will appear here.';$('messages').appendChild(d)};$('backBtn').onclick=()=>document.body.classList.remove('chat-open');$('logoutBtn').onclick=()=>{localStorage.removeItem('praxchat-name');manualClose=true;if(ws)ws.close();myName='';$('login').style.display='grid';$('nameInput').value='';document.body.classList.remove('chat-open')};
if(localStorage.getItem('praxchat-dark')==='1')document.documentElement.classList.add('dark');const saved=localStorage.getItem('praxchat-name');if(saved){myName=saved;$('nameInput').value=saved;connect()}
</script></body></html>)HTML";

struct Message { bool dm; std::string from, to, text; std::time_t when; };
struct Client {
    socket_t fd = INVALID_SOCKET_VALUE;
    std::string input, output, fragment, name, address;
    bool websocket = false, joined = false, closing = false;
    unsigned messages_this_second = 0;
    std::time_t rate_window = 0;
};
static std::map<socket_t, Client> clients;
static std::vector<Message> history;

static std::string now_string() { return std::to_string((long long)std::time(nullptr)); }
static std::string trim_copy(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
static std::string utf8_cut(std::string s, size_t max) {
    if (s.size() <= max) return s;
    size_t n = max;
    while (n > 0 && ((unsigned char)s[n] & 0xC0) == 0x80) --n;
    s.resize(n); return s;
}
static std::string clean_name(const std::string& raw) {
    std::string out;
    for (unsigned char ch : raw) if (ch >= 32 && ch != 127 && ch != '|' && ch != ',') out.push_back((char)ch);
    out = utf8_cut(trim_copy(out), 20);
    return out.empty() ? "Guest" : out;
}
static std::string sha1(const std::string& msg) {
    uint32_t h0=0x67452301,h1=0xEFCDAB89,h2=0x98BADCFE,h3=0x10325476,h4=0xC3D2E1F0;
    std::string m=msg; uint64_t bits=(uint64_t)m.size()*8; m.push_back((char)0x80);
    while(m.size()%64!=56)m.push_back(0);
    for(int i=7;i>=0;--i)m.push_back((char)((bits>>(i*8))&255));
    auto rol=[](uint32_t x,int n){return (x<<n)|(x>>(32-n));};
    for(size_t off=0;off<m.size();off+=64){uint32_t w[80];
        for(int i=0;i<16;++i){const unsigned char*p=(const unsigned char*)m.data()+off+i*4;w[i]=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
        for(int i=16;i<80;++i)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        uint32_t a=h0,b=h1,c=h2,d=h3,e=h4;
        for(int i=0;i<80;++i){uint32_t f,k;if(i<20){f=(b&c)|(~b&d);k=0x5A827999;}else if(i<40){f=b^c^d;k=0x6ED9EBA1;}else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}else{f=b^c^d;k=0xCA62C1D6;}uint32_t t=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=t;}
        h0+=a;h1+=b;h2+=c;h3+=d;h4+=e;
    }
    uint32_t hs[5]={h0,h1,h2,h3,h4};std::string out(20,0);
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 4; ++j) out[i * 4 + j] = (char)(hs[i] >> (24 - 8 * j));
    }
    return out;
}
static std::string base64(const std::string& in) {
    static const char* t="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string out;size_t i=0;
    for(;i+2<in.size();i+=3){uint32_t v=((unsigned char)in[i]<<16)|((unsigned char)in[i+1]<<8)|(unsigned char)in[i+2];out+=t[v>>18];out+=t[(v>>12)&63];out+=t[(v>>6)&63];out+=t[v&63];}
    if(i+1==in.size()){uint32_t v=(unsigned char)in[i]<<16;out+=t[v>>18];out+=t[(v>>12)&63];out+="==";}else if(i+2==in.size()){uint32_t v=((unsigned char)in[i]<<16)|((unsigned char)in[i+1]<<8);out+=t[v>>18];out+=t[(v>>12)&63];out+=t[(v>>6)&63];out+='=';}return out;
}
static bool queue_raw(Client& c,const std::string& data){if(c.output.size()+data.size()>MAX_PENDING_OUTPUT){c.closing=true;return false;}c.output+=data;return true;}
static bool send_frame(Client& c,int opcode,const std::string& payload){if(payload.size()>MAX_FRAME&&opcode<8){c.closing=true;return false;}std::string f;f.push_back((char)(0x80|opcode));size_t n=payload.size();if(n<126)f.push_back((char)n);else if(n<65536){f.push_back(126);f.push_back((char)(n>>8));f.push_back((char)n);}else{f.push_back(127);for(int i=7;i>=0;--i)f.push_back((char)(((uint64_t)n>>(i*8))&255));}f+=payload;return queue_raw(c,f);}
static bool send_text(Client& c,const std::string& s){return send_frame(c,1,s);}
static void broadcast(const std::string& s,socket_t except=INVALID_SOCKET_VALUE){for(auto&kv:clients)if(kv.first!=except&&kv.second.websocket&&kv.second.joined&&!kv.second.closing)send_text(kv.second,s);}
static bool name_taken(const std::string& n){for(auto&kv:clients)if(kv.second.joined&&kv.second.name==n)return true;return false;}
static std::vector<std::string> online_names(){std::vector<std::string> v;for(auto&kv:clients)if(kv.second.joined)v.push_back(kv.second.name);std::sort(v.begin(),v.end());return v;}
static void send_users(){auto names=online_names();std::string s="users|";for(size_t i=0;i<names.size();++i){if(i)s+=",";s+=names[i];}broadcast(s);std::cout<<"Online ("<<names.size()<<"): ";for(auto&n:names)std::cout<<n<<" ";std::cout<<std::endl;}
static void remember(const Message&m){history.push_back(m);if(history.size()>MAX_HISTORY)history.erase(history.begin(),history.begin()+(history.size()-MAX_HISTORY));}
static void send_history(Client& c){for(const auto&m:history){if(!m.dm)send_text(c,"msg|"+m.from+"|"+std::to_string((long long)m.when)+"|"+m.text);else if(m.from==c.name||m.to==c.name)send_text(c,"dm|"+m.from+"|"+m.to+"|"+std::to_string((long long)m.when)+"|"+m.text);}}
static Client* find_user(const std::string& n){for(auto&kv:clients)if(kv.second.joined&&kv.second.name==n)return &kv.second;return nullptr;}
static void handle_text(Client& c,const std::string&s){
    if(!c.joined){if(s.compare(0,5,"join|")!=0)return;std::string base=clean_name(s.substr(5)),name=base;for(int i=2;name_taken(name);++i)name=utf8_cut(base,16)+std::to_string(i);c.name=name;c.joined=true;send_text(c,"welcome|"+name);send_history(c);broadcast("sys|"+now_string()+"|"+name+" joined",c.fd);send_users();return;}
    std::time_t now=std::time(nullptr);if(now!=c.rate_window){c.rate_window=now;c.messages_this_second=0;}if(++c.messages_this_second>12){send_text(c,"sys|"+now_string()+"|You are sending messages too quickly");return;}
    if(s.compare(0,4,"msg|")==0){std::string text=utf8_cut(trim_copy(s.substr(4)),MAX_MESSAGE);if(text.empty())return;Message m{false,c.name,"",text,now};remember(m);broadcast("msg|"+c.name+"|"+std::to_string((long long)now)+"|"+text);std::cout<<"[group] "<<c.name<<": "<<text<<std::endl;}
    else if(s.compare(0,3,"dm|")==0){size_t p=s.find('|',3);if(p==std::string::npos)return;std::string to=clean_name(s.substr(3,p-3)),text=utf8_cut(trim_copy(s.substr(p+1)),MAX_MESSAGE);if(text.empty())return;Client*t=find_user(to);if(!t){send_text(c,"sys|"+now_string()+"|"+to+" is not online");return;}Message m{true,c.name,to,text,now};remember(m);std::string packet="dm|"+c.name+"|"+to+"|"+std::to_string((long long)now)+"|"+text;send_text(*t,packet);if(t!=&c)send_text(c,packet);std::cout<<"[private] "<<c.name<<" -> "<<to<<": "<<text<<std::endl;}
    else if(s.compare(0,7,"typing|")==0){size_t p=s.find('|',7);if(p==std::string::npos)return;std::string to=s.substr(7,p-7),flag=s.substr(p+1)=="1"?"1":"0";std::string packet="typing|"+c.name+"|"+to+"|"+flag;if(to=="*")broadcast(packet,c.fd);else{Client*t=find_user(to);if(t)send_text(*t,packet);}}
}
static std::string lower(std::string s){for(char&c:s)c=(char)std::tolower((unsigned char)c);return s;}
static void handle_http(Client&c,const std::string&head){std::istringstream is(head);std::string line,method,path;std::getline(is,line);{std::istringstream req(line);req>>method>>path;}std::map<std::string,std::string> h;while(std::getline(is,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();size_t p=line.find(':');if(p!=std::string::npos)h[lower(line.substr(0,p))]=trim_copy(line.substr(p+1));}size_t q=path.find('?');if(q!=std::string::npos)path.resize(q);
    if(method=="GET"&&path=="/ws"&&lower(h["upgrade"])=="websocket"&&!h["sec-websocket-key"].empty()){std::string accept=base64(sha1(h["sec-websocket-key"]+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));queue_raw(c,"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+accept+"\r\n\r\n");c.websocket=true;}
    else if(method=="GET"&&(path=="/"||path=="/index.html")){std::string body=PAGE;queue_raw(c,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "+std::to_string(body.size())+"\r\nCache-Control: no-store\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\nContent-Security-Policy: default-src 'self' 'unsafe-inline' ws: wss:; img-src 'self' data:;\r\n\r\n"+body);c.closing=true;}
    else{std::string body="Not found";queue_raw(c,"HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\n\r\n"+body);c.closing=true;}}
static void protocol_close(Client&c){send_frame(c,8,std::string("\x03\xEA",2));c.closing=true;}
static void parse_frames(Client&c){while(!c.closing){if(c.input.size()<2)return;const unsigned char*p=(const unsigned char*)c.input.data();bool fin=(p[0]&0x80)!=0;unsigned char rsv=p[0]&0x70;int op=p[0]&15;bool masked=(p[1]&0x80)!=0;uint64_t len=p[1]&127;size_t hdr=2;if(rsv||!masked){protocol_close(c);return;}if(len==126){if(c.input.size()<4)return;len=((uint64_t)p[2]<<8)|p[3];hdr=4;if(len<126){protocol_close(c);return;}}else if(len==127){if(c.input.size()<10)return;if(p[2]&0x80){protocol_close(c);return;}len=0;for(int i=0;i<8;++i)len=(len<<8)|p[2+i];hdr=10;if(len<65536){protocol_close(c);return;}}
    bool control=op>=8;if((control&&(!fin||len>125))||len>MAX_FRAME||(op!=0&&op!=1&&op!=2&&op!=8&&op!=9&&op!=10)){protocol_close(c);return;}if(c.input.size()<hdr+4+(size_t)len)return;unsigned char mask[4];std::memcpy(mask,p+hdr,4);std::string payload((size_t)len,0);for(size_t i=0;i<(size_t)len;++i)payload[i]=(char)(p[hdr+4+i]^mask[i&3]);c.input.erase(0,hdr+4+(size_t)len);
    if(op==1||op==0){if(op==1&&!c.fragment.empty()){protocol_close(c);return;}if(op==0&&c.fragment.empty()){protocol_close(c);return;}c.fragment+=payload;if(c.fragment.size()>MAX_FRAME){protocol_close(c);return;}if(fin){if(op==0||op==1)handle_text(c,c.fragment);c.fragment.clear();}}
    else if(op==8){send_frame(c,8,payload.substr(0,125));c.closing=true;}else if(op==9)send_frame(c,10,payload);}}
static void process_input(Client&c){if(c.closing&&!c.websocket){c.input.clear();return;}if(!c.websocket){size_t pos=c.input.find("\r\n\r\n");if(pos==std::string::npos){if(c.input.size()>MAX_HTTP_HEADER)c.closing=true;return;}handle_http(c,c.input.substr(0,pos));c.input.erase(0,pos+4);if(!c.websocket)return;}parse_frames(c);}
static void flush_output(Client&c,bool&dead){while(!c.output.empty()){int n=(int)send(c.fd,c.output.data(),(int)std::min<size_t>(c.output.size(),65536),MSG_NOSIGNAL);if(n>0)c.output.erase(0,(size_t)n);else{if(!socket_would_block())dead=true;return;}}}
static void drop_client(socket_t fd){auto it=clients.find(fd);if(it==clients.end())return;bool was=it->second.joined;std::string name=it->second.name;CLOSESOCK(fd);clients.erase(it);if(was){broadcast("sys|"+now_string()+"|"+name+" left");send_users();}}
static std::string lan_ip(){socket_t s=socket(AF_INET,SOCK_DGRAM,0);if(s==INVALID_SOCKET_VALUE)return"";sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons(53);inet_pton(AF_INET,"8.8.8.8",&a.sin_addr);std::string ip;if(connect(s,(sockaddr*)&a,sizeof a)==0){sockaddr_in local{};
#ifdef _WIN32
int len=sizeof local;
#else
socklen_t len=sizeof local;
#endif
if(getsockname(s,(sockaddr*)&local,&len)==0){char b[64]{};if(inet_ntop(AF_INET,&local.sin_addr,b,sizeof b))ip=b;}}CLOSESOCK(s);return ip;}
int main(int argc,char**argv){int port=argc>1?std::atoi(argv[1]):8080;if(port<1||port>65535){std::cerr<<"Invalid port. Use 1-65535.\n";return 1;}
#ifdef _WIN32
WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){std::cerr<<"WSAStartup failed\n";return 1;}
#else
signal(SIGPIPE,SIG_IGN);
#endif
socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET_VALUE){std::cerr<<"socket() failed\n";
#ifdef _WIN32
WSACleanup();
#endif
return 1;}int yes=1;setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,(const char*)&yes,sizeof yes);sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons((uint16_t)port);if(bind(listener,(sockaddr*)&addr,sizeof addr)!=0||listen(listener,64)!=0){std::cerr<<"Cannot listen on port "<<port<<". It may already be in use.\n";CLOSESOCK(listener);
#ifdef _WIN32
WSACleanup();
#endif
return 1;}set_nonblocking(listener);std::string ip=lan_ip();std::cout<<"\n  PraxChat server is running\n  Local: http://localhost:"<<port<<"\n  Wi-Fi: http://"<<(ip.empty()?"<your-laptop-ip>":ip)<<":"<<port<<"\n  Max clients: "<<MAX_CLIENTS<<" | Recent history: "<<MAX_HISTORY<<" messages\n  Press Ctrl+C to stop.\n"<<std::endl;
    while(true){std::vector<pollfd_t> fds;pollfd_t lp{};lp.fd=listener;lp.events=POLLIN;fds.push_back(lp);for(auto&kv:clients){pollfd_t p{};p.fd=kv.first;p.events=POLLIN;if(!kv.second.output.empty())p.events|=POLLOUT;fds.push_back(p);}int ready=POLL_CALL(fds.data(),fds.size(),500);if(ready<0){if(socket_would_block())continue;std::cerr<<"poll() failed\n";break;}
        if(fds[0].revents&POLLIN){while(true){sockaddr_in ca{};
#ifdef _WIN32
int len=sizeof ca;
#else
socklen_t len=sizeof ca;
#endif
socket_t fd=accept(listener,(sockaddr*)&ca,&len);if(fd==INVALID_SOCKET_VALUE)break;if(clients.size()>=MAX_CLIENTS){CLOSESOCK(fd);continue;}set_nonblocking(fd);int one=1;setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(const char*)&one,sizeof one);Client c;c.fd=fd;char b[64]="?";inet_ntop(AF_INET,&ca.sin_addr,b,sizeof b);c.address=b;clients.emplace(fd,std::move(c));}}
        std::vector<socket_t> dead;for(size_t i=1;i<fds.size();++i){auto it=clients.find(fds[i].fd);if(it==clients.end())continue;Client&c=it->second;bool kill=false;if(fds[i].revents&POLLIN){char b[4096];int n=(int)recv(c.fd,b,sizeof b,0);if(n>0){c.input.append(b,(size_t)n);process_input(c);}else if(n==0||!socket_would_block())kill=true;}if(fds[i].revents&(POLLERR|POLLHUP|POLLNVAL))kill=true;if(!kill&&(fds[i].revents&POLLOUT))flush_output(c,kill);if(!kill&&c.closing&&c.output.empty())kill=true;if(kill)dead.push_back(c.fd);}for(auto&kv:clients){bool kill=false;if(!kv.second.output.empty())flush_output(kv.second,kill);if(kill)dead.push_back(kv.first);}std::sort(dead.begin(),dead.end());dead.erase(std::unique(dead.begin(),dead.end()),dead.end());for(socket_t fd:dead)drop_client(fd);
    }
    for (auto& kv : clients) CLOSESOCK(kv.first);
    clients.clear();
    CLOSESOCK(listener);
#ifdef _WIN32
WSACleanup();
#endif
return 0;}
