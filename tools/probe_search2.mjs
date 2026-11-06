import { chromium } from 'playwright';

const counts = () => ({
  nodes: document.getElementById('skiff').querySelectorAll('*').length,
  said: document.getElementById('skiff').innerText.length,
});
const titles = () =>
  ['What is in the kitchen', 'A sieve of Eratosthenes', 'Notices', 'Weekend']
      .map((name) => {
        const walk = Array.from(document.querySelectorAll('#skiff *'));
        const one = walk.find((el) => el.children.length === 0 &&
                                      (el.textContent || '').trim() === name);
        if (!one) return `${name}: gone`;
        const r = one.getBoundingClientRect();
        const shown = getComputedStyle(one).visibility;
        return `${name}: ${Math.round(r.x)},${Math.round(r.y)} ${Math.round(r.width)}x${Math.round(r.height)} ${shown}`;
      });

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);
console.log('at start:', JSON.stringify(await page.evaluate(counts)));
console.log('titles at start:', JSON.stringify(await page.evaluate(titles)));

const field = page.locator('textarea[aria-label="Search conversations"]').first();
await field.click();
await page.waitForTimeout(400);
await page.keyboard.type('lemon');
await page.waitForTimeout(1200);
console.log('lemon:', JSON.stringify(await page.evaluate(counts)));
console.log('titles for lemon:', JSON.stringify(await page.evaluate(titles)));
await page.screenshot({ path: 'search-lemon.png' });
await browser.close();
