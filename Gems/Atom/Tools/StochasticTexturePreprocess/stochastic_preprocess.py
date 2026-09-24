#!/usr/bin/env python3

"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Stochastic Tiling & Blending texture preprocessor.

Implements the offline steps of "Procedural Stochastic Textures by Tiling and Blending"
(Deliot & Heitz 2019):

 1. Transforms the input image to a PCA-decorrelated color space (section 1.4).
 2. Gaussianizes each channel to N(0.5, 1/12) (section 1.3.2) and writes it as <name>_gauss.png.
 3. Computes the inverse histogram LUT (section 1.3.3), prefiltering it per mip level
    (section 1.5) and writes it as <name>_lut.png.
 4. Prints the material property JSON snippet (color space constants) to configure a material.

Usage:
    python3 stochastic_preprocess.py <input_image> [output_dir]

The generated textures must be imported into O3DE as LINEAR (non-sRGB) data, for example by
setting the sRGB flag to off in the Asset Browser image settings (or a .image source override).
"""

import argparse
import json
import os
import sys

import numpy as np

try:
    from PIL import Image
except ImportError:
    print("This tool requires Pillow and numpy: pip install numpy pillow")
    sys.exit(1)

GAUSSIAN_AVERAGE = 0.5
GAUSSIAN_VARIANCE = 1.0 / 12.0
LUT_WIDTH = 256


def erf(x):
    a1, a2, a3, a4, a5 = 0.254829592, -0.284496736, 1.421413741, -1.453152027, 1.061405429
    p = 0.3275911
    sign = np.sign(x)
    z = np.abs(x)
    t = 1.0 / (1.0 + p * z)
    y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * np.exp(-z * z)
    return sign * y


def cdf(x, mu=GAUSSIAN_AVERAGE, var=GAUSSIAN_VARIANCE):
    return 0.5 * (1.0 + erf((x - mu) / np.sqrt(2.0 * var)))


def inv_cdf(u, mu=GAUSSIAN_AVERAGE, var=GAUSSIAN_VARIANCE):
    lo = np.full_like(u, -1.0)
    hi = np.full_like(u, 2.0)
    for _ in range(40):
        mid = 0.5 * (lo + hi)
        below = cdf(mid) < u
        lo = np.where(below, mid, lo)
        hi = np.where(below, hi, mid)
    return 0.5 * (lo + hi)


def compute_color_space(img):
    """PCA-decorrelated color space (section 1.4). Returns origin, axes and the decorrelated image."""
    flat = img.reshape(-1, 3).astype(np.float64)
    mean = flat.mean(axis=0)
    cov = np.cov((flat - mean).T)
    evals, evecs = np.linalg.eigh(cov)
    axes = evecs[:, ::-1].T
    proj = (flat - mean) @ axes.T
    vmin = proj.min(axis=0)
    vmax = proj.max(axis=0)
    extent = np.maximum(vmax - vmin, 1e-6)
    origin = mean + (vmin @ axes)
    scaled = axes * extent[:, None]
    decorrelated = ((proj - vmin) / extent).reshape(img.shape)
    return origin, scaled, decorrelated


def gaussianize(img):
    """Histogram transformation T to the target Gaussian (Listing 1.8)."""
    out = np.empty_like(img)
    n = img.shape[0] * img.shape[1]
    u = (np.arange(n) + 0.5) / n
    g = np.clip(inv_cdf(u), 0.0, 1.0)
    for c in range(3):
        order = np.argsort(img[..., c].ravel())
        chan = np.empty(img.shape[:2])
        chan.reshape(-1)[order] = g
        out[..., c] = chan
    return out


def compute_inverse_lut(img, lut_width=LUT_WIDTH):
    """Inverse histogram transformation T^-1 (Listing 1.9)."""
    lut = np.empty((lut_width, 3))
    g = (np.arange(lut_width) + 0.5) / lut_width
    u = cdf(g)
    for c in range(3):
        sorted_vals = np.sort(img[..., c].ravel())
        idx = np.clip((u * sorted_vals.size).astype(np.int64), 0, sorted_vals.size - 1)
        lut[:, c] = sorted_vals[idx]
    return lut


def circular_box_mean(a, s):
    """Mean over every s-by-s wrap-around window of a 2D array."""
    n0, n1 = a.shape
    b = np.concatenate([a, a[: s - 1]], axis=0)
    c = np.cumsum(b, axis=0)
    c = np.concatenate([np.zeros_like(c[:1]), c], axis=0)
    m = (c[s : n0 + s] - c[0:n0]) / s
    b = np.concatenate([m, m[:, : s - 1]], axis=1)
    c = np.cumsum(b, axis=1)
    c = np.concatenate([np.zeros_like(c[:, :1]), c], axis=1)
    m = (c[:, s : n1 + s] - c[:, 0:n1]) / s
    return m


def prefilter_lut(lut, mip_levels, gaussian_img):
    """Prefiltered LUT (section 1.5): row L is filtered with a Gaussian whose variance is the actual
    average window variance of the Gaussian texture at window size 2^L."""
    rows = np.empty((mip_levels, lut.shape[0], 3))
    for level in range(mip_levels):
        s = min(2 ** level, gaussian_img.shape[0], gaussian_img.shape[1])
        for c in range(3):
            if level == 0:
                var_win = 1e-9
            else:
                e1 = circular_box_mean(gaussian_img[..., c], s)
                e2 = circular_box_mean(gaussian_img[..., c] ** 2, s)
                var_win = max(float((e2 - e1 ** 2).mean()), 1e-9)
            sigma = np.sqrt(var_win) * lut.shape[0]
            half = int(np.ceil(3.0 * sigma)) + 1
            kernel = np.exp(-0.5 * (np.arange(-half, half + 1) / sigma) ** 2)
            kernel /= kernel.sum()
            rows[level, :, c] = np.convolve(
                np.concatenate([lut[-half:, c], lut[:, c], lut[:half, c]]), kernel, mode="same"
            )[half : half + lut.shape[0]]
    return rows


def save_png(data, path):
    Image.fromarray((np.clip(data, 0.0, 1.0) * 255.0).round().astype(np.uint8)).save(path)


def main():
    parser = argparse.ArgumentParser(description="Preprocess a texture for stochastic tiling and blending.")
    parser.add_argument("image", help="Source texture (png/jpg)")
    parser.add_argument("output_dir", nargs="?", default=None, help="Output directory (default: alongside input)")
    args = parser.parse_args()

    source = os.path.abspath(args.image)
    out_dir = os.path.abspath(args.output_dir) if args.output_dir else os.path.dirname(source)
    os.makedirs(out_dir, exist_ok=True)

    img = np.asarray(Image.open(source).convert("RGB")).astype(np.float64) / 255.0
    stem = os.path.splitext(os.path.basename(source))[0]

    print(f"Input: {source} ({img.shape[1]}x{img.shape[0]})")

    mip_levels = int(np.floor(np.log2(min(img.shape[0], img.shape[1])))) + 1

    origin, axes, decorrelated = compute_color_space(img)
    print("Gaussianizing image ...")
    gaussian_img = gaussianize(decorrelated)
    print(f"Computing prefiltered inverse LUT ({LUT_WIDTH} x {mip_levels}) ...")
    lut = prefilter_lut(compute_inverse_lut(decorrelated), mip_levels, gaussian_img)

    gauss_path = os.path.join(out_dir, f"{stem}_gauss.png")
    lut_path = os.path.join(out_dir, f"{stem}_lut.png")
    save_png(gaussian_img, gauss_path)
    # The LUT image is addressed as [y = mip level, x = Gaussian value] in the shader, which matches
    # the row-major (mip_levels, lut_width, channels) layout of the prefiltered LUT directly:
    # width = 256, height = mip levels.
    save_png(lut, lut_path)
    print(f"LUT dimensions: {lut.shape[1]}x{lut.shape[0]} (width x height)")

    def fmt(v):
        return ", ".join(f"{x:.6f}" for x in v)

    print("\nGenerated:")
    print(f"  {gauss_path}")
    print(f"  {lut_path}")
    print("\nIMPORTANT: import both textures as LINEAR (non-sRGB) data in O3DE.")
    print("\nAdd these properties to your material file inside 'propertyValues':\n")
    snippet = {
        "stochasticTiling.enabled": True,
        "stochasticTiling.tilingBlendingEnabled": True,
        "stochasticTiling.gaussianMap": f"{stem}_gauss.png",
        "stochasticTiling.inverseLut": f"{stem}_lut.png",
        "stochasticTiling.colorSpaceOrigin": [round(float(x), 6) for x in origin],
        "stochasticTiling.colorSpaceAxis1": [round(float(x), 6) for x in axes[0]],
        "stochasticTiling.colorSpaceAxis2": [round(float(x), 6) for x in axes[1]],
        "stochasticTiling.colorSpaceAxis3": [round(float(x), 6) for x in axes[2]],
    }
    print(json.dumps(snippet, indent=4))


if __name__ == "__main__":
    main()
