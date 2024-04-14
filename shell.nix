let
  nixpkgs_rev = "1f7e3343e3de9e8d05d8316262a95409a390c83b";  # master on 2024-03-20

  # TODO: change to mirrexagon's repo after merges of PR 42 and 44
  nixpkgs-esp-dev_rev = "b3594b490ed1ffb40e5dd20ea7074b73ae34ea27"; # main on 2024-03-20
  nixpkgs-esp-dev_sha256 = "sha256:0fqvk9va6rqd9pmjd82pmikzc30d174jcgmvfyacg875qgrxjii8";
  nixpkgs-esp-dev_src = (builtins.fetchTarball {
    url = "https://github.com/cyber-murmel/nixpkgs-esp-dev/archive/${nixpkgs-esp-dev_rev}.tar.gz";
    sha256 = nixpkgs-esp-dev_sha256;
  });
in

{ pkgs ? import (builtins.fetchTarball "https://github.com/NixOS/nixpkgs/archive/${nixpkgs_rev}.tar.gz") {
  overlays = [
    (import "${nixpkgs-esp-dev_src}/overlay.nix")
  ];
}
}:

with pkgs;
let
  riscv32-unknown-linux-gnu = stdenv.mkDerivation rec {
    name = "riscv32-unknown-linux-gnu";
    version = "13.2.0";

    release = "2024.03.01";
    ubuntu_version = "22.04";

    src = (builtins.fetchTarball "https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/${release}/riscv32-glibc-ubuntu-${ubuntu_version}-gcc-nightly-${release}-nightly.tar.gz");

    buildInputs = [
      autoPatchelfHook

      zlib
      glib
      gmp
      zstd
      mpfr
      libmpc
      lzma
      expat
      python310
      ncurses
    ];

    installPhase = ''
      runHook preInstall

      cp -r . $out

      runHook postInstall
    '';
  };
in
mkShell {
  buildInputs = [
    riscv32-unknown-linux-gnu
    cmake
    esptool
    picocom
    esp-idf-esp32c6
  ];
}
