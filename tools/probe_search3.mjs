import { chromium } from 'playwright';

const titles = () =>
  ['What is in the kitchen', 'A sieve of Eratosthenes', 'Notices', 'Weekend']
      .map((name) => {
        const one = Array.from(document.querySelectorAll('#skiff *')).find(
            (el) => el.children.length === 0 &&
                    (el.textContent || '').trim() === name);
        return `${name.slice(0, 7)}:${one ? 'kept' : 'gone'}`;
      })
      .join(' ');
const values = () =>
  Array.from(document.querySelectorAll('textarea, input')).map(
      (t) => `${(t.getAttribute('aria-label') || t.type).slice(0, 22)} = ${t.value}`);

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);
console.log('start:', await page.evaluate(titles));

const field = page.locator('textarea[aria-label="Search conversations"]').first();
await field.click();
await page.waitForTimeout(400);
await page.keyboard.type('lemon');
await page.waitForTimeout(1500);
console.log('typed:', await page.evaluate(titles),
            '| field:', (await page.evaluate(values)).find((o) => o.startsWith('Search')));

// Enter: what the field is asked for when it is pressed and not typed into.
await page.keyboard.press('Enter');
await page.waitForTimeout(1500);
console.log('enter:', await page.evaluate(titles),
            '| field:', (await page.evaluate(values)).find((o) => o.startsWith('Search')));
await browser.close();
