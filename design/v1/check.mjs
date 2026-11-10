import { chromium } from 'playwright';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
const browser=await chromium.launch({headless:true,executablePath:process.env.CHROME_PATH || 'C:/Program Files/Google/Chrome/Application/chrome.exe'});
const page=await browser.newPage({viewport:{width:1440,height:960}});
for(const theme of ['light','dark','contrast','purple','blue','yellow']) for(const screen of ['library','library-queue-rechecking','library-queue-mismatch','library-empty','track','jobs','dj','dj-recovery','overlay','overlay-context','settings','recovery']) {
 const url=pathToFileURL(resolve('index.html')); url.search=new URLSearchParams({screen,theme}).toString(); await page.goto(url.href);
 const content=await page.locator('main').innerText();
 if(!await page.locator('h1').count() || !content) throw Error(`${theme}/${screen} missing content`);
 if(screen==='library' && (!content.includes('Track-level key: unavailable.') || await page.locator('.tr:not(.th) > span:nth-child(4)').filter({hasNotText:'—'}).count())) throw Error(`${theme}/library must show unknown track keys`);
 if(screen==='track' && (!content.includes('No validated track-level key') || !content.includes('WINDOW AT 0:42 · PROVISIONAL'))) throw Error(`${theme}/track must distinguish unknown track key from provisional window`);
 if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)) throw Error(`${theme}/${screen} overflows desktop viewport`);
}
await browser.close(); console.log('60 theme/screen combinations have content and no desktop overflow; track-key distinctions pass');
