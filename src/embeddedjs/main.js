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
    for (let y = 0; y < H; y += 6) {
        render.fillRectangle(COL_BG_LINE, 0, y, W, 1);
    }
}

// ---------- LUMON logo bitmap -----------------------------------------------
// Rasterized from the real lumon.industries embedded "Lumon Industries" font,
// then reduced to horizontal spans so it can be drawn directly with Poco.
// This preserves the extended geometry and the signature teardrop O.
const LOGO_W = 164;
const LOGO_H = 34;
const LOGO_RECTS = [
    [0,6,6,18],[30,6,5,16],[55,6,6,17],[64,6,6,2],[92,6,6,2],[105,6,24,1],[135,6,6,1],[158,6,6,14],
    [103,7,28,1],[135,7,7,1],[64,8,7,1],[91,8,7,1],[102,8,30,2],[135,8,8,1],[64,9,8,2],[90,9,8,1],[135,9,9,1],
    [89,10,9,2],[101,10,15,2],[118,10,14,2],[135,10,11,1],[64,11,9,1],[135,11,12,1],[64,12,10,1],[88,12,10,1],
    [101,12,14,2],[119,12,13,2],[135,12,13,1],[64,13,11,2],[87,13,11,2],[135,13,14,1],[101,14,13,1],
    [120,14,12,2],[135,14,6,14],[142,14,8,1],[64,15,12,1],[86,15,12,1],[101,15,12,2],[143,15,9,1],
    [64,16,13,1],[85,16,13,1],[121,16,11,2],[144,16,9,1],[64,17,6,11],[71,17,7,1],[84,17,7,1],[92,17,6,11],
    [101,17,11,5],[146,17,8,1],[72,18,6,1],[84,18,6,1],[122,18,10,3],[147,18,9,1],[73,19,6,1],[83,19,6,1],
    [149,19,8,1],[73,20,7,1],[82,20,7,1],[150,20,14,1],[74,21,6,1],[81,21,7,1],[121,21,11,1],[151,21,13,1],
    [30,22,6,1],[75,22,12,2],[101,22,12,1],[120,22,12,2],[152,22,12,1],[30,23,7,1],[54,23,7,1],[101,23,13,1],
    [153,23,11,1],[0,24,26,4],[30,24,30,2],[76,24,10,1],[102,24,30,2],[155,24,9,1],[77,25,8,1],[156,25,8,1],
    [31,26,28,1],[77,26,7,1],[103,26,28,1],[157,26,7,1],[33,27,25,1],[78,27,5,1],[104,27,25,1],[158,27,6,1]
];

function drawLumonLogo(cx, y) {
    const x = Math.trunc(cx - LOGO_W / 2);
    for (const s of LOGO_RECTS) {
        render.fillRectangle(COL_FG, x + s[0], y + s[1], s[2], s[3]);
    }
}

// ---------- MDR number grid with random time/date placement ----------------
// Simple grid of small random digits. Time and date overlay at pseudo-random 
// positions each minute, rendered larger and bolder.

const GRID_TOP    = 44;
const GRID_BOTTOM = H - 34;
const CELL_W      = 18;
const CELL_H      = 19;
const GRID_COLS   = Math.trunc(W / CELL_W);
const GRID_ROWS   = Math.trunc((GRID_BOTTOM - GRID_TOP) / CELL_H);
const GRID_LEFT   = Math.trunc((W - GRID_COLS * CELL_W) / 2);

function cellDigit(col, row, seed) {
    let h = ((col * 73856093) ^ (row * 19349663) ^ (seed * 83492791)) >>> 0;
    h = (h ^ (h >>> 13)) >>> 0;
    h = Math.imul(h, 0x5bd1e995) >>> 0;
    h = (h ^ (h >>> 15)) >>> 0;
    return h % 10;
}

function isReservedCell(row, col, placements) {
    for (const p of placements) {
        if (row === p.row && col >= p.col && col < p.col + p.text.length) return true;
    }
    return false;
}

function drawMDRGrid(now, placements) {
    const seed = now.getHours() * 60 + now.getMinutes();
    for (let row = 0; row < GRID_ROWS; row++) {
        for (let col = 0; col < GRID_COLS; col++) {
            if (isReservedCell(row, col, placements)) continue;

            const d = String(cellDigit(col, row, seed));
            const cx = GRID_LEFT + col * CELL_W + Math.trunc(CELL_W / 2);
            const cy = GRID_TOP + row * CELL_H + Math.trunc(CELL_H / 2);
            const tw = render.getTextWidth(d, fontSmall);
            const th = fontSmall.height;
            render.drawText(d, fontSmall, COL_FG_DIMR, Math.trunc(cx - tw / 2), Math.trunc(cy - th / 2));
        }
    }
}

function randomGridPos(seed, offsetSalt, maxRow, maxCol) {
    const h = ((seed ^ offsetSalt) * 2654435761) >>> 0;
    const row = h % Math.max(1, maxRow);
    const col = Math.trunc(h / maxRow) % Math.max(1, maxCol);
    return { row, col };
}

function overlaps(a, b) {
    return a.row === b.row && a.col < b.col + b.text.length && b.col < a.col + a.text.length;
}

function getGridPlacements(now) {
    const seed = now.getHours() * 60 + now.getMinutes();
    const time = `${String(now.getHours()).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}`;
    const date = `${DAYS[now.getDay()]}${MONTHS[now.getMonth()]}${String(now.getDate()).padStart(2, "0")}`;

    const datePos = randomGridPos(seed, 0xABCDEF00, GRID_ROWS, GRID_COLS - date.length + 1);
    const datePlacement = { text: date, row: datePos.row, col: datePos.col, type: "date" };

    let timePlacement;
    for (let attempt = 0; attempt < 8; attempt++) {
        const timePos = randomGridPos(seed, 0x12345678 + attempt * 997, GRID_ROWS, GRID_COLS - time.length + 1);
        timePlacement = { text: time, row: timePos.row, col: timePos.col, type: "time" };
        if (!overlaps(timePlacement, datePlacement)) break;
    }

    return [datePlacement, timePlacement];
}

function drawGridPlacement(p) {
    const font = (p.type === "time") ? fontMid : fontDate;

    for (let i = 0; i < p.text.length; i++) {
        const ch = p.text[i];
        const col = p.col + i;
        const cx = GRID_LEFT + col * CELL_W + Math.trunc(CELL_W / 2);
        const cy = GRID_TOP + p.row * CELL_H + Math.trunc(CELL_H / 2);
        const tw = render.getTextWidth(ch, font);
        const th = font.height;
        render.drawText(ch, font, COL_FG, Math.trunc(cx - tw / 2), Math.trunc(cy - th / 2));
    }
}

// ---------- Status and badge -------------------------------------------------
function getBatteryPercent() {
    // Try to get real battery percentage from system power state.
    try {
        // Moddable/Pebble may expose power info through global or device modules
        if (globalThis.power?.battery !== undefined) {
            return Math.round(globalThis.power.battery * 100);
        }
    } catch {}
    try {
        // Some environments expose it through a device module
        const device = require("device");
        if (device?.power?.battery !== undefined) {
            return Math.round(device.power.battery * 100);
        }
    } catch {}
    // Fallback: simulate battery discharge based on time of day
    const now = new Date();
    const minutes = now.getHours() * 60 + now.getMinutes();
    const baseLevel = 100;
    const discharge = Math.floor(minutes / 10);
    return Math.max(5, baseLevel - discharge);
}

function drawStatusRow(now, y) {
    const steps = simulatedSteps(now);
    const stepsTxt = `STEPS ${fmtSteps(steps)}`;
    render.drawText(stepsTxt, fontTag, COL_FG_DIM, 10, y);

    // Right: MDR file or PRAISE KIER
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

function drawBatteryAndBadge(y) {
    // Left: battery percentage
    const bat = getBatteryPercent();
    const batTxt = `BAT ${bat}%`;
    render.drawText(batTxt, fontTag, COL_FG_DIM, 10, y);

    // Right: DEPT MDR badge
    const badge = "DEPT MDR";
    const bw = render.getTextWidth(badge, fontTag);
    render.drawText(badge, fontTag, COL_FG_DIMR, W - bw - 10, y);
}

function drawDividers() {
    render.fillRectangle(COL_FG_DIMR, 8, 40,     W - 16, 1);
    render.fillRectangle(COL_FG_DIMR, 8, H - 32, W - 16, 1);
}

// ---------- Draw main screen ------------------------------------------------
function draw(event) {
    const now = event?.date ? event.date : new Date();

    render.begin();
    render.fillRectangle(COL_BG, 0, 0, W, H);
    drawScanlines();

    drawLumonLogo(Math.trunc(W / 2), 2);
    drawDividers();
    const placements = getGridPlacements(now);
    drawMDRGrid(now, placements);
    for (const p of placements) drawGridPlacement(p);
    drawStatusRow(now, H - 28);
    drawBatteryAndBadge(H - 14);

    render.end();
}

// ---------- Lifecycle -------------------------------------------------------
// `minutechange` invokes the callback immediately on registration, so do not
// call draw() again here; the font-derived logo bitmap makes double startup
// rendering too easy to trip the Pebble watchdog.
watch.addEventListener("minutechange", draw);
