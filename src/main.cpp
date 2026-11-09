#include <iostream>    // for cin and cout
#include <vector>      // for storing lists of primes
#include <thread>      // for std::thread
#include <mutex>       // for mutex to protect cout
#include <chrono>      // for measuring execution time
#include <functional>  // for std::ref
#include <iomanip>     // for formatting output (setw, setprecision)
#include <string>
#include <limits>      // to clear bad input from cin

using namespace std;
using namespace std::chrono;

// Mutex to protect cout so multiple threads don't print at the same time and scramble the text
mutex printMutex;

// Structure to store stats for each thread so we can show results and graph at the end
struct ThreadInfo {
    long long start = 0;  // starting number for this thread
    long long end = 0;    // ending number for this thread
    long long count = 0;  // how many primes this thread found
    double ms = 0;        // how long this thread took in milliseconds
};

// Basic prime check using trial division up to sqrt(n)
// Skipping even numbers to make it faster
bool isPrime(long long n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (long long i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

// Finds all primes in [start, end] and puts them in the out vector
void findPrimes(long long start, long long end, vector<long long>& out) {
    for (long long n = start; n <= end; n++)
        if (isPrime(n)) out.push_back(n);
}

// Worker function: this is what each thread runs
// It finds primes in its chunk, measures its own time, and reports its result
void worker(int id, long long start, long long end, vector<long long>& out,
            ThreadInfo& info, bool verbose) {
    auto t1 = steady_clock::now();
    findPrimes(start, end, out);
    auto t2 = steady_clock::now();

    // Save info for this thread
    info.start = start;
    info.end = end;
    info.count = (long long)out.size();
    info.ms = duration<double, milli>(t2 - t1).count();

    // If verbose is true (used in live demo), print when this thread finishes
    if (verbose) {
        lock_guard<mutex> lock(printMutex); // lock so only 1 thread prints at a time
        cout << "  [Thread " << id + 1 << "] finished " << start << " to " << end
             << " -> " << out.size() << " primes in "
             << fixed << setprecision(1) << info.ms << " ms\n";
    }
}

// Splits the work among threads, runs them in parallel, and gathers results
vector<long long> findPrimesParallel(long long start, long long end, int numThreads,
                                     vector<ThreadInfo>& infos, bool verbose) {
    long long total = end - start + 1;
    // Don't create more threads than numbers to check
    if (numThreads > total) numThreads = (int)total;

    // Each thread gets its own vector to avoid race conditions and lock contention
    vector<vector<long long>> parts(numThreads);
    infos.assign(numThreads, ThreadInfo());
    vector<thread> threads;
    long long chunk = total / numThreads;

    // Divide the range into chunks and launch each thread
    for (int i = 0; i < numThreads; i++) {
        long long s = start + i * chunk;
        // Last thread takes care of any remaining numbers due to division rounding
        long long e = (i == numThreads - 1) ? end : s + chunk - 1;
        threads.emplace_back(worker, i, s, e, ref(parts[i]), ref(infos[i]), verbose);
    }

    // Wait for all threads to finish before continuing
    for (auto& t : threads) t.join();

    // Combine all thread results into one final vector
    vector<long long> result;
    for (auto& p : parts) result.insert(result.end(), p.begin(), p.end());
    return result;
}

// Draws a simple horizontal bar chart using '#' characters
string bar(double value, double maxValue, int width = 30) {
    int n = (maxValue > 0) ? (int)(value / maxValue * width) : 0;
    return string(n, '#');
}

// Helper function to ask user for the start and end range and validate input
bool readRange(long long& s, long long& e) {
    cout << "Start of range: ";
    cin >> s;
    cout << "End of range:   ";
    cin >> e;
    
    // Check for bad input or invalid range
    if (cin.fail() || s < 1 || e < s) {
        cin.clear(); // clear error flags
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard bad input
        cout << "Invalid range. Start must be 1 or more and not greater than end.\n";
        return false;
    }
    return true;
}

// Runs sequential (1 thread) to get a baseline time for calculating speedup
double runSequential(long long s, long long e, vector<long long>& seq) {
    auto a = steady_clock::now();
    findPrimes(s, e, seq);
    auto b = steady_clock::now();
    return duration<double, milli>(b - a).count();
}

// Option 1: Live Demo
// Runs 1 thread first, then user-specified thread count, and prints a chart showing each thread's time
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

    // 1. Run sequential first
    cout << "\n--- Sequential (1 thread) ---\n";
    vector<long long> seq;
    double seqMs = runSequential(s, e, seq);
    cout << "Primes found: " << seq.size() << " | Time: "
         << fixed << setprecision(1) << seqMs << " ms\n";

    // 2. Run parallel
    cout << "\n--- Parallel (" << n << " threads) ---\n";
    vector<ThreadInfo> infos;
    auto a = steady_clock::now();
    vector<long long> par = findPrimesParallel(s, e, n, infos, true);
    auto b = steady_clock::now();
    double parMs = duration<double, milli>(b - a).count();

    // 3. Display visual bar chart of each thread's time
    cout << "\nTime per thread (longer bar = more work):\n";
    double maxMs = 0;
    for (auto& i : infos) maxMs = max(maxMs, i.ms);
    for (size_t i = 0; i < infos.size(); i++) {
        cout << "Thread " << setw(2) << i + 1 << " | " << left << setw(30)
             << bar(infos[i].ms, maxMs) << right << " | "
             << fixed << setprecision(1) << infos[i].ms << " ms\n";
    }

    // 4. Compare results and verify correctness
    cout << "\n===== RESULT =====\n";
    cout << "Sequential: " << seqMs << " ms\n";
    cout << "Parallel:   " << parMs << " ms (" << n << " threads)\n";
    cout << "Speedup:    " << setprecision(2) << seqMs / parMs << "x\n";
    cout << "Correct:    " << (par.size() == seq.size() ? "Yes, counts match" : "NO - BUG") << "\n";
}

// Option 2: Benchmark
// Automatically tests multiple thread counts (2, 4, 8, 12, 16) and shows a comparison table
void runBenchmark() {
    long long s, e;
    if (!readRange(s, e)) return;

    // Get baseline time with 1 thread
    vector<long long> seq;
    double seqMs = runSequential(s, e, seq);
    cout << "\nPrimes found: " << seq.size() << "\n\n";

    // Table header
    cout << left << setw(10) << "Threads" << setw(14) << "Time (ms)" << setw(10)
         << "Speedup" << setw(10) << "Correct" << "Chart (each # = 0.25x)\n";
    cout << string(70, '-') << "\n";
    cout << left << setw(10) << "1 (seq)" << setw(14) << fixed << setprecision(1)
         << seqMs << setw(10) << "1.00x" << setw(10) << "-" << "####\n";

    // Test different thread counts
    int counts[] = {2, 4, 8, 12, 16};
    for (int n : counts) {
        vector<ThreadInfo> infos;
        auto a = steady_clock::now();
        vector<long long> par = findPrimesParallel(s, e, n, infos, false);
        auto b = steady_clock::now();
        double ms = duration<double, milli>(b - a).count();
        double sp = seqMs / ms;

        // Print row for this thread count
        cout << left << setw(10) << n << setw(14) << fixed << setprecision(1) << ms
             << setw(10) << (to_string(sp).substr(0, 4) + "x")
             << setw(10) << (par.size() == seq.size() ? "Yes" : "NO")
             << string((int)(sp * 4), '#') << "\n";
    }
}

// Main menu loop
int main() {
    while (true) {
        cout << "\n=== Parallel Prime Number Finder ===\n";
        // Check how many CPU logical processors this machine has
        cout << "CPU logical processors: " << thread::hardware_concurrency() << "\n";
        cout << "1. Live demo (see each thread work)\n";
        cout << "2. Benchmark (compare 2, 4, 8, 12, 16 threads)\n";
        cout << "3. Exit\n";
        cout << "Choose: ";

        int choice;
        cin >> choice;

        // Handle non-numeric or bad input
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
