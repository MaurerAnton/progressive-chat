// pchat.probe.fold -- What a fold above an answer holds, said by pressing it:
// the line as it stands shut, the greentext behind it once it is open, and the
// line whole again once it is shut back. Said of the deployed page, because a
// fold answers only a question a running page can answer -- and found by its
// own words in the mirror rather than by a place somebody guessed.
// This round maps where the fold answers: a dense walk across the pill in
// both directions and across the words, the styles the mirror wrote on both
// elements beside what the browser laid them out as, and one press on the
// second fold. What a press reaches is read from the flip of the fold itself.
// No backticks and no dollar-brace sequences anywhere in this file: the probe
// workflow hands it to the shell inside double quotes, where those belong to
// the shell and not to us.
import { chromium } from 'playwright';

const browser = await chromium.launch({
  args: [
    '--no-sandbox',
    '--disable-dev-shm-usage',
    '--use-gl=angle',
    '--use-angle=swiftshader',
    '--enable-unsafe-swiftshader',
    '--js-flags=--max-old-space-size=3072',
  ],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
const loud = [];
page.on('console', (m) => {
  const kind = m.type();
  if (kind === 'error' || kind === 'warning') loud.push(kind + ': ' + m.text());
});
page.on('pageerror', (e) => loud.push('pageerror: ' + String(e).slice(0, 400)));
// The page itself: the one deployed, where nothing says otherwise -- and the
// one a ref just built, where the workflow serves it (PCHAT_PAGE) and asks it
// what has not reached the deployment yet.
const target = process.env.PCHAT_PAGE || 'https://progressive-chat.github.io/progressive-chat/';
const asked = target + (target.includes('?') ? '&' : '?') + 'asked=' + Date.now();
await page.goto(asked, { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(2500);
const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);

let missed = 0;
const said = (name, holds) => {
  console.log(name + ': ' + holds);
  if (!holds) missed += 1;
};

const thought = 'no recipe of its own known; three reads of the pantry';
const shut = await mirror();
said('line shut', shut.includes('Thought for 4.2 s \u00b7 3 tool calls'));
said('thought behind it', !shut.includes('no recipe of its own known'));
await page.screenshot({ path: 'fold-shut.png' }).catch(() => {});

// Both elements of the first fold, and the words of the second.
const elements = await page.evaluate(() => {
  const pick = (x, y) => {
    const words = document.elementFromPoint(x, y).closest('[data-skiff-h]');
    const pill = words.parentElement;
    const r1 = pill.getBoundingClientRect(), r2 = words.getBoundingClientRect();
    return {
      pill: { h: Number(pill.getAttribute('data-skiff-h')), r: r1, style: pill.getAttribute('style') || '' },
      words: { h: Number(words.getAttribute('data-skiff-h')), r: r2, style: words.getAttribute('style') || '' },
    };
  };
  return { one: pick(463, 286), two: pick(463, 499) };
});
const one = elements.one;
console.log('pill: h=' + one.pill.h + ' dom=[' + Math.round(one.pill.r.left) + ',' +
  Math.round(one.pill.r.top) + ',' + Math.round(one.pill.r.width) + ',' + Math.round(one.pill.r.height) + ']');
console.log('pill style: ' + JSON.stringify(one.pill.style));
console.log('words: h=' + one.words.h + ' dom=[' + Math.round(one.words.r.left) + ',' +
  Math.round(one.words.r.top) + ',' + Math.round(one.words.r.width) + ',' + Math.round(one.words.r.height) + ']');
console.log('words style: ' + JSON.stringify(one.words.style));
console.log('second fold words: h=' + elements.two.words.h);

// One press through the host's own door; whether the first fold's thought
// stands after it -- the fold flips where a press reaches it.
const via = async (handle, x, y) => {
  await page.evaluate((p) => {
    window.Module.ccall('skiff_browser_pointer', null,
      ['number', 'number', 'number', 'number', 'number'], [p.h, 1, p.x, p.y, 1]);
  }, { h: handle, x: x, y: y });
  await page.waitForTimeout(240);
  await page.evaluate((p) => {
    window.Module.ccall('skiff_browser_pointer', null,
      ['number', 'number', 'number', 'number', 'number'], [p.h, 3, p.x, p.y, 1]);
  }, { h: handle, x: x, y: y });
  await page.waitForTimeout(400);
  return (await mirror()).includes(thought);
};

const walkX = async (handle, from, to, step, y) => {
  const trail = [];
  let state = (await mirror()).includes(thought);
  for (let x = from; x <= to; x += step) {
    const after = await via(handle, x, y);
    const flipped = after !== state;
    state = after;
    trail.push(x + (flipped ? (after ? '+' : '-') : '.'));
  }
  return trail.join(' ');
};
const walkY = async (handle, x, from, to, step) => {
  const trail = [];
  let state = (await mirror()).includes(thought);
  for (let y = from; y <= to; y += step) {
    const after = await via(handle, x, y);
    const flipped = after !== state;
    state = after;
    trail.push(y + (flipped ? (after ? '+' : '-') : '.'));
  }
  return trail.join(' ');
};

const pillMidY = one.pill.r.height / 2;
const wordsMidY = one.words.r.height / 2;
console.log('x along the pill: ' + await walkX(one.pill.h, 4, Math.round(one.pill.r.width) - 4, 4, pillMidY));
console.log('y down the pill at x=10: ' + await walkY(one.pill.h, 10, 4, Math.round(one.pill.r.height) - 4, 5));
console.log('x along the words: ' + await walkX(one.words.h, 4, Math.round(one.words.r.width) - 4, 8, wordsMidY));

// Shut it again for the reads after this, at a place that answers.
let open = (await mirror()).includes(thought);
for (let tries = 0; tries < 3 && open; ++tries) {
  open = await via(one.pill.h, 10, pillMidY);
}
console.log('shut at the pill start: ' + !open);

// The second fold, pressed at its words the same way: whether it is the same
// story there. Its own thought is not the first's, so the mirror's growth is
// the reading.
const beforeTwo = await mirror();
await page.evaluate((p) => {
  window.Module.ccall('skiff_browser_pointer', null,
    ['number', 'number', 'number', 'number', 'number'], [p.h, 1, p.x, p.y, 1]);
}, { h: elements.two.words.h, x: Math.round(elements.two.words.r.width / 2), y: Math.round(wordsMidY) });
await page.waitForTimeout(240);
await page.evaluate((p) => {
  window.Module.ccall('skiff_browser_pointer', null,
    ['number', 'number', 'number', 'number', 'number'], [p.h, 3, p.x, p.y, 1]);
}, { h: elements.two.words.h, x: Math.round(elements.two.words.r.width / 2), y: Math.round(wordsMidY) });
await page.waitForTimeout(500);
const afterTwo = await mirror();
console.log('second fold press grew the mirror by: ' + (afterTwo.length - beforeTwo.length));
if (afterTwo.length > beforeTwo.length) {
  // Shut it back for the reads after this.
  await page.evaluate((p) => {
    window.Module.ccall('skiff_browser_pointer', null,
      ['number', 'number', 'number', 'number', 'number'], [p.h, 1, p.x, p.y, 1]);
  }, { h: elements.two.words.h, x: Math.round(elements.two.words.r.width / 2), y: Math.round(wordsMidY) });
  await page.waitForTimeout(240);
  await page.evaluate((p) => {
    window.Module.ccall('skiff_browser_pointer', null,
      ['number', 'number', 'number', 'number', 'number'], [p.h, 3, p.x, p.y, 1]);
  }, { h: elements.two.words.h, x: Math.round(elements.two.words.r.width / 2), y: Math.round(wordsMidY) });
  await page.waitForTimeout(400);
}

// The four reads of this probe: a press where the pill's own edge stands
// (the place that answers today) opens and shuts the fold again.
const fold = page.locator('text=Thought for').first();
const box = await fold.boundingBox();
const edge = { x: box.x - 6, y: box.y + box.height / 2 };
await page.mouse.click(edge.x, edge.y);
await page.waitForTimeout(1100);
const opened = (await mirror()).includes(thought);
said('press opens it', opened);
said('line keeps the count', opened && (await mirror()).includes('Thought for 4.2 s'));
await page.screenshot({ path: 'fold-open.png' }).catch(() => {});
if (opened) {
  await page.mouse.click(edge.x, edge.y);
  await page.waitForTimeout(1100);
}
const again = await mirror();
said('press shuts it', !again.includes(thought));
said('line says the count once more', again.includes('Thought for 4.2 s \u00b7 3 tool calls'));
await page.screenshot({ path: 'fold-closed.png' }).catch(() => {});
console.log('page said: ' + JSON.stringify(loud.slice(0, 10)));

await browser.close();
console.log('summary: ' + missed + ' of 4 answers did not hold');
