/*
 * Copyright 2021 Yaroslav de la Peña Smirnov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef BSTREE_H
#define BSTREE_H

#include <stdbool.h>

/* < 0 means a less than b; 0 means equal; > 0 means a more than b. */
typedef int (*bst_cmp_fn)(const void *a, const void *b);
typedef void (*bst_free_fn)(void *data);

struct bstree {
	struct bstnode *root;
	bst_cmp_fn cmp;
	/* It can be NULL, in which case you will have to free the values manually */
	bst_free_fn free;
};

struct bstnode {
	void *value;
	struct bstnode *parent;
	struct bstnode *left;
	struct bstnode *right;
};

typedef bool (*bst_walk_cb)(struct bstnode *, void *data);

struct bstree *bstree_new(bst_cmp_fn, bst_free_fn);

struct bstnode *bstree_add(struct bstree *, void *val);

/* Returns NULL if a node with the value wasn't found */
struct bstnode *bstree_search(struct bstree *, void *val);

struct bstnode *bstree_min(struct bstnode *);

struct bstnode *bstree_max(struct bstnode *);

struct bstnode *bstree_predecessor(struct bstnode *);

struct bstnode *bstree_successor(struct bstnode *);

bool bstree_inorder_walk(struct bstnode *, bst_walk_cb, void *data);

void bstree_remove(struct bstree *, struct bstnode *);

void bstree_destroy(struct bstree *);

#endif
