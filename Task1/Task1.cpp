#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <string>

using namespace std;

struct DataRow {
    double x, T, U;
};

class ErrorRange : public exception {
    double val;
public:
    ErrorRange(double v) : val(v) {}
    const char* what() const noexcept override { return "Error: Value out of range."; }
    double value() const { return val; }
};

class ErrorNoFile : public exception {
    string file;
public:
    ErrorNoFile(const string& f) : file(f) {}
    const char* what() const noexcept override { return "Error: Cannot open file."; }
    string filename() const { return file; }
};

vector<DataRow> readTable(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) throw ErrorNoFile(filename);

    vector<DataRow> table;
    DataRow row;
    while (file >> row.x >> row.T >> row.U) table.push_back(row);

    return table;
}

double interpolate(const vector<DataRow>& table, double x, bool isT) {
    for (size_t i = 1; i < table.size(); ++i) {
        if ((table[i - 1].x <= x && x <= table[i].x) || (table[i].x <= x && x <= table[i - 1].x)) {
            double x0 = table[i - 1].x, x1 = table[i].x;
            double y0 = isT ? table[i - 1].T : table[i - 1].U;
            double y1 = isT ? table[i].T : table[i].U;
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
        }
    }
    throw ErrorRange(x);
}

double T(double x) {
    string file;
    if (x == 1) file = "dat_X_1_1.dat";
    else if (x > 1) { x = 1 / x; file = "dat_X1_00.dat"; }
    else if (x < -1) { x = 1 / x; file = "dat_X00_1.dat"; }
    else file = "dat_X_1_1.dat";

    return interpolate(readTable("data/" + file), x, true);
}

double U(double x) {
    string file;
    if (x == 1) file = "dat_X_1_1.dat";
    else if (x > 1) { x = 1 / x; file = "dat_X1_00.dat"; }
    else if (x < -1) { x = 1 / x; file = "dat_X00_1.dat"; }
    else file = "dat_X_1_1.dat";

    return interpolate(readTable("data/" + file), x, false);
}

double Srz(double x, double y, double z) {
    if (x > y) return T(x) + U(z) - T(y);
    return T(y) + U(y) - U(z);
}

double Glr(double x, double y) {
    if (abs(x) >= 1 && abs(y) < 1) return y;
    double tmp = x * x + y * y - 4;
    if (tmp < 0.1) throw runtime_error("Glr fallback required");
    return y / tmp;
}

double Gold(double x, double y) {
    if (x < y && x != 0) return y / x;
    if (x > y && y != 0) return x / y;
    throw runtime_error("Gold fallback required");
}

double Grs(double x, double y) {
    try {
        return 0.1389 * Srz(x + y, Gold(x, y), Glr(x, x * y)) +
            1.8389 * Srz(x - y, Gold(y, x / 5), Glr(5 * x, x * y)) +
            0.83 * Srz(x - 0.9, Glr(y, x / 5), Gold(5 * y, y));
    }
    catch (...) {
        throw;
    }
}

double Gold1(double x, double y) {
    if (x <= y && abs(x) > 0.1) return y / x;
    else if (x < y && x > 0.1) return 0.15;
    else if (y == 0) return x;
    else if (x < 1) return x;
    return y;
}

double Glr1(double x, double y) {
    if (abs(x) < 1) return x;
    return y;
}

double Grs1(double x, double y) {
    return 0.14 * Srz(x + y, Gold1(x, y), Glr1(x, x * y)) +
        1.83 * Srz(x - y, Gold1(y, x / 5), Glr1(4 * x, x * y)) +
        0.83 * Srz(x, Glr1(y, x / 4), Gold1(4 * y, y));
}

double fun(double x, double y, double z) {
    try {
        return x * Grs(y, z) + y * Grs(x, z) + 0.33 * x * y * Grs(x, z);
    }
    catch (...) {
        try {
            return x * Grs1(x, y) + y * Grs1(y, z) + z * Grs1(z, x);
        }
        catch (...) {
            cerr << "Falling back to Alg3\n";
            return 1.3498 * x + 2.2362 * y - 2.348 * x * y * z;
        }
    }
}

int main() {
    double x, y, z;
    cout << "Enter x, y, z: ";
    cin >> x >> y >> z;

    try {
        double result = fun(x, y, z);
        cout << "Result fun(x,y,z) = " << result << endl;
    }
    catch (const ErrorRange& e) {
        cerr << "Range error: x = " << e.value() << endl;
    }
    catch (const ErrorNoFile& e) {
        cerr << "File error: can't open " << e.filename() << endl;
    }
    catch (const exception& e) {
        cerr << "General error: " << e.what() << endl;
    }

    return 0;
}