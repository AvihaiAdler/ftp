#include "bst.h"

#include <stdlib.h>
#include <string.h>

struct node {
  void *key;
  void *value;

  struct node *parent;
  struct node *left;
  struct node *right;
};

struct bst {
  struct node *root;

  int (*cmpr)(void *key, void *other);
  void (*destroy_key)(void *key);
  void (*destroy_value)(void *value);
};

static struct node *node_create(const void *const key, size_t key_size, const void *const value, size_t value_size) {
  struct node *node = calloc(1, sizeof *node);
  if (!node) return NULL;

  node->key = malloc(key_size);
  if (!node->key) {
    free(node);
    return NULL;
  }

  memcpy(node->key, key, key_size);

  if (value && value_size) {
    node->value = malloc(value_size);
    if (!node->value) {
      free(node->key);
      free(node);
      return NULL;
    }

    memcpy(node->value, value, value_size);
  }

  return node;
}

static void node_swap(struct node *node, struct node *other) {
  if (!node || !other) return;

  void *key = node->key;
  void *value = node->value;

  node->key = other->key;
  node->value = other->value;

  other->key = key;
  other->value = value;
}

static void node_destroy(struct node *node, void (*destroy_key)(void *key), void (*destroy_value)(void *value)) {
  if (!node) return;

  node_destroy(node->left, destroy_key, destroy_value);
  node_destroy(node->right, destroy_key, destroy_value);

  if (destroy_key) destroy_key(node->key);
  if (destroy_value) destroy_value(node->value);
  free(node->key);
  free(node->value);
  free(node);
}

static struct node *node_insert(struct node *root,
                                struct node *node,
                                int (*cmpr)(void *key, void *other),
                                void (*destroy_key)(void *key),
                                void (*destroy_value)(void *value)) {
  if (!node) return NULL;

  if (!root) return node;

  // duplicate keys aren't allowed. if cmpr returns 0, the function will update
  // the value which was paired with key
  struct node *ret = root;

  int cmpr_res = cmpr(root->key, node->key);
  if (cmpr_res == 0) {
    node_swap(root, node);
    node_destroy(node, destroy_key, destroy_value);
  } else if (cmpr_res > 0) {
    // node::key < root::key
    ret->left = node_insert(root->left, node, cmpr, destroy_key, destroy_value);
    ret->left->parent = ret;
  } else if (cmpr_res < 0) {
    // node::key > root::key
    ret->right = node_insert(root->right, node, cmpr, destroy_key, destroy_value);
    ret->right->parent = ret;
  }

  return ret;
}

static void *node_find(struct node *node, void *key, int (*cmpr)(void *key, void *other)) {
  if (!node) return NULL;

  // if cmpr_ptr == 0 - the function will return node::value
  void *ret_ptr = node->value;

  int cmpr_ret = cmpr(node->key, key);
  if (cmpr_ret > 0) {
    // key < node::key
    ret_ptr = node_find(node->left, key, cmpr);
  } else if (cmpr_ret < 0) {
    // key > node::key
    ret_ptr = node_find(node->right, key, cmpr);
  }
  return ret_ptr;
}

static struct node *find_predecessor(struct node *node) {
  if (!node) return NULL;  // should never happens

  if (!node->left) return node;

  return find_predecessor(node->left);
}

static struct node *find_successor(struct node *node) {
  if (!node) return NULL;  // should never happens

  if (!node->right) return node;

  return find_successor(node->left);
}

static struct node *node_delete(struct node *node,
                                void *key,
                                int (*cmpr)(void *key, void *other),
                                void (*destroy_key)(void *key),
                                void (*destroy_value)(void *value)) {
  if (!node) return NULL;

  struct node *ret = node;

  int cmpr_ret = cmpr(node->key, key);
  if (cmpr_ret == 0) {
    // node has no children
    if (!node->left && !node->right) {
      ret = NULL;
      node_destroy(node, destroy_key, destroy_value);
    } else if (!node->left) {
      // node has a child on the right
      ret = node->right;
      node_destroy(node, destroy_key, destroy_value);
    } else if (!node->right) {
      // node has a child on the left
      ret = node->left;
      node_destroy(node, destroy_key, destroy_value);
    } else {
      // node has 2 children
      struct node *smallest = find_predecessor(node->right);
      node_swap(node, smallest);

      smallest->parent->left = NULL;

      node_destroy(smallest, destroy_key, destroy_value);
    }
  } else if (cmpr_ret > 0) {
    // key < node::key
    node->left = node_delete(node->left, key, cmpr, destroy_key, destroy_value);
  } else {
    // key > node::key
    node->right = node_delete(node->right, key, cmpr, destroy_key, destroy_value);
  }

  return ret;
}

struct bst *bst_create(int (*cmpr)(void *key, void *other),
                       void (*destroy_key)(void *key),
                       void (*destroy_value)(void *value)) {
  if (!cmpr) return NULL;

  struct bst *bst = calloc(1, sizeof *bst);
  if (!bst) return NULL;

  bst->cmpr = cmpr;
  bst->destroy_key = destroy_key;
  bst->destroy_value = destroy_value;
  return bst;
}

static void node_print(struct node *node, void (*print_key)(void *key), void (*print_value)(void *value)) {
  if (!node) return;

  node_print(node->left, print_key, print_value);
  if (print_key) print_key(node->key);
  if (print_value) print_value(node->value);
  node_print(node->right, print_key, print_value);
}

void bst_destroy(struct bst *bst) {
  if (!bst) return;

  node_destroy(bst->root, bst->destroy_key, bst->destroy_value);
  free(bst);
}

bool bst_upsert(struct bst *bst, const void *const key, size_t key_size, const void *const value, size_t value_size) {
  if (!bst) return false;

  if (!key || !key_size) return false;

  struct node *tmp = node_create(key, key_size, value, value_size);
  if (!tmp) return false;

  bst->root = node_insert(bst->root, tmp, bst->cmpr, bst->destroy_key, bst->destroy_value);

  return true;
}

void *bst_find(struct bst *bst, void *key) {
  if (!bst || !key) return NULL;

  return node_find(bst->root, key, bst->cmpr);
}

bool bst_delete(struct bst *bst, void *key) {
  if (!bst || !key) return NULL;

  bst->root = node_delete(bst->root, key, bst->cmpr, bst->destroy_key, bst->destroy_value);

  return true;
}

void bst_print(struct bst *bst, void (*print_key)(void *key), void (*print_value)(void *value)) {
  if (!bst) return;

  node_print(bst->root, print_key, print_value);
}
