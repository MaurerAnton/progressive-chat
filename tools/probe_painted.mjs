// What the deployed page paints, shot and said: a probe of a real browser,
// so that "it is not implemented" can be answered from what a window shows
// and not from what the code seems to say.
import { chromium } from 'playwright';

const url = process.env.PCHAT_PAGE ?? 'https://progressive-chat.github.io/progressive-chat/';

// Two kinds of page: one the worker isolates and so draws on more than one
// thread, and one that blocks it and so is drawn on one -- which is how the
// client answers a host that says no to isolation. Both are asked, because
// which of them a tester's browser got is half of what a report leaves out.
const shoot = async (browser, name, viewport, alone) => {
  const context = await browser.newContext({
    viewport,
    deviceScaleFactor: 1,
    isMobile: alone,
    hasTouch: alone,
    ...(alone ? { serviceWorkers: 'block' } : {}),
  });
  const page = await context.newPage();
  page.on('crash', () => console.log(`crashed[${name}]: the page's renderer went`));
  page.on('close', () => console.log(`closed[${name}]`));
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
  // Said before anything else, and everything after it said even if the page
  // is afterwards taken: a browser given this much to draw has been known to
  // go, and what it said before going is what answers the question.
  try {
    await page.waitForTimeout(6000);
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
  } catch (error) {
    console.log(`gone[${name}]: ${String(error).slice(0, 300)}`);
  }
  await context.close().catch(() => {});
};

// Nothing of a GPU is here to draw with, and a renderer that asks the one a
// runner has not got goes rather than draws: said so to Chromium instead.
const browser = await chromium.launch({
  args: [
    '--no-sandbox',
    '--disable-dev-shm-usage',
    '--disable-gpu',
    '--use-gl=swiftshader',
    '--enable-features=SharedArrayBuffer',
  ],
});
await shoot(browser, 'painted-wide', { width: 1280, height: 900 }, false);
await shoot(browser, 'painted-narrow', { width: 390, height: 844 }, true);
await shoot(browser, 'painted-one-thread', { width: 1280, height: 900 }, false);
await browser.close();
