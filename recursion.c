#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    int data;
    struct Node *next;
};

void swap(char *x, char *y) {
    char temp = *x;
    *x = *y;
    *y = temp;
}

int ArraySum(int arr[], int n) {
    if (n == 0) return 0;
    return arr[n-1] + ArraySum(arr, n-1);
}

void reverse(char arr[], int n) {
    if (n <= 1) return;
    char temp = arr[0];
    arr[0] = arr[n-1];
    arr[n-1] = temp;
    reverse(arr + 1, n - 2);
}

int isPalindrome(char arr[], int n) {
    if (n <= 1) return 1;
    if (arr[0] != arr[n-1]) return 0;
    return isPalindrome(arr + 1, n - 2);
}

int power(int x, int y) {
    if (y == 0) return 1;
    return x * power(x, y - 1);
}

void permutation(char arr[], int start, int n) {
    if (start == n - 1) {
        printf("%s\n", arr);
        return;
    }
    for (int i = start; i < n; i++) {
        swap(&arr[start], &arr[i]);
        permutation(arr, start + 1, n);
        swap(&arr[start], &arr[i]);
    }
}

int length(struct Node *head) {
    if (head == NULL) return 0;
    return 1 + length(head->next);
}

int search(struct Node *head, int key) {
    if (head == NULL) return 0;
    if (head->data == key) return 1;
    return search(head->next, key);
}

struct Node* merge(struct Node *p, struct Node *q) {
    if (p == NULL) return q;
    if (q == NULL) return p;
    if (p->data <= q->data) {
        p->next = merge(p->next, q);
        return p;
    } else {
        q->next = merge(p, q->next);
        return q;
    }
}

struct Node *reverseList(struct Node *head) {
    if (head == NULL || head->next == NULL) return head;

    struct Node *last = reverseList(head->next);
    head->next->next = head;
    head->next = NULL;
    return last;
}

struct Node* newNode(int d) {
    struct Node *n = malloc(sizeof(struct Node));
    if (n == NULL) exit(1);
    n->data = d;
    n->next = NULL;
    return n;
}

void printList(struct Node *h) {
    for (; h != NULL; h = h->next) printf("%d ", h->data);
    printf("\n");
}

void freeList(struct Node *h) {
    while (h != NULL) {
        struct Node *t = h;
        h = h->next;
        free(t);
    }
}

int main() {
    // ArraySum
    int nums[] = {1, 2, 3, 4, 5};
    printf("ArraySum: %d\n", ArraySum(nums, 5));

    // reverse
    char s1[] = "abcd";
    reverse(s1, strlen(s1));
    printf("reverse(abcd): %s\n", s1);

    // isPalindrome
    char s2[] = "madam";
    char s3[] = "hello";
    printf("isPalindrome(madam): %d\n", isPalindrome(s2, strlen(s2)));
    printf("isPalindrome(hello): %d\n", isPalindrome(s3, strlen(s3)));

    // power
    printf("power(2,10): %d\n", power(2, 10));

    // permutation
    char s4[] = "abc";
    printf("permutations of abc:\n");
    permutation(s4, 0, strlen(s4));

    // linked list: merge, length, search, reverseList
    struct Node *a = newNode(1); a->next = newNode(3);
    struct Node *b = newNode(2); b->next = newNode(4);

    struct Node *m = merge(a, b);
    printf("merged: ");
    printList(m);
    printf("length: %d\n", length(m));
    printf("search 3: %d\n", search(m, 3));
    printf("search 9: %d\n", search(m, 9));

    m = reverseList(m);
    printf("reversed: ");
    printList(m);

    freeList(m);
    return 0;
}