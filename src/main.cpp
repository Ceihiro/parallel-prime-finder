#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <iomanip>
#include <string>
#include <limits>
using namespace std;
using namespace std::chrono;

mutex printMutex; // protects cout, which all threads share

struct ThreadInfo {
    long long start = 0, end = 0, count = 0;
    double ms = 0;
};

bool isPrime(long long n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (long long i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

void findPrimes(long long start, long long end, vector<long long>& out) {
    for (long long n = start; n <= end; n++)
        if (isPrime(n)) out.push_back(n);
}

// Each thread runs this: finds primes in its chunk, times itself, reports
void worker(int id, long long start, long long end, vector<long long>& out,
            ThreadInfo& info, bool verbose) {
    auto t1 = steady_clock::now();
    findPrimes(start, end, out);
    auto t2 = steady_clock::now();

    info.start = start;
    info.end = end;
    info.count = (long long)out.size();
    info.ms = duration<double, milli>(t2 - t1).count();

    if (verbose) {
        lock_guard<mutex> lock(printMutex); // only one thread prints at a time
        cout << "  [Thread " << id + 1 << "] finished " << start << " to " << end
             << " -> " << out.size() << " primes in "
             << fixed << setprecision(1) << info.ms << " ms\n";
    }
}

vector<long long> findPrimesParallel(long long start, long long end, int numThreads,
                                     vector<ThreadInfo>& infos, bool verbose) {
    long long total = end - start + 1;
    if (numThreads > total) numThreads = (int)total;

    vector<vector<long long>> parts(numThreads); // each thread has its OWN list
    infos.assign(numThreads, ThreadInfo());
    vector<thread> threads;
    long long chunk = total / numThreads;

    for (int i = 0; i < numThreads; i++) {
        long long s = start + i * chunk;
        long long e = (i == numThreads - 1) ? end : s + chunk - 1;
        threads.emplace_back(worker, i, s, e, ref(parts[i]), ref(infos[i]), verbose);
    }
    for (auto& t : threads) t.join();

    vector<long long> result;
    for (auto& p : parts) result.insert(result.end(), p.begin(), p.end());
    return result;
}

string bar(double value, double maxValue, int width = 30) {
    int n = (maxValue > 0) ? (int)(value / maxValue * width) : 0;
    return string(n, '#');
}

bool readRange(long long& s, long long& e) {
    cout << "Start of range: ";
    cin >> s;
    cout << "End of range:   ";
    cin >> e;
    if (cin.fail() || s < 1 || e < s) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid range. Start must be 1 or more and not greater than end.\n";
        return false;
    }
    return true;
}

double runSequential(long long s, long long e, vector<long long>& seq) {
    auto a = steady_clock::now();
    findPrimes(s, e, seq);
    auto b = steady_clock::now();
    return duration<double, milli>(b - a).count();
}

// Option 1: live demo with one thread count
void runDemo() {
    long long s, e;
    int n;
    if (!readRange(s, e)) return;
    cout << "Number of threads: ";
    cin >> n;
    if (cin.fail() || n < 1) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid thread count.\n";
        return;
    }

    cout << "\n--- Sequential (1 thread) ---\n";
    vector<long long> seq;
    double seqMs = runSequential(s, e, seq);
    cout << "Primes found: " << seq.size() << " | Time: "
         << fixed << setprecision(1) << seqMs << " ms\n";

    cout << "\n--- Parallel (" << n << " threads) ---\n";
    vector<ThreadInfo> infos;
    auto a = steady_clock::now();
    vector<long long> par = findPrimesParallel(s, e, n, infos, true);
    auto b = steady_clock::now();
    double parMs = duration<double, milli>(b - a).count();

    cout << "\nTime per thread (longer bar = more work):\n";
    double maxMs = 0;
    for (auto& i : infos) maxMs = max(maxMs, i.ms);
    for (size_t i = 0; i < infos.size(); i++) {
        cout << "Thread " << setw(2) << i + 1 << " | " << left << setw(30)
             << bar(infos[i].ms, maxMs) << right << " | "
             << fixed << setprecision(1) << infos[i].ms << " ms\n";
    }

    cout << "\n===== RESULT =====\n";
    cout << "Sequential: " << seqMs << " ms\n";
    cout << "Parallel:   " << parMs << " ms (" << n << " threads)\n";
    cout << "Speedup:    " << setprecision(2) << seqMs / parMs << "x\n";
    cout << "Correct:    " << (par.size() == seq.size() ? "Yes, counts match" : "NO - BUG") << "\n";
}

// Option 2: benchmark table with several thread counts
void runBenchmark() {
    long long s, e;
    if (!readRange(s, e)) return;

    vector<long long> seq;
    double seqMs = runSequential(s, e, seq);
    cout << "\nPrimes found: " << seq.size() << "\n\n";

    cout << left << setw(10) << "Threads" << setw(14) << "Time (ms)" << setw(10)
         << "Speedup" << setw(10) << "Correct" << "Chart (each # = 0.25x)\n";
    cout << string(70, '-') << "\n";
    cout << left << setw(10) << "1 (seq)" << setw(14) << fixed << setprecision(1)
         << seqMs << setw(10) << "1.00x" << setw(10) << "-" << "####\n";

    int counts[] = {2, 4, 8, 12, 16};
    for (int n : counts) {
        vector<ThreadInfo> infos;
        auto a = steady_clock::now();
        vector<long long> par = findPrimesParallel(s, e, n, infos, false);
        auto b = steady_clock::now();
        double ms = duration<double, milli>(b - a).count();
        double sp = seqMs / ms;

        cout << left << setw(10) << n << setw(14) << fixed << setprecision(1) << ms
             << setw(10) << (to_string(sp).substr(0, 4) + "x")
             << setw(10) << (par.size() == seq.size() ? "Yes" : "NO")
             << string((int)(sp * 4), '#') << "\n";
    }
}

int main() {
    while (true) {
        cout << "\n=== Parallel Prime Number Finder ===\n";
        cout << "CPU logical processors: " << thread::hardware_concurrency() << "\n";
        cout << "1. Live demo (see each thread work)\n";
        cout << "2. Benchmark (compare 2, 4, 8, 12, 16 threads)\n";
        cout << "3. Exit\n";
        cout << "Choose: ";
        int choice;
        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        if (choice == 1) runDemo();
        else if (choice == 2) runBenchmark();
        else if (choice == 3) break;
    }
    return 0;
}
