#include <bits/stdc++.h>
using namespace std;

// Persistent file-based key-value storage with limited memory.
// We shard the log by hash bucket into 16 files to limit scan size per find.

static const int NUM_BUCKETS = 16; // <= 20 file count limit
static const char* STORE_DIR = "."; // use current directory

// FNV-1a 64-bit hash for strings
static uint64_t fnv1a64(const string &s) {
    const uint64_t FNV_offset_basis = 14695981039346656037ull;
    const uint64_t FNV_prime = 1099511628211ull;
    uint64_t hash = FNV_offset_basis;
    for (unsigned char c : s) {
        hash ^= c;
        hash *= FNV_prime;
    }
    return hash;
}

static string bucket_filename(int b) {
    char buf[64];
    snprintf(buf, sizeof(buf), "kv_bucket_%02d.dat", b);
    return string(STORE_DIR) + "/" + buf;
}

static void ensure_bucket_files() {
    for (int b = 0; b < NUM_BUCKETS; ++b) {
        string fname = bucket_filename(b);
        ifstream in(fname, ios::binary);
        if (!in.good()) {
            ofstream out(fname, ios::binary);
        }
    }
}

struct RecordHeader {
    uint8_t type; // 0 = insert, 1 = delete
    uint8_t key_len; // up to 255, spec says <= 64
    uint32_t value; // non-negative int
};

static void append_record(const string &key, uint32_t value, uint8_t type) {
    int bucket = (int)(fnv1a64(key) % NUM_BUCKETS);
    string fname = bucket_filename(bucket);
    ofstream out(fname, ios::binary | ios::app);
    RecordHeader hdr;
    hdr.type = type;
    hdr.key_len = (uint8_t)min<size_t>(255, key.size());
    hdr.value = value;
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(key.data(), hdr.key_len);
}

static void find_and_output(const string &key) {
    int bucket = (int)(fnv1a64(key) % NUM_BUCKETS);
    string fname = bucket_filename(bucket);
    ifstream in(fname, ios::binary);
    if (!in.good()) {
        cout << "null\n";
        return;
    }
    unordered_set<uint32_t> values;
    RecordHeader hdr;
    string kbuf;
    while (true) {
        in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
        if (!in) break;
        kbuf.resize(hdr.key_len);
        in.read(&kbuf[0], hdr.key_len);
        if (!in) break;
        if (kbuf == key) {
            if (hdr.type == 0) {
                values.insert(hdr.value);
            } else if (hdr.type == 1) {
                values.erase(hdr.value);
            }
        }
    }
    if (values.empty()) {
        cout << "null\n";
    } else {
        vector<uint32_t> v(values.begin(), values.end());
        sort(v.begin(), v.end());
        for (size_t i = 0; i < v.size(); ++i) {
            if (i) cout << ' ';
            cout << v[i];
        }
        cout << '\n';
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ensure_bucket_files();

    int n;
    if (!(cin >> n)) return 0;
    string cmd;
    for (int i = 0; i < n; ++i) {
        if (!(cin >> cmd)) break;
        if (cmd == "insert") {
            string key; long long val;
            cin >> key >> val;
            append_record(key, (uint32_t)val, 0);
        } else if (cmd == "delete") {
            string key; long long val;
            cin >> key >> val;
            append_record(key, (uint32_t)val, 1);
        } else if (cmd == "find") {
            string key; cin >> key;
            find_and_output(key);
        } else {
            string rest;
            getline(cin, rest);
        }
    }
    return 0;
}
