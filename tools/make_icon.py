"""The app's icon at every size a platform asks for, each drawn from the board's own art by the render tool
(connect4_render <out.png> icon <size>) rather than scaled from one picture, packed into assets/icon/: icon.ico (the
sizes Explorer and the task bar take), icon.icns (a macOS bundle's) and icon_256.png (a Linux window's, embedded).
Run it after changing the icon's drawing (render_main.cpp's RenderIcon) and commit what it writes; the build needs no
Python. Needs Pillow.

    python tools/make_icon.py <path to connect4_render>
"""

import pathlib
import subprocess
import sys
import tempfile

from PIL import Image

ICO_SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]
ICNS_SIZES = [16, 32, 64, 128, 256, 512, 1024]


def render(tool, size, folder):
    """The icon at `size` pixels, drawn by the render tool."""
    path = folder / f"icon_{size}.png"
    subprocess.run([str(tool), str(path), "icon", str(size)], check=True, stdout=subprocess.DEVNULL)
    return Image.open(path).convert("RGBA")


def main(tool):
    out = pathlib.Path(__file__).resolve().parent.parent / "assets" / "icon"
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temporary:
        folder = pathlib.Path(temporary)
        images = {size: render(tool, size, folder) for size in sorted(set(ICO_SIZES + ICNS_SIZES))}
        largest_ico = images[max(ICO_SIZES)]
        largest_ico.save(out / "icon.ico", format="ICO", sizes=[(s, s) for s in ICO_SIZES],
                         append_images=[images[s] for s in ICO_SIZES if s != max(ICO_SIZES)])
        images[1024].save(out / "icon.icns", format="ICNS",
                          append_images=[images[s] for s in ICNS_SIZES if s != 1024])
        images[256].save(out / "icon_256.png", format="PNG")
    print(f"wrote {out / 'icon.ico'}, {out / 'icon.icns'} and {out / 'icon_256.png'}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    main(pathlib.Path(sys.argv[1]))
