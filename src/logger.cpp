#include "logger.h"
#include <stdlib.h>
#include <string>
#include <iostream>

using namespace std;

void logger::info(string msg) {
    cout << "[INFO] " + msg << endl;
}

void logger::warning(string msg) {
    cout << "[WARN] " + msg << endl;
}

void logger::err(string msg) {
    cout << "[ERRO] " + msg << endl;
}