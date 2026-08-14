#!/usr/bin/env python3
"""Generate the twelve chess piece SVGs.

The artwork is authored here rather than taken from a set found elsewhere, so
its provenance is not a thing anyone has to take on trust: the shapes are in
this file, and running it reproduces the assets exactly. That is what allows
the project to be licensed permissively without asking anyone's permission.

Each piece is one silhouette, drawn once and emitted twice -- light pieces are
a pale body with a dark outline, dark pieces the reverse. Bodies are filled
with a top-to-bottom gradient and carry a soft highlight near the top left, so
they read as turned pieces catching a light rather than as flat cut-outs.

Two rules learned the hard way, both of which cost a rendering pass:

  A stroked shape takes the outline colour. The dark piece's outline is pale,
  so anything drawn as strokes alone disappears on pale squares -- which is
  where the dark king spends half its life. Shapes that must read on any
  square are filled outlines, not strokes.

  A shape narrower than twice the stroke width has no interior left to show,
  so it reads as an outline of nothing. Nothing thinner than about six units.

    python3 packaging/generate-pieces.py assets/pieces
"""

from __future__ import annotations

import pathlib
import sys

SIZE = 64

# Every piece stands on the same foot, which is what makes the set look like a
# set rather than six unrelated drawings.
FOOT = "M 12.5 57.5 L 51.5 57.5 L 47.5 50 L 16.5 50 Z"
COLLAR = "M 17.5 50 L 46.5 50 L 43.5 44.5 L 20.5 44.5 Z"

# Bodies may reference {accent}, filled in with the outline colour.
BODIES: dict[str, str] = {
    "pawn": (
        '<circle cx="32" cy="17" r="7.5"/>'
        '<path d="M 26 23.5 C 26 30.5 22.5 37 20.5 44.5 L 43.5 44.5 '
        'C 41.5 37 38 30.5 38 23.5 Z"/>'
    ),
    "rook": (
        '<path d="M 15 11.5 L 15 23 L 20 23 L 20 17 L 26.5 17 L 26.5 23 '
        "L 37.5 23 L 37.5 17 L 44 17 L 44 23 L 49 23 L 49 11.5 L 42 11.5 "
        "L 42 16 L 36 16 L 36 11.5 L 28 11.5 L 28 16 L 22 16 L 22 11.5 Z\"/>"
        '<path d="M 19.5 23 L 17.5 44.5 L 46.5 44.5 L 44.5 23 Z"/>'
        '<path d="M 19 30 L 45 30" stroke-width="1.6" fill="none"/>'
    ),
    # A horse in profile facing left, in the Staunton pattern the piece has
    # had since 1849.
    #
    # Four things carry the resemblance, and the drawing is worth nothing
    # without them:
    #
    #   The head is large -- roughly half the piece above the collar. A small
    #   head on a thick neck reads as a dog.
    #
    #   The muzzle is long and blunt, squared off at the front, and drops well
    #   below the eye. A muzzle that tapers to a point reads as a fox.
    #
    #   The jaw is one long sweep from under the muzzle back to the throat,
    #   not a small notch, which is what gives the head its weight.
    #
    #   The mane is a row of separate angled segments down the back edge. It
    #   is the most recognisable knight cue there is, and no amount of work on
    #   the head makes up for leaving it out.
    "knight": (
        '<path d="M 23 44.5 '
        # Throat rising from the chest to the cheek.
        "C 23.5 39 24.5 35 27 32.5 "
        # The long jaw sweeping forward under the head.
        "C 21.5 34.5 15 34.5 11 32.5 "
        # Blunt front of the muzzle.
        "L 7 29 L 8.5 21.5 "
        # Bridge of the nose rising back towards the brow.
        "C 10.5 17.5 14.5 14 18.5 12 "
        "C 20 10.5 21.5 9.5 23 8.5 "
        # One ear.
        "L 24 2 L 29.5 9 "
        # Back of the head running into the mane.
        "C 34.5 9 39.5 12.5 42.5 17.5 "
        # The mane down the back of the neck.
        "C 45.5 23 47 29.5 46.5 35 "
        "C 46 39 44.5 42 44 44.5 "
        'Z"/>'
        # The mane segments, the eye, the nostril and the mouth, all in the
        # outline colour so they read on either piece.
        '<path d="M 41.5 15.5 L 36.5 18.5 M 44 21 L 38.5 23.5 '
        'M 45.5 27 L 40 29 M 46 33 L 40.5 34.5" '
        'stroke="{accent}" stroke-width="1.6" fill="none"/>'
        '<path d="M 12 31 C 15.5 29.5 18.5 29 21.5 29.5" '
        'stroke="{accent}" stroke-width="1.4" fill="none"/>'
        '<circle cx="17" cy="19.5" r="2.1" fill="{accent}" stroke="none"/>'
        '<circle cx="10" cy="27" r="1.1" fill="{accent}" stroke="none"/>'
    ),
    # A mitre with the diagonal cut that tells a bishop from anything else.
    "bishop": (
        '<circle cx="32" cy="7.5" r="3.6"/>'
        '<path d="M 32 11.5 '
        "C 39.5 15 43.5 22.5 42.5 30 "
        "C 42 33.5 39.5 36 32 36 "
        "C 24.5 36 22 33.5 21.5 30 "
        'C 20.5 22.5 24.5 15 32 11.5 Z"/>'
        '<path d="M 35.5 16 L 28.5 26.5" stroke-width="2.4" fill="none"/>'
        '<path d="M 21.5 36 L 42.5 36 L 41.5 44.5 L 22.5 44.5 Z"/>'
    ),
    "queen": (
        '<path d="M 18.5 44.5 L 15 20.5 L 22.5 30 L 27 15.5 L 32 27.5 '
        'L 37 15.5 L 41.5 30 L 49 20.5 L 45.5 44.5 Z"/>'
        '<circle cx="15" cy="18" r="3.4"/>'
        '<circle cx="27" cy="13" r="3.4"/>'
        '<circle cx="32" cy="24.5" r="3.4"/>'
        '<circle cx="37" cy="13" r="3.4"/>'
        '<circle cx="49" cy="18" r="3.4"/>'
        '<path d="M 20 38.5 L 44 38.5" stroke-width="1.6" fill="none"/>'
    ),
    # A crown with a band. Without the band it reads as a bishop that has lost
    # its slit, which is the one confusion that actually matters on a board.
    "king": (
        '<path d="M 28.5 2.5 L 35.5 2.5 L 35.5 8 L 41 8 L 41 15 '
        'L 35.5 15 L 35.5 20.5 L 28.5 20.5 L 28.5 15 L 23 15 L 23 8 '
        'L 28.5 8 Z"/>'
        '<path d="M 20 44.5 C 16 33 21.5 25 32 20.5 '
        'C 42.5 25 48 33 44 44.5 Z"/>'
        '<path d="M 21.5 35 L 42.5 35" stroke-width="2.2" fill="none"/>'
    ),
}

# The gloss. A vertical gradient does the turning, and one soft highlight near
# the top left does the specular. Kept gentle: a piece that looks wet is worse
# than a piece that looks flat.
TEMPLATE = """<?xml version="1.0" encoding="UTF-8"?>
<!--
  {name}, {colour}.

  Generated by packaging/generate-pieces.py. Edit that, not this.
  Original artwork for this project; see LICENSE and NOTICE.
-->
<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}"
     viewBox="0 0 {size} {size}">
  <defs>
    <linearGradient id="body" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="{top}"/>
      <stop offset="0.55" stop-color="{fill}"/>
      <stop offset="1" stop-color="{bottom}"/>
    </linearGradient>
    <radialGradient id="sheen" cx="0.34" cy="0.26" r="0.42">
      <stop offset="0" stop-color="#FFFFFF" stop-opacity="{sheen}"/>
      <stop offset="1" stop-color="#FFFFFF" stop-opacity="0"/>
    </radialGradient>
  </defs>
  <g fill="url(#body)" stroke="{stroke}" stroke-width="{width}"
     stroke-linejoin="round" stroke-linecap="round">
    {body}
    <path d="{collar}"/>
    <path d="{foot}"/>
  </g>
  <g fill="url(#sheen)" stroke="none" pointer-events="none">
    {body_plain}
  </g>
</svg>
"""

# Near-white rather than pure white, and near-black rather than pure black, so
# neither piece disappears into a light or dark square.
STYLES = {
    "white": {
        "fill": "#F4F2EC",
        "top": "#FFFFFF",
        "bottom": "#D8D4C8",
        "stroke": "#191B1D",
        "width": "2.4",
        "sheen": "0.55",
    },
    "black": {
        "fill": "#282C31",
        "top": "#454B53",
        "bottom": "#15181B",
        "stroke": "#E9E9E5",
        "width": "1.8",
        "sheen": "0.22",
    },
}


def plain(body: str, accent: str) -> str:
    """The body with its own paint stripped, for the sheen layer to fill.

    The highlight is the same silhouette drawn again on top, so it can never
    escape the outline of the piece it is lighting.
    """
    stripped = body.format(accent=accent)
    for attribute in (
        f'stroke="{accent}"',
        'stroke-width="1.6"',
        'stroke-width="2.2"',
        'stroke-width="2.4"',
        'fill="none"',
        'stroke="none"',
        f'fill="{accent}"',
    ):
        stripped = stripped.replace(attribute, "")
    return stripped


def main() -> int:
    target = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "assets/pieces")
    target.mkdir(parents=True, exist_ok=True)

    for name, body in BODIES.items():
        for colour, style in STYLES.items():
            svg = TEMPLATE.format(
                name=name,
                colour=colour,
                size=SIZE,
                body=body.format(accent=style["stroke"]),
                body_plain=plain(body, style["stroke"]),
                collar=COLLAR,
                foot=FOOT,
                **style,
            )
            (target / f"{name}_{colour}.svg").write_text(svg)

    print(f"wrote {len(BODIES) * len(STYLES)} pieces to {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
