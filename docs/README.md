# AREA512 website

Static English website. No build step or runtime dependencies.

## Local preview

From the repository root:

```sh
python3 -m http.server 8000 --directory sites
```

Open `http://localhost:8000/`.

## GitHub Pages

Publish the **contents of `sites/`** as the Pages artifact. All local URLs are relative, so the site works at both a domain root and a repository path such as `/area512/`. No custom domain is assumed.

GitHub Pages branch publishing supports a repository root or `/docs`, not `/sites`. Use a Pages Actions workflow that uploads `sites/`, or copy its contents to the publishing branch root. No deployment workflow has been enabled by this change.

## Content

- `index.html`: the four approved headlines, translated into English.
- `term512.html`: TERM512 enclosure, external display, locks, and folding stand.
- `applications.html`, `programming.html`, `install.html`, `manual.html`: factual descriptions and instructions.
- `*-api.html`: API references from repository documentation.
- `assets/images/`: copies of the repository's photographs and application screens.
- `AREA512_emblem.png`: supplied emblem; color treatment is applied with CSS.
- `firmware/`: copies of the four repository firmware images. Update these when replacing the source firmware.
- `assets/fonts/`: DejaVu Sans Bold and its license.

Update the corresponding site content when repository features or APIs change. The source and issue links point to `engneer-hamachan/area512`.

The cover also uses CSS crops of the supplied reference advertisement. Paper texture source and generation notes are in `assets/IMAGE-NOTES.md`. The narrow text faces are Liberation Sans Narrow, distributed with `assets/fonts/LIBERATION-LICENSE.txt`.
