import { chromium } from 'playwright';

// What each field is for is said in its label, which is what it shows empty:
// that, and what it holds, said for every field on the page.
const values = () =>
  Array.from(document.querySelectorAll('textarea, input')).map(
      (t) => `${t.getAttribute('aria-label') || t.type} = ${t.value}`);
const focused = () => {
  const one = document.activeElement;
  return `${one.tagName}:${one.getAttribute('aria-label') || ''}`;
};
const said_of = (all) =>
    all.find((one) => one.startsWith('You are answering in')) || '';

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

// Where the field stands is said by the card it is in and not by the page:
// one press of a few down the card's breadth, the first that is answered.
for (const y of [448, 440, 456, 390, 382]) {
  await page.mouse.click(640, y);
  await page.waitForTimeout(600);
  const now = await page.evaluate(focused);
  console.log('pressed at', y, 'focus:', now);
  if (now.startsWith('TEXTAREA')) break;
}
await page.keyboard.press('Control+a');
await page.keyboard.type('Answer in one line.');
await page.waitForTimeout(900);
const typed = await page.evaluate(values);
console.log('fields typed:', JSON.stringify(typed));
console.log('wrote:', said_of(typed));

// Put away, opened again: what the conversation keeps is what the card is
// filled with, which is only what this wrote into it.
await page.mouse.click(882, 380);
await page.waitForTimeout(900);
const put = await page.evaluate(values);
console.log('fields put away:', JSON.stringify(put));
await page.mouse.click(1166, 26);
await page.waitForTimeout(1000);
const opened = await page.evaluate(values);
console.log('fields opened again:', JSON.stringify(opened));
console.log('kept:', said_of(opened).includes('Answer in one line.'),
            'gone while shut:', said_of(put));
await browser.close();
