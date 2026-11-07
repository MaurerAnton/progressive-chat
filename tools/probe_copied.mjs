import { chromium } from 'playwright';

// Whether what a Copy chip copies is really given to the machine whose
// browser this page is read in, and not only kept in the program. Nothing
// says so from inside: the host's clipboard is asked for by permission, and
// what is in it is read back with the words that were there before, so that
// a clipboard nobody wrote to says "before anything is pressed" and not the
// answer.

const browser = await chromium.launch();
const context = await browser.newContext({
  viewport: { width: 1280, height: 900 },
  permissions: ['clipboard-read', 'clipboard-write'],
});
const page = await context.newPage();
await page.goto(
    `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(2500);

const read = async () => await page.evaluate(
    () => document.getElementById('skiff').innerText);
// The first Copy chip on the page: its own words, whichever answer it stands
// under, and its left padding rather than its right -- the place a press was
// sure to reach even when the box was no wider than nothing.
const chip = await page.evaluate(() => {
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const own = [...one.childNodes]
                    .map((n) => (n.nodeType === 3 ? n.textContent : ''))
                    .join(' ')
                    .replace(/\s+/g, ' ')
                    .trim();
    if (own !== 'Copy') continue;
    const box = one.getBoundingClientRect();
    if (box.width < 1) continue;
    return [box.x, box.y, box.width, box.height];
  }
  return null;
});
if (!chip) {
  console.log('no Copy was said on the page at all');
  await browser.close();
  process.exit(1);
}

// What the answer above the chip says, which is what is copied: the line of
// it that stands alone, answer's own, above the six chips.
const answer = await read();
await page.evaluate(
    () => navigator.clipboard.writeText('before anything is pressed'));
await page.mouse.click(Math.round(chip[0] + 4), Math.round(chip[1] + chip[3] / 2));
await page.waitForTimeout(900);
const held = await page.evaluate(
    () => navigator.clipboard.readText().catch((e) => 'read refused: ' + e.message));
console.log('pressed: the clipboard keeps', JSON.stringify(String(held).slice(0, 120)));
console.log('said before: it was not there already:',
            String(held) !== 'before anything is pressed',
            '; it is part of what the page says:',
            String(held).length > 8 && answer.includes(String(held).slice(0, 20)));
await browser.close();
