#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <cmath>
#include <ppl.h>	//parallel_for
#include <ctime>
#include <stdint.h>
#include <intrin.h>


using namespace std;
using namespace concurrency;	//parallel_for

long double finalValue = 0;

using matrix = vector<vector<double>>;
using set = vector<vector<int>>;	//sets are actual objects in C++ so make sure to not #include <set>
									//in this code


void ClockTicks(const unsigned __int64 &ticks);

//variable prep
vector<int> readNandM(ifstream &inFile);
matrix readW(ifstream &inFile);

//major calculations
set subsetsToList(const int &n, const int &r);
void subsetsToList2(
	set &matrix1, matrix &matrixW, const int &n, const int & m, const int &k, const int &threadIndex, const int &threadCount);
long roUnion(set &vec);
double matProduct(matrix &matrixW, set &set1);
void bigSum(matrix &matrixW, const int &n, const int &m, const int &k, const int &threadCount);

//minor calculation
long long partialFactorial(const long &n, const long &n2, const long &k);

//testing and display purposes
void printVector(const set &vec);
void printVector(const matrix &vec);
void printVector(const vector<set> &vec);




int main() {
	clock_t beginClockTimer = clock();
	unsigned __int64 ticks = __rdtsc();

	int numThreads = 2;
	string fileName;

	cout << "Please enter the file name: ";
	cin >> fileName;
	ifstream inFile(fileName);
	vector<int> nAndm(readNandM(inFile));
	int n = nAndm[0], m = nAndm[1];
	vector<vector<double>> W(readW(inFile));
	inFile.close();

	ofstream outFile(fileName, ostream::app);

	outFile << "(" << flush;
	for (int k = 0; k < ceil(log(m)); ++k) {
		bigSum(W, n, m, k, numThreads);
		if (k)
			outFile << finalValue;
		else
			outFile << log(finalValue);
		if (k != ceil(log(m)) - 1) outFile << ",";
	}
	outFile << ")" << endl;
	outFile.close();
	






	clock_t endClockTimer = clock();
	double elapsed_secs = double(endClockTimer - beginClockTimer) / CLOCKS_PER_SEC;
	cout << endl << endl << endl << "Clock is: " << elapsed_secs << endl;
	unsigned __int64 updatedTicks = __rdtsc() - ticks;
	ClockTicks(updatedTicks);
	cout << endl << updatedTicks << flush;

	return 0;
}



inline void ClockTicks(const unsigned __int64 &ticks) {

	if (ticks < 1000)
		cout << ticks << " clock ticks" << flush;
	else if (ticks < 1000000)
		cout << (ticks / 1000) << " thousand clock ticks" << flush;
	else if (ticks < 1000000000)
		cout << (ticks / 1000000) << " million clock ticks" << flush;
	else if (ticks < 1000000000000)
		cout << (ticks / 1000000000) << " billion clock ticks" << flush;
	else if (ticks < 1000000000000000)
		cout << (ticks / 1000000000000) << " trillion clock ticks" << flush;
	else if (ticks < 1000000000000000000)
		cout << (ticks / 1000000000000000) << " quadrillion clock ticks" << flush;
}





inline vector<int> readNandM(ifstream &inFile) {
	string line;
	getline(inFile, line);
	istringstream is(line);
	return vector<int>(istream_iterator<int>(is),
			istream_iterator<int>());
}

inline matrix readW(ifstream &inFile) {

	matrix W;
	//delimited by spaces and enters
	string line;
	while (getline(inFile, line)) {
		istringstream is(line);
		W.push_back(
			vector<double>(istream_iterator<double>(is),
				istream_iterator<double>()));
	}
	return W;

}


set subsetsToList(const int &n, const int &r) {

	set toReturn;
	toReturn.reserve(round(tgamma(n + 1)) / (2 * round(tgamma(n - 1))));	// n choose 2

	long index = 0;

	vector<bool> v(n);
	fill(v.begin(), v.begin() + r, true);

	do {
		toReturn.resize(++index);
		for (int i = 0; i < n; ++i) {
			if (v[i]) {
				toReturn[index - 1].push_back(i + 1);
			}
		}
	} while (prev_permutation(v.begin(), v.end()));


	return toReturn;

}

void subsetsToList2(
	set &matrix1, matrix &matrixW, const int &n, const int & m, const int &k, const int &threadIndex, const int &threadCount) {


	unsigned long long num = round(tgamma(n + 1)) / (2 * round(tgamma(n - 1)));	// n choose 2
	long skippingIndex = 0;

	int counter = 0;


	set XsubK(1);

	vector<bool> v(matrix1.size());
	fill(v.begin(), v.begin() + k, true);
	vector<set> toReturn(1);
	long double returnValue = 0;
	long long temp = 0, ro = 0;


	do {
		if ((++skippingIndex + threadIndex) % threadCount != 0)
			continue;

		for (long i = 0; i < matrix1.size(); ++i)
			if (v[i])
				XsubK[0].push_back(i + 1);


		for (long j = 0; j < XsubK[0].size(); ++j)
			toReturn[0].push_back(matrix1[XsubK[0][j] - 1]);

		ro = roUnion(toReturn[0]);
		temp = (round(tgamma(n - ro)) / (round(tgamma(m - ro)) * round(tgamma(n - m))));
		if (temp >= 0)
			returnValue += temp * matProduct(matrixW, toReturn[0]);

		toReturn[0].clear();
		XsubK[0].clear();

	} while (prev_permutation(v.begin(), v.end()));

	finalValue += returnValue  * tgamma(k + 1);
}


inline long roUnion(set &vec) {
	vector<int> startingVec(1), temp(1);
	for (const auto &index : vec) {
		set_union(index.begin(), index.end(), startingVec.begin(), startingVec.end(), back_inserter(temp));
		startingVec = temp;
		sort(startingVec.begin(), startingVec.end());
		sort(temp.begin(), temp.end());
	}

	startingVec.erase(unique(startingVec.begin(), startingVec.end()), startingVec.end());

	return startingVec.size() - 1;	//removes the first element, which is 0
}



inline double matProduct(matrix &matrixW, set &set1) {
	double returnValue = 1;
	for (const auto &index : set1)
		returnValue *= matrixW[index[0] - 1][index[1] - 1] - 1;
	return returnValue;

}

inline void bigSum(matrix &matrixW, const int &n, const int &m, const int &k, const int &threadCount) {

	long double returnValue = 0;

	auto X = subsetsToList(n, 2);

	parallel_for(0, threadCount, [&](int i) {
		subsetsToList2(ref(X), ref(matrixW), n, m, k, i, threadCount); });

}







long long partialFactorial(const long &n, const long &n2, const long &k) {
	long temp = n;
	if (temp > n2 - k)
		return partialFactorial(temp - 1, n2, k) * temp;
	return 1;
}





void printVector(const set &vec) {

	cout << endl << endl << "vec has " << vec.size() << " elements: " << endl << endl << "{" << endl;
	for (const auto &index1 : vec) {
		cout << "{ ";
		for (const auto &index2 : index1)
			cout << index2 << ", ";
		cout << "\b\b }" << endl;
	}
	cout << "}" << endl << endl;

}


void printVector(const matrix &vec) {

	cout << endl << endl << "vec has " << vec.size() << " elements: " << endl << endl << "{" << endl;
	for (const auto &index1 : vec) {
		cout << "{ ";
		for (const auto &index2 : index1)
			cout << index2 << ", ";
		cout << "\b\b }" << endl;
	}
	cout << "}" << endl << endl;

}

void printVector(const vector<set> &vec) {

	cout << "vec has " << vec.size() << " elements of size " << vec[0].size() << ":" << endl << endl << "{" << endl;
	for (const auto &index1 : vec) {
		cout << "{   ";
		for (const auto &index2 : index1) {
			cout << "{ ";
			for (const auto &index3 : index2)
				cout << index3 << ", ";
			cout << "\b\b }, ";
		}
		cout << "\b\b   }," << endl;
	}
	cout << "}" << endl << endl;

}

