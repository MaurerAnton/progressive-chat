import { chromium } from 'playwright';

// What a press on the fold above an answer does: what it thought, written in
// green under the line that says how long it thought, coming and going as
// often as it is pressed.
//
// The place of the press is read off the page and not written here, for the
// same reason nothing else here is: what the fold says is pressed is whatever
// element under the pointer holds those words, at whatever width the window
// was opened at, and a point written down for one width is a point in nothing
// at another.

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(
    `https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null,
                           { timeout: 180000 });
await page.waitForTimeout(2500);

const read = async () => await page.evaluate(
    () => document.getElementById('skiff').innerText);
// The fold of the first answer, by the words in it rather than by where they
// stand: "Thought for", which is the only thing said above an answer.
const fold = async () => await page.evaluate(() => {
  for (const one of document.getElementById('skiff').querySelectorAll('*')) {
    const own = [...one.childNodes]
                    .map((n) => (n.nodeType === 3 ? n.textContent : ''))
                    .join(' ')
                    .replace(/\s+/g, ' ')
                    .trim();
    if (!own.startsWith('Thought for')) continue;
    const box = one.getBoundingClientRect();
    if (box.width < 1) continue;
    return [box.x, box.y, box.width, box.height, own];
  }
  return null;
});
const start = await fold();
console.log('start: the fold says "' + (start ? start[4] : 'nothing') +
            '", ' + (start ? `${Math.round(start[2])}x${Math.round(start[3])}`
                           : 'no box') +
            '; green read: ' + (await read()).includes('no recipe of its own'));

if (!start) {
  console.log('no fold was said on the page at all');
  await browser.close();
  process.exit(1);
}

// Pressed in the middle of what it says, which is where a reader presses it,
// and not in its left padding -- the only place a box too narrow for its own
// words ever answered a press.
await page.mouse.click(Math.round(start[0] + start[2] / 2),
                       Math.round(start[1] + start[3] / 2));
await page.waitForTimeout(1200);
const open = await read();
const said = await fold();
console.log('pressed once: green read: ' + open.includes('no recipe of its own'),
            'and the fold says "' + (said ? said[4] : 'nothing') + '"');

await page.mouse.click(Math.round(start[0] + start[2] / 2),
                       Math.round(start[1] + start[3] / 2));
await page.waitForTimeout(1200);
const shut = await read();
console.log('pressed twice: green read: ' +
            shut.includes('no recipe of its own'));
await page.screenshot({ path: 'fold.png' });
await browser.close();
