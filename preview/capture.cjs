// Run with: NODE_PATH=/opt/homebrew/lib/node_modules node preview/capture.cjs
// Requires Playwright and Google Chrome. Does not scan Bluetooth or use network.
const {chromium}=require('playwright');
const fs=require('node:fs');
const path=require('node:path');
(async()=>{
 const browser=await chromium.launch({channel:'chrome',headless:true});
 try {
  const page=await browser.newPage({viewport:{width:1640,height:1500},deviceScaleFactor:1});
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto('file://'+path.join(__dirname,'index.html'));
  await page.evaluate(()=>window.preview.drawAt(3));
  for(const id of ['radar','rain','city']){
   const png=await page.locator('#'+id).evaluate(c=>{
    if(c.width!==240||c.height!==135)throw Error('Incorrect framebuffer size');
    return c.toDataURL('image/png').split(',')[1];
   });
   fs.writeFileSync(path.join(__dirname,id+'-240x135.png'),Buffer.from(png,'base64'));
  }
  await page.screenshot({path:path.join(__dirname,'comparison.png'),fullPage:true});
  for(const scene of ['quiet','empty','busy'])await page.selectOption('#scene',scene);
  await page.selectOption('#scale','1');
  const size=await page.locator('#radar').boundingBox();
  if(size.width!==240||size.height!==135)throw Error('Native scale mismatch');
  await page.getByRole('button',{name:'Play animation'}).click();
  await page.getByRole('button',{name:'Pause animation'}).click();
  await page.setViewportSize({width:390,height:844});
  const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth);
  if(overflow)throw Error('Mobile page overflows');
  if(errors.length)throw Error(errors.join('\n'));
  console.log('Verified three 240x135 canvases, scene controls, playback, native scale and mobile layout. PNGs captured.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
