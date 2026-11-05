import { chromium } from 'playwright';

const what = () => {
  const all = Array.from(document.querySelectorAll('textarea, input'));
  const one = all.find((t) => (t.getAttribute('aria-label') || '').startsWith('You are answering'));
  if (!one) return 'no field found';
  const out = [];
  let e = one;
  for (let depth = 0; depth < 4 && e; ++depth, e = e.parentElement) {
    const r = e.getBoundingClientRect();
    out.push(`${depth}:${e.tagName} ${Math.round(r.x)},${Math.round(r.y)} ${Math.round(r.width)}x${Math.round(r.height)}`);
  }
  return out.join(' | ');
};
const values = () =>
  Array.from(document.querySelectorAll('textarea, input')).map(
      (t) => `${(t.getAttribute('aria-label') || t.type).slice(0, 20)}:${t.value}`);

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);
await page.mouse.click(1166, 26);
await page.waitForTimeout(1200);
console.log('rects:', await page.evaluate(what));
for (const y of [388, 392, 396, 404, 420, 450, 480]) {
  await page.mouse.click(640, y);
  await page.waitForTimeout(400);
  await page.keyboard.type('ab');
  await page.waitForTimeout(400);
  const said = await page.evaluate(values);
  console.log('y', y, JSON.stringify(said[2]));
}
await browser.close();
