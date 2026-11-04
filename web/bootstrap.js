/* SPDX-License-Identifier: AGPL-3.0-only */
// The page's own part: isolate it, then load the client into it.
(async () => {
  const waiting = document.getElementById('waiting');
  const say = message => {
    waiting.style.display = 'grid';
    waiting.textContent = message;
  };
  try {
    // No service worker, no isolation, no threads: said with what turns them
    // off -- a private window, a privacy setting, a page not over HTTPS --
    // rather than "not supported", which a tester's up-to-date browser was
    // told.
    if (!('serviceWorker' in navigator))
      throw new Error(isSecureContext
        ? 'progressive-chat needs service workers, and this browser has them turned off here: a private window, or a privacy setting or extension, does that. Open progressive-chat in a normal window, or allow service workers for this site.'
        : 'progressive-chat needs a secure (HTTPS) page: open it over https://.');
    await navigator.serviceWorker.register('./worker.js', { scope: './', updateViaCache: 'none' });
    await navigator.serviceWorker.ready;
    if (!navigator.serviceWorker.controller) {
      await new Promise(resolve => navigator.serviceWorker.addEventListener('controllerchange', resolve, { once: true }));
    }
    // Isolated only once the worker serves the page: reloaded for that, twice
    // at most -- a reload that came before the worker took the page (a slow
    // activation) was counted as a failure at once. Where it still is not,
    // what was seen is said, for a report.
    if (!crossOriginIsolated) {
      const tries = Number(sessionStorage.getItem('pchat-isolation-reload') || 0);
      if (tries >= 2) {
        sessionStorage.removeItem('pchat-isolation-reload');
        throw new Error('Cross-origin isolation could not be enabled. Please allow service workers for this site and reload. '
          + `(served by the worker: ${navigator.serviceWorker.controller ? 'yes' : 'no'}; ${navigator.userAgent})`);
      }
      sessionStorage.setItem('pchat-isolation-reload', String(tries + 1));
      location.reload();
      return;
    }
    sessionStorage.removeItem('pchat-isolation-reload');
    // Each deployment's assets are named by what they are made of, so a page
    // left open does not keep asking for a client that has been replaced.
    const response = await fetch('./build.json', { cache: 'no-store' });
    if (!response.ok) throw new Error('Could not load progressive-chat. Please reload.');
    const build = await response.json(), base = new URL('./', location.href);
    const asset = name => {
      const url = new URL(name, base);
      url.searchParams.set('v', build.revision);
      return url.href;
    };
    window.Module = {
      locateFile: asset,
      mainScriptUrlOrBlob: asset(build.program + '.js'),
      printErr(message) { console.error(message); },
      onAbort(reason) { say('progressive-chat could not start: ' + reason); },
      onExit(code) { if (code) say('progressive-chat could not start. Please reload.'); },
      onRuntimeInitialized() {
        waiting.style.display = 'none';
        window.__pchat_started = true;
        dispatchEvent(new Event('pchat-started'));
      }
    };
    // The client is linked `-sEXPORT_ES6`: it is a module, its factory is
    // the module's default export, and what it spawns its threads from is
    // that same module again -- a `new Worker` of `type: 'module'`. Named in
    // a script tag it is parsed as an old script instead: no import.meta to
    // read, no export to give, and so no factory to find -- which is a page
    // still saying that it is starting, with nothing else ever having run.
    const made = await import(asset(build.program + '.js'));
    const start = made.default ?? made.createProgressiveChat;
    if (typeof start !== 'function')
      throw new Error('progressive-chat is not the client it should be. Please reload.');
    await start(window.Module);
  } catch (error) {
    say(error.message || String(error));
  }
})();
