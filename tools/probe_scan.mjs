import { chromium } from 'playwright';

const page_mirror = () => document.getElementById('skiff').innerText;

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
await page.goto(`https://progressive-chat.github.io/progressive-chat/?asked=${Date.now()}`);
await page.waitForFunction('window.__pchat_started === true', null, { timeout: 180000 });
await page.waitForTimeout(2500);
const before = await page.evaluate(page_mirror);
console.log('before:', before.length);

for (let x = 1000; x <= 1270; x += 10) {
  await page.mouse.click(x, 26);
  await page.waitForTimeout(220);
  const said = await page.evaluate(page_mirror);
  if (said.length === before.length && said.includes('What is in the kitchen'))
    continue;
  console.log('x', x, 'len', said.length, 'card:', said.includes('Told to every model'),
              'sonnet:', said.includes('sonnet-4'), 'newchat:', said.includes('New chat'));
  break;
}
const end = await page.evaluate(page_mirror);
console.log('end len', end.length, JSON.stringify(end.slice(-200)));
await browser.close();
