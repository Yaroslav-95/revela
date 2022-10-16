# revela

A static web image gallery generator. It optimizes images for the web and
generates HTML files to create a photo/image gallery web site ready to be served
by an HTML server.

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

## Usage

For information on how to use revela, consult `man revela` if installed on your
system, or read the contents in `docs/` in the source.

## TODO:

* Add exif tags to template hashmap.
* Better test coverage? (if I am not too lazy).
* Document templates.
