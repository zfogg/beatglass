import { chromium } from 'playwright';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_PATH || 'C:/Program Files/Google/Chrome/Application/chrome.exe'});
const page=await browser.newPage({viewport:{width:1440,height:960}});
for(const theme of ['light','dark','contrast','purple','blue','yellow']) for(const screen of ['library','library-empty','track','jobs','dj','dj-recovery','overlay','overlay-context','settings','recovery']) {
 const url=pathToFileURL(resolve('index.html')); url.search=new URLSearchParams({screen,theme}).toString(); await page.goto(url.href);
 if(!await page.locator('h1').count() || !await page.locator('main').innerText()) throw Error(`${theme}/${screen} missing content`);
 if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)) throw Error(`${theme}/${screen} overflows desktop viewport`);
}
await browser.close(); console.log('60 theme/screen combinations have content and no desktop overflow');


