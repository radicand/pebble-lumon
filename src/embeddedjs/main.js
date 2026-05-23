import Poco from "commodetto/Poco";

const render = new Poco(screen);
const W = render.width;
const H = render.height;

// ---------- Lumon palette (matched to lumon.industries/intranet/terminal/) ----
const COL_BG       = render.makeColor(0,  29,  47);    // #001D2F
const COL_BG_LINE  = render.makeColor(12, 36,  56);    // scanline alt
const COL_FG       = render.makeColor(189, 255, 255);  // #BDFFFF
const COL_FG_DIM   = render.makeColor(120, 200, 210);  // dim cyan
const COL_FG_DIMR  = render.makeColor(60,  120, 140);  // very dim
const COL_ACCENT   = render.makeColor(220, 255, 230);  // warm pale — KIER

// ---------- Fonts -----------------------------------------------------------
const fontTime   = new render.Font("Bitham-Bold", 42);
const fontTag    = new render.Font("Gothic-Bold", 14);
const fontLabel  = new render.Font("Gothic-Bold", 18);

const DAYS   = ["SUN","MON","TUE","WED","THU","FRI","SAT"];
const MONTHS = ["JAN","FEB","MAR","APR","MAY","JUN",
                "JUL","AUG","SEP","OCT","NOV","DEC"];

// Macrodata Refinement file codenames from the show — rotates each minute.
// (Tumwater, Cairns, Siena... season-1+2 file references.)
const MDR_FILES = ["TUMWATER","CAIRNS","SIENA","ALLENTOWN","WELLINGTON",
                   "PACIFICA","BELLEFONTE","NANTUCKET","DRANESVILLE",
                   "KIER","ZURICH","ASCALON","BILLINGS","BOSTON","COLE",
                   "EAGAN","MILCHICK","COBEL","GAITAN","HELLY"];

// ---------- State -----------------------------------------------------------

// ---------- Helpers ---------------------------------------------------------
// Pseudo-random but deterministic per-day (so the step count is stable
// across re-renders on the same day).
function dayHash(now) {
    const k = now.getFullYear() * 10000 + (now.getMonth() + 1) * 100 + now.getDate();
    let h = k;
    h ^= h << 13; h ^= h >>> 17; h ^= h << 5;
    return (h >>> 0);
}

// Realistic simulated step count: tiny overnight, ramps through the workday,
// finishes around 9–12k. Deterministic per day so doesn't jitter.
function simulatedSteps(now) {
    const totalForDay = 8500 + (dayHash(now) % 3500); // 8500–12000
    const minutes = now.getHours() * 60 + now.getMinutes();
    // S-curve: slow before 7am, steep 7am–7pm, plateau after.
    const t = minutes / 1440;
    // smoothstep between 0.25 and 0.85 of the day
    const a = 0.25, b = 0.85;
    let x = (t - a) / (b - a);
    if (x < 0) x = 0; else if (x > 1) x = 1;
    const s = x * x * (3 - 2 * x);
    return Math.floor(totalForDay * s);
}

function fmtSteps(n) {
    if (n < 1000) return String(n);
    if (n < 10000) return (Math.floor(n / 100) / 10).toFixed(1) + "k";
    return Math.floor(n / 1000) + "k";
}

function drawScanlines() {
    // CRT-style scanlines: every 2nd pixel row is darkened,
    // matching the lumon.industries CSS: linear-gradient 50%/50% at 2px.
    for (let y = 0; y < H; y += 2) {
        render.fillRectangle(COL_BG_LINE, 0, y, W, 1);
    }
}

// ---------- Pixel-drawn LUMON logo ------------------------------------------
// The real Lumon logotype uses a custom font where "O" is a teardrop/raindrop.
// The L has a top bar (bracket-style). Letters are blocky, geometric, uppercase.
// We render pixel-art at G=4 thickness for a 30px-tall logo on the 200px-wide screen.

const G = 4;   // bar / stroke thickness
const SP = 4;  // letter gap

function R(color, x, y, w, h) { render.fillRectangle(color, x, y, w, h); }

// Letter widths
const LW_L = 14, LW_U = 16, LW_M = 20, LW_O = 16, LW_N = 16;
const LOGO_W = LW_L + LW_U + LW_M + LW_O + LW_N + SP * 4;
const LOGO_H = 28;

function drawGlyphL(c, x, y) {
    R(c, x, y, G, LOGO_H);                   // left vertical
    R(c, x, y, 7, G);                        // top serif (bracket)
    R(c, x, y + LOGO_H - G, LW_L, G);       // bottom bar
}

function drawGlyphU(c, x, y) {
    R(c, x, y, G, LOGO_H);                   // left vertical
    R(c, x + LW_U - G, y, G, LOGO_H);       // right vertical
    R(c, x, y + LOGO_H - G, LW_U, G);       // bottom bar
}

function drawGlyphM(c, x, y) {
    R(c, x, y, G, LOGO_H);                   // left vertical
    R(c, x + LW_M - G, y, G, LOGO_H);       // right vertical
    // Inner V-shape meeting at center bottom — draw as stepped diagonals
    const mid = (LW_M / 2) | 0;
    const vDepth = (LOGO_H * 0.55) | 0;  // how far down the V goes
    // Left arm of V: from top-left inner edge down to center
    R(c, x + G, y, 3, 5);
    R(c, x + G + 2, y + 4, 3, 5);
    R(c, x + G + 4, y + 8, 3, 5);
    R(c, x + mid - 2, y + 12, 3, 4);
    // Right arm of V: mirror
    R(c, x + LW_M - G - 3, y, 3, 5);
    R(c, x + LW_M - G - 5, y + 4, 3, 5);
    R(c, x + LW_M - G - 7, y + 8, 3, 5);
    R(c, x + mid - 1, y + 12, 3, 4);
}

// The signature Lumon teardrop "O": pointed at top, rounded bowl at bottom.
function drawGlyphO(c, bg, x, y) {
    const w = LW_O;
    const h = LOGO_H;
    const mid = (w / 2) | 0;

    // Teardrop point at top (narrow, 2px)
    R(c, x + mid - 1, y, 2, 3);
    // Widen outward
    R(c, x + mid - 3, y + 3, 2, 3);
    R(c, x + mid + 1, y + 3, 2, 3);
    R(c, x + mid - 5, y + 5, 2, 3);
    R(c, x + mid + 3, y + 5, 2, 3);

    // Full-width bowl (bottom ~60%)
    const bowlTop = y + 8;
    const bowlH = h - 8;
    R(c, x, bowlTop, G, bowlH);             // left wall
    R(c, x + w - G, bowlTop, G, bowlH);     // right wall
    R(c, x, y + h - G, w, G);               // bottom bar
    // Connect top diagonals to bowl walls
    R(c, x + G, bowlTop, 2, G);
    R(c, x + w - G - 2, bowlTop, 2, G);

    // Hollow "eye" cutout inside the bowl — small circle-like void
    const eyeX = x + mid - 2;
    const eyeY = y + h - 12;
    R(bg, eyeX, eyeY, 4, 4);                // square cutout
    // Round it by filling corners back in
    R(c, eyeX, eyeY, 1, 1);
    R(c, eyeX + 3, eyeY, 1, 1);
    R(c, eyeX, eyeY + 3, 1, 1);
    R(c, eyeX + 3, eyeY + 3, 1, 1);
}

function drawGlyphN(c, x, y) {
    R(c, x, y, G, LOGO_H);                   // left vertical
    R(c, x + LW_N - G, y, G, LOGO_H);       // right vertical
    // Diagonal from top-left to bottom-right
    const steps = 6;
    const stepH = (LOGO_H / steps) | 0;
    for (let i = 0; i < steps; i++) {
        const dx = G + ((LW_N - 2 * G) * i / (steps - 1)) | 0;
        R(c, x + dx, y + i * stepH, 3, stepH + 1);
    }
}

function drawLumonLogo(centerX, y) {
    let x = (centerX - LOGO_W / 2) | 0;
    drawGlyphL(COL_FG, x, y);       x += LW_L + SP;
    drawGlyphU(COL_FG, x, y);       x += LW_U + SP;
    drawGlyphM(COL_FG, x, y);       x += LW_M + SP;
    drawGlyphO(COL_FG, COL_BG, x, y); x += LW_O + SP;
    drawGlyphN(COL_FG, x, y);
}

function drawTime(now, cy) {
    const hh = String(now.getHours()).padStart(2, "0");
    const mm = String(now.getMinutes()).padStart(2, "0");
    const s = `${hh}:${mm}`;
    const tw = render.getTextWidth(s, fontTime);
    const x = ((W - tw) / 2) | 0;
    const y = (cy - fontTime.height / 2) | 0;
    render.drawText(s, fontTime, COL_FG, x, y);

    // Terminal-style brackets either side.
    const hgt = fontTime.height;
    render.fillRectangle(COL_FG_DIM, x - 10, y + 4, 2, hgt - 8);
    render.fillRectangle(COL_FG_DIM, x - 10, y + 4, 6, 2);
    render.fillRectangle(COL_FG_DIM, x - 10, y + hgt - 6, 6, 2);
    render.fillRectangle(COL_FG_DIM, x + tw + 8, y + 4, 2, hgt - 8);
    render.fillRectangle(COL_FG_DIM, x + tw + 4, y + 4, 6, 2);
    render.fillRectangle(COL_FG_DIM, x + tw + 4, y + hgt - 6, 6, 2);
}

function drawDateRow(now, y) {
    const day = DAYS[now.getDay()];
    const mo  = MONTHS[now.getMonth()];
    const dd  = String(now.getDate()).padStart(2, "0");
    const s = `${day} ${mo} ${dd}`;
    const tw = render.getTextWidth(s, fontLabel);
    render.drawText(s, fontLabel, COL_FG, ((W - tw) / 2) | 0, y);
}

function drawStatusRow(now, y) {
    // Left: STEPS 1.5k    Right: rotating MDR file name (or PRAISE KIER on :00)
    const steps = simulatedSteps(now);
    const stepsTxt = `STEPS ${fmtSteps(steps)}`;
    render.drawText(stepsTxt, fontTag, COL_FG_DIM, 10, y);

    let right, rightColor;
    if (now.getMinutes() === 0) {
        // Top of the hour: hail the founder.
        right = "PRAISE KIER";
        rightColor = COL_ACCENT;
    } else {
        const idx = (now.getHours() * 60 + now.getMinutes()) % MDR_FILES.length;
        right = MDR_FILES[idx];
        rightColor = COL_FG_DIM;
    }
    const rw = render.getTextWidth(right, fontTag);
    render.drawText(right, fontTag, rightColor, W - rw - 10, y);
}

function drawDeptBadge(y) {
    const badge = "DEPT MDR";
    const bw = render.getTextWidth(badge, fontTag);
    render.drawText(badge, fontTag, COL_FG_DIMR, ((W - bw) / 2) | 0, y);
}

function drawDividers() {
    render.fillRectangle(COL_FG_DIM,  8, 42,     W - 16, 1);
    render.fillRectangle(COL_FG_DIMR, 8, H - 36, W - 16, 1);
}

function draw(event) {
    const now = (event && event.date) ? event.date : new Date();

    render.begin();
    render.fillRectangle(COL_BG, 0, 0, W, H);
    drawScanlines();

    drawLumonLogo((W / 2) | 0, 8);
    drawDividers();
    drawTime(now, ((H / 2) | 0) + 6);
    drawDateRow(now, ((H / 2) | 0) + 40);
    drawStatusRow(now, H - 30);
    drawDeptBadge(H - 16);

    render.end();
}

// ---------- Lifecycle -------------------------------------------------------
watch.addEventListener("minutechange", draw);
watch.addEventListener("secondchange", draw);

draw();
