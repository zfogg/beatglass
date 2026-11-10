import { chromium } from 'playwright';
import { mkdir } from 'node:fs/promises';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
const output = resolve(process.argv[2] || 'output');
await mkdir(output, {recursive:true});
const browser = await chromium.launch({headless:true, executablePath: process.env.CHROME_PATH || 'C:/Program Files/Google/Chrome/Application/chrome.exe'});
const page = await browser.newPage({viewport:{width:1440,height:960},deviceScaleFactor:1});
for (const theme of ['light','dark','contrast','purple','blue','yellow']) for (const screen of ['library','library-queue-rechecking','library-queue-verified','library-queue-removal-confirm','library-queue-removed','library-queue-undo','library-queue-verification-failed','library-queue-retry-canceled','library-queue-retry-ready','library-queue-retry-admitted','library-queue-retry-analyzing','library-queue-retry-accepted','library-queue-retry-playing','library-queue-mismatch','library-empty','track','jobs','dj','dj-recovery','overlay','overlay-context','settings','recovery']) {
  const url = pathToFileURL(resolve('index.html')); url.search = new URLSearchParams({screen,theme}).toString();
  await page.goto(url.href); await page.screenshot({path:resolve(output,`${theme}-${screen}.png`),fullPage:true});
}
await browser.close(); console.log(`Rendered 60 screens to ${output}`);


