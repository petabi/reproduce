#include <iostream>

#include "config.h"
#include "controller.h"
#include "converter.h"
#include "producer.h"

using namespace std;

int main(int argc, char** argv)
{
  Config conf;

  if (!conf.set(argc, argv)) {
    exit(0);
  }

  try {
    Controller ctrl(conf);
    ctrl.run();
  } catch (exception const& e) {
    cerr << "[EXCEPTION] " << e.what() << "\n";
  }

  return 0;
}

// vim:et:ts=2:sw=2
