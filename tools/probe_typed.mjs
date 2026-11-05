import { chromium } from 'playwright';

const values = () =>
  Array.from(document.querySelectorAll('textarea, input')).map((t) => t.value);

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);
console.log('fields at start:', JSON.stringify(await page.evaluate(values)));

// The card opened, and what is written in its field: a textarea keeps what
// is in it where innerText of the page cannot see it.
await page.mouse.click(1166, 26);
await page.waitForTimeout(1000);
console.log('fields open:', JSON.stringify(await page.evaluate(values)));

await page.mouse.click(640, 440);
await page.waitForTimeout(700);
await page.keyboard.type('Answer in one line.');
await page.waitForTimeout(900);
console.log('fields typed:', JSON.stringify(await page.evaluate(values)));

// Put away, opened again: what the conversation keeps is what the card is
// filled with, which is only what this wrote into it.
await page.mouse.click(882, 308);
await page.waitForTimeout(900);
const put = await page.evaluate(values);
console.log('fields put away:', JSON.stringify(put));
await page.mouse.click(1166, 26);
await page.waitForTimeout(1000);
const again1 = await page.evaluate(values);
console.log('fields opened again:', JSON.stringify(again1));
console.log('kept:', again1.some((one) => one === 'Answer in one line.'));
await browser.close();
