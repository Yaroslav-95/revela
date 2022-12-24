# revela

A static web image gallery generator. It optimizes images for the web and
generates HTML files to create a photo/image gallery web site ready to be served
by an HTML server.

## Building

revela depends on GraphicsMagick (1.3+ tested) and libexif (0.6+ tested).

After just cloning, just need to execute this once:

```sh
git submodule update --init --recursive
```

Then you can proceed to build:

```sh
make
```

Or for debugging:

```sh
DEBUG=1 make
```

## Usage

For information on how to use revela, consult `man revela` if installed on your
system, or read the contents in `docs/` in the source.

## TODO:

* Add exif tags to template hashmap.
* Better test coverage? (if I am not too lazy).
* Document templates.
* Improve performance? by generating html in a separate thread, e.g. send the
  jobs to a queue in a separate thread in order to render at the same time as
  images are converted with GraphicsMagick. Not sure if it would actually
  improve performance or be worth the added complexity.
