import { chromium } from 'playwright';

const page_mirror = () => document.getElementById('skiff').innerText;

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);

const read = async () => await page.evaluate(page_mirror);
const has = (needle, said) => said.includes(needle);

// What the search field keeps is asked for as it is typed: a word said in one
// conversation and named in none of them.
await page.mouse.click(190, 62);
await page.waitForTimeout(500);
await page.keyboard.type('lemon');
await page.waitForTimeout(900);
const said = await read();
console.log('lemon  kitchen:', has('What is in the kitchen', said),
            'notices:', has('Notices', said),
            'weekend:', has('Weekend', said),
            'none line:', has('No conversation is named that', said));
await page.screenshot({ path: 'search-lemon.png' });

// A name said whole, then nothing said anywhere.
for (let i = 0; i < 5; ++i) await page.keyboard.press('Backspace');
await page.keyboard.type('weekend');
await page.waitForTimeout(900);
const named = await read();
console.log('weekend  kitchen:', has('What is in the kitchen', named),
            'weekend:', has('Weekend', named),
            'none line:', has('No conversation is named that', named));

for (let i = 0; i < 7; ++i) await page.keyboard.press('Backspace');
await page.keyboard.type('zzzz');
await page.waitForTimeout(900);
const none = await read();
console.log('zzzz  none line:', has('No conversation is named that', none),
            'kitchen:', has('What is in the kitchen', none));
await page.screenshot({ path: 'search-none.png' });

// Back to everything: nothing said keeps every conversation.
for (let i = 0; i < 4; ++i) await page.keyboard.press('Backspace');
await page.waitForTimeout(900);
const back = await read();
console.log('empty  kitchen:', has('What is in the kitchen', back),
            'notices:', has('Notices', back),
            'weekend:', has('Weekend', back));
await browser.close();
