#include <bits/stdc++.h>
using namespace std;

string rev(string s) {
    reverse(s.begin(), s.end());
    return s;
}

int main() {
    int t;
    cin >> t;
    while (t--) {
        string s;
        cin >> s;
        cout<<s+rev(s)<<endl;
    }
    return 0;
}