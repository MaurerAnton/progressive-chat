// pchat.probe.generation -- What the Generation row says under Responses,
// and whether a press either side of one of the five moves it: the settings
// opened, the five read back out of the text mirror, and then each row found
// by pressing for it -- where a row stands in the column is not what a probe
// is told, so it is found by what answering to a press looks like.
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
await page.goto('https://progressive-chat.github.io/progressive-chat/', { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
await page.waitForTimeout(1500);
const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
// What the page says of the five: every line that is one of them, as the
// mirror keeps its own line for each row of the column.
const names = ['Temperature', 'Top P', 'Maximum tokens', 'Presence penalty',
  'Frequency penalty'];
const five = async () => {
  const said = (await mirror()).split('\n').map((one) => one.trim()).filter(Boolean);
  return names.map((name) => {
    const at = said.indexOf(name);
    return `${name} = ${at < 0 ? 'not said' : (said[at + 1] ?? 'nothing under it')}`;
  });
};
// The settings plate: the right of the drawer, which is 360 wide here.
await page.mouse.click(330, 30);
await page.waitForTimeout(900);
console.log(`said: ${JSON.stringify(await five())}`);
// Where each of the five rows stands, found by pressing: across the right of
// the card and down the column, until a press moves something and what moved
// says which row it was pressed in.
let before = JSON.stringify(await five());
const found = [];
for (let y = 380; y <= 660 && found.length < 5; y += 4) {
  for (const x of [900, 930, 955, 975]) {
    await page.mouse.click(x, y);
    await page.waitForTimeout(120);
    const now = JSON.stringify(await five());
    if (now !== before) {
      const moved = await five();
      const which = moved.findIndex((one, at) => one !== JSON.parse(before)[at]);
      found.push(`${names[which]} at ${x},${y}: ${moved[which]}`);
      before = now;
      break;
    }
  }
}
console.log(`found: ${JSON.stringify(found)}`);
console.log(`after: ${JSON.stringify(await five())}`);
await page.screenshot({ path: 'generation.png' }).catch(() => {});
await browser.close();
