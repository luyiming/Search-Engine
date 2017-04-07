/**
 * @file
 * @author  Lu Yiming <luyimingchn@gmail.com>
 * @version 1.0
 * @date 2017-1-4

 * @section DESCRIPTION
 *
 * Pagerank algorithms
 * !!!!!!!!!!!!!!!!!!!!!
 * !!!! Deprecated !!!!!
 * !!!!!!!!!!!!!!!!!!!!!
 */

#include <iostream>
#include <fstream>
#include "Matrix.hpp"
#include "SparseMatrix.hpp"
#include <string>

using namespace std;

const int MAXN = 10000;
double d = 0.85;

int main() {
	ifstream fin("../../data/graph.txt");
	int r, c, n = 0, L[MAXN];
	Matrix A(MAXN, MAXN);
	for (int i = 0; i < MAXN; i++)
		for (int j = 0; j < MAXN; j++)
			A[i][j] = 0.0;
	for (int i = 0; i < MAXN; i++)
		L[i] = 0;
	while (fin >> r >> c) {
		A[r][c] = 1.0;
		L[r]++;
		n = r > n ? r : n;
		n = c > n ? c : n;
	}
	for (int i = 0; i < n; i++) {
		if (L[i]) {
			for (int j = 0; j < n; j++) {
				A[i][j] /= L[i];
			}
		}
		else {
			for (int j = 0; j < n; j++) {
				A[i][j] = 1.0 / n;
			}
		}
	}
	n = n + 1;
	A.resize(n, n);
	Matrix res(1, n, 1.0 / n), last_res(1, n, 1.0 / n);

	int iter_times = 30;
	for (int i = 0; i < iter_times; i++) {
		res = (1.0 - d) / n + d * res * A;
		res.display();
		cout << "gap: " << (res - last_res).dot() << endl;
		last_res = res;
	}

	Matrix t = res;
	for (int i = 0; i < n; i++) {
		int index = i;
		for (int j = i + 1; j < n; j++) {
			if(t[0][j] > t[0][index])
				index = j;
		}
		double x = t[0][i];
		t[0][i] = t[0][index];
		t[0][index] = x;
	}
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (res[0][j] == t[0][i]) {
				cout << j << ": " << t[0][i] << endl;
				break;
			}
		}
	}
	return 0;
}
