#!/usr/bin/env python3
"""Generate the twelve chess piece SVGs.

The artwork is authored here rather than taken from a set found elsewhere, so
its provenance is not a thing anyone has to take on trust: the shapes are in
this file, and running it reproduces the assets exactly. That is what lets the
project be licensed permissively without asking anyone's permission.

The set is a Staunton one -- the pattern the pieces have carried since 1849 --
drawn as turned wood/ivory. Every piece shares one turned base, so the set
reads as a set, and each is finished with its own distinctive top.

Lighting is what makes a flat SVG read as a lathe-turned object. Three layers
do it, all in user space so the light direction is consistent across every
piece:

  * a horizontal gradient across the width, bright a third of the way from the
    left and falling to shadow at both edges -- this is the roundness of a
    cylinder seen from the side;
  * a soft vertical darkening towards the foot, to sit the piece down rather
    than let it float;
  * a small specular highlight up and to the left, where the light would catch.

Two rules learned the hard way, each at the cost of a rendering pass, kept here
so they are not rediscovered:

  A stroked shape takes the OUTLINE colour, and the dark piece's outline is
  pale -- so anything drawn as strokes alone vanishes on a matching square.
  Shapes that must read on any square are filled, not stroked.

  A shape narrower than twice the stroke width has no interior left to show and
  reads as an outline of nothing. Nothing thinner than about six units.

    python3 packaging/generate-pieces.py assets/pieces
"""

from __future__ import annotations

import pathlib
import sys

SIZE = 64


def circle(cx: float, cy: float, r: float) -> str:
    return f'<circle cx="{cx}" cy="{cy}" r="{r}"/>'


# The turned base every piece stands on: a flared foot, a rounded bead above
# it, and a narrow collar the top rises from. Shared, so the set is a set.
BASE_PARTS = [
    # Foot.
    "<path d=\"M 11.5 58 L 52.5 58 L 52.5 55.4 "
    "C 52.5 52.6 49.6 50.6 45.6 50.5 L 18.4 50.5 "
    "C 14.4 50.6 11.5 52.6 11.5 55.4 Z\"/>",
    # Bead.
    "<path d=\"M 17.5 50.5 C 18 46.8 21 44 25.3 43.8 "
    "L 38.7 43.8 C 43 44 46 46.8 46.5 50.5 Z\"/>",
]

# One incised groove around the foot, in the outline colour so it reads as a
# turned line on either piece.
BASE_ACCENTS = [
    '<path d="M 14.5 54.3 L 49.5 54.3" stroke="{accent}" '
    'stroke-width="1.2" fill="none" opacity="0.45"/>',
]


PIECES: dict[str, dict[str, list[str]]] = {
    "pawn": {
        "parts": [
            # Flared neck.
            "<path d=\"M 27.5 43.8 C 26.5 39.8 26.8 36 29.2 33 "
            "L 34.8 33 C 37.2 36 37.5 39.8 36.5 43.8 Z\"/>",
            # Bead under the ball.
            "<path d=\"M 28.5 33 L 35.5 33 L 34.2 30 L 29.8 30 Z\"/>",
            circle(32, 23.5, 8),
        ],
        "accents": [],
    },
    "rook": {
        "parts": [
            # Tower.
            "<path d=\"M 22 43.8 L 20.5 29.5 L 43.5 29.5 L 42 43.8 Z\"/>",
            # Ring below the battlements.
            "<path d=\"M 20.5 29.5 L 43.5 29.5 L 42.6 26 L 21.4 26 Z\"/>",
            # Battlements: three merlons, two gaps.
            "<path d=\"M 21.4 26 L 21.4 15.5 L 25.6 15.5 L 25.6 19.5 "
            "L 29.8 19.5 L 29.8 15.5 L 34.2 15.5 L 34.2 19.5 "
            "L 38.4 19.5 L 38.4 15.5 L 42.6 15.5 L 42.6 26 Z\"/>",
        ],
        "accents": [
            '<path d="M 21 33.5 L 43 33.5" stroke="{accent}" '
            'stroke-width="1.2" fill="none" opacity="0.4"/>',
        ],
    },
    # The Staunton horse, in profile facing left. Four things carry the
    # resemblance and the drawing is worth nothing without them: a large head,
    # a long blunt muzzle dropping below the eye, one long jaw sweeping forward
    # under the head, and a mane of separate angled segments down the back.
    "knight": {
        "parts": [
            "<path d=\"M 23 43.8 "
            "C 23.5 38.5 24.5 34.6 27 32.2 "
            "C 21.5 34.2 15 34.2 11 32.2 "
            "L 7 28.8 L 8.5 21.4 "
            "C 10.5 17.5 14.5 14 18.5 12 "
            "C 20 10.6 21.5 9.6 23 8.6 "
            "L 24 2.2 L 29.5 9 "
            "C 34.5 9 39.5 12.4 42.5 17.4 "
            "C 45.5 22.8 47 29.2 46.5 34.6 "
            "C 46 38.5 44.5 41.6 44 43.8 Z\"/>",
        ],
        "accents": [
            # Mane segments.
            '<path d="M 41.5 15.4 L 36.6 18.4 M 44 20.8 L 38.6 23.3 '
            'M 45.5 26.7 L 40.1 28.7 M 46 32.6 L 40.6 34.1" '
            'stroke="{accent}" stroke-width="1.6" fill="none"/>',
            # Mouth.
            '<path d="M 12 30.8 C 15.5 29.4 18.5 28.9 21.5 29.4" '
            'stroke="{accent}" stroke-width="1.4" fill="none"/>',
            # Eye and nostril.
            '<circle cx="17" cy="19.4" r="2.1" fill="{accent}" stroke="none"/>',
            '<circle cx="10" cy="26.8" r="1.1" fill="{accent}" stroke="none"/>',
        ],
    },
    "bishop": {
        "parts": [
            # Collar bead.
            "<path d=\"M 27.5 43.8 L 36.5 43.8 L 35 39.5 L 29 39.5 Z\"/>",
            # Mitre.
            "<path d=\"M 32 39.5 C 39 39.5 43 33.5 42.5 25.6 "
            "C 42 19.6 37.5 14.2 32 11.2 "
            "C 26.5 14.2 22 19.6 21.5 25.6 "
            "C 21 33.5 25 39.5 32 39.5 Z\"/>",
            # Finial.
            circle(32, 8.6, 3.6),
        ],
        "accents": [
            # The cut across the mitre, the one cue that tells a bishop apart.
            '<path d="M 35.5 15.5 L 28.5 26.5" stroke="{accent}" '
            'stroke-width="2.3" fill="none"/>',
        ],
    },
    "queen": {
        "parts": [
            # Collar.
            "<path d=\"M 25.5 43.8 L 38.5 43.8 L 37 38.6 L 27 38.6 Z\"/>",
            # Coronet.
            "<path d=\"M 21 38.6 L 16.5 21.5 L 23 30 L 27.5 15.2 "
            "L 32 27.6 L 36.5 15.2 L 41 30 L 47.5 21.5 L 43 38.6 Z\"/>",
            # A ball on each point.
            circle(16.5, 19, 3.5),
            circle(27.5, 12.8, 3.5),
            circle(32, 24.8, 3.5),
            circle(36.5, 12.8, 3.5),
            circle(47.5, 19, 3.5),
        ],
        "accents": [
            '<path d="M 22.5 34 L 41.5 34" stroke="{accent}" '
            'stroke-width="1.3" fill="none" opacity="0.45"/>',
        ],
    },
    "king": {
        "parts": [
            # Collar.
            "<path d=\"M 25.5 43.8 L 38.5 43.8 L 37 38.6 L 27 38.6 Z\"/>",
            # Crown.
            "<path d=\"M 21.5 38.6 C 17.5 29.6 22.5 23.6 32 20.6 "
            "C 41.5 23.6 46.5 29.6 42.5 38.6 Z\"/>",
            # Cross, filled so it reads on any square. A little taller than the
            # queen's coronet, which is how the two are told apart.
            "<path d=\"M 29 4.5 L 35 4.5 L 35 9.5 L 40 9.5 L 40 15.5 "
            "L 35 15.5 L 35 22 L 29 22 L 29 15.5 L 24 15.5 L 24 9.5 "
            "L 29 9.5 Z\"/>",
        ],
        "accents": [
            '<path d="M 22.5 33 L 41.5 33" stroke="{accent}" '
            'stroke-width="1.3" fill="none" opacity="0.45"/>',
        ],
    },
}


TEMPLATE = """<?xml version="1.0" encoding="UTF-8"?>
<!--
  {name}, {colour}.

  Generated by packaging/generate-pieces.py. Edit that, not this.
  Original artwork for this project; see LICENSE and NOTICE.
-->
<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}"
     viewBox="0 0 {size} {size}">
  <defs>
    <linearGradient id="round" gradientUnits="userSpaceOnUse"
                    x1="9" y1="0" x2="55" y2="0">
      <stop offset="0" stop-color="{edge}"/>
      <stop offset="0.27" stop-color="{lit}"/>
      <stop offset="0.52" stop-color="{fill}"/>
      <stop offset="0.78" stop-color="{shade}"/>
      <stop offset="1" stop-color="{edge}"/>
    </linearGradient>
    <linearGradient id="ground" gradientUnits="userSpaceOnUse"
                    x1="0" y1="4" x2="0" y2="58">
      <stop offset="0" stop-color="#000000" stop-opacity="0"/>
      <stop offset="0.66" stop-color="#000000" stop-opacity="0"/>
      <stop offset="1" stop-color="#000000" stop-opacity="0.24"/>
    </linearGradient>
    <radialGradient id="spec" gradientUnits="userSpaceOnUse"
                    cx="23" cy="17" r="19">
      <stop offset="0" stop-color="#FFFFFF" stop-opacity="{sheen}"/>
      <stop offset="0.75" stop-color="#FFFFFF" stop-opacity="0"/>
    </radialGradient>
  </defs>

  <!-- Roundness and outline. -->
  <g fill="url(#round)" stroke="{stroke}" stroke-width="{width}"
     stroke-linejoin="round" stroke-linecap="round">{parts}</g>
  <!-- Sit it on its foot. -->
  <g fill="url(#ground)" stroke="none">{parts}</g>
  <!-- Catch the light. -->
  <g fill="url(#spec)" stroke="none">{parts}</g>
  <!-- Turned lines and features. -->
  <g>{accents}</g>
</svg>
"""


# Near-white and near-black rather than the pure tones, so a piece never
# disappears into a light or dark square of its own colour.
STYLES = {
    "white": {
        "edge": "#B7B1A0",
        "shade": "#D6D0C0",
        "fill": "#EFEBE0",
        "lit": "#FEFDF9",
        "stroke": "#191B1D",
        "width": "2.4",
        "sheen": "0.58",
    },
    "black": {
        "edge": "#111316",
        "shade": "#23272C",
        "fill": "#31363D",
        "lit": "#586069",
        "stroke": "#E9E9E5",
        "width": "1.8",
        "sheen": "0.30",
    },
}


def main() -> int:
    target = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "assets/pieces")
    target.mkdir(parents=True, exist_ok=True)

    for name, piece in PIECES.items():
        parts = "".join(BASE_PARTS + piece["parts"])
        for colour, style in STYLES.items():
            accents = "".join(BASE_ACCENTS + piece["accents"]).format(
                accent=style["stroke"]
            )
            svg = TEMPLATE.format(
                name=name,
                colour=colour,
                size=SIZE,
                parts=parts,
                accents=accents,
                **style,
            )
            (target / f"{name}_{colour}.svg").write_text(svg)

    print(f"wrote {len(PIECES) * len(STYLES)} pieces to {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
