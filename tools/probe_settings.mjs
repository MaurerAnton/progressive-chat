// pchat.probe.settings -- What the settings page says, and whether what is
// said further down in it can be reached: the page opened, the names of the
// eight groups read back out of the text mirror, the column scrolled to its
// foot, and the row that offers the design pressed there.
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
const mirror = () => page.evaluate(() => document.getElementById('skiff').innerText);
// The settings plate: the right of the drawer, which is 360 wide here.
await page.mouse.click(330, 30);
await page.waitForTimeout(800);
const said = await mirror();
console.log(`settings: ${said.length} chars`);
for (const name of ['Services', 'Providers', 'Models', 'Responses',
  'Context', 'Multimodal', 'Image transcription', 'Tools', 'Web search',
  'Automation', 'Network', 'Proxy', 'Memory & Data', 'Skills',
  'Appearance & Language', 'Language', 'About'])
  console.log(`said ${name}: ${said.includes(name)}`);
// The card stands in the middle of the room and what it holds scrolls: to
// the foot of the column, where the row that offers the design is.
await page.mouse.move(640, 500);
await page.mouse.wheel(0, 4000);
await page.waitForTimeout(800);
const foot = await mirror();
console.log(`scrolled: ${foot.length} chars | moved: ${foot !== said}`);
console.log(`seeds still said: ${foot.includes('Midnight')}`);
console.log(`row offered: ${foot.includes('Read the design')}`);
await page.screenshot({ path: 'settings.png' }).catch(() => {});
await page.mouse.click(640, 700);
await page.waitForTimeout(1200);
const design = await mirror();
console.log(`design: ${design.length} chars | title said: ${design.includes('The design of a chat')}`);
await page.screenshot({ path: 'settings-design.png' }).catch(() => {});
await browser.close();
