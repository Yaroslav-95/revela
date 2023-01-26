# revela

A static web image gallery generator. It optimizes images for the web and
generates HTML files to create a photo/image gallery web site ready to be served
by an HTML server.

## Building

revela depends on GraphicsMagick (1.3+ tested) and libexif (0.6+ tested).
Optionally also depends on scdoc, if you want to build the man pages.

After just cloning, just need to execute this once:

```sh
git submodule update --init --recursive
```

Then you can proceed to build. This will build revela and generate man pages:

```sh
make
```

If you only want to build:

```sh
make revela
```

Or for debugging:

```sh
DEBUG=1 make revela
```

## Usage

For information on how to use revela, consult `man revela` if installed on your
system, or read the contents in `docs/` in the source.

## TODO:

* Add exif tags to template hashmap.
* Better test coverage? (if I am not too lazy).
* Document templates.
