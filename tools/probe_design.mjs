// pchat.probe.design -- What the design said in the room reads like: the
// page opened, the settings opened on it, the row said Read the design
// pressed, and every word of the column read back out of the text mirror.
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
const context = await browser.newContext({ viewport: { width: 1280, height: 900 } });
const page = await context.newPage();
page.on('console', (m) => console.log(`page: ${m.text()}`));
page.on('pageerror', (e) => console.log(`pageerror: ${e.message}`));
await page.goto('https://progressive-chat.github.io/progressive-chat/', { waitUntil: 'load' });
await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });

const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
console.log(`chat: ${(await mirror()).length} chars`);

// The settings plate: the right of the drawer, which is 360 wide here.
await page.mouse.click(330, 30);
await page.waitForTimeout(800);
let said = await mirror();
console.log(`settings: ${said.length} chars | row offered: ${said.includes('Read the design')}`);

// The row itself: the fourth under the switches, in a page 720 wide.
await page.mouse.click(640, 720);
await page.waitForTimeout(1200);
said = await mirror();
console.log(`design: ${said.length} chars`);
console.log(`title said: ${said.includes('The design of a chat')}`);
console.log(`spoken of: ${said.includes('Settings') === false}`);
console.log(`first: ${JSON.stringify(said.slice(0, 400))}`);
console.log(`said whole: ${JSON.stringify(said.slice(0, 6000))}`);
await page.screenshot({ path: 'design.png' }).catch(() => {});
await browser.close();
