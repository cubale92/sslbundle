#!/usr/bin/env python3
"""Writes each MPC port's params.json and presets.json.

The factory presets are read straight from the desktop plug-ins (plugins/<Name>/PluginProcessor.cpp, setPresets({...})),
so the MPC build always ships the same sounds. Values there are in real units (Hz, seconds, ratio); MPC moves every
parameter linearly, so frequency/time/Q/ratio controls are 0..100 positions on a log curve here (the engine converts
back, mpc/common/mpc_console.h logPos()).

    python3 mpc/tools/gen_ports.py
"""
import json
import math
import os
import re
import argparse
from pathlib import Path

HERE = os.path.dirname(os.path.abspath(__file__))
MPC = os.path.dirname(HERE)
REPO = os.path.dirname(MPC)
ONOFF = ["OFF", "ON"]


def pos(v, lo, hi):
    v = min(max(v, lo), hi)
    return round(100.0 * math.log(v / lo) / math.log(hi / lo), 2)


def knob(key, name, lo, hi, default, unit="", dyn=True, **kw):
    p = {"key": key, "name": name, "min": lo, "max": hi, "default": default}
    if unit:
        p["unit"] = unit
    if dyn:
        p["dynamic_display"] = True
    p.update(kw)
    return p


def curve(key, name, default):
    """A 0..100 position the engine shows in real units."""
    return knob(key, name, 0, 100, default)


def opt(key, name, options, default=0):
    return {"key": key, "name": name, "options": options, "default": default}


# --- per plug-in: parameters (VST index order = the engine's enum), unit conversion for presets ----------------------
CHANNEL = {
    "dir": "channel4k", "src": "Channel4K",
    "params": [
        knob("drive", "Drive", 0, 24, 0, "dB"),
        opt("phase", "Phase", ONOFF),
        curve("hpf", "HPF", 0),
        curve("lpf", "LPF", 100),
        opt("eqtype", "EQ Curve", ["BROWN", "BLACK"], 1),
        opt("eqin", "EQ In", ONOFF, 1),
        knob("hfg", "HF Gain", -16, 16, 0, "dB"),
        curve("hff", "HF Freq", pos(8000, 1500, 16000)),
        opt("hfbell", "HF Bell", ONOFF),
        knob("hmfg", "HMF Gain", -16, 16, 0, "dB"),
        curve("hmff", "HMF Freq", pos(2500, 600, 7000)),
        curve("hmfq", "HMF Q", pos(1.0, 0.4, 4.0)),
        knob("lmfg", "LMF Gain", -16, 16, 0, "dB"),
        curve("lmff", "LMF Freq", pos(600, 200, 2500)),
        curve("lmfq", "LMF Q", pos(1.0, 0.4, 4.0)),
        knob("lfg", "LF Gain", -16, 16, 0, "dB"),
        curve("lff", "LF Freq", pos(100, 30, 450)),
        opt("lfbell", "LF Bell", ONOFF),
        curve("cratio", "Ratio", 0),
        knob("cthresh", "Comp Thresh", -40, 10, 10, "dB"),
        curve("crel", "Comp Release", pos(0.3, 0.05, 4.0)),
        opt("cfast", "Comp Fast", ONOFF),
        knob("grange", "Gate Range", 0, 40, 0, "dB"),
        knob("gthresh", "Gate Thresh", -70, 0, -50, "dB"),
        curve("grel", "Gate Release", pos(0.25, 0.05, 4.0)),
        opt("gmode", "Gate Mode", ["EXP", "GATE"]),
        opt("gfast", "Gate Fast", ONOFF),
        opt("dynin", "Dyn In", ONOFF, 1),
        opt("dynpre", "Dyn > EQ", ONOFF),
        knob("mix", "Dyn Mix", 0, 100, 100, "%"),
        knob("output", "Output", -18, 18, 0, "dB"),
    ],
    "convert": {
        "hpf": lambda v: 0 if v <= 16 else max(1.0, pos(v, 20, 400)),
        "lpf": lambda v: 100 if v >= 21500 else min(99.0, pos(v, 3000, 22000)),
        "hff": lambda v: pos(v, 1500, 16000),
        "hmff": lambda v: pos(v, 600, 7000),
        "lmff": lambda v: pos(v, 200, 2500),
        "lff": lambda v: pos(v, 30, 450),
        "hmfq": lambda v: pos(v, 0.4, 4.0),
        "lmfq": lambda v: pos(v, 0.4, 4.0),
        "cratio": lambda v: 0 if v <= 1.01 else (100 if v >= 19.9 else pos(v, 1, 20)),
        "crel": lambda v: pos(v, 0.05, 4.0),
        "grel": lambda v: pos(v, 0.05, 4.0),
    },
}

BUS = {
    "dir": "bus4k", "src": "Bus4K",
    "params": [
        knob("threshold", "Threshold", -30, 10, -10, "dB"),
        opt("ratio", "Ratio", ["2", "4", "10"], 1),
        opt("attack", "Attack", [".1", ".3", "1", "3", "10", "30"], 5),
        opt("release", "Release", [".1", ".3", ".6", "1.2", "AUTO"], 4),
        knob("makeup", "Make-Up", 0, 20, 0, "dB"),
        opt("schpf", "S/C HPF", ["OFF", "60", "90", "150", "250"]),
        knob("mix", "Mix", 0, 100, 100, "%"),
        knob("drive", "Console Drive", 0, 100, 20, "%"),
        opt("compin", "Comp In", ONOFF, 1),
    ],
    "convert": {},
}

LISTEN = {
    "dir": "listen4k", "src": "Listen4K",
    "params": [
        knob("input", "Input", 0, 30, 10, "dB"),
        curve("release", "Release", pos(220, 30, 1200)),
        opt("tone", "Tone", ["FULL", "TALKBACK", "DARK"], 1),
        knob("crunch", "Crunch", 0, 100, 30, "%"),
        knob("mix", "Mix", 0, 100, 100, "%"),
        knob("output", "Output", -24, 12, 0, "dB"),
    ],
    "convert": {"release": lambda v: pos(v, 30, 1200)},
}


def desktop_presets(src):
    """[(name, {key: value})] from plugins/<src>/PluginProcessor.cpp's setPresets({...})."""
    text = Path(REPO, "plugins", src, "PluginProcessor.cpp").read_text(encoding="utf-8")
    body = text[text.index("setPresets ({"):]
    body = body[:body.index("});")]
    out = []
    for m in re.finditer(r'\{\s*"([^"]+)",\s*\{(.*?)\}\s*\}\s*,', body, re.S):
        vals = {k: float(v.rstrip("f")) for k, v in re.findall(r'\{\s*"(\w+)",\s*(-?[\d.]+f?)\s*\}', m.group(2))}
        out.append((m.group(1), vals))
    return out


def label_of(p, v):
    return p["options"][int(round(v))]


def build(port, check=False):
    params = port["params"]
    keys = [p["key"] for p in params]
    presets = []
    source_presets = desktop_presets(port["src"])
    if not source_presets or source_presets[0] != ("Init", {}):
        raise SystemExit("Failed to parse factory presets: " + port["src"])
    for name, vals in source_presets:
        unknown = set(vals) - set(keys)
        if unknown:
            raise SystemExit("%s preset %r: unknown keys %s" % (port["src"], name, sorted(unknown)))
        values = {}
        for p in params:   # every parameter, in index order (docs/PORTING.md: set every parameter in every preset)
            k = p["key"]
            if k in vals:
                v = vals[k]
                if k in port["convert"]:
                    v = port["convert"][k](v)
            else:
                v = p["default"]
            lo, hi = (0, len(p["options"]) - 1) if "options" in p else (p["min"], p["max"])
            if not math.isfinite(v) or not lo <= v <= hi:
                raise SystemExit(f"{port['src']} {name}: {k}={v} is outside {lo}..{hi}")
            if "options" in p:
                values[k] = label_of(p, v) if not isinstance(v, str) else v
            else:
                values[k] = round(float(v), 2)
        presets.append({"name": name, "values": values})
    d = os.path.join(MPC, port["dir"])
    definitions = ["// Generated by mpc/tools/gen_ports.py; do not edit.",
                   "const ParamDef kDefs[] = {"]
    for p in params:
        discrete = "options" in p
        lo, hi = (0, len(p["options"]) - 1) if discrete else (p["min"], p["max"])
        definitions.append('    { "%s", %sf, %sf, %sf, %s },' %
                           (p["key"], float(p["default"]), float(lo), float(hi), str(discrete).lower()))
    definitions.append("};\n")
    outputs = {"params.json": json.dumps({"params": params}, indent=1),
               "presets.json": json.dumps({"presets": presets}, indent=1),
               "param_defs.h": "\n".join(definitions)}
    for name, content in outputs.items():
        path = Path(d, name)
        if check:
            if not path.exists() or path.read_text(encoding="utf-8") != content:
                raise SystemExit(f"Stale {path}: run python mpc/tools/gen_ports.py")
        else:
            path.write_text(content, encoding="utf-8")
    print("%s: %d params, %d presets" % (port["dir"], len(params), len(presets)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="Fail if generated files are stale")
    args = parser.parse_args()
    for port in (CHANNEL, BUS, LISTEN):
        build(port, args.check)
