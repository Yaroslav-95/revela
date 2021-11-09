# revela

A static web image gallery generator. It optimizes images for the web and
generates HTML files to create a photo/image gallery web site ready to be served
by an HTML server.

VERY EARLY STAGES! It's semifunctional but not very useful yet.

## Building

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
* Clean out-of-date dirs and files (i.e. removed albums/images from the source
  dir).
* Improve unja or find better alternative.
* Better test coverage? (if I am not too lazy)
* Document.
