#!/usr/bin/env python3
"""Calculate expected C-star mass bounds from theory/limit crossings.

The Brazil plots use sigma_limit = r * sigma_theory with B(c* -> c gamma)=1.
Consequently the crossing occurs at r=1.  Between simulated mass points this
script interpolates linearly in log(sigma_limit / sigma_theory), equivalently
in log(r), which is appropriate for the log-scale cross-section plot.
"""

from __future__ import annotations

import argparse
import csv
import math
import re
import sys
from dataclasses import dataclass
from pathlib import Path


COUPLINGS = ("f0p1", "f0p5", "f1p0")
FILE_NAME = re.compile(
    r"higgsCombine_(?P<coupling>f\d+p\d+)\.AsymptoticLimits\.mH(?P<mass>\d+)\.root"
)
QUANTILES = {
    "expected_minus2sigma": 0.025,
    "expected_minus1sigma": 0.16,
    "expected": 0.5,
    "expected_plus1sigma": 0.84,
    "expected_plus2sigma": 0.975,
}


@dataclass(frozen=True)
class Point:
    mass_gev: int
    theory_pb: float
    limits: dict[str, float]


@dataclass(frozen=True)
class Crossing:
    mass_gev: float
    low: Point
    high: Point


def arguments() -> argparse.Namespace:
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description="Calculate expected mass bounds from Combine AsymptoticLimits outputs."
    )
    parser.add_argument(
        "-f",
        "--coupling",
        action="append",
        choices=COUPLINGS,
        help="coupling to calculate; may be repeated (default: all three)",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=Path("/eos/user/h/hsiaoche/workspace"),
        help="directory containing higgsCombine_*.root files",
    )
    parser.add_argument(
        "--cross-sections",
        type=Path,
        default=here.parent / "Signal" / "cross_sections.csv",
        help="theory cross-section CSV used by the Brazil plots",
    )
    parser.add_argument(
        "--csv-output",
        type=Path,
        help="optionally write the calculated crossings to a CSV file",
    )
    return parser.parse_args()


def read_cross_sections(path: Path) -> dict[tuple[str, int], float]:
    values: dict[tuple[str, int], float] = {}
    with path.open(newline="") as handle:
        for row in csv.reader(line for line in handle if not line.lstrip().startswith("#")):
            if not row:
                continue
            if len(row) != 5:
                raise ValueError(f"{path}: expected 5 columns, got {row}")
            model, mass, coupling, cross_section, _uncertainty = row
            if model == "CstarToGJ":
                values[(coupling, int(mass))] = float(cross_section)
    return values


def read_limits(path: Path) -> dict[str, float]:
    try:
        import ROOT  # type: ignore
    except ImportError as error:
        raise RuntimeError(
            "PyROOT is required; initialize the CMSSW environment before running"
        ) from error
    ROOT.gROOT.SetBatch(True)
    ROOT.gErrorIgnoreLevel = ROOT.kFatal + 1
    root_file = ROOT.TFile.Open(str(path), "READ")
    if not root_file or root_file.IsZombie():
        raise RuntimeError(f"cannot open {path}")
    try:
        tree = root_file.Get("limit")
        if not tree:
            raise RuntimeError(f"{path}: missing 'limit' tree")
        result: dict[str, float] = {}
        for entry in tree:
            quantile = float(entry.quantileExpected)
            for label, target in QUANTILES.items():
                if abs(quantile - target) < 1.0e-5:
                    result[label] = float(entry.limit)
        missing = set(QUANTILES) - set(result)
        if missing:
            raise RuntimeError(f"{path}: missing expected quantiles {sorted(missing)}")
        if any(value <= 0.0 or not math.isfinite(value) for value in result.values()):
            raise RuntimeError(f"{path}: limits must be finite and positive")
        return result
    finally:
        root_file.Close()


def read_points(
    work_dir: Path,
    coupling: str,
    cross_sections: dict[tuple[str, int], float],
) -> list[Point]:
    paths = []
    for path in work_dir.glob(f"higgsCombine_{coupling}.AsymptoticLimits.mH*.root"):
        match = FILE_NAME.fullmatch(path.name)
        if match and match["coupling"] == coupling:
            paths.append((int(match["mass"]), path))
    if not paths:
        raise FileNotFoundError(f"no AsymptoticLimits files found for {coupling} in {work_dir}")
    points = []
    for mass, path in sorted(paths):
        key = (coupling, mass)
        if key not in cross_sections:
            raise ValueError(f"no theory cross section for {coupling}, M={mass} GeV")
        points.append(Point(mass, cross_sections[key], read_limits(path)))
    return points


def find_crossings(points: list[Point], label: str) -> list[Crossing]:
    crossings = []
    for low, high in zip(points, points[1:]):
        low_ratio = low.limits[label]
        high_ratio = high.limits[label]
        low_log = math.log(low_ratio)
        high_log = math.log(high_ratio)
        if low_log == 0.0:
            crossings.append(Crossing(float(low.mass_gev), low, low))
        elif low_log * high_log < 0.0:
            fraction = -low_log / (high_log - low_log)
            mass = low.mass_gev + fraction * (high.mass_gev - low.mass_gev)
            crossings.append(Crossing(mass, low, high))
    if math.log(points[-1].limits[label]) == 0.0:
        crossings.append(Crossing(float(points[-1].mass_gev), points[-1], points[-1]))
    return crossings


def exclusion_boundary(points: list[Point], label: str) -> Crossing | None:
    # The reported lower mass bound is the highest transition from an excluded
    # point (limit/theory < 1) to an allowed point (limit/theory > 1).
    candidates = [
        crossing
        for crossing in find_crossings(points, label)
        if crossing.low.limits[label] <= 1.0 and crossing.high.limits[label] >= 1.0
    ]
    return max(candidates, key=lambda crossing: crossing.mass_gev, default=None)


def no_crossing_status(points: list[Point], label: str) -> str:
    ratios = [point.limits[label] for point in points]
    if all(ratio > 1.0 for ratio in ratios):
        return f"no exclusion in scan (mass bound < {points[0].mass_gev / 1000.0:.3f} TeV)"
    if all(ratio < 1.0 for ratio in ratios):
        return f"crossing above scan (mass bound > {points[-1].mass_gev / 1000.0:.3f} TeV)"
    return "no standard excluded-to-allowed crossing in scanned range"


def main() -> int:
    args = arguments()
    couplings = args.coupling or list(COUPLINGS)
    try:
        cross_sections = read_cross_sections(args.cross_sections)
        point_sets = {
            coupling: read_points(args.work_dir, coupling, cross_sections)
            for coupling in couplings
        }
    except (OSError, RuntimeError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1

    rows = []
    print("Expected 95% CL lower mass bounds")
    print("Interpolation: linear in log(limit/theory) between simulated masses\n")
    for coupling in couplings:
        points = point_sets[coupling]
        print(f"{coupling}:")
        for label in QUANTILES:
            crossing = exclusion_boundary(points, label)
            if crossing is None:
                status = no_crossing_status(points, label)
                print(f"  {label:22} {status}")
                rows.append((coupling, label, "", "", "", status))
                continue
            mass_tev = crossing.mass_gev / 1000.0
            bracket = f"M{crossing.low.mass_gev}-M{crossing.high.mass_gev}"
            if crossing.low.mass_gev == crossing.high.mass_gev:
                theory_at_crossing = crossing.low.theory_pb
            else:
                fraction = (
                    (crossing.mass_gev - crossing.low.mass_gev)
                    / (crossing.high.mass_gev - crossing.low.mass_gev)
                )
                theory_at_crossing = math.exp(
                    math.log(crossing.low.theory_pb)
                    + fraction
                    * (math.log(crossing.high.theory_pb) - math.log(crossing.low.theory_pb))
                )
            print(
                f"  {label:22} {mass_tev:.4f} TeV, "
                f"sigma x B = {theory_at_crossing:.6g} pb  ({bracket})"
            )
            rows.append(
                (
                    coupling,
                    label,
                    f"{crossing.mass_gev:.6f}",
                    crossing.low.mass_gev,
                    crossing.high.mass_gev,
                    f"{theory_at_crossing:.12g}",
                )
            )
        median = exclusion_boundary(points, "expected")
        if median:
            low = median.low
            high = median.high
            print(
                "  median crossing details: "
                f"r({low.mass_gev})={low.limits['expected']:.6g}, "
                f"r({high.mass_gev})={high.limits['expected']:.6g}"
            )
        print()

    if args.csv_output:
        args.csv_output.parent.mkdir(parents=True, exist_ok=True)
        with args.csv_output.open("w", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(
                [
                    "coupling",
                    "quantile",
                    "mass_limit_GeV",
                    "bracket_low_GeV",
                    "bracket_high_GeV",
                    "crossing_sigma_pb_or_status",
                ]
            )
            writer.writerows(rows)
        print(f"Wrote {args.csv_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
