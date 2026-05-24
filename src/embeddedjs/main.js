import Poco from "commodetto/Poco";
import Battery from "embedded:sensor/Battery";

const render = new Poco(screen);
const W = render.width;
const H = render.height;
let batterySensor;

const COL_BG = render.makeColor(0, 29, 47);
const COL_FG = render.makeColor(189, 255, 255);
const COL_DIM = render.makeColor(60, 120, 140);
const fontSmall = new render.Font("Gothic-Bold", 14);
const fontMid = new render.Font("Leco-Bold", 20);
const fontDate = new render.Font("Gothic-Bold", 18);

const DAYS = ["SUN","MON","TUE","WED","THU","FRI","SAT"];
const MONTHS = ["JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"];
const MDR_FILES = ["TUMWATER","CAIRNS","SIENA","ALLENTOWN","WELLINGTON","PACIFICA","BELLEFONTE","NANTUCKET","KIER"];

const LOGO_W = 164;
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
    const x = Math.floor(cx - LOGO_W / 2);
    for (let i = 0; i < LOGO_RECTS.length; i++) {
        const s = LOGO_RECTS[i];
        render.fillRectangle(COL_FG, x + s[0], y + s[1], s[2], s[3]);
    }
}

function drawDividers() {
    render.fillRectangle(COL_DIM, 8, 40, W - 16, 1);
    render.fillRectangle(COL_DIM, 8, H - 52, W - 16, 1);
}

const GRID_TOP = 44;
const GRID_BOTTOM = H - 54;
const CELL_W = 22;
const CELL_H = 22;
const GRID_COLS = Math.floor(W / CELL_W);
const GRID_ROWS = Math.floor((GRID_BOTTOM - GRID_TOP) / CELL_H);
const GRID_LEFT = Math.floor((W - GRID_COLS * CELL_W) / 2);

function cellDigit(col, row, seed) {
    let h = ((col * 73856093) ^ (row * 19349663) ^ (seed * 83492791)) >>> 0;
    h = (h ^ (h >>> 13)) >>> 0;
    h = (h * 1540483477) >>> 0;
    h = (h ^ (h >>> 15)) >>> 0;
    return h % 10;
}

function drawMDRGrid(now) {
    const seed = now.getHours() * 60 + now.getMinutes();
    for (let row = 0; row < GRID_ROWS; row++) {
        for (let col = 0; col < GRID_COLS; col++) {
            const d = "" + cellDigit(col, row, seed);
            const cx = GRID_LEFT + col * CELL_W + Math.floor(CELL_W / 2);
            const cy = GRID_TOP + row * CELL_H + Math.floor(CELL_H / 2);
            const tw = render.getTextWidth(d, fontSmall);
            render.drawText(d, fontSmall, COL_DIM, Math.floor(cx - tw / 2), Math.floor(cy - fontSmall.height / 2));
        }
    }
}

function randomGridPos(seed, salt, maxRow, maxCol) {
    const h = ((seed ^ salt) * 2654435761) >>> 0;
    return { row: h % maxRow, col: Math.floor(h / maxRow) % maxCol };
}

function drawGridText(text, row, col, fontToUse) {
    for (let i = 0; i < text.length; i++) {
        const ch = text[i];
        const cx = GRID_LEFT + (col + i) * CELL_W + Math.floor(CELL_W / 2);
        const cy = GRID_TOP + row * CELL_H + Math.floor(CELL_H / 2);
        const tw = render.getTextWidth(ch, fontToUse);
        render.drawText(ch, fontToUse, COL_FG, Math.floor(cx - tw / 2), Math.floor(cy - fontToUse.height / 2));
    }
}

function drawDateTimeInGrid(now) {
    const seed = now.getHours() * 60 + now.getMinutes();
    const time = two(now.getHours()) + ":" + two(now.getMinutes());
    const date = DAYS[now.getDay()] + MONTHS[now.getMonth()] + two(now.getDate());
    const timePos = randomGridPos(seed, 0x12345678, GRID_ROWS, GRID_COLS - time.length + 1);
    const datePos = randomGridPos(seed, 0xABCDEF00, GRID_ROWS, GRID_COLS - date.length + 1);

    drawGridText(time, timePos.row, timePos.col, fontMid);
    drawGridText(date, datePos.row, datePos.col, fontDate);
}

function two(n) {
    return (n < 10 ? "0" : "") + n;
}

function getStepsForDay() {
    return -1;
}

function fmtSteps(n) {
    if (n < 0) return "--";
    if (n < 1000) return "" + n;
    if (n < 10000) return (Math.floor(n / 100) / 10).toFixed(1) + "k";
    return Math.floor(n / 1000) + "k";
}

function getBatteryPercent() {
    try {
        if (!batterySensor) batterySensor = new Battery({});
        const sample = batterySensor.sample();
        return sample ? sample.percent : -1;
    } catch (e) {
        return -1;
    }
}

function drawStatusRows(now) {
    const stepsTxt = "STEPS " + fmtSteps(getStepsForDay());
    render.drawText(stepsTxt, fontDate, COL_DIM, 10, H - 48);

    let right;
    let rightColor = COL_DIM;
    if (now.getMinutes() === 0) {
        right = "PRAISE KIER";
        rightColor = COL_FG;
    } else {
        right = MDR_FILES[(now.getHours() * 60 + now.getMinutes()) % MDR_FILES.length];
    }
    let rw = render.getTextWidth(right, fontDate);
    render.drawText(right, fontDate, rightColor, W - rw - 10, H - 48);

    const bat = getBatteryPercent();
    const batTxt = "BAT " + ((bat < 0) ? "--" : (bat + "%"));
    render.drawText(batTxt, fontDate, COL_DIM, 10, H - 25);

    const badge = "DEPT MDR";
    rw = render.getTextWidth(badge, fontDate);
    render.drawText(badge, fontDate, COL_DIM, W - rw - 10, H - 25);
}

function draw(event) {
    const now = (event && event.date) ? event.date : new Date();

    render.begin();
    render.fillRectangle(COL_BG, 0, 0, W, H);
    drawLumonLogo(Math.floor(W / 2), 2);
    drawDividers();
    drawMDRGrid(now);
    drawDateTimeInGrid(now);
    drawStatusRows(now);
    render.end();
}

watch.addEventListener("minutechange", draw);
