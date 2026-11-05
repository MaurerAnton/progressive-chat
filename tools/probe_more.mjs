import { chromium } from 'playwright';

const page_mirror = () => document.getElementById('skiff').innerText;

const browser = await chromium.launch();
const width = 640;
const page = await browser.newPage({ viewport: { width, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);

const read = async () => await page.evaluate(page_mirror);
const start = await read();
console.log('start:', start.length, 'bubble:', start.includes('~4.1k / 1M tokens'));

// Three dots, where the room is too narrow for four presses to stand in it.
await page.mouse.click(566, 26);
await page.waitForTimeout(1200);
const menu = await read();
console.log('menu:', menu.length, 'system row:', menu.includes('System prompt'),
            'fork row:', menu.includes('Fork conversation'),
            'share row:', menu.includes('Share'));
await page.screenshot({ path: 'more.png' });

// The first of them: what every model is told, out of the three dots.
await page.mouse.click(480, 80);
await page.waitForTimeout(1200);
const card = await read();
console.log('card:', card.length, 'head:', card.includes('System prompt'),
            'said:', card.includes('Told to every model that answers here'));
await page.screenshot({ path: 'more-card.png' });

// Put away, and the room behind it read once more.
await page.mouse.click(566, 26);
await page.waitForTimeout(1000);
const back = await read();
console.log('back:', back.length, 'menu again:', back.includes('Fork conversation'));
console.log('tail:', JSON.stringify(back.slice(-200)));
await browser.close();
