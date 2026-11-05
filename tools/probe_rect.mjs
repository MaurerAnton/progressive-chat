import { chromium } from 'playwright';

const fields = () =>
  Array.from(document.querySelectorAll('textarea, input')).map((t) => {
    const r = t.getBoundingClientRect();
    return `${t.getAttribute('aria-label') || t.type} at ${Math.round(r.x)},${
        Math.round(r.y)} ${Math.round(r.width)}x${Math.round(r.height)}`;
  });

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);
console.log('closed:', JSON.stringify(await page.evaluate(fields)));
await page.mouse.click(1166, 26);
await page.waitForTimeout(1200);
console.log('open:', JSON.stringify(await page.evaluate(fields)));
await browser.close();
