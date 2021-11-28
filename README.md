# revela

A static web image gallery generator. It optimizes images for the web and
generates HTML files to create a photo/image gallery web site ready to be served
by an HTML server.

EARLY STAGES! It's functional but still rough around the edges.

## Building

revela depends on GraphicsMagick (1.3+ tested) and libexif (0.6+ tested).

```
git submodule update --init
make
```

or for debugging:

```
DEBUG=1 make
```

## TODO:

* Fix template generation on outdated html files. Right now if, for example, a
  new image was added, the templates for the next and prev images, and the album
  are not updated accordingly.
* Add exif tags to template hashmap.
* Improve unja or find better alternative.
* Get rid of previews variable. It is a temporary hack to limit the amount of
  thumbnails for each album in the index.
* Better test coverage? (if I am not too lazy)
* Document.
