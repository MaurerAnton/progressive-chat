// pchat.probe.generation -- What the Generation row says under Responses,
// and whether a press either side of one of the five moves it: the settings
// opened, the five read back out of the text mirror, and one of them pressed
// warmer once.
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
const five = async () => {
  const said = (await mirror()).split('\n').map((one) => one.trim()).filter(Boolean);
  const names = ['Temperature', 'Top P', 'Maximum tokens', 'Presence penalty',
    'Frequency penalty'];
  return names.map((name) => {
    const at = said.indexOf(name);
    return `${name} = ${at < 0 ? 'not said' : (said[at + 1] ?? 'nothing under it')}`;
  });
};
// The settings plate: the right of the drawer, which is 360 wide here.
await page.mouse.click(330, 30);
await page.waitForTimeout(900);
console.log(`said: ${JSON.stringify(await five())}`);
// One press warmer on the first of them: the plate at the right of its row,
// wherever in the column the row stands.
for (const y of [432, 484, 536]) {
  await page.mouse.click(966, y);
  await page.waitForTimeout(700);
  console.log(`pressed + at ${y}: ${JSON.stringify(await five())}`);
}
await page.screenshot({ path: 'generation.png' }).catch(() => {});
await browser.close();
