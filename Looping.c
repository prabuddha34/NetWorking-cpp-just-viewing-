#include <stdio.h>

void display(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void bubbleSort(int arr[], int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j=0;j<n-i-1;j++) {
            if (arr[j] > arr[j+1]) {
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}

void optimizedBubbleSort(int arr[], int n) {
    for (int i = 0; i < n-1; i++) {
        int swapped =0;
        for (int j=0;j<n-i-1;j++) {
            if (arr[j] > arr[j+1]) {
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
                swapped = 1;
            }
        }
        if (swapped == 0) {
            break;
        }
    }
}

void selectionSort(int arr[], int n) {
    for (int i = 0; i < n;i++){
        int min_index = i;
        for (int j=i+1;j<n;j++) {
            if (arr[j]<arr[min_index])
                min_index = j;
        }
        int temp=arr[i];
        arr[i] = arr[min_index];
        arr[min_index] = temp;
    }
}

void insertionSort(int arr[], int n) {
    for (int i=0;i<n;i++) {
        int key=arr[i];
        int j=i-1;
        while (j>=0 && arr[j]>key) {
            arr[j+1]=arr[j];
            j--;
        }
        arr[j+1]=key;
    }
}

void merge(int arr[],int low,int mid,int high) {
    int i=low,j=mid+1,k=low;
    int temp[1024];
    while (i<=mid && j<=high) {
        if (arr[i]<=arr[j]) {
            temp[k++]=arr[i++];
        }
        else {
            temp[k++]=arr[j++];
        }
    }
    while (i<=mid)
        temp[k++]=arr[i++];
    while (j<=high)
        temp[k++]=arr[j++];
    for (int x=low;x<=high;x++)      // fixed: <= high
        arr[x]=temp[x];
}

void mergeSort(int arr[],int low,int high) {
    if (low<high) {
        int mid=(low+high)/2;
        mergeSort(arr,low,mid);
        mergeSort(arr,mid+1,high);
        merge(arr,low,mid,high);
    }
}

int partition(int arr[],int low,int high) {
    int i=low-1,j,temp;
    int pivot=arr[high];             // fixed: pivot set once, outside loop
    for (j=low;j<high;j++) {
        if (arr[j]<pivot) {
            i++;
            temp=arr[i];
            arr[i]=arr[j];
            arr[j]=temp;
        }
    }
    temp=arr[i+1];                   // fixed: swap moved outside loop
    arr[i+1]=arr[high];
    arr[high]=temp;
    return i+1;
}

void quickSort(int arr[],int low,int high) {
    if (low<high) {
        int p=partition(arr,low,high);
        quickSort(arr,low,p-1);
        quickSort(arr,p+1,high);
    }
}

int main() {
    int choice,n;
    printf("Choose: 1 Bubble, 2 Optimized Bubble, 3 Selection, 4 Insertion, 5 Merge, 6 Quick: ");
    scanf("%d", &choice);
    printf("Enter the array size (n): ");
    scanf("%d", &n);
    int arr[n];
    printf("Enter %d elements: ", n);
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }
    switch (choice) {
        case 1:
            bubbleSort(arr, n);
            break;
        case 2:
            optimizedBubbleSort(arr, n);
            break;
        case 3:
            selectionSort(arr, n);
            break;
        case 4:
            insertionSort(arr, n);
            break;
        case 5:
            mergeSort(arr, 0, n-1);
            break;
        case 6:
            quickSort(arr, 0, n-1);
            break;
        default:
            printf("Invalid choice\n");
            return 1;
    }

    printf("Sorted array: ");
    display(arr, n);

    return 0;
}