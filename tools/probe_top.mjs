import { chromium } from 'playwright';

const page_mirror = () => document.getElementById('skiff').innerText;

const browser = await chromium.launch();
const width = 1280;
const height = 900;
const page = await browser.newPage({ viewport: { width, height } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);

const said = await page.evaluate(page_mirror);
const has = (needle) => said.includes(needle);
console.log('said:', said.length);
console.log('haiku-4 bubble:', has('haiku-4'));
console.log('opus-4 bubble:', has('opus-4'));
console.log('window said:', has('~4.1K / 1M tokens'));
console.log('tokens beside:', has('4.1k tokens'));
console.log('title:', has('What is in the kitchen'));
await page.screenshot({ path: 'top.png' });

// The press that opens what every model is told: second of the four from the
// right, once the plus that adds an answerer is counted.
await page.mouse.click(1166, 26);
await page.waitForTimeout(1500);
const opened = await page.evaluate(page_mirror);
console.log('card said:', opened.length);
console.log('head:', opened.includes('System prompt'));
console.log('said line:', opened.includes('Told to every model that answers here'));
console.log('what is told where nothing is written:',
            opened.includes('You are answering in Progressive Chat, briefly and plainly.'));
await page.screenshot({ path: 'prompt.png' });

// Written into the field: what is typed goes into the conversation whole.
// The field is where the card keeps it, in the middle of the card.
await page.mouse.click(640, 440);
await page.waitForTimeout(600);
await page.keyboard.type('Answer in one line.');
await page.waitForTimeout(1200);
const typed = await page.evaluate(page_mirror);
console.log('typed said:', typed.includes('Answer in one line.'));
await page.screenshot({ path: 'prompt-typed.png' });

// Put away again, and the room read once more: the press is in the card's
// own top corner, over the room and over nothing else.
await page.mouse.click(882, 308);
await page.waitForTimeout(1200);
const again1 = await page.evaluate(page_mirror);
console.log('put away:', !again1.includes('Told to every model that answers here'));
console.log('tail:', JSON.stringify(again1.slice(-260)));
await browser.close();
