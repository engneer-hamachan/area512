"""Regenerates the paper substrate and the ink layer."""

import numpy as np
from PIL import Image

PAPER = (0xEF, 0xE6, 0xD6)


def build_noise(rng, shape, beta, lo, hi):
    """Seamless noise with |F| ~ f**-beta between lo and hi cycles/px, unit variance."""
    spectrum = np.fft.fft2(rng.normal(size=shape))
    fy = np.fft.fftfreq(shape[0])[:, None]
    fx = np.fft.fftfreq(shape[1])[None, :]
    radius = np.sqrt(fy ** 2 + fx ** 2)
    radius[0, 0] = 1e-9
    gain = radius ** -beta
    gain[(radius < lo) | (radius > hi)] = 0
    gain[0, 0] = 0
    out = np.real(np.fft.ifft2(spectrum * gain))
    return out / out.std()


def build_zones(field, low, high):
    """Maps a field to 0..1 between two of its quantiles, leaving flat regions at 0."""
    lo, hi = np.quantile(field, low), np.quantile(field, high)
    return ((field - lo) / (hi - lo)).clip(0, 1)


def scatter_specks(rng, shape, count, radius, weight):
    """Seamless specks with squared-uniform heights, skipped where weight is 0."""
    field = np.zeros(shape)
    offset = np.arange(-1, 2)
    dy, dx = np.meshgrid(offset, offset, indexing='ij')
    dot = np.exp(-(dy ** 2 + dx ** 2) / (2 * radius ** 2))
    for y, x, peak in zip(rng.integers(0, shape[0], count),
                          rng.integers(0, shape[1], count), rng.random(count) ** 2):
        if weight[y, x] <= 0.02:
            continue
        rows = np.ix_((y + offset) % shape[0], (x + offset) % shape[1])
        field[rows] = np.maximum(field[rows], peak * weight[y, x] * dot)
    return field


def save_ink_layer(alpha, path):
    rgba = np.zeros(alpha.shape + (4,), np.uint8)
    rgba[..., 0], rgba[..., 1], rgba[..., 2] = PAPER
    rgba[..., 3] = (alpha.clip(0, 1) * 255).round()
    Image.fromarray(rgba, 'RGBA').save(path, lossless=True, quality=100)


rng = np.random.default_rng(7)

# Uncoated offset substrate: fine grain over broad formation mottle, multiplied
# over --paper, so the sheet keeps its declared colour.
n = 1024
grain = build_noise(rng, (n, n), 0.5, 1.5 / n, 0.5)
mottle = build_noise(rng, (n, n), 1.2, 1.0 / n, 12.0 / n)
sheet = 1.0 - (0.0070 * grain + 0.0075 * mottle).clip(-0.05, 0.05)
Image.fromarray((sheet.clip(0, 1) * 255).round().astype(np.uint8), 'L') \
     .save('paper-offset.webp', lossless=True, quality=100)

# Ink over the dark and orange panels. Density, blotches and pinholes are all
# scaled by the same zones, so roughly half the area stays flat ink.
n = 1024
density = build_noise(rng, (n, n), 0.5, 1.5 / n, 0.5)
blotch = build_noise(rng, (n, n), 1.2, 1.0 / n, 10.0 / n)
zones = 0.12 + 0.88 * build_zones(build_noise(rng, (n, n), 1.6, 1.0 / n, 5.0 / n), 0.15, 0.95)
pinholes = scatter_specks(rng, (n, n), 24000, 0.75, zones)
save_ink_layer(zones * (0.075 * density + 0.022 * blotch) + 0.24 * pinholes, 'ink-grain.webp')
