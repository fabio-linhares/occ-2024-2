#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include "input/input_parser.h"
#include "output/output_writer.h"
#include "algorithm/greedy_algorithm.h"
#include "utils/logger.h"
#include "app/app_controller.h"

int main(int argc, char* argv[]) {
    AppController app;
    return app.run();
}