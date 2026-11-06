// SPDX-License-Identifier: GPL-3.0-only
// One question asked of what Memory says: everything said in the conversation
// that is open taken away by a press on the row that offers it, and every
// conversation taken away by the row beside it -- each of them said before it
// goes with how much there is of it, and nothing of either asked back
// afterwards. What is read is what the page shows: the mirror skiff keeps says
// every line as an element of its own, and what the scene has put away keeps
// its words in it, so a line is read here only where the page is showing it.
//
// The rows stand low in Settings, which scrolls: they are looked for where
// they are showing, and where they are not, the column is wheeled down until
// they are. Nothing here knows where anything stands.
import { chromium } from 'playwright';

const url = process.env.PCHAT_PAGE ?? 'https://progressive-chat.github.io/progressive-chat/';

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
page.on('pageerror', error => console.log(`pageerror: ${String(error).slice(0, 400)}`));
await page.goto(url, { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(6000);

// What the mirror says, and where it says each of them, read one element at a
// time: the middle of the place it stands, and whether the page is showing it.
const lines = () =>
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
            shown: one.checkVisibility
                ? one.checkVisibility({ checkOpacity: true, checkVisibilityCSS: true })
                : true,
            // Kept out of what is read: a line pressed from outside the room
            // is a line no finger could have reached.
            in_room: r.top >= 0 && r.bottom <= window.innerHeight &&
                     r.left >= 0 && r.right <= window.innerWidth,
          };
        })
        .filter((one) => one.said.length > 0 && one.wide > 0 && one.tall > 0 &&
                         one.shown && one.in_room)
        .slice(0, 400);
  });
const shown_text = async () => (await lines()).map((one) => one.said).join(' ');

// Every place inside the room that says it, smallest first: what is looked for
// is said once by the line itself and again by everything holding it.
const places_saying = async (wants) => {
  const every = await lines();
  return every.filter((one) => one.said.includes(wants))
      .sort((a, b) => a.wide * a.tall - b.wide * b.tall)
      .slice(0, 5);
};

// Rolled down until what is looked for stands in the room, or until there is
// nowhere further to roll: the card holds a column twice the room's height.
const find_by_rolling = async (wants) => {
  for (let rolled = 0; rolled <= 10; rolled++) {
    const said = await places_saying(wants);
    if (said.length > 0) return said;
    await page.mouse.move(640, 500);
    await page.mouse.wheel(0, 400);
    await page.waitForTimeout(400);
  }
  return [];
};

// Pressed once, in the middle of the smallest thing inside the room that says
// it.
const press = async (wants) => {
  for (const at of await find_by_rolling(wants)) {
    await page.mouse.click(at.x, at.y);
    await page.waitForTimeout(900);
    console.log(`pressed "${wants}" at ${at.x},${at.y} (${at.wide}x${at.tall})`);
    return at;
  }
  console.log(`nothing in the room said "${wants}"`);
  return null;
};

// What the room says before anything is taken from it, and what of it is gone
// when it has been: one answer whole, and the name of the conversation it was
// said in.
const spoken = await shown_text();
const answer = 'lemony egg on toast';
const named = 'kitchen';
console.log(`the room as it was: ${spoken.includes(answer) ? 'an answer is said in it' : 'no answer is said in it at all'}`);

// Settings, opened the way a finger opens it.
await press('Settings');
const opened = await shown_text();
console.log(`settings opened: ${opened.includes('Appearance') ? 'yes' : 'no -- nothing that says Appearance came'}`);

// How much there is of it, said beside the row before anything is taken: the
// number Memory counted of what the window is holding.
const rows = await find_by_rolling('taken away');
for (const row of rows.slice(0, 2))
  console.log(`memory says: ${row.said.slice(0, 120)}`);

// Everything said in the conversation that is open, taken away.
if (await press('Everything said here, taken away')) {
  const after = await shown_text();
  console.log(
    `everything said here, taken away: ` +
      `${!after.includes(answer) ? 'the answer is said no more' : 'the answer is still said'} -- ` +
      `${after.includes(named) ? 'and the conversation is still named' : 'and the conversation is named no more'}`);
  await page.screenshot({ path: 'forgotten-said.png' }).catch(() => {});
}

// Every conversation, taken away: what is left of the room afterwards.
if (await press('Every conversation, taken away')) {
  const after = await shown_text();
  console.log(
    `every conversation, taken away: ` +
      `${!after.includes(named) ? 'nothing is named anywhere' : 'the room still names one'} -- ` +
      `${after.includes('Appearance') ? 'and settings is still open' : 'and settings went away with them'}`);
  console.log(`what the room says now: ${JSON.stringify(after.slice(0, 300))}`);
  await page.screenshot({ path: 'forgotten-everything.png' }).catch(() => {});
}

await page.screenshot({ path: 'forgotten.png' }).catch(() => {});
await browser.close();
