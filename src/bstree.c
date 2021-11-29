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
#include "bstree.h"

#include <stdlib.h>

struct bstree *
bstree_new(bst_cmp_fn bstcmp, bst_free_fn bstfree)
{
	struct bstree *tree = malloc(sizeof *tree);
	tree->root = NULL;
	tree->cmp = bstcmp;
	tree->free = bstfree;

	return tree;
}

struct bstnode *
bstree_add(struct bstree *tree, void *val)
{
	struct bstnode **cur;
	struct bstnode *prev = NULL;
	cur = &tree->root;
	while (*cur != NULL) {
		prev = *cur;
		if (tree->cmp(val, prev->value) < 0) {
			cur = &prev->left;
		} else {
			cur = &prev->right;
		}
	}
	*cur = calloc(1, sizeof **cur);
	(*cur)->value = val;
	(*cur)->parent = prev;

	return *cur;
}

struct bstnode *
bstree_search(struct bstree *tree, void *val)
{
	struct bstnode *node = tree->root;
	while (node != NULL) {
		int res;
		if ((res = tree->cmp(val, node->value)) == 0) {
			break;
		}
		if (res < 0) {
			node = node->left;
		} else {
			node = node->right;
		}
	}

	return node;
}

#define bstree_extreme(node, d) \
	while (node != NULL && node->d != NULL) { \
		node = node->d; \
	}

struct bstnode *
bstree_max(struct bstnode *node)
{
	bstree_extreme(node, right)
	return node;
}

struct bstnode *
bstree_min(struct bstnode *node)
{
	bstree_extreme(node, left)
	return node;
}

#define bstree_xcessor(na, d, m) \
	if (na->d != NULL) { \
		return bstree_##m(na->d); \
	} \
	struct bstnode *nb = na->parent; \
	while (nb != NULL && nb->d == na) { \
		na = nb; \
		nb = nb->parent; \
	} \
	return nb

struct bstnode *
bstree_successor(struct bstnode *na)
{
	bstree_xcessor(na, right, min);
}

struct bstnode *
bstree_predecessor(struct bstnode *na)
{
	bstree_xcessor(na, left, max);
}

bool
bstree_inorder_walk(struct bstnode *node, bst_walk_cb cb, void *data)
{
	if (node->left != NULL) {
		if (!bstree_inorder_walk(node->left, cb, data)) return false;
	}
	if (!cb(node, data)) return false;
	if (node->right != NULL) {
		return bstree_inorder_walk(node->right, cb, data);
	}
	return true;
}

static void
bstree_transplant(struct bstree *tree, struct bstnode *a, struct bstnode *b)
{
	if (a->parent == NULL) {
		tree->root = b;
	} else if (a == a->parent->left) {
		a->parent->left = b;
	} else {
		a->parent->right = b;
	}
	if (b != NULL) {
		b->parent = a->parent;
	}
}

void
bstree_remove(struct bstree *tree, struct bstnode *na)
{
	if (na->left == NULL) {
		bstree_transplant(tree, na, na->right);
	} else if (na->right == NULL) {
		bstree_transplant(tree, na, na->left);
	} else {
		struct bstnode *nb = bstree_min(na->right);
		if (nb->parent != na) {
			bstree_transplant(tree, nb, nb->right);
			nb->right = na->right;
			nb->right->parent = nb;
		}
		bstree_transplant(tree, na, nb);
		nb->left = na->left;
		nb->left->parent = nb;
	}
	tree->free(na->value);
	free(na);
}

static void
bstree_subdestroy(struct bstree *tree, struct bstnode *node)
{
	if (node->right) bstree_subdestroy(tree, node->right);
	if (node->left) bstree_subdestroy(tree, node->left);
	tree->free(node->value);
	free(node);
}

void
bstree_destroy(struct bstree *tree)
{
	if (tree->root) bstree_subdestroy(tree, tree->root);
	free(tree);
}
