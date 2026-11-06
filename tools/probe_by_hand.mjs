// SPDX-License-Identifier: AGPL-3.0-only
// One question asked of the things drawn in this tree that no probe has ever
// pressed: the plus at the editor's left and the three dots beside it, the
// fold above an answer, and what a press of Copy keeps for the machine. Asked
// beside them, because they are asked of the same page: whether the page even
// starts, in each of the sizes and of the threads a tester's browser has said
// it could not be asked in.
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
            // Kept in the mirror though it is folded away: a line nothing is
            // showing is not one a press can be aimed at.
            shown: one.checkVisibility
                ? one.checkVisibility({ checkOpacity: true, checkVisibilityCSS: true })
                : true,
          };
        })
        .filter((one) => one.said.length > 0 && one.wide > 0 && one.tall > 0 && one.shown)
        .slice(0, 400);
  });

// Every place a line that says it could be pressed at, smallest first: what
// is looked for is said once by the line itself and again by everything
// holding it, so several of them are said of the same words.
const places_saying = async (page, wants) => {
  const every = await lines(page);
  return every
      .filter((one) => one.said.includes(wants))
      .sort((a, b) => a.wide * a.tall - b.wide * b.tall)
      .slice(0, 5);
};

// Pressed in the middle of the smallest thing that says it.
const press = async (page, wants) => {
  const said = await places_saying(page, wants);
  if (said.length === 0) return null;
  const at = said[0];
  await page.mouse.click(at.x, at.y);
  await page.waitForTimeout(900);
  console.log(`pressed "${wants}" at ${at.x},${at.y} (${at.wide}x${at.tall})`);
  return at;
};

// Pressed until something else is said, one place after another: a line keeps
// what a press is for inside a row of its own, and which of them the press
// belongs to -- the words, the row, the whole card -- is not said anywhere but
// by pressing them.
const press_until = async (page, wants, came) => {
  const said = await places_saying(page, wants);
  if (said.length === 0) {
    console.log(`nothing anywhere said "${wants}"`);
    return null;
  }
  for (const at of said) {
    await page.mouse.click(at.x, at.y);
    await page.waitForTimeout(900);
    console.log(`pressed "${wants}" at ${at.x},${at.y} (${at.wide}x${at.tall})`);
    const after = await came();
    if (after) return { at, after };
    // Taken back where it was, for the next place is asked of the page as it
    // was before anything was pressed at all.
    await page.mouse.click(at.x, at.y);
    await page.waitForTimeout(500);
  }
  return null;
};

// Whether it starts at all, asked of one kind of browser at a time: how wide
// the room is, whether the browser says it is a phone, and whether the worker
// that isolates the client is kept. Narrow and alone together is the one that
// has never answered yes, and nobody has said which of the three stops it.
const starts = async (browser, name, viewport, phone, alone) => {
  const context = await browser.newContext({
    viewport,
    deviceScaleFactor: 1,
    isMobile: phone,
    hasTouch: phone,
    ...(alone ? { serviceWorkers: 'block' } : {}),
  });
  const page = await context.newPage();
  page.on('console', message =>
    console.log(`console[${name}] ${message.type()}: ${String(message.text()).slice(0, 300)}`));
  page.on('pageerror', error => console.log(`pageerror[${name}]: ${String(error).slice(0, 400)}`));
  await page.goto(url, { waitUntil: 'load' });
  let started = 'no -- still saying it is starting';
  try {
    await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 120000 });
    started = 'yes';
  } catch {
    // Said as it is said by every probe before it.
  }
  await page.waitForTimeout(4000);
  const said = await page.evaluate(() => {
    const host = document.getElementById('skiff');
    return {
      nodes: host ? host.querySelectorAll('*').length : 0,
      text: (host?.innerText ?? host?.textContent ?? '').replace(/\s+/g, ' ').slice(0, 400),
    };
  });
  console.log(`started[${name}]: ${started} -- ${said.nodes} nodes -- ${said.text}`);
  await page.screenshot({ path: `started-${name}.png` }).catch(() => {});
  await context.close().catch(() => {});
};

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});

for (const [name, viewport, phone, alone] of [
  ['narrow-and-alone', { width: 390, height: 844 }, true, true],
  ['narrow-and-a-phone-only', { width: 390, height: 844 }, true, false],
  ['narrow-and-alone-only', { width: 390, height: 844 }, false, true],
  ['narrow-and-neither', { width: 390, height: 844 }, false, false],
  ['half-of-a-room-and-alone', { width: 700, height: 900 }, true, true],
])
  await starts(browser, name, viewport, phone, alone);

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
//
// Read out of the mirror and not of the page as one text: what the scene has
// put away keeps its words in the mirror, so a page read as one text says
// them whether they are showing or not, and a fold would look open before
// anything was pressed at it.
const greentext = 'three reads of the pantry';
const shown_text = async (page) => (await lines(page)).map((one) => one.said).join(' ');
// What came out of nothing: the words the page shows that it did not show a
// moment ago.
const came_of = (was, now) => {
  let at = 0;
  while (at < was.length && at < now.length && was[at] === now[at]) at++;
  let to = 0;
  while (to < was.length - at && to < now.length - at &&
         was[was.length - 1 - to] === now[now.length - 1 - to])
    to++;
  return now.slice(at, now.length - to);
};
const before = await shown_text(page);
const opened = await press_until(page, 'Thought for', async () => {
  const open = await shown_text(page);
  const came = came_of(before, open);
  if (came.length <= 8) return null;
  console.log(`came out of pressing the fold: ${came.slice(0, 1200)}`);
  return open;
});
if (opened) {
  console.log(
    `the fold pressed open at ${opened.at.x},${opened.at.y}: ` +
      `${opened.after.includes(greentext) ? 'what it thought is said' : 'it is open, and what it said was other than expected'}`);
  await page.screenshot({ path: 'pressed-fold.png' });
  await page.mouse.click(opened.at.x, opened.at.y);
  await page.waitForTimeout(900);
  const shut = await shown_text(page);
  console.log(`the fold pressed again: ${came_of(opened.after, shut).length === 0 && shut.length < opened.after.length ? 'folded away again' : 'still open'}`);
} else {
  console.log('the fold pressed open: no press anywhere opened it');
}

// That this browser's clipboard can be written and read at all, asked before
// the program is asked to write to it: a probe that is given no leave to read
// it would say of every program that its copy does nothing.
const held = await page.evaluate(async () => {
  try {
    await navigator.clipboard.writeText('written by the probe itself');
    return await navigator.clipboard.readText();
  } catch (gone) {
    return `this browser lets nothing be read back: ${gone}`;
  }
});
console.log(`the clipboard read by the probe itself: ${JSON.stringify(String(held).slice(0, 200))}`);
await page.evaluate(async () => {
  try {
    await navigator.clipboard.writeText('');
  } catch (gone) {
    // Left as it was: the next line is the first thing written since.
  }
});

// What a press of Copy keeps for the machine itself, read out of the host.
const kept_text = async () => {
  await page.waitForTimeout(1200);
  const kept = await page.evaluate(async () => {
    try {
      return await navigator.clipboard.readText();
    } catch (gone) {
      return '';
    }
  });
  return kept.length > 0 ? kept : null;
};
const copied = await press_until(page, 'Copy', kept_text);
console.log(
  `what the host's clipboard keeps: ` +
    `${copied ? JSON.stringify(String(copied.after).slice(0, 400)) : 'nothing at all, whichever Copy was pressed'}`);

// Whether a press inside the room reaches anything at all. The two asked of
// it above -- the fold, and Copy under an answer -- both came back with
// nothing said and nothing kept, and which they are, two things that do
// nothing of their own or one room no press reaches into, is said by neither
// of them alone. So: one of the six pressed where only an answer stands, and
// what it did read as what is no longer said in the room.
const spoken = await shown_text(page);
const rubbed = await press_until(page, 'Delete', async () => {
  const now = await shown_text(page);
  return now.length < spoken.length - 40 ? now : null;
});
console.log(
  `a press inside the room: ` +
    `${rubbed ? 'it reaches what stands there, and one answer is gone' : 'nothing of the room is reached by a press at all'}`);

await context.close();
await browser.close();
