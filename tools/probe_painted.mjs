// What the deployed page paints, shot and said: a probe of a real browser,
// so that "it is not implemented" can be answered from what a window shows
// and not from what the code seems to say.
import { chromium } from 'playwright';

// An answer a wire would say, written here so that a page can be asked what
// it does with one: what the client asks for is read back out of the request
// it made, and what it did with the answer is read off the screen. Nothing
// here is a server, and nothing here says the client is right -- only what
// it asked for and what it showed.
const stream_body = [
  'data: {"choices":[{"delta":{"reasoning_content":"they asked, so answering\n"},"index":0}]}',
  '',
  'data: {"choices":[{"delta":{"content":"answered"},"index":0}]}',
  '',
  'data: {"choices":[{"delta":{"content":"-by-wire"},"index":0}]}',
  '',
  'data: {"choices":[{"delta":{},"finish_reason":"stop"}],"usage":{"prompt_tokens":7,"completion_tokens":3}}',
  '',
  'data: [DONE]',
  '',
].join('\n');

// The same, said by a wire that stops on the way: it asks for one tool, and
// only answers once the program has answered that. Who asked what is read out
// of every request, so a loop of two askings can be told from one.
const stream_body_stopped = [
  'data: {"choices":[{"delta":{"content":"Looking it up."},"index":0}]}',
  '',
  'data: {"choices":[{"delta":{"tool_calls":[{"index":0,"id":"call_probe","function":{"name":"how_much_has_been_said","arguments":"{}"}}]},"index":0}]}',
  '',
  'data: {"choices":[{"delta":{},"finish_reason":"tool_calls"}],"usage":{"prompt_tokens":9,"completion_tokens":4}}',
  '',
  'data: [DONE]',
  '',
].join('\n');

const stream_body_answered = [
  'data: {"choices":[{"delta":{"content":"asked-twice"},"index":0}]}',
  '',
  'data: {"choices":[{"delta":{},"finish_reason":"stop"}],"usage":{"prompt_tokens":30,"completion_tokens":2}}',
  '',
  'data: [DONE]',
  '',
].join('\n');

let webgl = 'not asked';
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
  // What the client can draw through: it draws through WebGL, and a machine
  // that has none of it is not one that can be asked what the page painted
  // -- it goes inside Skia asking GL what it knows, which is asked of a
  // browser too and not only of a phone.
  webgl = await page.evaluate(() => {
    const one = document.createElement('canvas');
    const gl = one.getContext('webgl2') ?? one.getContext('webgl');
    return gl ? gl.getParameter(gl.VERSION) : 'none';
  });
  console.log(`webgl[${name}]: ${webgl}`);
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

// Nothing of a GPU is here to draw with, so Chromium is told to draw with
// SwiftShader, its own GL in software -- and not told to have no GPU at all,
// which is what it was told before: what the client draws through is WebGL,
// and a renderer asked for none of it asked Skia to ask GL what it knows.
// That is where this program went on a desktop with no window either, and
// it is a browser's answer and not an answer about the page.
// Whether a page asks anything of a wire at all, and what it paints when the
// wire answers: the asking is caught on its way out, so that what a client
// put in its body can be read, and the screen is read afterwards for what it
// did with the answer.
const shoot_answering = async (browser, name) => {
  const context = await browser.newContext({
    viewport: { width: 1280, height: 900 },
    deviceScaleFactor: 1,
  });
  const page = await context.newPage();
  page.on('console', message => console.log(`console[${name}] ${message.type()}: ${String(message.text()).slice(0, 400)}`));
  page.on('pageerror', error => console.log(`pageerror[${name}]: ${String(error).slice(0, 600)}`));
  let asked_url = 'nothing was asked';
  let asked_body = 'nothing was asked';
  await page.route('**/chat/completions', async route => {
    asked_url = route.request().url();
    asked_body = route.request().postData() ?? 'nothing in the body';
    await route.fulfill({
      status: 200,
      headers: { 'Content-Type': 'text/event-stream', 'Access-Control-Allow-Origin': '*' },
      body: stream_body,
    });
  });
  await page.goto(url, { waitUntil: 'load' });
  try {
    await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
    console.log(`started[${name}]: yes`);
  } catch {
    console.log(`started[${name}]: no`);
  }
  await page.waitForTimeout(4000);
  // Said into the composer, the way a reader says something: the window is
  // all one canvas, so where a composer is is not a question a browser can
  // answer, and it is put instead to the bottom of it by pointer, then typed.
  try {
    await page.mouse.click(640, 855);
    await page.keyboard.type('Say something, anything.');
    await page.keyboard.press('Enter');
    await page.waitForTimeout(12000);
  } catch (error) {
    console.log(`said[${name}]: could not -- ${String(error).slice(0, 300)}`);
  }
  console.log(`asked[${name}]: ${asked_url}`);
  console.log(`body[${name}]: ${asked_body.replace(/\s+/g, ' ').slice(0, 1200)}`);
  const said = await page.evaluate(() => {
    const host = document.getElementById('skiff');
    return (host?.innerText ?? host?.textContent ?? '').replace(/\s+/g, ' ').slice(0, 4000);
  });
  console.log(`text[${name}]: ${said}`);
  await page.screenshot({ path: `${name}.png` }).catch(() => {});
  await context.close().catch(() => {});
};

// One question asked twice: the wire stops to ask for a tool, the program
// answers it, and the question is asked again. Two bodies are said back in
// turn, so that the asking that came second can be read -- and the tools it
// was offered read out of the first one.
const shoot_with_tools = async (browser, name) => {
  const context = await browser.newContext({ viewport: { width: 1280, height: 900 }, deviceScaleFactor: 1 });
  const page = await context.newPage();
  page.on('pageerror', error => console.log(`pageerror[${name}]: ${String(error).slice(0, 600)}`));
  let round = 0;
  const bodies = [];
  await page.route('**/chat/completions', route => {
    round += 1;
    bodies.push(route.request().postData() ?? 'nothing in the body');
    console.log(`asked[${name}]: round ${round}`);
    console.log(`body[${name}][${round}]: ${(route.request().postData() ?? '').replace(/\s+/g, ' ').slice(0, 1600)}`);
    return route.fulfill({
      status: 200,
      headers: { 'Content-Type': 'text/event-stream', 'Access-Control-Allow-Origin': '*' },
      body: round === 1 ? stream_body_stopped : stream_body_answered,
    });
  });
  await page.goto(url, { waitUntil: 'load' });
  try {
    await page.waitForFunction(() => window.__pchat_started === true, null, { timeout: 180000 });
    console.log(`started[${name}]: yes`);
  } catch {
    console.log(`started[${name}]: no`);
  }
  await page.waitForTimeout(4000);
  try {
    await page.mouse.click(640, 855);
    await page.keyboard.type('Ask me something you have to look up.');
    await page.keyboard.press('Enter');
    await page.waitForTimeout(16000);
  } catch (error) {
    console.log(`said[${name}]: could not -- ${String(error).slice(0, 300)}`);
  }
  const said = await page.evaluate(() => {
    const host = document.getElementById('skiff');
    return (host?.innerText ?? host?.textContent ?? '').replace(/\s+/g, ' ').slice(0, 4000);
  });
  console.log(`rounds[${name}]: ${round}`);
  console.log(`text[${name}]: ${said}`);
  await page.screenshot({ path: `${name}.png` }).catch(() => {});
  await context.close().catch(() => {});
};

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
await shoot(browser, 'painted-wide', { width: 1280, height: 900 }, false);
await shoot(browser, 'painted-narrow', { width: 390, height: 844 }, true);
await shoot(browser, 'painted-one-thread', { width: 1280, height: 900 }, false);
await shoot_answering(browser, 'painted-answering');
await shoot_with_tools(browser, 'painted-tools');
await browser.close();
