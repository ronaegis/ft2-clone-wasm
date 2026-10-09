# Deploying the FT2 WASM Port

The port is a static site. Any web server or static host works.

## Files

Upload these four files from `wasm/web` into one directory:

- `index.html`
- `ft2-clone.css`
- `ft2-wasm.js`
- `ft2-wasm.wasm` (about 1.9 MB uncompressed)

Nothing else is required.

## Server requirements

- **MIME type**: serve `.wasm` as `application/wasm`.
- **Compression**: enable gzip or brotli for `.wasm`, `.js`, `.html` and `.css`.
- **Caching**: the file names are not versioned, so if you set long cache lifetimes, users may keep an old `ft2-wasm.js`/`ft2-wasm.wasm` pair after an update.
- **No special headers**: the build uses no threads, no SharedArrayBuffer and no AudioWorklet, so `Cross-Origin-Opener-Policy`/`Cross-Origin-Embedder-Policy` are not needed.
- **HTTPS**: nothing in the port requires it, but it is recommended for any public site.

Nginx example:

```nginx
types {
    application/wasm wasm;
}
gzip on;
gzip_types application/wasm application/javascript text/css;
```

Apache example:

```apache
AddType application/wasm .wasm
AddOutputFilterByType DEFLATE application/wasm application/javascript text/css
```

## Security headers

`index.html` sets its Content-Security-Policy in a `<meta>` tag, so it applies even on hosts where you cannot set HTTP headers. The parts the page depends on:

- `script-src 'self' 'unsafe-inline' 'unsafe-eval'`: the inline startup script and the Emscripten loader.
- `style-src 'self' 'unsafe-inline'`
- `connect-src 'self' https://api.modarchive.org https://modland.com`: the sample module list. Add your own host here if you load modules from elsewhere.
- `img-src 'self' data:`

The meta tag also contains `worker-src 'self' blob:`. The port does not use workers or AudioWorklet, so this directive is not required.

If you can set HTTP headers, these are reasonable additions:

```
X-Content-Type-Options: nosniff
X-Frame-Options: SAMEORIGIN
Referrer-Policy: strict-origin-when-cross-origin
```

`X-Frame-Options` only works as an HTTP header; `index.html` does not and cannot set it. Leave it out (or use CSP `frame-ancestors`) if you want other sites to embed the page in an iframe.

## Cross-origin module loading

The "Load Sample Module" list in `index.html` fetches from api.modarchive.org and modland.com. This works only as long as those hosts allow cross-origin requests; it is outside your control.

If you host module files on a different origin than the page, that origin must send `Access-Control-Allow-Origin` for your site, and the page's `connect-src` must list it.

## Browser storage

Settings saved with "Save config" are stored in the browser's IndexedDB, per origin. Moving the site to another origin, or the user clearing site data, resets them. Files saved from FT2 are delivered as browser downloads and are not stored on the server.

## Troubleshooting

- **WASM does not load**: check the MIME type with `curl -I https://yoursite/ft2-wasm.wasm`, and that `ft2-wasm.wasm` is in the same directory as `ft2-wasm.js`.
- **No audio**: browsers require a user gesture before audio starts; click the canvas or press a key.
- **Audio glitches**: audio runs on the main thread, so heavy UI work can interrupt it.
- **Sample modules do not load**: look for CORS or CSP errors in the browser console (see above).
- **Page freezes during WAV render or resampling**: expected; there are no threads, so long operations block the page until done.
- **No input on phones/tablets**: touch input is not supported.
