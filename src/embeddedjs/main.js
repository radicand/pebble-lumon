import Poco from "commodetto/Poco";

const render = new Poco(screen);
const W = render.width;
const H = render.height;

// ---------- Lumon palette ---------------------------------------------------
const COL_BG       = render.makeColor(6,  18,  36);
const COL_BG_LINE  = render.makeColor(20, 42,  68);
const COL_FG       = render.makeColor(170, 232, 232);
const COL_FG_DIM   = render.makeColor(96,  168, 184);
const COL_FG_DIMR  = render.makeColor(56,  112, 128);
const COL_ACCENT   = render.makeColor(220, 235, 215); // warm pale — for KIER

// ---------- Fonts -----------------------------------------------------------
const fontLogo   = new render.Font("Bitham-Black", 30);
const fontTime   = new render.Font("Bitham-Bold", 42);
const fontTag    = new render.Font("Gothic-Bold", 14);
const fontLabel  = new render.Font("Gothic-Bold", 18);
const fontPrompt = new render.Font("Gothic-Bold", 24);

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
let cursorOn = true;

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
    for (let y = 1; y < H; y += 3) {
        render.fillRectangle(COL_BG_LINE, 0, y, W, 1);
    }
}

function drawLumonLogo(cx, y) {
    const text = "LUMON";
    const w = render.getTextWidth(text, fontLogo);
    const x = (cx - w / 2) | 0;
    render.drawText(text, fontLogo, COL_FG, x, y);

    const h = fontLogo.height;
    render.fillRectangle(COL_BG, x - 2, y + ((h * 0.38) | 0), w + 4, 2);
    render.fillRectangle(COL_BG, x - 2, y + ((h * 0.66) | 0), w + 4, 2);

    const tag = "INDUSTRIES";
    const tw = render.getTextWidth(tag, fontTag);
    render.drawText(tag, fontTag, COL_FG_DIM, (cx - tw / 2) | 0, y + h - 2);
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

function drawPrompt(y) {
    const px = 12;
    render.drawText(">", fontPrompt, COL_FG, px, y);
    if (cursorOn) {
        const cw = 12;
        const ch = fontPrompt.height - 8;
        const cx = px + render.getTextWidth("> ", fontPrompt);
        render.fillRectangle(COL_FG, cx, y + 4, cw, ch);
    }
    // Tiny "MDR" department badge on the right of the prompt line.
    const badge = "DEPT MDR";
    const bw = render.getTextWidth(badge, fontTag);
    render.drawText(badge, fontTag, COL_FG_DIMR, W - bw - 10, y + 6);
}

function drawDividers() {
    render.fillRectangle(COL_FG_DIM,  8, 60,     W - 16, 1);
    render.fillRectangle(COL_FG_DIMR, 8, H - 56, W - 16, 1);
    render.fillRectangle(COL_FG_DIM,  8, H - 30, W - 16, 1);
}

function draw(event) {
    const now = (event && event.date) ? event.date : new Date();

    render.begin();
    render.fillRectangle(COL_BG, 0, 0, W, H);
    drawScanlines();

    drawLumonLogo((W / 2) | 0, 6);
    drawDividers();
    drawTime(now, ((H / 2) | 0) - 4);
    drawDateRow(now, ((H / 2) | 0) + 30);
    drawStatusRow(now, H - 50);
    drawPrompt(H - 26);

    render.end();
}

// ---------- Lifecycle -------------------------------------------------------
watch.addEventListener("minutechange", draw);

// 1 Hz cursor blink.
watch.addEventListener("secondchange", (ev) => {
    cursorOn = !cursorOn;
    draw(ev);
});

draw();
