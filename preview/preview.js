/* All artwork is drawn on a 240×135 framebuffer using integer pixel geometry.
 * Synthetic observations only. This renderer never requests Bluetooth access.
 */
'use strict';
const glyphs = {
 A:'01110/10001/10001/11111/10001/10001/10001',B:'11110/10001/10001/11110/10001/10001/11110',
 C:'01111/10000/10000/10000/10000/10000/01111',D:'11110/10001/10001/10001/10001/10001/11110',
 E:'11111/10000/10000/11110/10000/10000/11111',F:'11111/10000/10000/11110/10000/10000/10000',
 G:'01111/10000/10000/10111/10001/10001/01111',H:'10001/10001/10001/11111/10001/10001/10001',
 I:'11111/00100/00100/00100/00100/00100/11111',J:'00111/00010/00010/00010/10010/10010/01100',
 K:'10001/10010/10100/11000/10100/10010/10001',L:'10000/10000/10000/10000/10000/10000/11111',
 M:'10001/11011/10101/10101/10001/10001/10001',N:'10001/11001/10101/10011/10001/10001/10001',
 O:'01110/10001/10001/10001/10001/10001/01110',P:'11110/10001/10001/11110/10000/10000/10000',
 Q:'01110/10001/10001/10001/10101/10010/01101',R:'11110/10001/10001/11110/10100/10010/10001',
 S:'01111/10000/10000/01110/00001/00001/11110',T:'11111/00100/00100/00100/00100/00100/00100',
 U:'10001/10001/10001/10001/10001/10001/01110',V:'10001/10001/10001/10001/10001/01010/00100',
 W:'10001/10001/10001/10101/10101/10101/01010',X:'10001/10001/01010/00100/01010/10001/10001',
 Y:'10001/10001/01010/00100/00100/00100/00100',Z:'11111/00001/00010/00100/01000/10000/11111',
 0:'01110/10001/10011/10101/11001/10001/01110',1:'00100/01100/00100/00100/00100/00100/01110',
 2:'01110/10001/00001/00010/00100/01000/11111',3:'11110/00001/00001/01110/00001/00001/11110',
 4:'00010/00110/01010/10010/11111/00010/00010',5:'11111/10000/10000/11110/00001/00001/11110',
 6:'01110/10000/10000/11110/10001/10001/01110',7:'11111/00001/00010/00100/01000/01000/01000',
 8:'01110/10001/10001/01110/10001/10001/01110',9:'01110/10001/10001/01111/00001/00001/01110',
 '-':'00000/00000/00000/11111/00000/00000/00000','/':'00001/00001/00010/00100/01000/10000/10000',
 ':':'00000/00100/00100/00000/00100/00100/00000','.':'00000/00000/00000/00000/00000/00110/00110',
 '>':'10000/01000/00100/00010/00100/01000/10000','?':'01110/10001/00001/00010/00100/00000/00100',
 'c':'00000/00000/01111/10000/10000/10000/01111','o':'00000/00000/01110/10001/10001/10001/01110',
 'n':'00000/00000/11110/10001/10001/10001/10001',
};
const C={bg:'#03040b',cyan:'#65ffe0',purple:'#b080ff',pink:'#ff59cc',white:'#e4fff9',dim:'#293052',amber:'#ffca73'};
const canvases=['radar','rain','city'].map(id=>document.getElementById(id));
const contexts=canvases.map(c=>c.getContext('2d'));
const samples=[{name:'DECK-09',rssi:-43},{name:'GHOST-7',rssi:-61},{name:'NIGHT OWL',rssi:-72},{name:'ANON-3F',rssi:-80},{name:'PIXEL BUDS',rssi:-56},{name:'NEON FOX',rssi:-67}];
let scene='busy', paused=matchMedia('(prefers-reduced-motion: reduce)').matches, elapsed=3;
function rect(c,x,y,w,h,color){c.fillStyle=color;c.fillRect(Math.round(x),Math.round(y),w,h);}
function text(c,s,x,y,color=C.cyan,scale=1){
 for(const ch of s){const rows=(glyphs[ch]||glyphs[ch.toUpperCase()]||'').split('/');
  rows.forEach((row,j)=>[...row].forEach((v,i)=>{if(v==='1')rect(c,x+i*scale,y+j*scale,scale,scale,color);}));x+=6*scale;}
}
function line(c,x,y,xx,yy,color){ // Bresenham: sharp pixels, including diagonals.
 x=Math.round(x);y=Math.round(y);xx=Math.round(xx);yy=Math.round(yy);
 const dx=Math.abs(xx-x),sx=x<xx?1:-1,dy=-Math.abs(yy-y),sy=y<yy?1:-1;let e=dx+dy;
 for(;;){rect(c,x,y,1,1,color);if(x===xx&&y===yy)break;const e2=2*e;if(e2>=dy){e+=dy;x+=sx;}if(e2<=dx){e+=dx;y+=sy;}}
}
function ring(c,x,y,r,color){for(let a=0;a<6.29;a+=.025)rect(c,x+Math.cos(a)*r,y+Math.sin(a)*r,1,1,color);}
function box(c,x,y,w,h,color){rect(c,x,y,w,1,color);rect(c,x,y+h-1,w,1,color);rect(c,x,y,1,h,color);rect(c,x+w-1,y,1,h,color);}
function label(c,s,x,y,color=C.cyan){rect(c,x-2,y-2,s.length*6+3,11,C.bg);text(c,s,x,y,color);}
function base(c,title,n){rect(c,0,0,240,135,C.bg);text(c,title,5,4,C.purple);text(c,`BLE ${String(n).padStart(2,'0')}`,199,4,C.cyan);line(c,4,15,235,15,C.dim);}
function footer(c,s){rect(c,0,122,240,13,'#101024');line(c,0,121,239,121,C.dim);text(c,s,5,125,C.purple);}
function radar(c,t,devices){
 base(c,'LABScon / SIGNAL FIELD',devices.length);
 for(let x=10;x<240;x+=12)for(let y=22;y<120;y+=12)rect(c,x,y,1,1,'#131929');
 const cx=119,cy=70;[17,33,49].forEach(r=>ring(c,cx,cy,r,'#24334b'));
 line(c,cx-64,cy,cx+64,cy,'#203549');line(c,cx,20,cx,118,'#203549');
 const angle=t*.7;for(let j=12;j>=0;j--)line(c,cx,cy,cx+Math.cos(angle-j*.018)*49,cy+Math.sin(angle-j*.018)*49,j<2?C.purple:'#342447');
 const spots=[[83,42,17,28],[154,53,165,36],[152,98,165,104],[82,92,8,99],[102,29,110,20],[174,77,180,83]];
 devices.forEach((d,i)=>{const [x,y,lx,ly]=spots[i];const p=(Math.sin(t*2+i)+1)/2;ring(c,x,y,3+Math.floor(p*3),i===3?'#614b32':'#235850');rect(c,x-1,y-1,3,3,i===3?C.amber:C.white);line(c,x,y,lx+8,ly+5,C.dim);label(c,d.name,lx,ly,i===3?C.amber:C.cyan);});
 label(c,'LABScon',99,67,C.white);if(!devices.length)label(c,'LISTENING...',86,102);
 footer(c,devices.length?`> ${devices[0].name}  ${devices[0].rssi} DBM / SCAN`:'> WAITING FOR SIGNALS');
}
function rain(c,t,devices){
 base(c,'NAME CASCADE / LABScon',devices.length);
 for(let col=0;col<30;col++){
  const head=((t*(13+col%5*3)+col*37)%164)-12;
  for(let k=0;k<10;k++){const y=Math.floor(head-k*9);if(y<19||y>113)continue;
   const source=devices.length?devices[col%devices.length].name:'01:/';const ch=source[(col+k)%source.length];
   text(c,ch,col*8,y,k===0?C.white:k<3?'#568e9a':k<6?'#294d62':'#172235');}
 }
 rect(c,24,46,193,39,'#060712');box(c,24,46,193,39,'#4c386c');
 rect(c,21,46,3,13,C.purple);rect(c,217,72,3,13,C.cyan);
 text(c,'LABScon',38,54,C.purple,4);
 if(devices.length){const index=Math.floor(t/3)%devices.length;label(c,`> ${devices[index].name}`,10,100,C.cyan);label(c,`${devices[index].rssi} DBM`,185,100,C.pink);}
 else label(c,'AWAITING SIGNAL',76,101);
 footer(c,'GHOSTBLE / NAMES IN THE NOISE');
}
function city(c,t,devices){
 base(c,'GHOST DISTRICT',devices.length);
 for(let i=0;i<40;i++)rect(c,(i*71)%240,20+(i*19)%75,1,1,i%4?'#28233e':'#70688b');
 ring(c,196,40,15,'#53406c');ring(c,196,40,13,'#372747');
 const buildings=[[0,63,27],[29,46,29],[60,70,22],[84,31,66],[152,61,32],[187,52,28],[218,73,22]];
 buildings.forEach(([x,y,w],b)=>{
  rect(c,x,y,w,113-y,'#0c0d1c');line(c,x,y,x+w-1,y,'#534169');line(c,x,y,x,112,'#32304e');
  for(let wx=x+4;wx<x+w-3;wx+=6)for(let wy=y+8;wy<110;wy+=8){const lit=(wx+wy+b*7+Math.floor(t/3))%7<2;rect(c,wx,wy,2,3,lit?(b%2?'#71548a':'#386c70'):'#1e2034');}
 });
 line(c,116,20,116,31,C.purple);rect(c,115,19,3,2,C.pink);
 rect(c,88,37,59,24,'#141024');box(c,88,37,59,24,C.purple);text(c,'LABScon',97,41,C.white);text(c,'2026',106,52,C.pink);
 const signs=[[4,78],[155,70],[37,95],[151,101],[66,65],[3,49]];
 devices.forEach((d,i)=>{let[x,y]=signs[i];const name=d.name;const color=i%2?C.pink:C.cyan;rect(c,x,y,name.length*6+7,13,'#101123');box(c,x,y,name.length*6+7,13,color);text(c,name,x+4,y+3,color);});
 for(let i=0;i<27;i++){const x=(i*43+Math.floor(t*6))%240,y=18+(i*29+Math.floor(t*42))%99;line(c,x,y,x-2,Math.min(119,y+5),'#31405a');}
 rect(c,0,113,240,8,'#131427');for(let i=0;i<30;i++)rect(c,(i*37+Math.floor(t*8))%240,115+i%5,2+i%5,1,i%2?'#504171':'#315950');
 if(!devices.length)label(c,'DISTRICT OFFLINE',74,101,C.cyan);
 footer(c,`LABScon / ${String(devices.length).padStart(2,'0')} SIGNALS ALIVE`);
}
const renderers=[radar,rain,city];
function draw(t){const devices=scene==='empty'?[]:samples.slice(0,scene==='quiet'?2:6);renderers.forEach((render,i)=>render(contexts[i],t,devices));}
const pause=document.getElementById('pause');
function pauseLabel(){pause.textContent=paused?'Play animation':'Pause animation';pause.setAttribute('aria-pressed',String(paused));}
pause.onclick=()=>{paused=!paused;pauseLabel();};pauseLabel();
document.getElementById('scale').onchange=e=>document.documentElement.style.setProperty('--scale',e.target.value);
document.getElementById('scene').onchange=e=>{scene=e.target.value;draw(elapsed);};
document.querySelectorAll('[data-save]').forEach(button=>button.onclick=()=>{
 const link=document.createElement('a');link.download=`cyberputer-${button.dataset.save}-240x135.png`;link.href=document.getElementById(button.dataset.save).toDataURL('image/png');link.click();
});
let previous=0;
function frame(now){if(previous&&!paused)elapsed+=Math.min((now-previous)/1000,.1);previous=now;draw(elapsed);requestAnimationFrame(frame);}
// Deterministic snapshots for design review and automated browser checks.
window.preview={drawAt(t){elapsed=t;paused=true;pauseLabel();draw(t);}};
draw(elapsed);requestAnimationFrame(frame);
