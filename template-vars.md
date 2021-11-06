# Variables

A very rough and dirty representation of the variables in each template.

## Common for all templates

* `title`
* `index`

## index.html

* `albums` (vector)
* `years` (vector)
	- `name`
	- `albums` (vector)

## album.html

* `album`
	- `title`
	- `desc`
	- `link`
	- `date`
	- `previews` (vector of thumbs)
	- `thumbs` (vector)
		- `link`
		- `source`

## image.html

* `album`
* `image`
	- `exif` **TODO!**
	- `date`
	- `source`
	- `prev`
	- `next`
