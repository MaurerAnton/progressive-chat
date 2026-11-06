import { chromium } from 'playwright';

const page_mirror = () => document.getElementById('skiff').innerText;
const values = () =>
  Array.from(document.querySelectorAll('textarea, input')).map(
      (t) => `${(t.getAttribute('aria-label') || t.type).slice(0, 22)} = ${t.value}`);
const focused = () => {
  const one = document.activeElement;
  return `${one.tagName}:${(one.getAttribute('aria-label') || '').slice(0, 22)}`;
};

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);

const read = async () => await page.evaluate(page_mirror);
const has = (needle, said) => said.includes(needle);

// The drawer's field, pressed where it is and not where it was thought to be.
const field = page.locator('textarea[aria-label="Search conversations"]').first();
const box = await field.boundingBox();
console.log('field box:', JSON.stringify(box));
await field.click();
await page.waitForTimeout(600);
console.log('focus:', await page.evaluate(focused));

const say = async (what) => {
  await page.keyboard.press('Control+a');
  await page.keyboard.type(what);
  await page.waitForTimeout(900);
  const said = await read();
  console.log(`${what || 'nothing'}  kept:`, JSON.stringify({
    kitchen: has('What is in the kitchen', said),
    sieve: has('A sieve of Eratosthenes', said),
    notices: has('Notices', said),
    weekend: has('Weekend', said),
    none: has('No conversation is named that', said),
  }), 'field says:', (await page.evaluate(values)).find((o) => o.startsWith('Search')));
};

await say('lemon');
await say('weekend');
await say('zzzz');
await say('');
await browser.close();
