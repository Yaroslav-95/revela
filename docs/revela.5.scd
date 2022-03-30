revela(5)

# NAME

revela - static image gallery generator.

# DESCRIPTION

This manual page documents the format and syntax of the _\*.ini_ or configuration
files found in revela source galleries.

# PREAMBLE

There are two kinds of configuration files:

*site.ini* - There is only one _site.ini_ file in a revela gallery and it should
be located at the root of the gallery directory. This file contains the title of
the gallery, the base url, and image optimization settings.

*album.ini* - Each album should contain a file with this name. This simply
contains the title of the album and optionally a description.

# SYNTAX

Both files are in what's called 'ini format'. It merely consists of _key=value_
pairs and _[sections]_. You should already be familiar with it.

## SITE CONFIGURATION

_site.ini_ may contain the following sections and keys:

*title*=string
	The name or title of the gallery to be used inside templates.

*base_url*=string
	The base url. For example, if the web gallery is not at the root of the
	website, it could be "/photos". _Optional_.

*[images]*
	This section contains settings for optimization of the main image files.
	_This section and all its keys are optional_.

	*strip*=boolean
		Whether to strip images of their EXIF tags and other metainformation.

	*quality*=integer
		From 0 to 100, 0 being the lowest quality and highest compression, and
		100 being the highest quality but no compression. This corresponds to
		JPEG "quality" levels.

	*max_width*=integer
		The maximum width the image file should have in pixels.

	*max_height*=integer
		The maximum height the image file should have in pixels.

	*smart_resize*=boolean
		Whether to preserve the aspect ratio of the image when resizing.

	*blur*=integer
		A value from 0 to 100, where 0 is no blur and 100 is the maximum amount
		of blur.

*[thumbnails]*
	This section contains settings for optimization of the thumbnails files of
	images. All of the keys in this section are the same as in the _images_
	sections. _This section and all its keys are optional_.

## ALBUM CONFIGURATION

_album.ini_ may contain the following keys:

*title*=string
	The name of the album.

*desc*=string
	A short description of the album. _Optional_.

# NOTES

If you initialize your gallery with the helper script _revela-init_ a _site.ini_
should be generated. You can tweak the settings by using it as a basis.

# SEE ALSO

*revela*(1)

# AUTHORS

Yaroslav de la Peña Smirnov <yps@yaroslavps.com>.
