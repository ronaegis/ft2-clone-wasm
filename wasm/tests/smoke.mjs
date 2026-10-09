// Headless smoke test for the FT2 clone WASM build.
//
//   npm install && npx playwright install chromium && npm test
//
// Environment:
//   FT2_WEB_DIR   directory to serve (default: ../web)
//   FT2_BROWSER   Playwright browser channel, e.g. "chrome" to use an installed Chrome
//   FT2_SHOTS     directory to write screenshots of each step into (for debugging)

import { chromium } from 'playwright';
import { createServer } from 'node:http';
import { readFile, mkdir } from 'node:fs/promises';
import { extname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert/strict';

const here = fileURLToPath(new URL('.', import.meta.url));
const webDir = resolve(process.env.FT2_WEB_DIR ?? join(here, '../web'));
const shotsDir = process.env.FT2_SHOTS ? resolve(process.env.FT2_SHOTS) : null;

const SCREEN_W = 632, SCREEN_H = 400;
const CONFIG_FILE = '/home/web_user/.config/FT2 clone/FT2.CFG';

// FT2 screen coordinates of the buttons the test clicks (see src/ft2_pushbuttons.c)
const BTN_ZAP = { x: 322, y: 44 };
const BTN_DISK_OP = { x: 388, y: 95 };
const BTN_CONFIG = { x: 388, y: 146 };
const BTN_DISKOP_SAVE = { x: 99, y: 10 };
const BTN_CONFIG_SAVE = { x: 55, y: 146 };

const MIME = {
	'.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.wasm': 'application/wasm',
};

function serve(dir) {
	const server = createServer(async (req, res) => {
		const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
		try {
			const data = await readFile(join(dir, path === '/' ? 'index.html' : path));
			res.writeHead(200, { 'Content-Type': MIME[extname(path)] ?? 'application/octet-stream' });
			res.end(data);
		} catch {
			res.writeHead(404).end();
		}
	});
	return new Promise((ok) => server.listen(0, '127.0.0.1', () => ok(server)));
}

// Smallest valid XM: 2 channels, one empty 64-row pattern, no instruments
function minimalXM() {
	const buf = Buffer.alloc(60 + 276 + 9);
	buf.write('Extended Module: ', 0, 'latin1');
	buf.write('smoke test'.padEnd(20), 17, 'latin1');
	buf[37] = 0x1a;
	buf.write('FastTracker v2.00   ', 38, 'latin1');
	buf.writeUInt16LE(0x0104, 58); // version
	buf.writeUInt32LE(276, 60);    // header size
	buf.writeUInt16LE(1, 64);      // song length
	buf.writeUInt16LE(0, 66);      // restart position
	buf.writeUInt16LE(2, 68);      // channels
	buf.writeUInt16LE(1, 70);      // patterns
	buf.writeUInt16LE(0, 72);      // instruments
	buf.writeUInt16LE(1, 74);      // flags: linear frequency table
	buf.writeUInt16LE(6, 76);      // speed
	buf.writeUInt16LE(125, 78);    // BPM
	const patt = 60 + 276;
	buf.writeUInt32LE(9, patt);      // pattern header length
	buf[patt + 4] = 0;               // packing type
	buf.writeUInt16LE(64, patt + 5); // rows
	buf.writeUInt16LE(0, patt + 7);  // packed size (0 = empty pattern)
	return [...buf];
}

const sleep = (ms) => new Promise((ok) => setTimeout(ok, ms));

async function main() {
	const server = await serve(webDir);
	const url = `http://127.0.0.1:${server.address().port}/index.html`;

	const browser = await chromium.launch({
		channel: process.env.FT2_BROWSER || undefined,
		args: ['--autoplay-policy=no-user-gesture-required'],
	});
	const context = await browser.newContext({ viewport: { width: 1280, height: 900 }, acceptDownloads: true });
	const page = await context.newPage();

	const errors = [];
	page.on('response', (r) => { if (r.status() >= 400) errors.push(`HTTP ${r.status()}: ${r.url()}`); });
	page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
	page.on('console', (m) => { if (m.type() === 'error') errors.push(`console: ${m.text()}`); });
	page.on('dialog', (d) => { errors.push(`unexpected browser dialog: ${d.message()}`); d.dismiss(); });

	let step = 0;
	const shot = async (name) => {
		if (!shotsDir) return;
		await mkdir(shotsDir, { recursive: true });
		await page.locator('#ft2-canvas').screenshot({ path: join(shotsDir, `${String(++step).padStart(2, '0')}-${name}.png`) });
	};

	// click at FT2 screen coordinates, whatever the canvas is scaled to
	const click = async ({ x, y }) => {
		const box = await page.locator('#ft2-canvas').boundingBox();
		await page.mouse.move(box.x + (x + 0.5) * box.width / SCREEN_W, box.y + (y + 0.5) * box.height / SCREEN_H);
		await sleep(100);
		await page.mouse.down(); // FT2 wants the button held for at least a frame
		await sleep(100);
		await page.mouse.up();
		await sleep(250);
	};

	const boot = async () => {
		await page.goto(url);
		await page.waitForFunction(() => typeof renderLoop === 'function', null, { timeout: 60000 });
		await sleep(500);
	};

	// evaluate() only returns if the main thread is not stuck in a C loop
	const row = () => page.evaluate(() => ft2Module._ft2_get_position_row());
	const dialogOpen = () => page.evaluate(() => !!ft2Module._ft2_is_dialog_open());
	const fileExists = (path) => page.evaluate((p) => {
		try { ft2Module.FS.readFile(p); return true; } catch { return false; }
	}, path);
	const assertPlaying = async (what) => {
		const seen = new Set();
		for (let i = 0; i < 10; i++) { seen.add(await row()); await sleep(150); }
		assert.ok(seen.size >= 3, `${what}: song position should advance (rows seen: ${[...seen]})`);
	};

	console.log('boot');
	await boot();
	await shot('boot');
	const png = await page.locator('#ft2-canvas').screenshot();
	assert.ok(png.length > 8000, `canvas looks blank (screenshot is only ${png.length} bytes)`);
	assert.equal(await fileExists(CONFIG_FILE), false, 'fresh profile should have no saved config');

	console.log('load module and play');
	await page.setInputFiles('#module-file', { name: 'smoke.xm', mimeType: 'application/octet-stream', buffer: Buffer.from(minimalXM()) });
	await page.waitForFunction(() => document.getElementById('load-status-title').textContent === 'Loaded smoke.xm', null, { timeout: 5000 });
	await sleep(500);
	await shot('loaded');
	await assertPlaying('after opening a local file');

	console.log('sample module downloads with progress, then plays');
	await page.route('https://api.modarchive.org/**', (route) => route.fulfill({
		status: 200,
		headers: { 'Content-Type': 'application/octet-stream', 'Access-Control-Allow-Origin': '*' },
		body: Buffer.from(minimalXM()),
	}));
	await page.click('#load-sample-btn');
	await page.click('#sample-module-dropdown .dropdown-item');
	await page.waitForFunction(() => document.getElementById('load-status-title').textContent.startsWith('Now playing'), null, { timeout: 5000 });
	await page.waitForFunction(() => document.getElementById('load-status').hidden, null, { timeout: 5000 });
	await assertPlaying('after loading a sample module');

	console.log('error dialog is a real FT2 dialog and does not freeze the page');
	const badXM = minimalXM();
	badXM[59] = 0x7f; // unsupported XM version: recognised as a module, then rejected by the loader
	await page.evaluate((bytes) => {
		ft2Module.FS.writeFile('/tmp-garbage.xm', new Uint8Array(bytes));
		window.badLoadDone = false;
		ft2Module.ccall('ft2_load_file', null, ['string'], ['/tmp-garbage.xm'], { async: true }).then(() => { window.badLoadDone = true; });
	}, badXM);
	await page.waitForFunction(() => !!ft2Module._ft2_is_dialog_open(), null, { timeout: 5000 });
	await shot('dialog');
	await assertPlaying('while dialog is open');
	await page.keyboard.press('Enter');
	await page.waitForFunction(() => window.badLoadDone, null, { timeout: 5000 });
	assert.equal(await dialogOpen(), false, 'dialog should close on Enter');
	await assertPlaying('after rejected file (the loaded song must be untouched)');

	console.log('multi-button dialog opens from the UI and closes on Escape');
	await click(BTN_ZAP);
	assert.equal(await dialogOpen(), true, 'Zap should open its All/Song/Instruments/Cancel dialog');
	await shot('zap-dialog');
	await assertPlaying('while Zap dialog is open');
	await page.keyboard.press('Escape');
	await sleep(250);
	assert.equal(await dialogOpen(), false, 'Zap dialog should close on Escape');
	await assertPlaying('after cancelling Zap');

	console.log('jamming a note does not hang');
	await page.keyboard.press('KeyQ');
	await assertPlaying('after key jam');

	console.log('saving a module downloads it');
	await click(BTN_DISK_OP);
	await shot('diskop');
	const download = page.waitForEvent('download', { timeout: 10000 });
	await click(BTN_DISKOP_SAVE);
	await shot('diskop-save');
	const file = await download;
	assert.match(file.suggestedFilename(), /\.xm$/i, 'saved module should be offered as .xm');
	const saved = await readFile(await file.path());
	assert.equal(saved.subarray(0, 17).toString('latin1'), 'Extended Module: ', 'download should be a valid XM');
	await click(BTN_DISK_OP); // close disk op

	console.log('config survives a reload');
	await click(BTN_CONFIG);
	await shot('config');
	await click(BTN_CONFIG_SAVE);
	await shot('config-save');
	assert.equal(await fileExists(CONFIG_FILE), true, 'Save config should write FT2.CFG');
	await sleep(1000); // IndexedDB sync
	await boot();
	assert.equal(await fileExists(CONFIG_FILE), true, 'FT2.CFG should be restored from IndexedDB after reload');

	assert.deepEqual(errors, [], 'no errors should be logged by the page');

	await browser.close();
	server.close();
	console.log('OK');
}

main().catch((e) => {
	console.error(e);
	process.exit(1);
});
