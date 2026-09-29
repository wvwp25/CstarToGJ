#!/usr/bin/env python3
"""Update signal yields and normalization nuisances in C-star datacards.

The inputs are the histograms in ``CstarToGJ.root`` produced by
Signal/CstarToGJ_analysis.C.  Every yield is integrated over the corresponding
observable range in ``Signal/fit_ranges.csv``.  The pileup nuisance uses a
symmetric envelope whose size is the larger absolute shift of the raw Up and
Down yields.
Background rates and every unrelated datacard entry are deliberately left
unchanged.  The default mode is a dry run; pass ``--write`` to replace the
datacards after all selected inputs validate.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import stat
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


CARD_NAME = re.compile(r"datacard_M(?P<mass>\d+)_(?P<coupling>f\d+p\d+)\.txt")
NUMBER = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"


@dataclass(frozen=True)
class SignalValues:
    rate: float
    ctag_down: float
    ctag_up: float
    pileup_down: float
    pileup_up: float

    @property
    def pileup_delta(self) -> float:
        """Largest absolute yield shift from the two pileup variations."""
        return max(abs(self.pileup_down - 1.0), abs(self.pileup_up - 1.0))

    @property
    def pileup_envelope_down(self) -> float:
        return 1.0 - self.pileup_delta

    @property
    def pileup_envelope_up(self) -> float:
        return 1.0 + self.pileup_delta


@dataclass(frozen=True)
class CardUpdate:
    path: Path
    original: str
    updated: str
    old_rate: str
    values: SignalValues
    signal_file: Path
    mass_range: tuple[float, float]


def parse_arguments() -> argparse.Namespace:
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description=(
            "Update signal rate, CMS_ctag, and CMS_pileup from signal ROOT "
            "histograms integrated over each mass-dependent observable range. "
            "The default is a dry run."
        )
    )
    parser.add_argument(
        "--write",
        action="store_true",
        help="atomically replace validated datacards (default: show changes only)",
    )
    parser.add_argument(
        "--card-dir",
        type=Path,
        default=here,
        help=f"datacard directory (default: {here})",
    )
    parser.add_argument(
        "--signal-base",
        type=Path,
        default=Path("/eos/user/h/hsiaoche/Signal"),
        help="directory containing CstarToGJ_M*_f*_13TeV_NANOAOD samples",
    )
    parser.add_argument(
        "--fit-ranges",
        type=Path,
        default=here.parent / "Signal" / "fit_ranges.csv",
        help="CSV containing mass-dependent observable ranges",
    )
    parser.add_argument(
        "-f",
        "--coupling",
        action="append",
        help="update only this coupling; may be specified more than once",
    )
    parser.add_argument(
        "-m",
        "--mass",
        type=int,
        action="append",
        help="update only this mass; may be specified more than once",
    )
    return parser.parse_args()


def read_fit_ranges(path: Path) -> dict[int, tuple[float, float]]:
    if not path.is_file():
        raise FileNotFoundError(f"missing fit-range configuration: {path}")
    ranges: dict[int, tuple[float, float]] = {}
    with path.open(newline="") as handle:
        for row in csv.reader(line for line in handle if not line.lstrip().startswith("#")):
            if not row:
                continue
            if len(row) != 5:
                raise ValueError(f"{path}: expected 5 columns, got {row}")
            mass, x_min, x_max, _fit_min, _fit_max = row
            mass_value = int(mass)
            bounds = (float(x_min), float(x_max))
            if bounds[0] >= bounds[1]:
                raise ValueError(f"{path}: invalid range for M{mass_value}: {bounds}")
            if mass_value in ranges:
                raise ValueError(f"{path}: duplicate range for M{mass_value}")
            ranges[mass_value] = bounds
    return ranges


def read_signal_values(
    signal_file: Path, x_min: float, x_max: float
) -> SignalValues:
    if not signal_file.is_file():
        raise FileNotFoundError(f"missing signal analysis output: {signal_file}")
    try:
        import ROOT  # type: ignore
    except ImportError as error:
        raise RuntimeError(
            "PyROOT is required; initialize the CMSSW environment before running"
        ) from error

    root_file = ROOT.TFile.Open(str(signal_file), "READ")
    if not root_file or root_file.IsZombie():
        raise RuntimeError(f"cannot open signal analysis output: {signal_file}")
    histograms = {}
    try:
        for name in ("sig", "sig_CTagDown", "sig_CTagUp", "sig_PUDown", "sig_PUUp"):
            histogram = root_file.Get(name)
            if not histogram:
                raise RuntimeError(f"{signal_file}: missing histogram '{name}'")
            first_bin = histogram.GetXaxis().FindBin(x_min)
            last_bin = histogram.GetXaxis().FindBin(x_max)
            histograms[name] = float(histogram.Integral(first_bin, last_bin))
    finally:
        root_file.Close()

    nominal = histograms["sig"]
    if nominal <= 0.0:
        raise ValueError(
            f"{signal_file}: non-positive nominal yield {nominal} in [{x_min}, {x_max}]"
        )
    for name, value in histograms.items():
        if value <= 0.0:
            raise ValueError(
                f"{signal_file}: non-positive {name} yield {value} in [{x_min}, {x_max}]"
            )
    values = SignalValues(
        rate=nominal,
        ctag_down=histograms["sig_CTagDown"] / nominal,
        ctag_up=histograms["sig_CTagUp"] / nominal,
        pileup_down=histograms["sig_PUDown"] / nominal,
        pileup_up=histograms["sig_PUUp"] / nominal,
    )
    if values.pileup_delta >= 1.0:
        raise ValueError(
            f"{signal_file}: pileup envelope is non-positive on its down side "
            f"(delta={values.pileup_delta})"
        )
    return values


def replace_one(text: str, pattern: str, replacement, label: str, card: Path) -> tuple[str, re.Match[str]]:
    matches = list(re.finditer(pattern, text, re.MULTILINE))
    if len(matches) != 1:
        raise ValueError(f"{card}: expected one {label} line, found {len(matches)}")
    match = matches[0]
    return text[: match.start()] + replacement(match) + text[match.end() :], match


def prepare_update(
    card: Path,
    signal_base: Path,
    mass: str,
    coupling: str,
    fit_ranges: dict[int, tuple[float, float]],
) -> CardUpdate:
    sample = f"CstarToGJ_M{mass}_{coupling}_13TeV_NANOAOD"
    signal_file = signal_base / sample / "CstarToGJ.root"
    mass_value = int(mass)
    if mass_value not in fit_ranges:
        raise ValueError(f"no observable range configured for M{mass}")
    mass_range = fit_ranges[mass_value]
    values = read_signal_values(signal_file, *mass_range)
    original = card.read_text()

    rate_pattern = rf"^(?P<prefix>[ \t]*rate[ \t]+)(?P<signal>{NUMBER})(?P<gap>[ \t]+)(?P<background>{NUMBER})(?P<suffix>[ \t]*)$"
    updated, rate_match = replace_one(
        original,
        rate_pattern,
        lambda match: (
            f"{match['prefix']}{values.rate:.10g}{match['gap']}"
            f"{match['background']}{match['suffix']}"
        ),
        "two-process rate",
        card,
    )

    def nuisance_pattern(name: str) -> str:
        return rf"^(?P<prefix>[ \t]*{re.escape(name)}[ \t]+lnN[ \t]+)(?P<signal>\S+)(?P<gap>[ \t]+)(?P<background>-)(?P<suffix>[ \t]*)$"

    updated, _ = replace_one(
        updated,
        nuisance_pattern("CMS_ctag"),
        lambda match: (
            f"{match['prefix']}{values.ctag_down:.10g}/{values.ctag_up:.10g}"
            f"{match['gap']}{match['background']}{match['suffix']}"
        ),
        "CMS_ctag",
        card,
    )
    updated, _ = replace_one(
        updated,
        nuisance_pattern("CMS_pileup"),
        lambda match: (
            f"{match['prefix']}{values.pileup_envelope_down:.10g}/"
            f"{values.pileup_envelope_up:.10g}"
            f"{match['gap']}{match['background']}{match['suffix']}"
        ),
        "CMS_pileup",
        card,
    )
    return CardUpdate(
        card, original, updated, rate_match["signal"], values, signal_file, mass_range
    )


def atomic_write(path: Path, text: str) -> None:
    mode = stat.S_IMODE(path.stat().st_mode)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "w") as handle:
            handle.write(text)
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise


def main() -> int:
    args = parse_arguments()
    try:
        fit_ranges = read_fit_ranges(args.fit_ranges)
    except (OSError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    cards = []
    for card in sorted(args.card_dir.glob("datacard_M*_f*.txt")):
        match = CARD_NAME.fullmatch(card.name)
        if match is None:
            continue
        if args.coupling and match["coupling"] not in args.coupling:
            continue
        if args.mass and int(match["mass"]) not in args.mass:
            continue
        cards.append((card, match["mass"], match["coupling"]))

    if not cards:
        print("ERROR: no matching datacards", file=sys.stderr)
        return 1

    # Validate every selected card and ROOT input before writing any file.
    try:
        updates = [
            prepare_update(card, args.signal_base, mass, coupling, fit_ranges)
            for card, mass, coupling in cards
        ]
    except (OSError, RuntimeError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1

    changed = [update for update in updates if update.original != update.updated]
    for update in updates:
        status = "CHANGE" if update.original != update.updated else "UNCHANGED"
        values = update.values
        print(
            f"{status:9} {update.path.name}: "
            f"range [{update.mass_range[0]:g},{update.mass_range[1]:g}]; "
            f"rate {update.old_rate} -> {values.rate:.10g}; "
            f"ctag {values.ctag_down:.10g}/{values.ctag_up:.10g}; "
            f"pileup envelope {values.pileup_envelope_down:.10g}/"
            f"{values.pileup_envelope_up:.10g} "
            f"(raw Down/Up {values.pileup_down:.10g}/{values.pileup_up:.10g})"
        )

    if args.write:
        for update in changed:
            atomic_write(update.path, update.updated)
        print(f"Updated {len(changed)} of {len(updates)} selected datacards.")
    else:
        print(
            f"Dry run: {len(changed)} of {len(updates)} selected datacards would change. "
            "Run again with --write to apply."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
