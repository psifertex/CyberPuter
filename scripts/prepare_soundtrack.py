#!/usr/bin/env python3
"""Convert a licensed recording to the Cardputer SD playlist's WAV format."""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path)
parser.add_argument("output", type=Path, help="e.g. sdcard/cyberputer/music/03-My-Track.wav")
args = parser.parse_args()
if args.output.suffix.lower() != ".wav":
    parser.error("output must end in .wav")
if len(args.output.name.encode("utf-8")) > 47:
    parser.error("filename must be at most 47 bytes")
args.output.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([
    "ffmpeg", "-v", "error", "-n", "-i", str(args.source), "-map_metadata", "-1",
    "-ac", "1", "-ar", "22050", "-af", "volume=0.72",
    "-c:a", "pcm_s16le", str(args.output)
], check=True)
print(f"Ready to copy to /cyberputer/music/: {args.output}")
