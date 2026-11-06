// SPDX-License-Identifier: AGPL-3.0-only
// One question asked of the editor's row: that its plus is there to be
// pressed, that four things come out of it, and that the three dots beside
// the field open too. Nothing of what they say is yet answered on a machine
// that cannot answer it, but all of it is said, which is what a probe can
// read back.
//
// Run instead of tools/probe_painted.mjs where a tree keeps it (.github/
// workflows/probe.yml), and kept only while there is something in this row
// that has never been pressed by anybody.
import { chromium } from 'playwright';

const url = process.env.PCHAT_PAGE ?? 'https://maurerant.github.io/progressive-chat/';

const read = async page =>
  page.evaluate(() => {
    const host = document.getElementById('skiff');
    return (host?.innerText ?? host?.textContent ?? '').replace(/\s+/g, ' ').slice(0, 6000);
  });

const browser = await chromium.launch({
  args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const context = await browser.newContext({ viewport: { width: 1280, height: 900 }, deviceScaleFactor: 1 });
const page = await context.newPage();
page.on('pageerror', error => console.log(`pageerror: ${String(error).slice(0, 400)}`));
await page.goto(url, { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(6000);

const before = await read(page);
console.log(`said before anything was pressed: ${before}`);

// Pressed where it should be, and a little way either side of there: a probe
// that clicks one place has not said whether nothing answers it or whether
// the place was wrong. Each press says what came out of it, and what came out
// of it is how the right place is known -- four things named, and the press
// that named them said back.
const presses = [
  ["the plus at the editor's left", 398, 815, 'Camera'],
  ['the three dots beside the field', 1255, 815, 'Advanced'],
];
for (const [name, x, y, wants] of presses) {
  let said = '';
  const places = [[x, y], [x, y - 12], [x, y + 12], [x - 14, y], [x + 14, y]];
  for (const [at_x, at_y] of places) {
    await page.mouse.click(at_x, at_y);
    await page.waitForTimeout(900);
    said = await read(page);
    if (said.includes(wants)) {
      console.log(`${name}: pressed at ${at_x},${at_y} -- ${wants} came out of it`);
      break;
    }
  }
  if (!said.includes(wants)) {
    console.log(`${name}: pressed at five places and nothing of ${wants} came`);
    continue;
  }
  console.log(`said with it open: ${said}`);
  await page.screenshot({ path: `pressed-${name.replace(/[^a-z]+/g, '-')}.png` });
  // Pressed again, which puts it away.
  await page.mouse.click(x, y);
  await page.waitForTimeout(900);
  const shut = await read(page);
  console.log(`${name}: put away again -- ${shut.includes(wants) ? 'still said' : 'said no more'}`);
}

await context.close();
await browser.close();
