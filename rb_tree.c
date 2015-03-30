#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_NAME_LEN 10
#define BLACK 1
#define RED 0
struct Node {
	char user_name[MAX_NAME_LEN];
	int socket_fd;
	int color;
	struct Node *left;
	struct Node *right;
	struct Node *parent;
};

static struct Node *NIL;
struct Node *head;

/*初始化
 */
void initialize() {
	NIL = (struct Node*)malloc(sizeof(struct Node));
	NIL->color = BLACK;
	head = NIL;
}

struct Node *create_node(char *name, int len, int fd) {
	if (len < 0 || len > MAX_NAME_LEN)
		return NIL;
	struct Node *node = (struct Node*)malloc(sizeof(struct Node));
	memcpy(node->user_name, name, len);
	node->socket_fd = fd;
	node->color = RED;
	node->parent = NIL;
	node->left = NIL;
	node->right = NIL;
	return node;
}

struct Node *minimum(struct Node *root) {
	if (root == NIL) return NIL;
	struct Node *tmp = root;
	while (tmp->left != NIL) {
		tmp = tmp->left;
	}
	return tmp;
}

struct Node *maximum(struct Node *root) {
	if (root == NIL) return NULL;
	struct Node *tmp = root;
	while (tmp->right != NIL) {
		tmp = tmp->right;
	}
	return tmp;
}

void left_rotate(struct Node *node) {
	struct Node *p = node->parent;
	struct Node *r = node->right;
	node->right = r->left;
	if (r->left != NIL)
		r->left->parent = node;
	r->parent = p;
	if (p == NIL) {
		head = r;
	} else if (node == p->left) {
		p->left = r;
	} else {
		p->right = r;
	}
	r->left = node;
	node->parent = r;
}

void right_rotate(struct Node *node) {
	struct Node *p = node->parent;
	struct Node *l = node->left;
	node->left = l->right;
	if (l->right != NIL) {
		l->right->parent = node;
	}
	l->parent = p;
	if (p == NIL) {
		head = l;
	} else if (node == p->left) {
		p->left = l;
	} else {
		p->right = l;
	}
	l->right = node;
	node->parent = l;
}

void insert_fix(struct Node *node) {
	while (node->parent->color == RED) {
		if (node->parent == node->parent->parent->left) {
			struct Node *tmp = node->parent->parent->right;
			if (tmp->color == RED) {
				tmp->color = BLACK;
				node->parent->color = BLACK;
				node->parent->parent->color = RED;
				node = node->parent->parent;
			} else {
				if (node == node->parent->right) {
					node = node->parent;
					left_rotate(node);
				}
				node->parent->color = BLACK;
				node->parent->parent->color = RED;
				right_rotate(node->parent->parent);
			}
		} else {
			struct Node *tmp = node->parent->parent->left;
			if (tmp->color == RED) {
				tmp->color = BLACK;
				node->parent->color =BLACK;
				node->parent->parent->color = RED;
				node = node->parent->parent;
			} else {
				if (node == node->parent->left) {
					node = node->parent;
					right_rotate(node);
				}
				node->parent->color = BLACK;
				node->parent->parent->color = RED;
				left_rotate(node->parent->parent);
			}
		}
	}
	head->color = BLACK;
}
			

int insert(struct Node *node) {
	if (head == NIL) {
		node->parent = node->left = node->right = NIL;
		node->color = BLACK;
		head = node;
		return 1;
	}
	struct Node *cur = head, *pre = head;
	int flag;
	while (cur != NIL) {
		pre = cur;
		flag = strcmp(node->user_name, cur->user_name);
		if (flag < 0) {
			cur = cur->left;
		} else if (flag > 0) {
			cur = cur->right;
		} else {
			return 0;
		}
	}
	if (strcmp(pre->user_name, node->user_name) > 0)
		pre->left = node;
	else
		pre->right = node;
	node->parent = pre;
	node->color = RED;
	node->left = NIL;
	node->right = NIL;
	insert_fix(node);
	return 1;
}

void tranplant(struct Node *node1, struct Node *node2) {
	struct Node *p = node1->parent;
	if (p == NIL) {
		node2->parent = NIL;
		head = node2;
		return;
	}
	node2->parent = p;
	if (node1 == p->left) {
		p->left = node2;
	} else {
		p->right = node2;
	}
}

void delete_fix(struct Node *node) {
	while (node != head && node->color == BLACK) {
		if (node == node->parent->left) {
			struct Node *tmp = node->parent->right;
			if (tmp->color == RED) {
				tmp->color = BLACK;
				tmp->parent->color = RED;
				left_rotate(tmp->parent);
				tmp = node->parent->right;
			}
			if (tmp->left->color == BLACK && tmp->right->color == BLACK) {
				tmp->color = RED;
				node = node->parent;
			} else {
				if (tmp->right->color == BLACK) {
					tmp->color = RED;
					tmp->left->color = BLACK;
					right_rotate(tmp);
					tmp = tmp->parent;
				}
				tmp->color = tmp->parent->color;
				tmp->parent->color = BLACK;
				tmp->right->color = BLACK;
				left_rotate(tmp->parent);
				node = head;
			}
		} else {
			struct Node *tmp = node->parent->left;
			if (tmp->color == RED) {
				tmp->color = BLACK;
				tmp->parent->color = RED;
				right_rotate(tmp->parent);
				tmp = node->parent->left;
			}
			if (tmp->left->color == BLACK && tmp->right->color == BLACK) {
				tmp->color = RED;
				node = node->parent;
			} else {
				if (tmp->left->color == BLACK) {
					tmp->color = RED;
					tmp->right->color = BLACK;
					left_rotate(tmp);
					tmp = tmp->parent;
				}
				tmp->color = tmp->parent->color;
				tmp->parent->color = BLACK;
				tmp->left->color = BLACK;
				right_rotate(tmp->parent);
				node = head;
			}
		}
	}
	node->color = BLACK;
}


int delete(struct Node *node) {
	int color = node->color;
	struct Node *p = node->parent;
	struct Node *next;
	if (node->left == NIL) {
		next = node->right;
		tranplant(node, next);
	} else if (node->right == NIL) {
		next = node->left;
		tranplant(node, next);
	} else {
		struct Node *suc = minimum(node->right);
		color = suc->color;
		next = suc->right;
		if (suc->parent == node) {
			next->parent = suc;
		} else {
			tranplant(suc, next);
			suc->right = node->right;
			suc->right->parent = suc;
		}
		tranplant(node, suc);
		suc->left = node->left;
		suc->left->parent = suc;
		suc->color = node->color;
	}
	free(node);
	if (color == BLACK)
		delete_fix(next);
}


struct Node *search(const char *name) {
	if(head == NIL) return NIL;
	struct Node *cur = head, *pre = head;
	int flag;
	while (cur != NIL) {
		flag = strcmp(name, cur->user_name);
		if (flag == 0) return cur;
		else if (flag > 0) cur = cur->right;
		else cur = cur->left;
	}
	return cur;
}

void print_tree(struct Node *root) {
	if (root == NIL) return;
	printf("%s, %d, %d\n", root->user_name, root->socket_fd, root->color);
	print_tree(root->left);
	print_tree(root->right);
}
void print() {
	printf("%d\n", max_depth(head));
	//print_tree(head);
}

int max_depth(struct Node *root) {
	if (root == NIL) return 0;
	int mleft = max_depth(root->left);
	int mright = max_depth(root->right);
	if (mleft > mright) return mleft + 1;
	else return mright + 1;
}
