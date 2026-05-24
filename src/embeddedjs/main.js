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
const fontTime   = new render.Font("Leco-Regular", 42);   // HH:MM
const fontMid    = new render.Font("Leco-Bold", 20);      // random grid time
const fontSmall  = new render.Font("Gothic-Bold", 14);    // grid digits
const fontTag    = new render.Font("Gothic-Bold", 14);    // chrome / labels
const fontDate   = new render.Font("Gothic-Bold", 18);    // date row

const DAYS   = ["SUN","MON","TUE","WED","THU","FRI","SAT"];
const MONTHS = ["JAN","FEB","MAR","APR","MAY","JUN",
                "JUL","AUG","SEP","OCT","NOV","DEC"];

// Macrodata Refinement file codenames from the show — rotates each minute.
const MDR_FILES = ["TUMWATER","CAIRNS","SIENA","ALLENTOWN","WELLINGTON",
                   "PACIFICA","BELLEFONTE","NANTUCKET","DRANESVILLE",
                   "KIER","ZURICH","ASCALON","BILLINGS","BOSTON","COLE",
                   "EAGAN","MILCHICK","COBEL","GAITAN","HELLY"];

// ---------- Helpers ---------------------------------------------------------
function dayHash(now) {
    const k = now.getFullYear() * 10000 + (now.getMonth() + 1) * 100 + now.getDate();
    let h = k;
    h ^= h << 13; h ^= h >>> 17; h ^= h << 5;
    return (h >>> 0);
}

function simulatedSteps(now) {
    const totalForDay = 8500 + (dayHash(now) % 3500);
    const minutes = now.getHours() * 60 + now.getMinutes();
    const t = minutes / 1440;
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
    for (let y = 0; y < H; y += 2) {
        render.fillRectangle(COL_BG_LINE, 0, y, W, 1);
    }
}

// ---------- Pixel-drawn LUMON logo ------------------------------------------
// Clean 16px height with 2px strokes. Teardrop "O" counter.
const G = 2;
const SP = 3;
const LH = 16;

function R(color, x, y, w, h) { render.fillRectangle(color, x, y, w, h); }

const LW_L = 10, LW_U = 12, LW_M = 14, LW_O = 12, LW_N = 12;
const LOGO_W = LW_L + LW_U + LW_M + LW_O + LW_N + SP * 4;

function drawGlyphL(c, x, y) {
    R(c, x, y, G, LH);
    R(c, x, y + LH - G, LW_L, G);
}

function drawGlyphU(c, x, y) {
    R(c, x, y, G, LH);
    R(c, x + LW_U - G, y, G, LH);
    R(c, x, y + LH - G, LW_U, G);
}

function drawGlyphM(c, x, y) {
    R(c, x, y, G, LH);
    R(c, x + LW_M - G, y, G, LH);
    const mid = (LW_M / 2) | 0;
    R(c, x + G, y, 2, 7);
    R(c, x + mid - 1, y + 5, 2, 4);
    R(c, x + LW_M - G - 2, y, 2, 7);
}

function drawGlyphO(c, bg, x, y) {
    const w = LW_O;
    const mid = (w / 2) | 0;
    
    R(c, x + mid, y, 1, 2);
    R(c, x + mid - 1, y + 2, 3, 2);
    R(c, x, y + 5, G, LH - 5);
    R(c, x + w - G, y + 5, G, LH - 5);
    R(c, x, y + LH - G, w, G);
    R(c, x + G, y + 5, mid - 2 * G, G);
    R(c, x + mid + G, y + 5, mid - 2 * G, G);
    
    const eyeX = x + mid - 1;
    const eyeY = y + LH - 8;
    R(bg, eyeX, eyeY, 3, 3);
}

function drawGlyphN(c, x, y) {
    R(c, x, y, G, LH);
    R(c, x + LW_N - G, y, G, LH);
    R(c, x + 3, y + 3, 2, 4);
    R(c, x + 6, y + 7, 2, 4);
    R(c, x + 9, y + 11, 2, 2);
}

function drawLumonLogo(cx, y) {
    let x = (cx - LOGO_W / 2) | 0;
    drawGlyphL(COL_FG, x, y);          x += LW_L + SP;
    drawGlyphU(COL_FG, x, y);          x += LW_U + SP;
    drawGlyphM(COL_FG, x, y);          x += LW_M + SP;
    drawGlyphO(COL_FG, COL_BG, x, y);  x += LW_O + SP;
    drawGlyphN(COL_FG, x, y);
}

// ---------- MDR number grid with random time/date placement ----------------
// Simple grid of small random digits. Time and date overlay at pseudo-random 
// positions each minute, rendered larger and bolder.

const GRID_TOP    = 44;
const GRID_BOTTOM = H - 34;
const CELL_W      = 18;
const CELL_H      = 19;
const GRID_COLS   = (W / CELL_W) | 0;
const GRID_ROWS   = ((GRID_BOTTOM - GRID_TOP) / CELL_H) | 0;
const GRID_LEFT   = ((W - GRID_COLS * CELL_W) / 2) | 0;

function cellDigit(col, row, seed) {
    let h = ((col * 73856093) ^ (row * 19349663) ^ (seed * 83492791)) >>> 0;
    h = (h ^ (h >>> 13)) >>> 0;
    h = Math.imul(h, 0x5bd1e995) >>> 0;
    h = (h ^ (h >>> 15)) >>> 0;
    return h % 10;
}

function drawMDRGrid(now) {
    const seed = now.getHours() * 60 + now.getMinutes();
    for (let row = 0; row < GRID_ROWS; row++) {
        for (let col = 0; col < GRID_COLS; col++) {
            const d = String(cellDigit(col, row, seed));
            const cx = GRID_LEFT + col * CELL_W + ((CELL_W / 2) | 0);
            const cy = GRID_TOP + row * CELL_H + ((CELL_H / 2) | 0);
            const tw = render.getTextWidth(d, fontSmall);
            const th = fontSmall.height;
            render.drawText(d, fontSmall, COL_FG_DIMR, (cx - (tw / 2)) | 0, (cy - (th / 2)) | 0);
        }
    }
}

function randomGridPos(seed, offsetSalt, maxRow, maxCol) {
    const h = ((seed ^ offsetSalt) * 2654435761) >>> 0;
    const row = h % Math.max(1, maxRow);
    const col = ((h / maxRow) | 0) % Math.max(1, maxCol);
    return { row, col };
}

function drawTimeInGrid(now) {
    const seed = now.getHours() * 60 + now.getMinutes();
    const pos = randomGridPos(seed, 0x12345678, Math.max(1, GRID_ROWS - 1), Math.max(1, GRID_COLS - 2));
    
    const hh = String(now.getHours()).padStart(2, "0");
    const mm = String(now.getMinutes()).padStart(2, "0");
    
    const x = GRID_LEFT + pos.col * CELL_W + 2;
    const y = GRID_TOP + pos.row * CELL_H + 1;
    
    render.drawText(hh, fontMid, COL_FG, x, y);
    render.drawText(mm, fontMid, COL_FG, x + ((CELL_W * 1.3) | 0), y);
}

function drawDateInGrid(now) {
    const seed = now.getHours() * 60 + now.getMinutes();
    const pos = randomGridPos(seed, 0xABCDEF00, Math.max(1, GRID_ROWS - 2), Math.max(1, GRID_COLS - 5));
    
    const dStr = `${DAYS[now.getDay()]} ${MONTHS[now.getMonth()]} ${String(now.getDate()).padStart(2, "0")}`;
    const x = GRID_LEFT + pos.col * CELL_W + 2;
    const y = GRID_TOP + pos.row * CELL_H + 2;
    
    render.drawText(dStr, fontTag, COL_FG, x, y);
}

// ---------- Status and badge -------------------------------------------------
function drawStatusRow(now, y) {
    const steps = simulatedSteps(now);
    const stepsTxt = `STEPS ${fmtSteps(steps)}`;
    render.drawText(stepsTxt, fontTag, COL_FG_DIM, 10, y);

    let right, rightColor;
    if (now.getMinutes() === 0) {
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
    render.fillRectangle(COL_FG_DIMR, 8, 38,     W - 16, 1);
    render.fillRectangle(COL_FG_DIMR, 8, H - 32, W - 16, 1);
}

// ---------- Draw main screen ------------------------------------------------
function draw(event) {
    const now = (event && event.date) ? event.date : new Date();

    render.begin();
    render.fillRectangle(COL_BG, 0, 0, W, H);
    drawScanlines();

    drawLumonLogo((W / 2) | 0, 12);
    drawDividers();
    drawMDRGrid(now);
    drawTimeInGrid(now);
    drawDateInGrid(now);
    drawStatusRow(now, H - 28);
    drawDeptBadge(H - 14);

    render.end();
}

// ---------- Lifecycle -------------------------------------------------------
watch.addEventListener("minutechange", draw);

draw();
