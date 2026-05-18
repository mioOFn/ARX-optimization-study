import os
import re
import sys
from dataclasses import dataclass
from typing import Dict, List, Tuple
 
LINE_RE = re.compile(
    r"^CSV,(?P<group>[^,]+),(?P<variant>[^,]+),lanes=(?P<lanes>\d+),iters=(?P<iters>\d+),reps=(?P<reps>\d+),"
    r"cycles_per_op_median=(?P<cpo_med>[^,]+),cycles_per_op_stddev=(?P<cpo_sd>[^,]+),"
    r"ns_per_op_median=(?P<npo_med>[^,]+),ns_per_op_stddev=(?P<npo_sd>[^,]+)\s*$"
)
 
 
@dataclass(frozen=True)
class Row:
    group: str
    variant: str
    lanes: int
    iters: int
    reps: int
    cycles_per_op_median: float | None
    cycles_per_op_stddev: float | None
    ns_per_op_median: float
    ns_per_op_stddev: float
 
 
def parse(text: str) -> List[Row]:
    rows: List[Row] = []
    for line in text.splitlines():
        m = LINE_RE.match(line.strip())
        if not m:
            continue
        cpo_med_raw = m.group("cpo_med")
        cpo_sd_raw = m.group("cpo_sd")
        cpo_med = None if cpo_med_raw == "N/A" else float(cpo_med_raw)
        cpo_sd = None if cpo_sd_raw == "N/A" else float(cpo_sd_raw)
        rows.append(
            Row(
                group=m.group("group"),
                variant=m.group("variant"),
                lanes=int(m.group("lanes")),
                iters=int(m.group("iters")),
                reps=int(m.group("reps")),
                cycles_per_op_median=cpo_med,
                cycles_per_op_stddev=cpo_sd,
                ns_per_op_median=float(m.group("npo_med")),
                ns_per_op_stddev=float(m.group("npo_sd")),
            )
        )
    return rows
 
 
def to_csv(rows: List[Row]) -> str:
    out = ["group,variant,lanes,iters,reps,cycles_per_op_median,cycles_per_op_stddev,ns_per_op_median,ns_per_op_stddev"]
    for r in rows:
        cpo_med = "" if r.cycles_per_op_median is None else f"{r.cycles_per_op_median:.6f}"
        cpo_sd = "" if r.cycles_per_op_stddev is None else f"{r.cycles_per_op_stddev:.6f}"
        out.append(
            f"{r.group},{r.variant},{r.lanes},{r.iters},{r.reps},{cpo_med},{cpo_sd},{r.ns_per_op_median:.6f},{r.ns_per_op_stddev:.6f}"
        )
    return "\n".join(out) + "\n"
 
 
def make_table(rows: List[Row], group: str) -> List[Row]:
    return sorted([r for r in rows if r.group == group], key=lambda x: (x.variant, x.lanes))
 
 
def svg_line_chart(
    series: Dict[str, List[Tuple[float, float]]],
    title: str,
    xlabel: str,
    ylabel: str,
    width: int = 720,
    height: int = 420,
    margin: int = 60,
) -> str:
    xs = [x for pts in series.values() for (x, _) in pts]
    ys = [y for pts in series.values() for (_, y) in pts]
    if not xs or not ys:
        return ""
    uniq_x = sorted(set(xs))
    xmin, xmax = min(xs), max(xs)
    ymin, ymax = min(ys), max(ys)
    if xmin == xmax:
        xmin = xmin - 1
        xmax = xmax + 1
    if ymin == ymax:
        ymax = ymin + 1
 
    def sx(x: float) -> float:
        return margin + (x - xmin) * (width - 2 * margin) / (xmax - xmin)
 
    def sy(y: float) -> float:
        return height - margin - (y - ymin) * (height - 2 * margin) / (ymax - ymin)
 
    colors = ["#1f77b4", "#d62728", "#2ca02c", "#9467bd", "#ff7f0e"]
    items = list(series.items())
    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">')
    svg.append(f'<rect x="0" y="0" width="{width}" height="{height}" fill="white"/>')
    svg.append(f'<text x="{width/2}" y="28" text-anchor="middle" font-family="Arial" font-size="16">{title}</text>')
    svg.append(f'<line x1="{margin}" y1="{height-margin}" x2="{width-margin}" y2="{height-margin}" stroke="#000"/>')
    svg.append(f'<line x1="{margin}" y1="{margin}" x2="{margin}" y2="{height-margin}" stroke="#000"/>')
    svg.append(f'<text x="{width/2}" y="{height-12}" text-anchor="middle" font-family="Arial" font-size="12">{xlabel}</text>')
    svg.append(
        f'<text x="16" y="{height/2}" text-anchor="middle" font-family="Arial" font-size="12" transform="rotate(-90 16 {height/2})">{ylabel}</text>'
    )
 
    if len(uniq_x) == 1:
        x0 = uniq_x[0]
        px = sx(x0)
        svg.append(f'<line x1="{px}" y1="{height-margin}" x2="{px}" y2="{height-margin+5}" stroke="#000"/>')
        svg.append(f'<text x="{px}" y="{height-margin+20}" text-anchor="middle" font-family="Arial" font-size="10">{x0:.0f}</text>')
    else:
        for t in range(6):
            x = xmin + (xmax - xmin) * t / 5
            px = sx(x)
            svg.append(f'<line x1="{px}" y1="{height-margin}" x2="{px}" y2="{height-margin+5}" stroke="#000"/>')
            svg.append(f'<text x="{px}" y="{height-margin+20}" text-anchor="middle" font-family="Arial" font-size="10">{x:.0f}</text>')
 
    for t in range(6):
        y = ymin + (ymax - ymin) * t / 5
        py = sy(y)
        svg.append(f'<line x1="{margin-5}" y1="{py}" x2="{margin}" y2="{py}" stroke="#000"/>')
        svg.append(f'<text x="{margin-8}" y="{py+3}" text-anchor="end" font-family="Arial" font-size="10">{y:.2f}</text>')
 
    legend_x = width - margin + 10
    legend_y = margin + 10
    for idx, (name, pts) in enumerate(items):
        color = colors[idx % len(colors)]
        pts2 = sorted(pts, key=lambda p: p[0])
        path = " ".join([f"{sx(x):.2f},{sy(y):.2f}" for (x, y) in pts2])
        svg.append(f'<polyline fill="none" stroke="{color}" stroke-width="2" points="{path}"/>')
        for (x, y) in pts2:
            svg.append(f'<circle cx="{sx(x):.2f}" cy="{sy(y):.2f}" r="3" fill="{color}"/>')
        ly = legend_y + idx * 18
        svg.append(f'<rect x="{legend_x}" y="{ly-10}" width="12" height="12" fill="{color}"/>')
        svg.append(f'<text x="{legend_x+16}" y="{ly}" font-family="Arial" font-size="11">{name}</text>')
 
    svg.append("</svg>")
    return "\n".join(svg) + "\n"
 
 
def md_table_ilp(rows: List[Row]) -> str:
    rows = [r for r in rows if r.group == "ILP" and r.cycles_per_op_median is not None]
    lanes = sorted({r.lanes for r in rows})
    by = {(r.variant, r.lanes): r for r in rows}
    out = []
    out.append("| lanes | ADD cycles/op | XOR cycles/op | ADD/XOR |")
    out.append("|---:|---:|---:|---:|")
    for l in lanes:
        a = by.get(("ADD", l))
        x = by.get(("XOR", l))
        if not a or not x:
            continue
        ratio = a.cycles_per_op_median / x.cycles_per_op_median if x.cycles_per_op_median else 0.0
        out.append(f"| {l} | {a.cycles_per_op_median:.3f} | {x.cycles_per_op_median:.3f} | {ratio:.3f} |")
    return "\n".join(out) + "\n"
 
 
def md_table_reg(rows: List[Row]) -> str:
    rows = [r for r in rows if r.group == "REG" and r.cycles_per_op_median is not None]
    by = {(r.variant, r.lanes): r for r in rows}
    lanes = sorted({r.lanes for r in rows})
    out = []
    out.append("| lanes | LOW cycles/op | HIGH cycles/op | HIGH/LOW |")
    out.append("|---:|---:|---:|---:|")
    for l in lanes:
        lo = by.get(("LOW", l))
        hi = by.get(("HIGH", l))
        if not lo or not hi:
            continue
        ratio = hi.cycles_per_op_median / lo.cycles_per_op_median if lo.cycles_per_op_median else 0.0
        out.append(f"| {l} | {lo.cycles_per_op_median:.3f} | {hi.cycles_per_op_median:.3f} | {ratio:.3f} |")
    return "\n".join(out) + "\n"
 
 
def main() -> int:
    if len(sys.argv) < 2:
        print("usage: analyze_arx_microarch.py <bench_output.txt> [out_dir]", file=sys.stderr)
        return 2
    inp = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) >= 3 else os.path.join(os.path.dirname(inp), "out")
    os.makedirs(out_dir, exist_ok=True)
 
    data = open(inp, "rb").read()
    if data.startswith(b"\xff\xfe") or data.startswith(b"\xfe\xff"):
        text = data.decode("utf-16", errors="ignore")
    elif data.startswith(b"\xef\xbb\xbf"):
        text = data.decode("utf-8-sig", errors="ignore")
    else:
        text = data.decode("utf-8", errors="ignore")
    rows = parse(text)
    if not rows:
        print("no rows parsed", file=sys.stderr)
        return 1
 
    open(os.path.join(out_dir, "raw.csv"), "w", encoding="utf-8").write(to_csv(rows))
 
    ilp = [r for r in rows if r.group == "ILP" and r.cycles_per_op_median is not None]
    reg = [r for r in rows if r.group == "REG" and r.cycles_per_op_median is not None]
    addxor = [r for r in rows if r.group == "ADDvXOR" and r.cycles_per_op_median is not None]
 
    open(os.path.join(out_dir, "ilp_table.md"), "w", encoding="utf-8").write(md_table_ilp(rows))
    open(os.path.join(out_dir, "reg_table.md"), "w", encoding="utf-8").write(md_table_reg(rows))
 
    def series_from(group_rows: List[Row]) -> Dict[str, List[Tuple[float, float]]]:
        s: Dict[str, List[Tuple[float, float]]] = {}
        for r in group_rows:
            s.setdefault(r.variant, []).append((float(r.lanes), float(r.cycles_per_op_median)))
        return s
 
    open(os.path.join(out_dir, "ilp.svg"), "w", encoding="utf-8").write(
        svg_line_chart(series_from(ilp), "ILP: independent lanes vs cycles/op", "independent lanes", "cycles per op")
    )
    open(os.path.join(out_dir, "reg.svg"), "w", encoding="utf-8").write(
        svg_line_chart(series_from(reg), "Register pressure: LOW vs HIGH", "lanes", "cycles per op")
    )
    open(os.path.join(out_dir, "addxor.svg"), "w", encoding="utf-8").write(
        svg_line_chart(series_from(addxor), "ADD vs XOR (same topology)", "lanes", "cycles per op")
    )
 
    report = []
    report.append("# ARX microarchitecture microbench report\n")
    report.append("## ILP\n")
    report.append(open(os.path.join(out_dir, "ilp_table.md"), "r", encoding="utf-8").read())
    report.append("## Register pressure\n")
    report.append(open(os.path.join(out_dir, "reg_table.md"), "r", encoding="utf-8").read())
    report.append("## Figures\n")
    report.append("- ilp.svg\n- addxor.svg\n- reg.svg\n")
    open(os.path.join(out_dir, "report.md"), "w", encoding="utf-8").write("\n".join(report))
 
    return 0
 
 
if __name__ == "__main__":
    raise SystemExit(main())
