#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

struct Node {
    int data;
    struct Node *left , *right;
};

struct Node* create() {
    int x;
    printf("Enter the data and -1 for NULL: ");
    scanf("%d",&x);
    if (x == -1) {
        return NULL;
    }
    struct Node *newNode=malloc(sizeof(struct Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    newNode->data=x;
    printf("Left child of %d:\n",x);
    newNode->left=create();

    printf("Right child of %d:\n",x);
    newNode->right=create();
    return newNode;
}

void inorder(struct Node *root) {
    if (root == NULL) {
        return;
    }
    inorder(root->left);
    printf("%d ",root->data);
    inorder(root->right);
}

void preorder(struct Node *root) {
    if (root == NULL) {
        return;
    }
    printf("%d ",root->data);
    preorder(root->left);
    preorder(root->right);
}

void postorder(struct Node *root) {
    if (root == NULL) {
        return;
    }
    postorder(root->left);
    postorder(root->right);
    printf("%d ",root->data);
}

int countLeafNodes(struct Node *root) {
    if (root == NULL) {
        return 0;
    }
    if (root->left == NULL && root->right == NULL) {
        return 1;
    }
    return countLeafNodes(root->left) + countLeafNodes(root->right);
}

int height(struct Node *root) {
    if (root == NULL) {
        return 0;                                   // fixed: was 1
    }
    int leftHeight = height(root->left);
    int rightHeight = height(root->right);

    return ((leftHeight>rightHeight)?leftHeight:rightHeight) + 1;   // fixed: +1
}

int search(struct Node *root, int x) {
    if (root == NULL) {
        return 0;
    }
    if (root->data == x) {
        return 1;
    }
    return search(root->left,x) || search(root->right,x);           // fixed: || not &&
}

int findMin(struct Node *root) {                    // fixed: works for any binary tree
    if (root == NULL) {
        return INT_MAX;
    }
    int m = root->data;
    int l = findMin(root->left);
    int r = findMin(root->right);
    if (l < m) m = l;
    if (r < m) m = r;
    return m;
}

void freeTree(struct Node *root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

int main() {
    struct Node *root = create();
    printf("The tree is created\n");

    printf("Inorder traversal: ");
    inorder(root);

    printf("\nPreorder traversal: ");
    preorder(root);

    printf("\nPostorder traversal: ");
    postorder(root);

    printf("\nLeaf nodes: %d\n", countLeafNodes(root));
    printf("Height: %d\n", height(root));

    int x;
    printf("Enter a value to search: ");
    scanf("%d", &x);
    printf("%d is %s\n", x, search(root, x) ? "found" : "not found");

    if (root != NULL)
        printf("Minimum value: %d\n", findMin(root));

    freeTree(root);
    return 0;
}