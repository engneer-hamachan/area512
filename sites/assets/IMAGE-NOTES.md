# Image sources

## Current paper treatment

`paper-fiber-v2.png` was generated with the built-in imagegen tool. It replaces
the bordered paper textures in the live stylesheet. The substrate and surface
relief share a 768 px scale and origin; the relief is dark-neutral to avoid
lifting black ink. Outer wear is confined to the sheet perimeter. Ink wear uses
the existing transparent `ink-wear.svg`, without repeated sheet edges or folds.
Photographs share a warm print tone; screen sources additionally compensate for
their baked-in lifted blacks. Fine screen dots are CSS overlays, not edits to
the source photographs.

Generation prompt:

> Use case: photorealistic-natural. Asset: seamless repeating paper substrate texture for an aged 1980s computer magazine advertisement website. Generate a square 1024x1024 full-bleed macro scan of warm cream uncoated printing paper, fine natural fibers, subtle irregular age mottling and sparse tiny embedded specks. The whole image is an interior section of a much larger sheet, evenly lit, same density at every edge. Tile seamlessly on all four sides. Pale warm ivory base, restrained fine tactile grain, a little authentic age, no large stains. Absolutely NO borders, worn edges, frames, folds, creases, shadows, vignette, tears, text, pictures or objects. Not parchment, not stone, not digital noise. Texture should remain visible but support small printed text.

The older generation notes below document retained source assets, not the
current page background.

- `images/reference-ad.png` is the user-supplied `siteimage.png`. The cover uses CSS viewports to show its enclosure, operator, outpost, and technical drawing without modifying the source raster. These are labeled as concept illustrations; hardware specifications describe the actual supported Cardputer devices.
- `paper-stock.png` was generated with the built-in imagegen tool using `siteimage.png` as the material reference.

## Paper generation prompt

Create one full-bleed portrait sheet of aged printing paper matching the pale cream newsprint substrate in the supplied computer advertisement. Remove all text, pictures, logos, lines and colored panels. Use mottled paper fibers, irregular light oxidation, tiny embedded dark specks, faint print wear, and thin softly rubbed margins. Keep the inner 85 percent pale and calm for readable text. No digital noise, brown parchment, heavy frame, vignette, tears, burn marks, objects or curled corners. This is a material background for HTML, not a website screenshot.

## Black printed stock

`black-paper-stock.png` was generated with the built-in imagegen tool using the reference advertisement's black sidebar as the material reference. Prompt: a full-bleed portrait rectangle of nearly black matte ink on warm cream fibrous paper, with patchy ink density, tiny paper pinholes, fine rubbed scratches, scuffed narrow edges and faint folds. Match the reference's physically printed surface. Remove all text, logos, planets, photos, colored bars and objects. Keep the center dark enough for cream menu text, without regular procedural noise or modern gradients.

## Screen photographs

`images/*-screen.jpg` come from `image/*_ui.jpg` (photographs of the device
display). Each one is cropped to the drawn frame plus 1.2 percent, normalised to
1.45:1, resized to 1160 px wide and saved at JPEG quality 82. Every channel is
then remapped from `[0,1]` to `[0.30,1]`, which raises the black field of the
display.

`img.screen-photo` replaces the filter the surrounding rules set for the pixel
artwork, which took `grayscale` to .65 and `saturate` to .45. It keeps the
display's orange and pulls the raised black field back down with `contrast`. The
plates measure a mean luminance of 145 on `applications.html`, against 176 for
the `paint.png` artwork in the neighbouring slot. The same rule turns off
`image-rendering:pixelated`.
