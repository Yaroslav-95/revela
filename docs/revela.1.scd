revela(1)

# NAME

revela - static image gallery generator.

# SYNOPSIS

*revela* [OPTIONS] -o <OUTPUT DIR>

*revela-init*

# DESCRIPTION

*revela* is a static web image gallery generator. It optimizes images for the web
and generates HTML files to create a photo/image gallery web site ready to be
served by an HTML server.

revela takes a directory with pictures, jinja-like templates and converts that
into a static web gallery of said pictures organized into albums.

The root of the input directory should contain at least the following
directories and files:

*content* - The directory with subdirectories that each contain pictures. Each
subdirectory with pictures is considered an album. Each album should contain an
album.ini file, for information on the format of this file read revela(5).

*static* - Directory with 'static' directories and files to be copied as-is to the
root of the output directory. Here you can put any static assets such as
favicons, css, etc.

*templates* - Here should go the templates for the website. There should be at
least three files: _index.html_, _image.html_ and _album.html_. There can also
be parent templates used by any of those three templates.

*site.ini* - The configuration for the site, such as title, base url and image
optimizations. For more information consult revela(5).

*revela-init* is an interactive helper script that generates a _site.ini_ file
and copies the default templates provided with revela in the working directory
from where it was called.

# OPTIONS

*-i* _DIRECTORY_
	Indicate the input directory. If not indicated it will default to the
	current working dir.

*-o* _DIRECTORY_
	The output directory. This is the only required flag.

*-n*
	Dry run. Show which files would be copied and which html files rendered but
	don't do anything.

*-V*
	Prints version information and exits.

# SEE ALSO

*revela*(5)

# AUTHORS

Yaroslav de la Peña Smirnov <yps@yaroslavps.com>.
