with import <nixpkgs> {};
  pkgsCross.riscv32-embedded.stdenv.mkDerivation {
    name = "BadgerOS";
    nativeBuildInputs = with pkgs.buildPackages; [
      cmake
      ninja
      gnumake # build tools
      clang-tools_16 # clang-format, clang-tidy
      python3
    ];
    # ++ [.gcc]
    buildInputs = [];
    src = ./.; # /home/felix/projects/operating-systems/BadgerOS;
  }
