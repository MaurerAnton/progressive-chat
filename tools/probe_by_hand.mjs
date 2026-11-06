// SPDX-License-Identifier: AGPL-3.0-only
// One question asked of the things drawn in this tree that no probe has ever
// pressed: the plus at the editor's left and the three dots beside it, the
// fold above an answer, and what a press of Copy keeps for the machine.
//
// Everything is said where it stands, so nothing here needs to know where a
// thing is: the mirror skiff keeps in the page says every line as an element
// of its own, and what is pressed is the middle of the smallest one that says
// what is looked for. Two places are said outright, because they hold an icon
// and no words: the plus and the three dots, whose middles were read out of
// the page once and have not moved.
//
// Run instead of tools/probe_painted.mjs where a tree keeps it (.github/
// workflows/probe.yml), and kept only while one of these has never been read
// back by anybody.
import { chromium } from 'playwright';

const url = process.env.PCHAT_PAGE ?? 'https://maurerant.github.io/progressive-chat/';

const read = async page =>
  page.evaluate(() => {
    const host = document.getElementById('skiff');
    return (host?.innerText ?? host?.textContent ?? '').replace(/\s+/g, ' ').slice(0, 6000);
  });

// What the mirror says, and where it says each of them: the place of every
// element that holds words of its own, read back beside them.
const lines = async page =>
  page.evaluate(() => {
    const host = document.getElementById('skiff');
    if (!host) return [];
    return Array.from(host.querySelectorAll('*'))
        .map((one) => {
          const r = one.getBoundingClientRect();
          return {
            said: (one.textContent ?? '').replace(/\s+/g, ' ').trim(),
            x: Math.round(r.x + r.width / 2),
            y: Math.round(r.y + r.height / 2),
            wide: Math.round(r.width),
            tall: Math.round(r.height),
          };
        })
        .filter((one) => one.said.length > 0 && one.wide > 0 && one.tall > 0)
        .slice(0, 400);
  });

// Pressed in the middle of the smallest thing that says it: what is looked
// for is said once by the line itself and again by everything holding it, so
// the little one is the line.
const press = async (page, wants) => {
  const every = await lines(page);
  const said = every
      .filter((one) => one.said.includes(wants))
      .sort((a, b) => a.wide * a.tall - b.wide * b.tall);
  if (said.length === 0) return false;
  const at = said[0];
  await page.mouse.click(at.x, at.y);
  await page.waitForTimeout(900);
  console.log(`pressed "${wants}" at ${at.x},${at.y} (${at.wide}x${at.tall})`);
  return true;
};

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const context = await browser.newContext({ viewport: { width: 1280, height: 900 }, deviceScaleFactor: 1 });
await context.grantPermissions(['clipboard-read', 'clipboard-write'], { origin: new URL(url).origin });
const page = await context.newPage();
page.on('pageerror', error => console.log(`pageerror: ${String(error).slice(0, 400)}`));
await page.goto(url, { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(6000);

// The two that hold nothing but an icon.
const places = [
  ["the plus at the editor's left", 398, 815, 'Camera'],
  ['the three dots beside the field', 1255, 815, 'Advanced'],
];
for (const [name, x, y, wants] of places) {
  await page.mouse.click(x, y);
  await page.waitForTimeout(900);
  const said = await read(page);
  console.log(
    `${name}: pressed at ${x},${y} -- ` +
      `${said.includes(wants) ? `${wants} came out of it` : `nothing of ${wants} came`}`);
  if (!said.includes(wants)) continue;
  await page.screenshot({ path: `pressed-${name.replace(/[^a-z]+/g, '-')}.png` });
  await page.mouse.click(x, y);
  await page.waitForTimeout(900);
  const shut = await read(page);
  console.log(`${name}: put away again -- ${shut.includes(wants) ? 'still said' : 'said no more'}`);
}

// The fold above the first answer: pressed open, what it wrote to itself is
// read back whole, and pressed again it is folded away again.
const greentext = 'three reads of the pantry';
if (await press(page, 'Thought for')) {
  const open = await read(page);
  console.log(
    `the fold pressed open: ` +
      `${open.includes(greentext) ? 'what it thought is said' : 'what it thought is still not said'}`);
  await page.screenshot({ path: 'pressed-fold.png' });
  await press(page, 'Thought for');
  const shut = await read(page);
  console.log(`the fold pressed again: ${shut.includes(greentext) ? 'still said' : 'said no more'}`);
} else {
  console.log('the fold pressed open: no line said "Thought for" at all');
}

// What a press of Copy keeps for the machine itself, read out of the host.
if (await press(page, 'Copy')) {
  const kept = await page.evaluate(async () => {
    try {
      return await navigator.clipboard.readText();
    } catch (gone) {
      return `nothing read out of it: ${gone}`;
    }
  });
  console.log(`what the host's clipboard keeps: ${JSON.stringify(String(kept).slice(0, 300))}`);
} else {
  console.log("a press of Copy: nothing anywhere said 'Copy'");
}

await context.close();
await browser.close();
