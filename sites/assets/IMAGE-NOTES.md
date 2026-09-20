# Image sources

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
