#include "logger.h"
#include <stdlib.h>
#include <string>
#include <iostream>

using namespace std;

void logger::info(const string& msg) {
    cout << "[INFO] " + msg << endl;
}

void logger::warning(const string& msg) {
    cout << "[WARN] " + msg << endl;
}

void logger::err(const string& msg) {
    cout << "[ERRO] " + msg << endl;
}