#include "config.h"
#include "controller.h"

using namespace std;

auto main(int argc, char** argv) -> int
{
  Config* conf = config_new(argc, argv);
  if (!conf) {
    cerr << "cannot initiate configurations\n";
    exit(1);
  }
  config_show(conf);

  try {
    Controller ctrl(conf);
    cout << "reproduce start\n";
    ctrl.run();
    cout << "reproduce end\n";
  } catch (exception const& e) {
    cerr << "[EXCEPTION] " << e.what() << "\n";
  }

  config_free(conf);
  return 0;
}

// vim:et:ts=2:sw=2
