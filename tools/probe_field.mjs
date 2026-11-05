import { chromium } from 'playwright';

const values = () =>
  Array.from(document.querySelectorAll('textarea, input')).map(
      (t) => `${t.getAttribute('aria-label') || t.type}:${t.value}`);

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);

// First whether writing works at all in this room: into the field every chat
// has, at the bottom of it.
await page.mouse.click(640, 862);
await page.waitForTimeout(700);
await page.keyboard.type('hello there');
await page.waitForTimeout(900);
console.log('composer:', JSON.stringify(await page.evaluate(values)));

// Then into the field of the card opened over the room.
await page.mouse.click(1166, 26);
await page.waitForTimeout(1000);
for (const y of [440, 400, 470, 380, 500]) {
  await page.mouse.click(640, y);
  await page.waitForTimeout(500);
  await page.keyboard.type('one');
  await page.waitForTimeout(500);
  console.log('card at y', y, JSON.stringify(await page.evaluate(values)));
}
await browser.close();
