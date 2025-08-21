import hsh.shell.app;

int main(int argc, char const** argv) noexcept {
  hsh::shell::App app;
  return app.run(argc, argv);
}
