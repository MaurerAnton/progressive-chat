// What the deployed page paints, shot and said: a probe of a real browser,
// so that "it is not implemented" can be answered from what a window shows
// and not from what the code seems to say.
import { chromium } from 'playwright';

const url = process.env.PCHAT_PAGE ?? 'https://progressive-chat.github.io/progressive-chat/';

const shoot = async (browser, name, viewport, wants_touch) => {
  const context = await browser.newContext({ viewport, deviceScaleFactor: 1, isMobile: wants_touch, hasTouch: wants_touch });
  const page = await context.newPage();
  page.on('console', message => console.log(`console[${name}] ${message.type()}: ${String(message.text()).slice(0, 400)}`));
  page.on('pageerror', error => console.log(`pageerror[${name}]: ${String(error).slice(0, 600)}`));
  await page.goto(url, { waitUntil: 'load' });
  try {
    await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
    console.log(`started[${name}]: yes`);
  } catch {
    console.log(`started[${name}]: no -- still saying it is starting`);
  }
  // Long enough for the first frames to be painted and the words to be shaped.
  await page.waitForTimeout(15000);
  const said = await page.evaluate(() => {
    const host = document.getElementById('skiff');
    return {
      children: host ? host.querySelectorAll('*').length : 0,
      text: (host?.innerText ?? host?.textContent ?? '').replace(/\s+/g, ' ').slice(0, 4000),
    };
  });
  console.log(`nodes[${name}]: ${said.children}`);
  console.log(`text[${name}]: ${said.text}`);
  await page.screenshot({ path: `${name}.png` });
  await context.close();
};

const browser = await chromium.launch({ args: ['--enable-features=SharedArrayBuffer'] });
await shoot(browser, 'painted-wide', { width: 1280, height: 900 }, false);
await shoot(browser, 'painted-narrow', { width: 390, height: 844 }, true);
await browser.close();
